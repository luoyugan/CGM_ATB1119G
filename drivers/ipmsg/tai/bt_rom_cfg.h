/*
 * Copyright (c) 2021 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef __BT_ROM_CFG_H_
#define __BT_ROM_CFG_H_

#include <string.h>             // used for memset.
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define BT40_VERSION 	(6)
#define BT41_VERSION 	(7)
#define BT42_VERSION 	(8)
#define BT50_VERSION 	(9)
#define BT51_VERSION 	(10)

#define BT_RAM_ADDR (0x10e000)

struct bt_rom_cfg_t {
    uint8_t MAGIC[4];
    uint8_t version;
    uint8_t size;
    uint8_t rsv[2];

    // offset:8, fast boot 
    uint32_t resume_sp;
    uint32_t resume_addr;

    // offset:16, ram boot,
    uint32_t ram_boot_addr;
    uint32_t other_bt_func;

    // offset:24, exchange memory between host and controller
    uint32_t rbuf_base;

    // offset:28, bt mac address
    uint8_t co_default_bdaddr[6];

    // offset:34
    uint16_t bt_version;
    uint16_t bt_subversion;
    uint16_t cmp_id;

    // offset:40
    uint8_t print_en:1;
    uint8_t mpu_en:1;
    uint8_t fast_boot:1;
    uint8_t force_light_sleep:1;
    uint8_t wdg_en:1;
    uint8_t dyn_tx_pwr:1;
    uint8_t fix_llcp_enc:1;
    uint8_t bt_not_request_ahb:1;

    uint8_t wdg_cfg;
    uint8_t wdg_reload_time;

    uint8_t normal_cpu_clk;
    uint8_t wakeup_cpu_clk;
    uint8_t bb_clk;

    // offset:46
    uint16_t lfrc_min;
    uint16_t lfrc_max;
    uint16_t lfrc_ctl;

    // offset:52, heap offset & size, start = off + 0x10e000
    uint16_t heap_env_off;
    uint16_t heap_env_size;
    uint16_t heap_msg_off;
    uint16_t heap_msg_size;
    uint16_t heap_non_ret_off;
    uint16_t heap_non_ret_size;

    // offset:64, stack top = off + 0x10e000
    uint16_t main_stack_top_off;

    // offset:66, rand seed
    uint32_t seed;

    // ble ip
    uint32_t ctrim;
    uint8_t rxoff_spi_dis;
    uint8_t sleep_enable;
    uint8_t ext_wakeup_enable;
    uint16_t twext;
    uint16_t twosc;
    uint16_t twrm;
    uint8_t prog_delay;

    // rf rssi threshold ('real' signed value in dBm)
    int8_t rssi_high_thr;
    int8_t rssi_low_thr;
    int8_t rssi_interf_thr;

    // Default Scan / Inq / Page event duration (in half-us)
    uint16_t scan_evt_dur_dft;

    // Extended scanning is enabled by default
    bool ext_scan;

    // Channel assessment
    bool ch_ass_en;

    uint8_t act_move_cfg;

    // Need to get a 32 Byte random number - which we will use as the secret key.
    // We then ECC multiply this by the Base Points, to get a new Public Key.
    bool forced_key;
    uint8_t secret_key256[32];

    /// Default MD bit used by slave when sending a data packet on a BLE connection (set to 1 helps receiving the ACK in the same event)
    uint8_t dft_slave_md;

    /// Maximum value of the local active clock drift (in ppm)
    uint8_t   local_drift_active;

    /// Maximum value of the local clock drift (in ppm)
    uint16_t  local_drift;

    /// rf ldo_ant, tx power related
    uint16_t  rf_ldo_ant;

    /// LE Coded PHY 500 Kbps selection (false: 125 Kbps, true: 500 Kbps)
    bool    le_coded_phy_500;

    /// rf rx timing related
    uint16_t  rf_tim_align;

    /// s3 retention ram for saving cpu/bb/rf regs
    uint16_t    s3_save_ram_start;

	/// rc32k calibration
	/* 11->500mmp. 0->close rc32k calib */
	uint8_t	lfrc_calibrate_diff;
};



#endif // __BT_ROM_CFG_H_
