/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Bluetooth driver for Actions SoC
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/__assert.h>
#include <stdbool.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/ipmsg.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <drivers/bluetooth/bt_drv.h>
#include "bt_rom_cfg.h"

#ifdef CONFIG_PROPERTY
#include <property_manager.h>
#endif

#include <logging/log.h>
LOG_MODULE_REGISTER(bt_drv, CONFIG_LOG_DEFAULT_LEVEL);

#define CONFIG_HCI_RX_THREAD 1

#ifdef CONFIG_BT_HCI_TX_PRINT
static const char *tx_type_str[] = {
	"NULL:",
	"TX:01 ",
	"TX:02 ",
	"TX:03 ",
	"TX:04 "
};
#endif

#ifdef CONFIG_BT_HCI_RX_PRINT
static const char *rx_type_str[] = {
	"NULL:",
	"RX:01 ",
	"RX:02 ",
	"RX:03 ",
	"RX:04 ",
};
#endif

#define BTC_BIN_ADDR (0x08100000)
#define BTC_BIN_SIZE (0x34000)

enum {
	BT_CPU_ENABLE,
	BT_CPU_READY,
	BT_CPU_NUM_FLAGS,
};

static struct {
	struct device *dev;
	ATOMIC_DEFINE(flags, BT_CPU_NUM_FLAGS);
	btdrv_hci_cb_t *hci_cb;
	struct k_sem ready_sem;
	struct k_sem hci_rx_sem;
	#ifdef CONFIG_RC32K_CALIB
	struct k_sem rc32k_calib_sem __attribute__((aligned(4)));
	#endif
	/* message send to bt cpu */
	rbuf_msg_t msg;
	uint32_t msg_tx_id;
	uint32_t msg_rx_id;
	uint32_t hci_tx_id;
	uint32_t hci_rx_id;
	uint32_t log_rx_id;
} btc = {
	.ready_sem = Z_SEM_INITIALIZER(btc.ready_sem, 0, 1),
	.hci_rx_sem = Z_SEM_INITIALIZER(btc.hci_rx_sem, 0, UINT_MAX),
	#ifdef CONFIG_RC32K_CALIB
	.rc32k_calib_sem = Z_SEM_INITIALIZER(btc.rc32k_calib_sem, 0, 1),
	#endif
};

#if CONFIG_HCI_RX_THREAD
static K_THREAD_STACK_DEFINE(rx_thread_stack, CONFIG_BT_HCI_RX_STACK_SIZE);
static struct k_thread rx_thread_data;
#endif

/* custom hci buf header */
struct bt_hci_hdr {
	/* length of hci packet, round up to 4-byte */
	uint16_t len;
	/* hci packet type */
	uint8_t type;
	uint8_t rfu;
} __packed;

#define BT_HCI_HDR_SIZE sizeof(struct bt_hci_hdr)

/* hci rx/tx data transmit on rbuf */
static struct hci_data {
	/* hci header */
	union {
		struct bt_hci_hdr hdr;
		uint8_t hdr_buff[BT_HCI_HDR_SIZE];
	};
	/* hci packet, Core 5.2 [Vol 4, Part E, 5.4] */
	uint8_t *pkt;
	/* length of hci packet */
	uint16_t pkt_len;
	/* total length of hci data, round up to 4-byte */
	uint16_t total;
	/* current offset of hci packet */
	uint16_t offset;
	/* need to copy header if false */
	uint8_t hdr_len;
	bool has_hdr;
	bool buf_get;
} rx, tx;

static int print_hex(const char *prefix, const uint8_t *data, int size)
{
	int n = 0;

	if (!size) {
		printk("%s zero-length signal packet\n", prefix);
		return 0;
	}

	if (prefix) {
		printk("%s", prefix);
	}

	while (size--) {
		printk("%02x ", *data++);
		n++;
		if (n % 16 == 0) {
			printk("\n");
		}
	}

	if (n % 16) {
		printk("\n");
	}

	return 0;
}

#ifdef CONFIG_BT_RBUF_DUMP
static void rbuf_dump(char *str, rbuf_t *buf, bool dump)
{
	uint16_t len;
	uint8_t *data;

	LOG_INF("%s tmp head %u, head %u, tmp tail %u, tail %u",
		str, buf->tmp_head, buf->head, buf->tmp_tail, buf->tail);

	if (!dump) {
		return;
	}

	if (buf->head < buf->tail) {
		len = buf->tail - buf->head;
		data = (uint8_t *)RBUF_FR_OF(buf->buf_off + buf->head);
		print_hex("Data: ", data, len);
	} else if (buf->head > buf->tail) {
		len = buf->size - buf->head;
		data = (uint8_t *)RBUF_FR_OF(buf->buf_off + buf->head);
		print_hex("Data: ", data, len);
		len = buf->tail;
		data = (uint8_t *)RBUF_FR_OF(buf->buf_off);
		print_hex(NULL, data, len);
	}
}

