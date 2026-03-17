#include <errno.h>
#include <stdbool.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <drivers/uart.h>
#include <drivers/nvram_config.h>
#include <drivers/bluetooth/bt_drv.h>

#if  defined(CONFIG_BT_CTRL_BLE_BQB) || defined(CONFIG_BT_CTRL_ST)

#define BT_BQB_MODE 0
#define BLE_BQB_MODE 1
#define SINGLE_TONE_MODE 2

int set_ble_mode(uint16_t mode);

static uint8_t bqb_mode;

#ifdef CONFIG_BT_CTRL_BLE_BQB
#ifdef CONFIG_USE_AL_ENCODE_1
extern int global_encode_buffer[4*1024];
static uint8_t *tx_data = (uint8_t *)global_encode_buffer;
static uint8_t *rx_data = (uint8_t *)global_encode_buffer + CONFIG_BQB_TX_RBUF_SIZE;
#else
static uint8_t tx_data[CONFIG_BQB_TX_RBUF_SIZE];
static uint8_t rx_data[CONFIG_BQB_RX_RBUF_SIZE];
#endif
static const struct device *uart_dev;
static uint8_t rx_type;
#endif



#ifdef CONFIG_BT_CTRL_ST

static void send_cmd_buf(const uint8_t *cmd_buf, uint8_t cmd_len)
{
	btdrv_send(HCI_CMD, (uint8_t *)cmd_buf, cmd_len);
	k_sleep(K_MSEC(50));
}

void hci_reset(void)
{
	uint8_t BUF_RESET[] = {0x03, 0x0c, 0x00};
	send_cmd_buf(BUF_RESET, sizeof(BUF_RESET));
}

void send_single_tone(uint8_t channel, uint8_t tx_pwr, uint8_t type)
{
//03 channel type tx_pwr
//04 channel type tx_pwr phy
	uint8_t BUF_CONTINUOUS_TX[] = {0x0F, 0xFE, 0x03, 0x13, 0x00, 0x00};
	BUF_CONTINUOUS_TX[3] = channel;
	BUF_CONTINUOUS_TX[4] = type;
	BUF_CONTINUOUS_TX[5] = tx_pwr;

	send_cmd_buf(BUF_CONTINUOUS_TX, sizeof(BUF_CONTINUOUS_TX));
}

static void single_tone_mode(void)
{
	printk("start send_single_tone\n");

	hci_reset();
	send_single_tone(0x13, 0, 0);

	printk("End send_single_tone\n");
}
#endif

#ifdef CONFIG_BT_CTRL_BLE_BQB
static void bqb_uart_send(uint8_t *tx_data, uint16_t len)
{
	int i;

	for (i = 0; i < len; i++) {
		uart_poll_out(uart_dev, tx_data[i]);
	}
}

static int bqb_uart_tx_hci(uint8_t type, uint8_t *data)
{
	uint16_t len;

	switch (type) {
	case HCI_ACL:
		len = (data[3]<<8 | data[2]) + HCI_ACL_HDR_SIZE;
		break;
	case HCI_SCO:
		len = data[2] + HCI_SCO_HDR_SIZE;
		break;
	case HCI_EVT:
		len = data[1] + HCI_EVT_HDR_SIZE;
		break;
	case HCI_ISO:
		len = (data[3]<<8 | data[2]) + HCI_ISO_HDR_SIZE;
		break;
	default:
		return -EINVAL;
	}

	uart_poll_out(uart_dev, type);
	bqb_uart_send(data, len);

	return 0;
}

static uint16_t bqb_uart_read(uint8_t *rx_data, uint16_t size)
{
	uint16_t i;

	for (i = 0; i < size; i++) {
		uart_poll_in(uart_dev, &rx_data[i]);
	}

	return i;
}

static void bqb_uart_rx_hci(uint8_t *buf)
{
	uint8_t type = 0;
	bool has_hdr = false;
	uint16_t len, rd_len, total = 0;

	uart_poll_in(uart_dev, &type);
	//printk("read type: %d\n", type);

	switch (type) {
	case HCI_CMD:
		rd_len = bqb_uart_read(buf, HCI_CMD_HDR_SIZE);
		if (rd_len == HCI_CMD_HDR_SIZE) {
			has_hdr = true;
			len = buf[2];
		}
		break;
	case HCI_ACL:
		rd_len = bqb_uart_read(buf, HCI_ACL_HDR_SIZE);
		if (rd_len == HCI_ACL_HDR_SIZE) {
			has_hdr = true;
			len = buf[3]<<8 | buf[2];
		}
		break;
	case HCI_SCO:
		rd_len = bqb_uart_read(buf, HCI_SCO_HDR_SIZE);
		if (rd_len == HCI_SCO_HDR_SIZE) {
			has_hdr = true;
			len = buf[2];
		}
		break;
	case HCI_ISO:
		rd_len = bqb_uart_read(buf, HCI_ISO_HDR_SIZE);
		if (rd_len == HCI_ISO_HDR_SIZE) {
			has_hdr = true;
			len = buf[3]<<8 | buf[2];
		}
		break;
	default:
		printk("unknown type: %d\n", type);
		break;
	}

	//printk("read hdr: %d %d\n", rd_len, len);
	if (!has_hdr) {
		return;
	}

	total += rd_len;

	rd_len = bqb_uart_read(buf+rd_len, len);
	if (rd_len != len) {
		printk("uart rx incomplete\n");
		return;
	}

	total += rd_len;
	btdrv_send(type, buf, total);
}

