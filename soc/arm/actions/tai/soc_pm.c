/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system reboot interface for Actions SoC
 */

#include <device.h>
#include <init.h>
#include <soc.h>
#include <power/power.h>
#include "policy/pm_policy.h"
#include <drivers/power_supply.h>
#include <drivers/bluetooth/bt_drv.h>

#define CONFIG_LOW_BATT_LPCMP_CHAN	5

/*
REF_VAL		VOLTAGE(v)
	15				2.4
	16				2.258824
	17				2.133333
	18				2.021053
	19				1.92
	20				1.828571
	21				1.745455
	22				1.669565
	23				1.6
*/
#define CONFIG_LOW_BATT_LPCMP_HIGH_REF_FOR_WAKEUP 17
#define CONFIG_LOW_BATT_LPCMP_LOW_REF_TO_SW_POWEROFF 20

static int lpcmp_acts_set_ch(uint8_t chan, uint8_t ref_high, uint8_t ref_low, uint8_t trig);

static hw_low_power_notify lpcmp_low_power_notify;
int register_hw_low_power_notify(hw_low_power_notify notify)
{
	lpcmp_low_power_notify = notify;
	
	return 0;
}

/*
**  system software power off
*/
void sys_pm_sw_poweroff(void)
{
#ifdef CONFIG_ACTIONS_PRINTK_DMA
	printk_dma_switch(0);
#endif

	printk("system software power down\n");

	//reset bt cpu/subsystem
	btdrv_exit();
	
	//set lpcmp ch to wakeup soc from sw_s5 when iovcc is higher than the upper threshold
	lpcmp_acts_set_ch(CONFIG_LOW_BATT_LPCMP_CHAN, CONFIG_LOW_BATT_LPCMP_HIGH_REF_FOR_WAKEUP, CONFIG_LOW_BATT_LPCMP_HIGH_REF_FOR_WAKEUP, 0);

	//force system into suspend state with only 1 wakesrc(lpcmp int)
	pm_policy_force_suspend_state();
}

/*
**  check sw poweroff state
*/
void sys_pm_check_sw_poweroff_state(void)
{
	if (pm_policy_get_force_suspend_state()) {
		printk("sw poweroff fail, need reboot!\n");
		sys_pm_reboot(0);
	}
}

/*
**  system power off
*/
void sys_pm_poweroff(void)
{
	unsigned int key;

#ifdef CONFIG_ACTIONS_PRINTK_DMA
	printk_dma_switch(0);
#endif

	printk("system power down\n");

	key = irq_lock();

	while(1) {
#if CONFIG_PM
			if (pm_poweroff_devices()) {
				goto poweroff_fail;
			}
#endif
		/* gpio_clk:8K */
		sys_write32(1, CMU_GPIOCLK);
		/* enter S5 */
		sys_write32(1, POWER_CTL);
		/* wait 10ms */
		k_busy_wait(10000);

poweroff_fail:
		printk("poweroff fail, need reboot!\n");
		sys_pm_reboot(0);
	}

	/* never return... */
}

void sys_pm_reboot(int type)
{
	unsigned int key;
	struct reboot_reason *reason;

#ifdef CONFIG_ACTIONS_PRINTK_DMA
	printk_dma_switch(0);
#endif
	printk("system reboot, type 0x%x!\n", type);

	key = irq_lock();
	/* store reboot reason in RTC_REMAIN2 & RTC_REMAIN3 for bootloader */
	reason = (struct reboot_reason *)REBOOT_REASON_ADDR;
	reason->magic = REBOOT_REASON_MAGIC;
	reason->reason = type & ~0xff;

	k_busy_wait(500);
	sys_write32(0x5f, WD_CTL);
	while (1) {
		;
	}

	/* never return... */
}

int sys_pm_get_reboot_reason(void)
{
	struct reboot_reason *reason = (struct reboot_reason *)REBOOT_REASON_ADDR;
	return reason->reason;
}