void btdrv_rbuf_dump(void)
{
	rbuf_t *buf;

	buf = RBUF_FR_OF(btc.hci_tx_id);
	rbuf_dump("HCI TX buf:", buf, true);

	buf = RBUF_FR_OF(btc.hci_rx_id);
	rbuf_dump("HCI RX buf:", buf, true);
}
#endif

static unsigned int rbuf_put_data(uint32_t id, void *data, uint16_t len)
{
	uint32_t size;
	void *buf = NULL;

	if (!data || !len) {
		return 0;
	}

	buf = rbuf_put_claim(RBUF_FR_OF(id), len, &size);
	if (!buf) {
		return 0;
	}

	memcpy(buf, data, MIN(len, size));
	rbuf_put_finish(RBUF_FR_OF(id), size);

	return size;
}

static unsigned int rbuf_get_data(uint32_t id, void *dst, uint16_t len)
{
	uint32_t size;
	void *buf = NULL;

	if (!dst || !len) {
		return 0;
	}

	buf = rbuf_get_claim(RBUF_FR_OF(id), len, &size);
	if (!buf) {
		return 0;
	}

	memcpy(dst, buf, MIN(len, size));
	rbuf_get_finish(RBUF_FR_OF(id), size);

	return size;
}

#if CONFIG_RC32K_CALIB
void rc32k_acts_calib_event(void)
{
	struct bt_rom_cfg_t *bt_rom_cfg = (struct bt_rom_cfg_t *)BT_RAM_ADDR;

	LOG_DBG("handle msg_32k_calibration");

	/* calibrate rc32k */
	while(acts_clock_rc32k_calibrate() < 0);

	/* unlock btc */
	bt_rom_cfg->force_light_sleep = 0;

	sys_write32(sys_read32(CMU_SYSCLK) & ~(0x1<<26), CMU_SYSCLK);
}
#endif

static void btc_msg_handler(void)
{
	uint16_t ret;
	rbuf_msg_t rx_msg = {0};
	struct bt_rom_cfg_t *bt_rom_cfg = (struct bt_rom_cfg_t *)BT_RAM_ADDR;

	ret = rbuf_get_data(btc.msg_rx_id, &rx_msg, sizeof(rbuf_msg_t));
	if (ret != sizeof(rbuf_msg_t)) {
		return;
	}

	LOG_DBG("msg type: %x, data size: %u\n", rx_msg.type, rx_msg.data.w[0]);

	switch (rx_msg.type) {
	case MSG_BT_HCI_OK:
		k_sem_give(&btc.ready_sem);
		atomic_set_bit(btc.flags, BT_CPU_READY);
		break;
	case MSG_32K_CALIBRATION:
		#ifdef CONFIG_RC32K_CALIB
		sys_write32(sys_read32(CMU_SYSCLK) | (0x1<<26), CMU_SYSCLK);
		bt_rom_cfg->force_light_sleep = 1;
		k_sem_give(&btc.rc32k_calib_sem);
		#endif
		break;

	default:
		LOG_ERR("unknown msg(type %x)!", rx_msg.type);
		break;
	}
}

/* process hci rx buf after reciving */
static inline void read_complete(void)
{
#ifdef CONFIG_BT_HCI_RX_PRINT
	print_hex(rx_type_str[rx.hdr.type], rx.pkt, rx.pkt_len);
#endif
	if (btc.hci_cb && rx.pkt) {
		btc.hci_cb->recv(rx.pkt_len);
	}

	memset(&rx, 0, sizeof(struct hci_data));

	/* if hci rx rbuf is not empty and hci rx sem is zero, then read again */
	if (!k_sem_count_get(&btc.hci_rx_sem) && ipmsg_pending(btc.hci_rx_id)) {
		k_sem_give(&btc.hci_rx_sem);
	}
}