static void bqb_uart_isr(const struct device *dev, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	if (uart_irq_rx_ready(uart_dev)) {
		bqb_uart_rx_hci(tx_data);
	}

	/* Clear the interrupt */
	uart_irq_pending_clear(dev);
}

extern void uart_console_hook_install_dummy(void);
static int bqb_uart_init(void)
{
	struct uart_config cfg;
	/* if use uart1, disable jtag */
	if (!strcmp(CONFIG_BLE_BQB_UART, CONFIG_UART_1_NAME)) {
		sys_write32(sys_read32(JTAG_CTL) & ~(0x1<<31), JTAG_CTL);
	}

#ifdef CONFIG_UART_CONSOLE
	if (!strcmp(CONFIG_BLE_BQB_UART, CONFIG_UART_CONSOLE_ON_DEV_NAME)) {
		uart_console_hook_install_dummy();
	}
#endif

	uart_dev = device_get_binding(CONFIG_BLE_BQB_UART);
	if (uart_dev == NULL) {
		printk("cannot get device %s\n", CONFIG_BLE_BQB_UART);
		return -ENODEV;
	}

	if (!strcmp(CONFIG_BLE_BQB_UART, CONFIG_UART_CONSOLE_ON_DEV_NAME)) {
		uart_config_get(uart_dev, &cfg);
		cfg.baudrate = CONFIG_BLE_BQB_UART_SPEED;
		uart_configure(uart_dev, &cfg);
	}

	uart_irq_callback_set(uart_dev, bqb_uart_isr);
	uart_irq_rx_enable(uart_dev);

	return 0;
}
#endif

static uint8_t *bqb_get_buf(uint8_t type, uint8_t evt, uint16_t exp_len)
{
	if (bqb_mode == BLE_BQB_MODE) {
		rx_type = type;
		return rx_data;
	}
	return NULL;
}

static int bqb_recv(uint16_t len)
{
	if (bqb_mode == BLE_BQB_MODE)
		bqb_uart_tx_hci(rx_type, rx_data);

	return 0;
}


static btdrv_hci_cb_t cb = {
	.get_buf = bqb_get_buf,
	.recv = bqb_recv,
};


struct soc_reg{
    char *name;
    uint32_t addr;
};

const static struct soc_reg regs[] = {
    {"HOSC_CTL", HOSC_CTL},
};

static int hex2int(char *hex_str, uint32_t *dst)
{
    int err;
    uint8_t tmp, i;
    uint32_t val = 0;

    for (i = 0; i < 8; i++) {
        err = char2hex(hex_str[i], &tmp);
        if (err) {
            return -EINVAL;
        }

        val = (val << 4) + (tmp & 0xf);
    }

    *dst = val;
    return 0;
}

static void soc_init(void)
{
    int ret;
    uint32_t val;
    char data[16];

    printk("bqb reg init\n");

    for (uint8_t i = 0; i < ARRAY_SIZE(regs); i++) {
        ret = nvram_config_get_factory(regs[i].name, (void *)data, 16);
        if ((ret == 9) && (hex2int(data, &val) == 0)) {
            printk("write %s: 0x%08x=0x%08x\n", regs[i].name, regs[i].addr, val);
            sys_write32(val, regs[i].addr);
        }
    }
}

int bqb_init(uint8_t mode)
{
	int err;
	bqb_mode = mode;

	soc_init();

	if (bqb_mode == SINGLE_TONE_MODE)
		set_ble_mode(REBOOT_TYPE_GOTO_STM);
	else if (bqb_mode == BLE_BQB_MODE)
		set_ble_mode(REBOOT_TYPE_GOTO_BQB);

	err = btdrv_init(&cb);
	if (err) {
		return -EINVAL;
	}

#ifdef CONFIG_BT_CTRL_ST
	if (bqb_mode == SINGLE_TONE_MODE) {
		single_tone_mode();
		return 0;
	}
#endif

#ifdef CONFIG_BT_CTRL_BLE_BQB
	err = bqb_uart_init();
	if (err) {
		return -EINVAL;
	}
	return 0;
#endif
	return 0;
}
#endif