/* control the ram region to power down or rentetion */
static void soc_pmu_ram_power_control(void)
{
	sys_write32(sys_read32(RAMPWR_CTL)
		| CONFIG_RAMPWR_CTL_MAIN_DOMAIN
		| CONFIG_RAMPWR_CTL_BT_DOMAIN, RAMPWR_CTL);
	sys_write32(CONFIG_RAMPWR_SET_RETENTION, RAMPWR);
}

static int soc_pm_init(const struct device *arg)
{
	uint8_t vddret_cal07 = 0;
	uint32_t reg;

	/* set VDD_SET_S1BT/VDD_SET_S1M 1.0v */
	sys_write32((sys_read32(VOUT_CTL_S1) & ~0xff) | (0x99), VOUT_CTL_S1);

	/* disable vc18/vd12_ldo in s3 */
	sys_write32(0, VOUT_CTL_S3);

	sys_write32(0x6bcae000, DCDC_CTL); //DCDC mode
	sys_write32((sys_read32(VOUT_CTL_S1) & ~(0xff<<8)) | (0x33<<8), VOUT_CTL_S1); //VD12=1.2
#ifndef CONFIG_PM_VOUT_LDO_MODE
	sys_write32((sys_read32(VOUT_CTL_S1) & ~(0x3<<20)) | (0x2<<20), VOUT_CTL_S1); //DCDC ON
#endif

	reg = sys_read32(VREF_CTL);

	/* VDDRET voltage = 0.7v */
	reg &= ~(1 << VREF_CTL_VDDRET_VOL);

	/* VDDRET calibration level */
	get_efuse_vddret_cal07(&vddret_cal07);
	if(vddret_cal07 > 0x0d)
		vddret_cal07 = 0x0f;
	else
		vddret_cal07 += 2;

	reg &= ~(0xf << 8);
	reg |= vddret_cal07<<8;
	//reg &= ~(3 << 9);

	/* disable VREF06_BUF debug */
	reg &= ~(1 << VREF_CTL_VREF06_BUF);

	sys_write32(reg, VREF_CTL);

	if ((sys_read32(CHIPVERSION) & 0xf) == RC32K_FILTER7) {
		sys_write32(sys_read32(SYSTEM_CTL) | (1<<7), SYSTEM_CTL); //IOVCC DIV/4
	} else {
	#ifdef CONFIG_PM_ENHANCE_S3VDDRET_VOL
		sys_write32(sys_read32(VREF_CTL) | (1<<16), VREF_CTL); //VDDRET_VOL 0.8V
		sys_write32((sys_read32(VREF_CTL) & ~(0xf<<12)) | (0xb<<12), VREF_CTL); //VDDRET_CAL08 0xb
	#endif
	}

	soc_pmu_ram_power_control();

	printk("%s\n", __func__);

	return 0;
}

SYS_INIT(soc_pm_init, PRE_KERNEL_1, 20);

#if CONFIG_LPCMP

static void lpcmp_acts_irq_config(void);

struct lpcmp_acts_config {
	u32_t	base;
	void (*irq_config_func)(void);
};

static const struct lpcmp_acts_config lpcmp_acts_config = {
	.base = PMU_REG_BASE,
	.irq_config_func = lpcmp_acts_irq_config,
};

void lpcmp_acts_isr(void *arg)
{
	uint32_t val;
	int ret = -1;

	val = sys_read32(PMU_PD);
	printk("%s,PMU_PD:0x%x\n", __func__, val);

	/* check sw poweroff state */
	sys_pm_check_sw_poweroff_state();

	if (val & (1<<PMU_PD_CMP_CH_PD(CONFIG_LOW_BATT_LPCMP_CHAN))) {
		sys_write32(sys_read32(LPCMP_INTMASK) & ~(1<<LPCMP_INTMASK_CMP_CH_INTEN(CONFIG_LOW_BATT_LPCMP_CHAN)), LPCMP_INTMASK);
		do {
			sys_write32((1<<PMU_PD_CMP_CH_PD(CONFIG_LOW_BATT_LPCMP_CHAN)), PMU_PD);
		} while(sys_read32(PMU_PD) & (1<<PMU_PD_CMP_CH_PD(CONFIG_LOW_BATT_LPCMP_CHAN)));

		if (lpcmp_low_power_notify)
			ret = lpcmp_low_power_notify();

		if (ret)
			sys_write32(sys_read32(LPCMP_INTMASK) | (1<<LPCMP_INTMASK_CMP_CH_INTEN(CONFIG_LOW_BATT_LPCMP_CHAN)), LPCMP_INTMASK);
	}
}