static uint16_t btdrv_cal_acl_packet_need_len(uint8_t *hdr, uint16_t hdr_size, uint16_t exp_len)
{
	uint16_t handle, need_len, l2cap_len;
	uint8_t flags;

	handle = (hdr[1] <<8 | hdr[0]);
	flags = (handle >> 12) & 0xF;

	if (flags == BT_ACL_HDL_FLAG_START) {
		if (hdr_size >= 6) {
			l2cap_len = (hdr[5] << 8 | hdr[4]);
			need_len = l2cap_len + HCI_ACL_HDR_SIZE + HCI_L2CAP_HEAD_SIZE;
			return (need_len > exp_len)? need_len : exp_len;
		} else {
			if (BT_MAX_RX_ACL_LEN) {
				if (exp_len > BT_MAX_RX_ACL_LEN) {
					/* exp_len large than BT_MAX_RX_ACL_LEN, maybe AAC packet,
					 * but can't calculate AAC packet need length, just use AAC max length.
					 */
					return (L2CAP_BR_MAX_MTU_A2DP_AAC + HCI_ACL_HDR_SIZE + HCI_L2CAP_HEAD_SIZE);
				} else {
					return (BT_MAX_RX_ACL_LEN > exp_len)? BT_MAX_RX_ACL_LEN : exp_len;
				}
			} else {
				/* default buffer lengh */
				return 0;
			}
		}
	} else {
		return exp_len;
	}
}

static void get_rx_buf(uint8_t *hdr, uint16_t hdr_size)
{
	uint8_t evt = 0;
	uint16_t need_buf_len;

	switch (rx.hdr.type) {
	case HCI_ACL:
		rx.pkt_len = (hdr[3]<<8 | hdr[2]) + HCI_ACL_HDR_SIZE;
		need_buf_len = btdrv_cal_acl_packet_need_len(hdr, hdr_size, rx.pkt_len);
		break;
	case HCI_SCO:
		rx.pkt_len = hdr[2] + HCI_SCO_HDR_SIZE;
		need_buf_len = rx.pkt_len;
		break;
	case HCI_EVT:
		evt = hdr[0];
		rx.pkt_len = hdr[1] + HCI_EVT_HDR_SIZE;
		need_buf_len = rx.pkt_len;
		break;
	case HCI_ISO:
		rx.pkt_len = (hdr[3]<<8 | hdr[2]) + HCI_ISO_HDR_SIZE;
		need_buf_len = rx.pkt_len;
		break;
	default:
		rx.pkt_len = 0;
		LOG_ERR("get_rx_buf unknow type %d !\n", rx.hdr.type);
		return;
	}

	if (btc.hci_cb && btc.hci_cb->get_buf) {
		rx.pkt = btc.hci_cb->get_buf(rx.hdr.type, evt, need_buf_len);
	}
}

static inline int read_hdr(void)
{
	uint32_t size = 0;
	void *data = NULL;
	uint8_t read_size = BT_HCI_HDR_SIZE - rx.hdr_len;

	if (rx.has_hdr) {
		return rx.hdr_len;
	}

	data = rbuf_get_claim(RBUF_FR_OF(btc.hci_rx_id), read_size, &size);
	if (!data) {
		LOG_DBG("get buf header size %d", size);
#ifdef CONFIG_BT_RBUF_DUMP
		//rbuf_dump("HCI RX buf:", RBUF_FR_OF(btc.hci_rx_id), false);
#endif
		return 0;
	}

	if (size != read_size) {
		LOG_INF("get buf header failed read %d size %d", read_size, size);
	}

	memcpy(&rx.hdr_buff[rx.hdr_len], data, size);
	rx.hdr_len += size;
	rbuf_get_finish(RBUF_FR_OF(btc.hci_rx_id), size);

	if (rx.hdr_len == BT_HCI_HDR_SIZE) {
		rx.total = rx.hdr.len;
		rx.has_hdr = true;
	}

	return rx.hdr_len;
}

static inline int read_buf(void)
{
	unsigned int size = 0;
	void *data = NULL;
	uint16_t cpy_len = 0;

	data = rbuf_get_claim(RBUF_FR_OF(btc.hci_rx_id), rx.total, &size);
	if (!data) {
		LOG_INF("alloc hci rx rbuf failed total %d(%d) size %d", rx.total, rx.hdr.len, size);
		return 0;
	}

	if (!rx.pkt && !rx.buf_get) {
		get_rx_buf((uint8_t *)data, (uint16_t)size);
		rx.buf_get = true;
	}

	if (rx.pkt) {
		cpy_len = MIN(rx.pkt_len - rx.offset, size);
		memcpy(rx.pkt + rx.offset, data, cpy_len);
		rx.offset += cpy_len;
	} else {
		LOG_ERR("read_buf total %d drop data %d!\n", rx.total, size);
	}

	rbuf_get_finish(RBUF_FR_OF(btc.hci_rx_id), size);
	rx.total -= size;
	return size;
}

