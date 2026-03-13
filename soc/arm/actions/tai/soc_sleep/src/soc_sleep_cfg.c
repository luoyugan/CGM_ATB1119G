/*
 * Copyright (c) 2022 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file soc_sleep.c  sleep  for Actions SoC
 */


#include <zephyr.h>
#include <soc.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include <drivers/timer/system_timer.h>
#include <linker/linker-defs.h>
#include <kernel_arch_func.h>

/* sleep wake up source struct */
struct sleep_wk_src_t {
	uint8_t is_en;
	uint8_t irq_id;
};

static const struct sleep_wk_src_t sleep_wk_src_list[] = CONFIG_SLEEP_WAKE_SRC_LIST;
static const struct sleep_wk_src_t sw_s5_wk_src_list[] = CONFIG_SW_S5_WAKE_SRC_LIST;
__acts_sleep_bss static uint32_t uart_ctl, uart_br;

/* The 'white list' of clock source that enabled during s3 stage */
static const u8_t deep_sleep_enable_clock_id[] = {
	CLOCK_ID_UART0,
	CLOCK_ID_SPI0,
	CLOCK_ID_SPI0CACHE,
	CLOCK_ID_GPIO,
	CLOCK_ID_RTC,
	CLOCK_ID_KEY,
};
static const uint32_t sleep_s2_backup_regs[] = {
	NVIC_ISER0,
};
__acts_sleep_bss static uint32_t s2_reg_backups[ARRAY_SIZE(sleep_s2_backup_regs)];

static const uint32_t sleep_s3_backup_regs[] = {
	CMU_DEVCLKEN0,
	RMU_MRCR0,
};
__acts_sleep_bss static uint32_t s3_reg_backups[ARRAY_SIZE(sleep_s3_backup_regs)];

/* NOTICE: DO NOT defined sleep_gpio_backup[] as const */
uint32_t sleep_gpio_backup[] = {
	GPIO_PIN(10), /* SPINOR IO2 */
	GPIO_PIN(11), /* SPINOR IO3 */
	GPIO_PIN(12), /* UART0 TX */
	GPIO_PIN(13), /* UART0 RX */
};

__acts_sleep_bss static uint32_t s3_pin_backups[ARRAY_SIZE(sleep_gpio_backup)];

void sys_pm_backup_pin_state(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(sleep_gpio_backup); i++)
		s3_pin_backups[i] = sys_read32(sleep_gpio_backup[i]);
}

__ramfunc void sys_pm_restore_pin_state(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(sleep_gpio_backup); i++)
		sys_write32(s3_pin_backups[i], sleep_gpio_backup[i]);
}

__ramfunc void sys_pm_reset_pin_state(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(sleep_gpio_backup); i++)
		sys_write32(0, sleep_gpio_backup[i]);
}

__ramfunc void uart_suspend(void)
{
	uart_ctl = sys_read32(UART0_REG_BASE);
	uart_br = sys_read32(UART0_REG_BASE + 0x10);
}

__ramfunc void uart_resume(void)
{
	/* resume uart */
	sys_write32(sys_read32(RMU_MRCR0) | (1 << RESET_ID_UART0), RMU_MRCR0);
	sys_write32(sys_read32(CMU_DEVCLKEN0) | (1 << CLOCK_ID_UART0), CMU_DEVCLKEN0);
	sys_write32(uart_ctl, UART0_REG_BASE);
	sys_write32(uart_br, UART0_REG_BASE + 0x10);
}

void soc_cmu_deep_sleep(void)
{
	/* 1. disable devclk except for 'clock white list' */
	acts_deep_sleep_clock_peripheral(deep_sleep_enable_clock_id,
		ARRAY_SIZE(deep_sleep_enable_clock_id));

	/* 2. set spi nor clock to HOSC/2 = 12MHz */
	//sys_write32((1 << 8) | (1 << 0), CMU_SPI0CLK);
}
void sys_pm_backup_registers(uint32_t *regs_list, uint32_t *regs_backup, uint32_t regs)
{
	int i;
	for (i = 0; i < regs; i++)
		regs_backup[i] = sys_read32(regs_list[i]);
}

void sys_pm_restore_registers(uint32_t *regs_list, uint32_t *regs_backup, uint32_t regs)
{
	int i;
	for (i = regs - 1; i >= 0; i--) {
		sys_write32(regs_backup[i], regs_list[i]);
	}
}

void sys_pm_s2_backup_registers(void)
{
	sys_pm_backup_registers((uint32_t *)sleep_s2_backup_regs, s2_reg_backups, ARRAY_SIZE(sleep_s2_backup_regs));
}

void sys_pm_s2_restore_registers(void)
{
	sys_pm_restore_registers((uint32_t *)sleep_s2_backup_regs, s2_reg_backups, ARRAY_SIZE(sleep_s2_backup_regs));
}

void sys_pm_s3_backup_registers(void)
{
	sys_pm_backup_registers((uint32_t *)sleep_s3_backup_regs, s3_reg_backups, ARRAY_SIZE(sleep_s3_backup_regs));
}

void sys_pm_s3_restore_registers(void)
{
	sys_pm_restore_registers((uint32_t *)sleep_s3_backup_regs, s3_reg_backups, ARRAY_SIZE(sleep_s3_backup_regs));
}

/* If wakeup irq pending has already happened, not enter deep sleep */
bool soc_pmu_check_irq_pending(void)
{
	uint8_t i;

	for (i = 0; i < ARRAY_SIZE(sleep_wk_src_list); i++) {
		if (sleep_wk_src_list[i].is_en) {
			if (sys_read32(NVIC_ICPR0) & BIT(sleep_wk_src_list[i].irq_id))
				return true;
		}
	}

	return false;
}

void soc_pmu_set_wakeup_src(bool force_suspend)
{
	uint8_t i;
	uint32_t wk_en0 = 0;

	sys_write32(sys_read32(NVIC_ISER0), NVIC_ICER0);
	sys_write32(0xffffffff, NVIC_ICPR0);

	if (force_suspend == false) {
		for (i = 0; i < ARRAY_SIZE(sleep_wk_src_list); i++) {
			if (sleep_wk_src_list[i].is_en)
				wk_en0 |= BIT(sleep_wk_src_list[i].irq_id);
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(sw_s5_wk_src_list); i++) {
			if (sw_s5_wk_src_list[i].is_en)
				wk_en0 |= BIT(sw_s5_wk_src_list[i].irq_id);
		}
	}

	sys_write32(wk_en0, NVIC_ISER0);
}
