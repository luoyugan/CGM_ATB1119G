/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file reboot configuration macros for Actions SoC
 */

#ifndef	_ACTIONS_SOC_PM_H_
#define	_ACTIONS_SOC_PM_H_

#define PARTITION_TABLE_ADDR    0x4000c064  /* RTC REMAIN1 */
#define REBOOT_REASON_ADDR		0x4000c068  /* RTC REMAIN2 & RTC REMAIN3 */
#define REBOOT_REASON_MAGIC     0x544f4252

#define REBOOT_TYPE_NORMAL		0x000
#define REBOOT_TYPE_GOTO_ADFU   0x1000
#define REBOOT_TYPE_GOTO_DTM    0x1200
#define REBOOT_TYPE_GOTO_OTA	0x1300
#define REBOOT_TYPE_GOTO_APP    0x1400
#define REBOOT_TYPE_GOTO_BQB	0x1500
#define REBOOT_TYPE_GOTO_STM	0x1600  /* single tone mode */

struct reboot_reason {
	unsigned int magic;
	unsigned int reason;
};

void sys_pm_reboot(int type);
int sys_pm_get_reboot_reason(void);

/* The system enters the poweroff state, with key as the only wake-up source.
 * The power consumption in this state is very low, about 100na.
 * NOTICE:
 *  Before entering this state, ensure that the battery voltage is above 2.2V.
 */
void sys_pm_poweroff(void);
/* The system enters the sw poweroff state, with lpcmp as the only wake-up source.
 * The power consumption in this state is low, about 1.x ua.
 * NOTICE:
 *  When the battery voltage is lower than 1.8V(configured by the software), the system will enter this state. Only when the battery voltage exceeds 2.15V, the system will wake up and start (the same as powering on the system).
 *  In this state, ensure that the battery voltage is above 2.2V.
 */
void sys_pm_sw_poweroff(void);

void sys_pm_check_sw_poweroff_state(void);
void lpcmp_acts_isr_enable(void);

/*soc_sleep.c*/
void soc_deep_sleep(void);
void soc_light_sleep(void);
void soc_exit_light_sleep(void);
int set_deepsleep_mode(unsigned char mode);
void set_deepsleep_log_level(unsigned char level);

/*soc_pm.c*/
void sys_pm_enter_deep_sleep(void);
void sys_pm_enter_light_sleep(void);

typedef int (*hw_low_power_notify)(void);
int register_hw_low_power_notify(hw_low_power_notify notify);

#endif /* _ACTIONS_SOC_PM_H_	*/