#if CONFIG_HCI_RX_THREAD
static void rx_thread(void *p1, void *p2, void *p3)
{
	unsigned int size = 0;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_INF("rx thread started");

	while (1) {
		k_sem_take(&btc.hci_rx_sem, K_FOREVER);
		//uint32_t cycle = k_cycle_get_32();

		if (!ipmsg_pending(btc.hci_rx_id)) {
			continue;
		}

		if (read_hdr() != BT_HCI_HDR_SIZE) {
			continue;
		}

		do {
			size = read_buf();
		} while (size && rx.total);

		if (!rx.total) {
			read_complete();
		} else {
			if (rx.pkt) {
				LOG_INF("read hci rx buf incompletely");
			} else {
				LOG_INF("read hci rx buf NULL");
			}
		}

		//LOG_DBG("[BT] rx cycle: %u\n", k_cycle_get_32() - cycle);
		k_yield();
	}
}
#endif

static inline unsigned int send_buf(void)
{
	uint8_t *buf = NULL;
	uint16_t copy_len;
	unsigned int size = 0;

	buf = rbuf_put_claim(RBUF_FR_OF(btc.hci_tx_id), tx.total, &size);
	if (buf == NULL) {
		LOG_ERR("alloc hci tx rbuf failed");
		return 0;
	}

	if (!tx.has_hdr) {
		memcpy(buf, &tx.hdr, BT_HCI_HDR_SIZE);
		buf += BT_HCI_HDR_SIZE;
		copy_len = MIN(size - BT_HCI_HDR_SIZE, tx.pkt_len - tx.offset);
		tx.has_hdr = true;
	} else {
		copy_len = MIN(size, tx.pkt_len - tx.offset);
	}

	memcpy(buf, tx.pkt + tx.offset, copy_len);
	tx.offset += copy_len;

	rbuf_put_finish(RBUF_FR_OF(btc.hci_tx_id), size);
	tx.total -= size;

	return size;
}

/* data fmt: hdr(4) + payload(n) */
int btdrv_send(uint8_t type, uint8_t *data, uint16_t len)
{
	int err = 0;
	unsigned int size = 0;
	//uint32_t cycle = k_cycle_get_32();

	if (!atomic_test_bit(btc.flags, BT_CPU_READY)) {
		return -ENODEV;
	}

	if (!data || !len) {
		return -EINVAL;
	}

	tx.pkt = data;
	tx.total = ROUND_UP(len, 4) + BT_HCI_HDR_SIZE;
	tx.hdr.len = tx.total - BT_HCI_HDR_SIZE;
	tx.hdr.type = type;
	tx.pkt_len = len;

	do {
		size = send_buf();
	} while (size && tx.total);

	if (!tx.total) {
		ipmsg_notify(btc.dev);
#ifdef CONFIG_BT_HCI_TX_PRINT
		print_hex(tx_type_str[tx.hdr.type], tx.pkt, tx.pkt_len);
#endif
	} else {
		err = -ENOBUFS;
		LOG_ERR("send hci rbuf failed");
	}

	memset(&tx, 0, sizeof(struct hci_data));

	//LOG_DBG("tx cycle: %u\n", k_cycle_get_32() - cycle);
	return err;
}

/* BTC interupt handler */
static void btc_recv_cb(void *context, void *arg)
{
	if (ipmsg_pending(btc.msg_rx_id)) {
		btc_msg_handler();
	}

	if (ipmsg_pending(btc.hci_rx_id)) {
#if CONFIG_HCI_RX_THREAD
		k_sem_give(&btc.hci_rx_sem);
#endif
	}
}

static void set_mac(void)
{
	int ret;
	const uint8_t *default_mac = "50c0f012a3b4";
	uint8_t addr[6], mac_str[13];

#ifdef CONFIG_PROPERTY
	ret = property_get(CFG_BT_MAC, mac_str, 12);
	if(ret < 12) {
		LOG_WRN("property_get CFG_BT_MAC ret: %d", ret);
		memcpy(mac_str, default_mac, 12);
	}
#else
	memcpy(mac_str, default_mac, 12);
#endif

	ret = hex2bin(mac_str, 12, addr, 6);
	if (ret != 6) {
		LOG_ERR("invalid bt address");
		return;
	}

	btc.msg.type = MSG_BT_INIT;
	btc.msg.data.w[0] = addr[2] << 24 | addr[3] << 16 | addr[4] << 8 | addr[5];
	btc.msg.data.w[1] = addr[0] << 8 | addr[1];

	rbuf_put_data(btc.msg_tx_id, &btc.msg, sizeof(rbuf_msg_t));
}