void lpcmp_acts_isr_enable(void)
{
	sys_write32(sys_read32(LPCMP_INTMASK) | (1<<LPCMP_INTMASK_CMP_CH_INTEN(CONFIG_LOW_BATT_LPCMP_CHAN)), LPCMP_INTMASK);
}

static int lpcmp_acts_init(const struct device *dev)
{
	const struct lpcmp_acts_config *cfg = dev->config;

	/* disable ch6 int */
	sys_write32(sys_read32(LPCMP_INTMASK) & ~(1<<LPCMP_INTMASK_CMP_CH_INTEN(6)), LPCMP_INTMASK);

	/* clear pending */
	sys_write32(sys_read32(PMU_PD), PMU_PD);
	k_busy_wait(100);

	/* disable ch6 */
	sys_write32(sys_read32(LPCMP_CTL) & ~(1<<LPCMP_CTL_CH_EN(6)), LPCMP_CTL);

	lpcmp_acts_set_ch(CONFIG_LOW_BATT_LPCMP_CHAN, CONFIG_LOW_BATT_LPCMP_LOW_REF_TO_SW_POWEROFF, CONFIG_LOW_BATT_LPCMP_LOW_REF_TO_SW_POWEROFF, 1);

	cfg->irq_config_func();

	return 0;
}

DEVICE_DEFINE(lpcmp_acts, CONFIG_LPCMP_NAME,
			lpcmp_acts_init, 
			device_pm_control_nop,
			NULL, &lpcmp_acts_config,
			APPLICATION, CONFIG_LPCMP_INIT_PRIORITY, NULL);

static void lpcmp_acts_irq_config(void)
{
	irq_clear_pending(IRQ_ID_LPCMP);
	IRQ_CONNECT(IRQ_ID_LPCMP, CONFIG_LPCMP_IRQ_PRI,
			lpcmp_acts_isr,
			DEVICE_GET(lpcmp_acts), 0);
	irq_enable(IRQ_ID_LPCMP);
}

static int lpcmp_acts_set_ch(uint8_t chan, uint8_t ref_high, uint8_t ref_low, uint8_t trig)
{
	printk("%s\n", __func__);

	sys_write32(sys_read32(VOUT_CTL_S3) | (1<<VOUT_CTL_S3_BDG1V2_EN_S3), VOUT_CTL_S3);

	//set lpcmp debounce
	sys_write32(3<<LPCMP_CTL_DEBOUNCE_SET_SHIFT, LPCMP_CTL);

	/* set ch compare threshold */
	sys_write32(0x1 | (trig<<LPCMP_CH_TRIG_SET_SHIFT)  | (ref_high<<LPCMP_CH_REF_TH_HIGH_SHIFT) | (ref_low<<LPCMP_CH_REF_TH_LOW_SHIFT), LPCMP_CH(chan));

	/* disable ch int for low battery detect */
	sys_write32(sys_read32(LPCMP_INTMASK) & ~(1<<LPCMP_INTMASK_CMP_CH_INTEN(chan)), LPCMP_INTMASK);

	/* clear pending */
	sys_write32(sys_read32(PMU_PD), PMU_PD);
	k_busy_wait(100);

	/* enable ch int for low battery detect */
	sys_write32(sys_read32(LPCMP_INTMASK) | (1<<LPCMP_INTMASK_CMP_CH_INTEN(chan)), LPCMP_INTMASK);

	/* enable lpcmp and ch */
	sys_write32(sys_read32(LPCMP_CTL) | (1<<LPCMP_CTL_LPCMP_EN) | (1<<LPCMP_CTL_CH_EN(chan)), LPCMP_CTL);

	return 0;
}

#endif
