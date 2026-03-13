/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SARADC for Actions SoC
 */

#ifndef SOC_SARADC_H_
#define SOC_SARADC_H_

#include <soc.h>
#include <kernel.h>

#define SARADC_IOVCC2VOL     saradc_iovcc2vol

/* rtc clock source selection */
typedef enum {
  IOVCC_CH = 0,
  SENSOR_CH,
  LRADC0_CH,
  LRADC1_CH,
  LRADC2_CH,
  LRADC3_CH,
  LRADC4_CH,
  LRADC5_CH
} acts_saradc_ch_sel;

static inline int saradc_iovcc2vol(uint32_t x)
{
	if ((sys_read32(CHIPVERSION) & 0xf) == RC32K_FILTER7)
		return (x * 4800 / 65536);
	else
		return (x * 3600 / 65536);
}

/* IOVCC ADC for battery sampling channel enable */
static inline void soc_saradc_iovcc_enable(void)
{
	uint32_t reg = sys_read32(SARADC_CTL);

	reg |= (BIT(SARADC_CTL_IOVCC_CHEN) | BIT(SARADC_CTL_SARADC_EN));

	sys_write32(reg, SARADC_CTL);

	sys_write32(1<<SARADC_PD_IOVCC_PD, SARADC_PD);
}

/* sensor ADC for temperature sampling channel enable */
static inline void soc_saradc_sensor_enable(void)
{
	uint32_t reg = sys_read32(SARADC_CTL);

	reg |= (BIT(SARADC_CTL_SENSOR_CHEN) | BIT(SARADC_CTL_SARADC_EN));

	sys_write32(reg, SARADC_CTL);
}

static inline void delay_for_saradc_clk(int cnt);
/* saradc channel disable */
static inline void soc_saradc_chan_disable(acts_saradc_ch_sel ch)
{
	uint32_t reg;
	unsigned int key;

	key = irq_lock();

	reg = sys_read32(SARADC_CTL);
	reg &= ~(BIT(ch));

	//wait 32k cycle rising egde
	sys_write32(0, LFRC_COUNTCTL);
	sys_write32((1 << 1), LFRC_COUNTCTL);
	/* wait calibration done */
	while (!(sys_read32(LFRC_COUNTCTL) & (1 << 4)));
	sys_write32(0, LFRC_COUNTCTL);

	//delay before disable saradc channel
	delay_for_saradc_clk(2);

	sys_write32(reg, SARADC_CTL);

	irq_unlock(key);
}

/* saradc iovcc disable */
static inline void soc_saradc_iovcc_disable(void)
{
	soc_saradc_chan_disable(IOVCC_CH);
}

/* saradc sensor disable */
static inline void soc_saradc_sensor_disable(void)
{
	soc_saradc_chan_disable(SENSOR_CH);
}

/* get IOVCC ADC sampling result */
static inline int soc_saradc_iovcc_data(void)
{
	uint32_t val = sys_read32(IOVCCADC_DATA);

	return SARADC_IOVCC2VOL(val);
}

/* get sensor ADC sampling result */
static inline int soc_saradc_sensor_data(void)
{
	return sys_read32(SENSADC_DATA);
}

static inline void delay_for_saradc_clk(int cnt)
{
	volatile int i;

	//600ns
	for (i=0; i<cnt; i++) {
	 __asm__ volatile ("nop");
	 __asm__ volatile ("nop");
	}
}

static inline int soc_saradc_reset(void)
{
	uint32_t reg = 0;
	uint32_t saradc_ctl_bak = sys_read32(SARADC_CTL);
	unsigned int key;

	key = irq_lock();

	//1-enable saradc and 1 channel before 32k cycle rising edge
	sys_write32(saradc_ctl_bak | (1<<SARADC_CTL_SARADC_EN), SARADC_CTL);

	//2-wait 32k cycle rising egde
	sys_write32(reg, LFRC_COUNTCTL);
	sys_write32(reg | (1 << 1), LFRC_COUNTCTL);
	/* wait calibration done */
	while (!(sys_read32(LFRC_COUNTCTL) & (1 << 4)));
	sys_write32(0, LFRC_COUNTCTL);

	//3-delay before disable saradc
	delay_for_saradc_clk(2);

	//4-disable saradc,ADC_EN=0
	sys_write32(saradc_ctl_bak & ~(1<<SARADC_CTL_SARADC_EN), SARADC_CTL);

	//5-delay before enable saradc
	delay_for_saradc_clk(4);

	//6-enable saradc.default ADC_EN=1, A rising edge from ADC_disable to enable is generated.
	sys_write32(saradc_ctl_bak, SARADC_CTL);

	irq_unlock(key);

	return 0;
}

#define SARADC_PENDING_TIMEOUT_CNT_MAX		400
#define SARADC_RESET_CNT_MAX		10

static inline int acts_get_saradc_data(acts_saradc_ch_sel ch)
{
	uint32_t channel_pd, adc_ch_data, wait, reset = 0;
	uint8_t  pd_shift = ch;
	static K_SEM_DEFINE(saradc_sample_lock, 1, 1);

	k_sem_take(&saradc_sample_lock, K_FOREVER);	

	/* clear the last pending */
	sys_write32(1<<pd_shift, SARADC_PD);

	k_busy_wait(50);

	while (1) {

		wait = 0;
		channel_pd = sys_read32(SARADC_PD) & (1<<pd_shift);

		/* wait SENSOR A/D data ready */
		while(!channel_pd && (wait++ < SARADC_PENDING_TIMEOUT_CNT_MAX)) {
			k_busy_wait(50);
			channel_pd = sys_read32(SARADC_PD) & (1<<pd_shift);
		}

		if (wait < SARADC_PENDING_TIMEOUT_CNT_MAX)
			break;

		reset++;
		if (reset >= SARADC_RESET_CNT_MAX)
			break;

		/* clear the last pending */
		sys_write32(1<<pd_shift, SARADC_PD);	

		soc_saradc_reset();
	}

	k_busy_wait(100);

	adc_ch_data = sys_read32(IOVCCADC_DATA + 4*ch);

	k_sem_give(&saradc_sample_lock);

	return adc_ch_data;
}
#endif /* SOC_SARADC_H_ */