static void send_init_msg(void)
{
	set_mac();

	btc.msg.type = MSG_BT_HCI_BUF;
	btc.msg.data.w[0] = btc.hci_rx_id;  /* bt -> main */
	btc.msg.data.w[1] = btc.hci_tx_id;  /* main -> bt */
	btc.msg.data.w[2] = btc.log_rx_id;  /* bt -> main */
	rbuf_put_data(btc.msg_tx_id, &btc.msg, sizeof(rbuf_msg_t));
}

static void init_rbuf(void)
{
	btc.msg_tx_id = rbuf_msg_create(CPU, BT, RB_MSG_SIZE);
	btc.msg_rx_id = rbuf_msg_create(BT, CPU, RB_MSG_SIZE);
	LOG_INF("msg tx id: %d, msg rx id: %d", btc.msg_tx_id, btc.msg_rx_id);

	btc.hci_rx_id = ipmsg_create(RBUF_RAW, CONFIG_BT_HCI_RX_RBUF_SIZE);
	btc.hci_tx_id = ipmsg_create(RBUF_RAW, CONFIG_BT_HCI_TX_RBUF_SIZE);
	LOG_INF("hci rx id: %d, hci tx id: %d", btc.hci_rx_id, btc.hci_tx_id);

	send_init_msg();
}

int btdrv_init(btdrv_hci_cb_t *cb)
{
	int ret;
	uint8_t irq;

	LOG_INF("bt driver init");

	if (atomic_test_bit(btc.flags, BT_CPU_ENABLE)) {
		goto start_cpu;
	}

	if (!cb) {
		return -EINVAL;
	}
	btc.hci_cb = cb;

	btc.dev = (struct device *)device_get_binding("BTC");
	if (!btc.dev) {
		LOG_ERR("get device BTC failed");
		return -EINVAL;
	}

	irq = IPMSG_BTC_IRQ;
	ipmsg_register_callback(btc.dev, btc_recv_cb, &irq);

	init_rbuf();

#if CONFIG_HCI_RX_THREAD
	/* Start RX thread */
	k_thread_create(&rx_thread_data, rx_thread_stack,
			K_THREAD_STACK_SIZEOF(rx_thread_stack),
			rx_thread, NULL, NULL, NULL,
			K_PRIO_COOP(CONFIG_BT_HCI_RX_PRIO),
			0, K_NO_WAIT);
	k_thread_name_set(&rx_thread_data, "BT HCI RX");
#endif

	atomic_set_bit(btc.flags, BT_CPU_ENABLE);

start_cpu:
	if (atomic_test_bit(btc.flags, BT_CPU_READY)) {
		return 0;
	}

	ret = ipmsg_load(btc.dev, (void *)BTC_BIN_ADDR, BTC_BIN_SIZE);
	if (ret) {
		LOG_ERR("load bt bin failed");
		return -EINVAL;
	}

	/* Start BT CPU */
	ret = ipmsg_start(btc.dev, NULL, NULL);
	if (ret) {
		LOG_ERR("start bt cpu failed");
		return -EINVAL;
	}

	/* BT CPU will let us know when it's ready */
	LOG_INF("wait bt cpu ready...");
	k_sem_take(&btc.ready_sem, K_FOREVER);
	LOG_INF("bt cpu ready");

	return 0;
}

int btdrv_exit(void)
{
	if (!atomic_test_bit(btc.flags, BT_CPU_ENABLE)) {
		return -ENODEV;
	}

	LOG_INF("exit");

	ipmsg_stop(btc.dev);
	btc.hci_cb = NULL;

	// destroy default message queue
	rbuf_msg_destroy(CPU, BT);
	rbuf_msg_destroy(BT, CPU);

	ipmsg_destroy(btc.hci_rx_id);
	ipmsg_destroy(btc.hci_tx_id);
	ipmsg_destroy(btc.log_rx_id);

	atomic_clear(btc.flags);

	return 0;
}

int btdrv_reset(void)
{
	/* Reset BT CPU */
	atomic_clear_bit(btc.flags, BT_CPU_READY);

	ipmsg_stop(btc.dev);

	send_init_msg();

	return btdrv_init(NULL);
}

#ifdef CONFIG_RC32K_CALIB
struct k_sem *btdrv_get_rc32k_calib_sem(void)
{
	return &btc.rc32k_calib_sem;
}
#endif

