/*
 * Copyright (c) 2021 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <device.h>
#include <soc.h>
#include <drivers/ipmsg.h>
#include <sdfs.h>
#include <drivers/nvram_config.h>
#include "bt_rom_cfg.h"
#include "bt_patch_hdr.h"
#include <drivers/cfg_drv/dev_config.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(ipmsg_btc, CONFIG_LOG_DEFAULT_LEVEL);

#define SEND_INT1 BIT(1)
#define SEND_INT0 BIT(0)
#define PD1 BIT(1)
#define PD0 BIT(0)

#define BT_BIN_FILE "bt.bin"
#define BT_RAM_BIN_FILE "bt_ram.bin"

#define BT_PATCH_BIN_FILE "bt_patch.bin"
#define BT_ST_PATCH_BIN_FILE "bt_pst.bin"

#define BT_PATCH_BIN_FILE_1 "bt_pat_1.bin"
#define BT_ST_PATCH_BIN_FILE_1 "bt_pst_1.bin"

#define BT_ROM_ADDR (0x00200000)
//#define BT_RAM_ADDR (0x10e000)

#define TX_POWER_MAX_DBM       10
#define TX_POWER_MIN_DBM       -20
#define TX_POWER_DEFAULT_DBM   0
#define TX_POWER_ADJUST_SCALE  ((TX_POWER_MAX_DBM - TX_POWER_MIN_DBM)/2 + 1)

struct ipmsg_btc_config {
	void *mem_base;
	uint32_t mem_size;
};

struct ipmsg_btc_data {
	ipmsg_callback_t btc_cb;
};

DEVICE_DECLARE(btc);

#define CONFIG_HARDWARE_GET_RANDOM	1
static uint8_t default_pre_mac[3] = {0x50, 0xC0, 0xF0};

//tx_power[-20, 10]dbm, every 2dbm increase
static const uint16_t rf_tx_power_array[TX_POWER_ADJUST_SCALE] = {
0x80c5, 0x8105, 0x8145, 0x8185,
0x8205, 0x8285, 0x8305, 0x8405,
0x8505, 0x8685, 0x8985, 0x8504,
0x8684, 0x8884, 0x89c6, 0x8d86
};

static int bt_mac_to_str(const uint8_t *mac, char *str, size_t len)
{
	return snprintk(str, len, "%02X:%02X:%02X:%02X:%02X:%02X",
			mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
}

static int bt_mac_from_str(const char *str, uint8_t *mac)
{
	int i, j;
	uint8_t tmp;

	if (strlen(str) != 17U) {
		return -EINVAL;
	}

	for (i = 5, j = 1; *str != '\0'; str++, j++) {
		if (!(j % 3) && (*str != ':')) {
			return -EINVAL;
		} else if (*str == ':') {
			i--;
			continue;
		}

		mac[i] = mac[i] << 4;

		if (char2hex(*str, &tmp) < 0) {
			return -EINVAL;
		}

		mac[i] |= tmp;
	}

	return 0;
}

static void ipmsg_btc_isr0(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct ipmsg_btc_data *data = dev->data;

	//clear irq pending
	sys_write32(PD0, PENDING_FROM_BT_CPU);

	if (data->btc_cb) {
		data->btc_cb(NULL, 0);
	}
}

static void ipmsg_btc_isr1(void *arg)
{
	//clear irq pending
	sys_write32(PD1, PENDING_FROM_BT_CPU);
}

static int sd_load(const char *filename, void *dst)
{
	struct sd_file *sdf;
	int ret;

	sdf = sd_fopen(filename);
	if (!sdf) {
		return -EINVAL;
	}

  ret = sd_fread(sdf, dst, sdf->size);
	LOG_INF("%s size: %d, load: %d", filename, sdf->size, ret);

	if (ret == sdf->size) {
		sd_fclose(sdf);
		return 0;
	} else {
		sd_fclose(sdf);
		LOG_ERR("load %s failed!", filename);
		return -EINVAL;
	}
}

#ifdef CONFIG_SD_FS
#ifdef CONFIG_HARDWARE_GET_RANDOM
uint32_t hardware_randomizer_gen_rand(void)
{
	uint32_t trng_low, trng_high;

	se_trng_init();
	se_trng_process(&trng_low, &trng_high);
	se_trng_deinit();

	return trng_low;
}
#endif

void update_bt_mac(struct bt_rom_cfg_t *bt_rom_cfg)
{
	int ret;
	uint8_t i;
	uint32_t rand_val;
	uint8_t mac[6], mac_str[18];

	ret = nvram_config_get("BT_MAC", mac_str, sizeof(mac_str));
	if (ret < 18) {
		LOG_WRN("get BT_MAC failed (ret %d)", ret);
#if defined(CONFIG_HARDWARE_GET_RANDOM)
		rand_val = hardware_randomizer_gen_rand();
#else
		rand_val = k_cycle_get_32();
#endif
		memcpy(&mac, &rand_val, 3);
		for (i = 0; i < 3; i++) {
			mac[5-i] = default_pre_mac[i];
		}

		/* save mac to nvram */
		bt_mac_to_str(mac, mac_str, sizeof(mac_str));
		LOG_INF("gen BT_MAC: %s", mac_str);
		nvram_config_set_factory("BT_MAC", mac_str, sizeof(mac_str));

		goto set_mac;
	}

	mac_str[17] = '\0';
	LOG_INF("get BT_MAC: %s", mac_str);

	ret = bt_mac_from_str(mac_str, mac);
	if (ret) {
		LOG_INF("invalid bt mac: %s", mac_str);
		return;
	}

set_mac:
	memcpy(bt_rom_cfg->co_default_bdaddr, mac, 6);
}

static uint32_t bt_patch_addr;
void update_bt_ram_layout(uint16_t bt_patch_size)
{
	struct bt_rom_cfg_t *bt_rom_cfg = (struct bt_rom_cfg_t *)BT_RAM_ADDR;
	
	bt_rom_cfg->heap_env_off = bt_patch_addr + ROUND_UP(bt_patch_size, 4);
	bt_rom_cfg->s3_save_ram_start = bt_rom_cfg->heap_env_off;
	bt_rom_cfg->s3_save_ram_start += bt_rom_cfg->heap_env_size + bt_rom_cfg->heap_msg_size;

	/* rc32k calibration threshhold */
	#if CONFIG_RC32K_CALIB
	bt_rom_cfg->lfrc_calibrate_diff = CONFIG_RC32K_CALIB_LEV;
	#else
	bt_rom_cfg->lfrc_calibrate_diff = 0;
	#endif
}

void update_tx_power(int dbm)
{
	struct bt_rom_cfg_t *bt_rom_cfg = (struct bt_rom_cfg_t *)BT_RAM_ADDR;

	sys_write32(sys_read32(CMU_SYSCLK) | (0x1<<26), CMU_SYSCLK);

	if((dbm >= TX_POWER_MIN_DBM) && (dbm <= TX_POWER_MAX_DBM))
		bt_rom_cfg->rf_ldo_ant = rf_tx_power_array[(dbm - TX_POWER_MIN_DBM)/2];
	else if(dbm < TX_POWER_MIN_DBM)
		bt_rom_cfg->rf_ldo_ant = rf_tx_power_array[0];
	else
		bt_rom_cfg->rf_ldo_ant = rf_tx_power_array[TX_POWER_ADJUST_SCALE - 1];

	LOG_INF("update_tx_power: %ddbm, rf_ldo_ant: %x", dbm, bt_rom_cfg->rf_ldo_ant);
	sys_write32(sys_read32(CMU_SYSCLK) & ~(0x1<<26), CMU_SYSCLK);
}

static uint16_t ble_mode;
int set_ble_mode(uint16_t mode)
{
	ble_mode = mode;
	return 0;
}

uint16_t get_ble_mode(void)
{
	return ble_mode;
}

static int load_bt_rom_cfg(void)
{
	int err = 0;
	struct bt_rom_cfg_t *bt_rom_cfg;

	err = sd_load(BT_RAM_BIN_FILE, (void *)BT_RAM_ADDR);
	if (err) {
		return -EINVAL;
	}

	bt_rom_cfg = (struct bt_rom_cfg_t *)BT_RAM_ADDR;

	update_bt_mac(bt_rom_cfg);

	bt_rom_cfg->rbuf_base = INTER_RAM_ADDR;
	bt_rom_cfg->heap_msg_size -= 256;
	bt_rom_cfg->print_en = 0;
	bt_rom_cfg->ext_scan = 0;

	if ((get_ble_mode() == REBOOT_TYPE_GOTO_BQB) || (get_ble_mode() == REBOOT_TYPE_GOTO_STM))
		bt_rom_cfg->force_light_sleep = 1;
	else
		bt_rom_cfg->force_light_sleep = 0;

	LOG_INF("force_light_sleep=%d", bt_rom_cfg->force_light_sleep);

#ifdef CONFIG_ACTIONS_BT_DEBUG
	*((volatile unsigned int*)(0x40068200)) = 0x1e;
	*((volatile unsigned int*)(0xc4068300)) = 0;
	*((volatile unsigned int*)(0x40068204)) = *((volatile unsigned int*)(0x40068204)) & (0xffff0f80);
	*((volatile unsigned int*)(0x40068208)) = *((volatile unsigned int*)(0x40068208)) | (0x0000f07f);
#endif

	bt_rom_cfg->rf_ldo_ant = rf_tx_power_array[(TX_POWER_DEFAULT_DBM - TX_POWER_MIN_DBM)/2];

	return 0;
}

static int load_bt_rom_patch(const char *patch_name)
{
	struct bt_patch_hdr_t bt_patch_hdr;
	struct sd_file *sdf;
	int i, ret;

	sdf = sd_fopen(patch_name);
	if (!sdf) {
		return -EINVAL;
	}

	ret = sd_fread(sdf, &bt_patch_hdr, sizeof(struct bt_patch_hdr_t));
	if (ret != sizeof(struct bt_patch_hdr_t)) {
		ret = -EINVAL;
		goto out;
	}

	ret = sd_fseek(sdf, sizeof(struct bt_patch_hdr_t), FS_SEEK_SET);
	if (ret < 0) {
		ret = -EINVAL;
		goto out;
	}

	bt_patch_addr = bt_patch_hdr.addr - BT_RAM_ADDR;

	ret = sd_fread(sdf, (void *)bt_patch_hdr.addr, bt_patch_hdr.len);
	if (ret != bt_patch_hdr.len) {
		ret = -EINVAL;
		goto out;
	}
	LOG_INF("%s build:%d, addr:0x%x, size: %d, load: %d", patch_name, bt_patch_hdr.entry, bt_patch_hdr.addr, bt_patch_hdr.len, ret);
	ret = 0;

	for (i=0; i<L1_TABLE_CNT_MAX; i++) {
		if (bt_patch_hdr.L1_table_bitmap & (1<<i)) {
			LOG_INF("%d,0x%x-->0x%x", i, bt_patch_hdr.L1_table[i], bt_patch_hdr.L1_table_addr + i*4);
			sys_write32(bt_patch_hdr.L1_table[i], bt_patch_hdr.L1_table_addr + i*4);
		}
	}
	
	update_bt_ram_layout(bt_patch_hdr.len);
out:
	sd_fclose(sdf);

	return ret;
}
#endif	/* CONFIG_SD_FS */

static int ipmsg_btc_load(struct device *dev, void *data, uint32_t size)
{
	int err = 0;
	uint8_t rc32k_param;
	char bt_pst_name[13];
	char bt_patch_name[13];

#ifndef CONFIG_SD_FS
	const struct ipmsg_btc_config *config = dev->config;
#endif

	acts_reset_peripheral(RESET_ID_BT);

	sys_write32(sys_read32(CMU_SYSCLK) | (0x1<<26), CMU_SYSCLK);

#ifdef CONFIG_SD_FS
	err = load_bt_rom_cfg();
	if (err) {
		goto out;
	}

	rc32k_param = acts_clock_get_rc32k_param();
	LOG_INF("rc32k_param=%d", rc32k_param);

	if (rc32k_param == 0) {
		strcpy(bt_patch_name, BT_PATCH_BIN_FILE);
		strcpy(bt_pst_name, BT_ST_PATCH_BIN_FILE);
	} else if (rc32k_param == 1) {
		strcpy(bt_patch_name, BT_PATCH_BIN_FILE_1);
		strcpy(bt_pst_name, BT_ST_PATCH_BIN_FILE_1);
	}
	LOG_INF("bt_pst_name:%s, bt_patch_name:%s", bt_pst_name, bt_patch_name);

	if ((get_ble_mode() == REBOOT_TYPE_GOTO_BQB) || (get_ble_mode() == REBOOT_TYPE_GOTO_STM)) {
		err = load_bt_rom_patch(bt_pst_name);
	} else {
		err = load_bt_rom_patch(bt_patch_name);
	}

	if (err) {
		goto out;
	}
#else
	memcpy(config->mem_base, data, size);
#endif

out:
	sys_write32(sys_read32(CMU_SYSCLK) & ~(0x1<<26), CMU_SYSCLK);
	return err;
}

/* Start BT CPU */
static inline int ipmsg_btc_start(struct device *dev, uint32_t *send_id, uint32_t *recv_id)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(send_id);
	ARG_UNUSED(recv_id);

	acts_reset_peripheral(RESET_ID_CPU1);

	irq_enable(IRQ_ID_BTCPU0);

	return 0;
}

static int ipmsg_btc_stop(struct device *dev)
{
	ARG_UNUSED(dev);
	
	// disable irq
	irq_disable(IRQ_ID_BTCPU0);
	irq_disable(IRQ_ID_BTCPU1);

	sys_write32(sys_read32(PENDING_FROM_BT_CPU), PENDING_FROM_BT_CPU);
	
	irq_clear_pending(IRQ_ID_BTCPU0);
	irq_clear_pending(IRQ_ID_BTCPU1);
	
	// reset btc
	acts_reset_peripheral_assert(RESET_ID_BT);
	acts_reset_peripheral_assert(RESET_ID_CPU1);

	return 0;
}

static int ipmsg_btc_notify(struct device *dev)
{
	ARG_UNUSED(dev);

	// send interrupt to btc

	while((sys_read32(INT_TO_BT_CPU) & 0x1) == 0x1);
	sys_write32(1, INT_TO_BT_CPU);

	return 0;
}

static void ipmsg_btc_register_callback(struct device *dev,
					 ipmsg_callback_t cb, void *context)
{
	struct ipmsg_btc_data *data = dev->data;

	data->btc_cb = cb;
}

static int ipmsg_btc_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* btc irq init */
	IRQ_CONNECT(IRQ_ID_BTCPU0, CONFIG_BTC_IRQ0_PRI,
			ipmsg_btc_isr0, DEVICE_GET(btc), 0);
	IRQ_CONNECT(IRQ_ID_BTCPU1, CONFIG_BTC_IRQ0_PRI,
			ipmsg_btc_isr1, DEVICE_GET(btc), 0);

	return 0;
}

static const struct ipmsg_driver_api ipmsg_btc_driver_api = {
	.load = ipmsg_btc_load,
	.start = ipmsg_btc_start,
	.stop = ipmsg_btc_stop,
	.notify = ipmsg_btc_notify,
	.register_callback = ipmsg_btc_register_callback,
};

static const struct ipmsg_btc_config ipmsg_btc_cfg = {
	.mem_base = (void *)BTC_REG_BASE,
	.mem_size = 0x40000,
};

static struct ipmsg_btc_data ipmsg_btc_dat;

#ifdef CONFIG_PM_DEVICE
static int btc_acts_pm_control(const struct device *device,
			       uint32_t command,
			       void *context, device_pm_cb cb, void *arg)
{
	int ret = 0;
	uint32_t state = *((uint32_t*)context);

	LOG_DBG("command:0x%x state:%d", command, state);

	if (command != DEVICE_PM_SET_POWER_STATE)
		return 0;

	switch (state) {
	case DEVICE_PM_ACTIVE_STATE:
		LOG_DBG("btc active");
		break;
	case DEVICE_PM_SUSPEND_STATE:
		LOG_DBG("btc suspend");
		break;
	case DEVICE_PM_EARLY_SUSPEND_STATE:
	case DEVICE_PM_LATE_RESUME_STATE:
	case DEVICE_PM_LOW_POWER_STATE:
	case DEVICE_PM_OFF_STATE:
		break;
	default:
		ret = -ESRCH;
	}

	return ret;
}
#else
#define btc_acts_pm_control NULL
#endif

DEVICE_DEFINE(btc, CONFIG_BTC_NAME, ipmsg_btc_init, btc_acts_pm_control,
	&ipmsg_btc_dat, &ipmsg_btc_cfg, POST_KERNEL,
	CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &ipmsg_btc_driver_api);
