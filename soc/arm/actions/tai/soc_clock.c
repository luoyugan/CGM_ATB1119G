/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file peripheral clock interface for Actions SoC
 */

#include <kernel.h>
#include <device.h>
#include <soc.h>
#include <stdlib.h>
#include <drivers/nvram_config.h>

#ifndef ABS
#define ABS(x) ((((int)(x)) < 0) ? -(x) : (x))
#endif

#define CMU_RC32K_COARSE_LEVEL_MAX          (0x3ff)
#define CMU_RC32K_COARSE_LEVEL_MAX_BIT_WIDTH (10)
#define CMU_RC32K_ACCURATE_LEVEL_MAX        (0x3f)

#define LFRC_COUNTER_2_RC32K_CYCLE(x, y)    (MHZ(CONFIG_HOSC_CLK_MHZ) * (unsigned int)y / (x))
#define LFRC_COUNTER_2_RC32K(x)             LFRC_COUNTER_2_RC32K_CYCLE(x, 16)
#define LFRC_COUNTER_2_RC32K_128_CYCLE(x)		LFRC_COUNTER_2_RC32K_CYCLE(x, 128)

#define CMU_RC32K_CALIBRATE_PRECISION_PPM   (500)

#define CMU_RC32K_PRECISION_THRESHOLD (RTC_32K_CYCLES_VALUE*CMU_RC32K_CALIBRATE_PRECISION_PPM/1000000)

static K_SEM_DEFINE(rc32k_calib_lock, 1, 1);

enum {
	RC32K_CALIB_NO_BIAS = 0, /* the rc32 calibrated value == 32768Hz */
	RC32K_CALIB_MISS = -1, /* cannot find the calibrated point */
	RC32K_CALIB_FOUND = 1, /* find the calibrated fitness point */
};

extern void cmu_dev_clk_enable(uint32_t id);
extern void cmu_dev_clk_disable(uint32_t id);

static void acts_clock_peripheral_control(int clock_id, int enable)
{
	unsigned int key;

	if (clock_id > CLOCK_ID_MAX_ID)
		return;

	key = irq_lock();

	if (enable) {
		if (clock_id < 32) {
			sys_set_bit(CMU_DEVCLKEN0, clock_id);
		} else {
			sys_set_bit(CMU_DEVCLKEN1, clock_id - 32);
		}
	} else {
		if (clock_id < 32) {
			sys_clear_bit(CMU_DEVCLKEN0, clock_id);
		} else {
			sys_clear_bit(CMU_DEVCLKEN1, clock_id - 32);
		}
	}
	irq_unlock(key);
}

void acts_clock_peripheral_enable(int clock_id)
{
	acts_clock_peripheral_control(clock_id, 1);
}

void acts_clock_peripheral_disable(int clock_id)
{
	acts_clock_peripheral_control(clock_id, 0);
}

static inline unsigned int calc_spi_clk_div(unsigned int max_freq, unsigned int spi_freq)
{
	unsigned int div;

	if (max_freq > spi_freq && max_freq <= (spi_freq * 3 / 2)) {
		/* /1.5 */
		div = 14;
	} else if ((max_freq > 2 * spi_freq) && max_freq <= (spi_freq * 5 / 2)) {
		/* /2.5 */
		div = 15;
	} else {
		/* /n */
		div = (max_freq + spi_freq - 1) / spi_freq - 1;
		if (div > 13) {
			div = 13;
		}
	}

	return div;
}

static void acts_clk_set_rate_spi(int clock_id, unsigned int rate_hz)
{
	unsigned int max_freq, div, reg = 0;

	if (clock_id == CLOCK_ID_SPI0) {
		if (CONFIG_SPI0_CLK_SOURCE == 1)
			max_freq = MHZ(24);
		else
			max_freq = MHZ(64);
	} else if (clock_id == CLOCK_ID_SPI2) {
		if (CONFIG_SPI2_CLK_SOURCE == 1)
			max_freq = MHZ(24);
		else
			max_freq = MHZ(64);
	} else if (clock_id == CLOCK_ID_SPI3) {
		if (CONFIG_SPI3_CLK_SOURCE == 1)
			max_freq = MHZ(24);
		else
			max_freq = MHZ(64);
	} else {
		printk("spi invalid clock_id:%d\n", clock_id);
		return ;
	}

	if (max_freq == MHZ(24))
		reg |= (1 << 8); /* select SPI clock source from HOSC */

	div = calc_spi_clk_div(max_freq, rate_hz);

	reg |= (div << 0);

	sys_write32(reg, CMU_SPI0CLK + ((clock_id - CLOCK_ID_SPI0) * 4));

	/*printk("SPI%d clock setting:0x%x\n", clock_id - CLOCK_ID_SPI0,
			sys_read32(CMU_SPI0CLK + ((clock_id - CLOCK_ID_SPI0) * 4)));*/
}

static void  acts_clk_set_rate_pwm(int clock_id, unsigned int rate_hz)
{
	unsigned int val, real_rate, div, div_temp;

	if(rate_hz <= 32000) {
		div_temp = 1;
		real_rate = 32000;
		div = 32000 / rate_hz;
		for(val = 0; val < 7; val++) {
			if(div <= div_temp)
				break;
			div_temp *= 2;
			real_rate /= 2;
		}
		sys_write32( val, CMU_PWM0CLK + (clock_id - 8)*4);//32k
	} else {
		div_temp = 2;
		real_rate = 12000000;
		div = 12000000 / rate_hz;
		for(val = 0; val < 7; val++) {
			if(div <= div_temp)
				break;
			div_temp *= 2;
			real_rate /= 2;
		}
		sys_write32((1 << 9) | val, CMU_PWM0CLK + (clock_id - 8)*4);//12M
	}
//	printk("pwm group%d: set rate %d Hz, real rate %d Hz\n", clock_id - 8, rate_hz, real_rate);
}

static void  acts_clk_set_rate_keypad(int clock_id, unsigned int rate_hz)
{
	sys_write32(0, CMU_KEYCLK);//32k 0div
}

static void  acts_clk_set_rate_qd(int clock_id, unsigned int rate_hz)
{
	unsigned int val, real_rate, div, div_temp;

	if(rate_hz <= 32000) {
		div_temp = 1;
		real_rate = 32000;
		div = 32000 / rate_hz;
		for(val = 0; val < 7; val++) {
			if(div <= div_temp)
				break;
			div_temp *= 2;
			real_rate /= 2;
		}
		sys_write32( val, CMU_QDCLK);//32k
	} else {
		div_temp = 2;
		real_rate = 12000000;
		div = 12000000 / rate_hz;
		for(val = 0; val < 7; val++) {
			if(div <= div_temp)
				break;
			div_temp *= 2;
			real_rate /= 2;
		}
		sys_write32((1 << 8) | val, CMU_QDCLK);//12M
	}
//	printk("qd: set rate %d Hz, real rate %d Hz\n", rate_hz, real_rate);
}

static void  acts_clk_set_rate_capture(int clock_id, unsigned int rate_hz)
{
	sys_write32(0x101, CMU_CAPCLK);//0div 0div
}

int clk_set_rate(int clock_id,  uint32_t rate_hz)
{
	int ret = 0;
	switch(clock_id) {
	case CLOCK_ID_SPI0:
	case CLOCK_ID_SPI2:
	case CLOCK_ID_SPI3:
		acts_clk_set_rate_spi(clock_id, rate_hz);
		break;
	case CLOCK_ID_CAPTURE:
		acts_clk_set_rate_capture(clock_id, rate_hz);
		break;
	case CLOCK_ID_PWM0:
	case CLOCK_ID_PWM1:
	case CLOCK_ID_PWM2:
	case CLOCK_ID_PWM3:
	case CLOCK_ID_PWM4:
	case CLOCK_ID_PWM5:
		acts_clk_set_rate_pwm(clock_id, rate_hz);
		break;
	case CLOCK_ID_KEY:
		acts_clk_set_rate_keypad(clock_id, rate_hz);
		break;
	case CLOCK_ID_QD:
		acts_clk_set_rate_qd(clock_id, rate_hz);
		break;
	default:
		printk("clkid=%d not support clk set\n",clock_id);
		ret = -1;
		break;
	}
	return 0;
}

uint32_t clk_get_rate(int clock_id)
{
	printk("clkid=%d not support clk get\n",clock_id);
	return 0;
}

#define CK32_HIGH_RES_MEASURE_CYCLES 7	//2^7, 128
#define CK32_LOW_RES_MEASURE_CYCLES 4	//2^4, 16

static uint32_t rtc_rc_step[2];
static uint32_t rtc_rc_cmp_msk[2];

static int acts_rtc_cal_rc_step_msk(uint8_t clksrc, uint32_t lfrc_counter, uint8_t count_cycle)
{
	/* RTC_RC32K_STEP = COUNTRESULT << (7 - COUNTCYCLE)
	 */
	if ((RTC_CLKSRC_SEL_RC32K == clksrc) || RTC_CLKSRC_SEL_LOSC == clksrc) {
		rtc_rc_step[0] = lfrc_counter << (7-count_cycle);
		rtc_rc_cmp_msk[0] = ~((1 << find_msb_set(lfrc_counter / (1<<(count_cycle + 2)))) - 1);
	} else if (RTC_CLKSRC_SEL_RC8K == clksrc) {
		rtc_rc_step[1] = (lfrc_counter * 4) << (7-count_cycle);
		rtc_rc_cmp_msk[1] = ~((1 << find_msb_set(lfrc_counter / (1<<count_cycle))) - 1);
	}

	return 0;
}

/* cmu low frequency calibration and note that HOSC shall be existed */
#define MAX_32KRC_MEASURE_TIMES 3
uint32_t acts_clock_lfrc_rc32k_measure(bool high_res)
{
	uint32_t i;
	uint32_t lfrc_counter[MAX_32KRC_MEASURE_TIMES], reg = 0;
	uint32_t cnt = MAX_32KRC_MEASURE_TIMES, round = 0;
	uint32_t max, min, val, valid_div;

	/* start once lfrc calibration as delay */
	if (high_res)
		reg = (4<<8);
	else 
		reg = (3<<8);

	sys_write32(reg, LFRC_COUNTCTL);
	sys_write32(reg | (1 << 1), LFRC_COUNTCTL);
	/* wait calibration done */
	while (!(sys_read32(LFRC_COUNTCTL) & (1 << 4)));
	/* disable RC calibration */
	sys_write32(0, LFRC_COUNTCTL);
	
	//restart lfrc measure
	if (high_res) {
		reg = CK32_HIGH_RES_MEASURE_CYCLES<<8;
		valid_div = (1<<CK32_HIGH_RES_MEASURE_CYCLES) * 2;
	} else {
		reg = CK32_LOW_RES_MEASURE_CYCLES<<8;
		valid_div = (1<<CK32_LOW_RES_MEASURE_CYCLES) * 2;
	}

	while (1) {
		max = 0;
		min = 0;

		for (i=0; i<cnt; i++) {
			sys_write32(reg, LFRC_COUNTCTL);
			sys_write32(reg | (1 << 1), LFRC_COUNTCTL);
			/* wait calibration done */
			while (!(sys_read32(LFRC_COUNTCTL) & (1 << 4)));
			lfrc_counter[i] = sys_read32(LFRC_COUNTER);
			/* disable RC calibration */
			sys_write32(0, LFRC_COUNTCTL);

			if((max == 0) || (max < lfrc_counter[i]))
				max=lfrc_counter[i];
			if((min == 0) || (min > lfrc_counter[i]))
				min=lfrc_counter[i];
		}
		round++;
		if((max-min) < (min/valid_div)) {
			//printk("calibration again:0x%x-0x%x, 0x%x < 0x%x\n", max, min, max-min, min/valid_div);
			break;
		} else {
			printk("round[%d]:0x%x > 0x%x\n", round, max-min, min/valid_div);
			for(i=0; i<cnt; i++)
				printk("lfrc_counter[%d]:0x%x\n", i,lfrc_counter[i]);
		}
	}

	val = 0;
	for(i=0; i<cnt; i++){
		val += lfrc_counter[i];
	}
	val /= cnt;

	//printk("RC32k clock result:0x%x\n", val);

	return val;
}

static inline int acts_clock_lfrc_rc32k_measure_set_param(uint16_t coarse_level, uint16_t accurate_level)
{
	uint32_t rc32k_ctl = sys_read32(RC32K_CTL);

	sys_write32((rc32k_ctl & 0x0000ffff) | (coarse_level << 22) | (accurate_level << 16), RC32K_CTL);

	return 0;
}

int acts_clock_lfrc_rc32k_measure_result(bool high_res, uint32_t *lfrc_counter, uint32_t *lfrc_freq)
{
  *lfrc_counter = acts_clock_lfrc_rc32k_measure(high_res);

	if (high_res)
		*lfrc_freq = LFRC_COUNTER_2_RC32K_128_CYCLE(*lfrc_counter);
	else
		*lfrc_freq = LFRC_COUNTER_2_RC32K(*lfrc_counter);

	return 0;
}

uint32_t acts_clock_rc32k_measure_result_high_res(void)
{
	uint32_t lfrc_counter, lfrc_freq;

	/* if calibration and measurement all comes, changing of RC32K_CTL in calibration may affect measurement. */
	k_sem_take(&rc32k_calib_lock, K_FOREVER);

	acts_clock_lfrc_rc32k_measure_result(1, &lfrc_counter, &lfrc_freq);

	k_sem_give(&rc32k_calib_lock);

	return lfrc_freq;
}

//#define CONFIG_RC32K_STATS
#define RC32K_STATS_COUNTER 500

void acts_clock_rc32k_stats(void)
{
	int i;
	uint32_t rc32k_freq, abs, ov=0;

	for (i=0; i<RC32K_STATS_COUNTER; i++) {
		rc32k_freq = acts_clock_rc32k_measure_result_high_res();
		abs = ABS(rc32k_freq - RTC_32K_CYCLES_VALUE);
		if (abs > CMU_RC32K_PRECISION_THRESHOLD) {
			ov++;
			printk("%d-[%d]=%d,abs=%d\n", ov, i, rc32k_freq, abs);
		}
	}

	while(ov > 0);
}

int rc32k_coarse_calibrate(uint16_t coarse_start_level, uint16_t accurate_level, \
						uint16_t *out_coarse_level, uint32_t *out_lfrc_counter, uint32_t *out_lfrc_freq)
{
	int i;
	uint16_t found_coarse_level = 0, diff = 0;
	uint16_t coarse_level = coarse_start_level;
	uint32_t lfrc_counter, lfrc_freq;

	for (i=CMU_RC32K_COARSE_LEVEL_MAX_BIT_WIDTH-1; i>=0; i--) {

		acts_clock_lfrc_rc32k_measure_set_param(coarse_level, accurate_level);
		acts_clock_lfrc_rc32k_measure_result(0, &lfrc_counter, &lfrc_freq);

		if ((found_coarse_level == 0) || (ABS(RTC_32K_CYCLES_VALUE - lfrc_freq) < diff)) {
			diff = ABS(RTC_32K_CYCLES_VALUE - lfrc_freq);
			found_coarse_level = coarse_level;
		}

		if (i == 0)
			break;

		if (lfrc_freq < RTC_32K_CYCLES_VALUE) {
			coarse_level |= 1<<(i-1);
		} else if (lfrc_freq > RTC_32K_CYCLES_VALUE) {
			coarse_level &= ~(1<<i);
			coarse_level |= 1<<(i-1);
		} else {
			*out_coarse_level = coarse_level;
			*out_lfrc_counter = lfrc_counter;
			*out_lfrc_freq = lfrc_freq;
			return RC32K_CALIB_NO_BIAS;
		}
	}

	__ASSERT(found_coarse_level, "rc32k coarse error");

	if (found_coarse_level) {
		coarse_level = found_coarse_level;
	}

	*out_coarse_level = coarse_level;

	return RC32K_CALIB_FOUND;
}

int rc32k_accurate_calibrate(uint16_t accurate_start_level, uint16_t coarse_level, \
							uint16_t *out_accurate_level, uint32_t *out_lfrc_counter, uint32_t *out_lfrc_freq)
{
	uint32_t lfrc_counter, lfrc_freq;
	bool found = false;
	uint16_t accurate_level = accurate_start_level;
	uint16_t diff = 0, direct;
	uint16_t max_accurate_cnt = (CMU_RC32K_ACCURATE_LEVEL_MAX + 1) / 2;

	/* 2. increase or decrease accurate level  */
	found = false;

	acts_clock_lfrc_rc32k_measure_set_param(coarse_level, accurate_level);
	acts_clock_lfrc_rc32k_measure_result(1, &lfrc_counter, &lfrc_freq);

	/* init output value */
	*out_accurate_level = accurate_start_level;
	*out_lfrc_counter   = lfrc_counter;
	*out_lfrc_freq      = lfrc_freq;

	if (lfrc_freq > RTC_32K_CYCLES_VALUE) {
		direct = 0;
	} else if (lfrc_freq < RTC_32K_CYCLES_VALUE) {
		direct = 1;
	} else {
		*out_accurate_level = accurate_level;
		*out_lfrc_counter = lfrc_counter;
		*out_lfrc_freq = lfrc_freq;
		return RC32K_CALIB_NO_BIAS;
	}

	do{
		if (direct == 0) {
			if(accurate_level == 0x00){
				accurate_level = CMU_RC32K_ACCURATE_LEVEL_MAX;
			}
			else{
				accurate_level--;
			}
		} else {
			if(accurate_level == CMU_RC32K_ACCURATE_LEVEL_MAX){
				accurate_level = 0;
			}
			else{
				accurate_level++;
			}
		}		

		acts_clock_lfrc_rc32k_measure_set_param(coarse_level, accurate_level);
		acts_clock_lfrc_rc32k_measure_result(1, &lfrc_counter, &lfrc_freq);

		if (lfrc_freq > RTC_32K_CYCLES_VALUE) {
			if (direct == 1) {
				if (lfrc_freq - RTC_32K_CYCLES_VALUE > diff) {
					--accurate_level;
					/* calibration of the second to last is optimal, need take effect */
					acts_clock_lfrc_rc32k_measure_set_param(coarse_level, accurate_level);
				}
				found = true;
				*out_accurate_level = accurate_level;
				*out_lfrc_counter = lfrc_counter;
				*out_lfrc_freq = lfrc_freq;
				break;
			} else {
				diff = lfrc_freq - RTC_32K_CYCLES_VALUE;
			}
		} else if (lfrc_freq < RTC_32K_CYCLES_VALUE) {
			if (direct == 0) {
				if (RTC_32K_CYCLES_VALUE - lfrc_freq > diff) {
					++accurate_level;
					/* calibration of the second to last is optimal, need take effect */
					acts_clock_lfrc_rc32k_measure_set_param(coarse_level, accurate_level);
				}
				found = true;
				*out_accurate_level = accurate_level;
				*out_lfrc_counter = lfrc_counter;
				*out_lfrc_freq = lfrc_freq;
				break;
			} else {
				diff = RTC_32K_CYCLES_VALUE - lfrc_freq;
			}
		} else {
			*out_accurate_level = accurate_level;
			*out_lfrc_counter = lfrc_counter;
			*out_lfrc_freq = lfrc_freq;
			return RC32K_CALIB_NO_BIAS;
		}
	} while(--max_accurate_cnt);

	if(found == false) {
		/* if not found, rc32k param returns to the started accurate_level */
		acts_clock_lfrc_rc32k_measure_set_param(coarse_level, accurate_start_level);
		return RC32K_CALIB_MISS;
	}
	else {
		return RC32K_CALIB_FOUND;
	}
}

int acts_clock_rc32k_calibrate(void)
{
	static uint16_t coarse_level = (CMU_RC32K_COARSE_LEVEL_MAX + 1) / 2;
	static uint16_t accurate_level = (CMU_RC32K_ACCURATE_LEVEL_MAX + 1) / 2;
	uint32_t start_time = k_cycle_get_32();
	uint32_t lfrc_counter, lfrc_freq;
	uint16_t level;
	int ret;
	static uint8_t coarse_flag = 1;
	bool calib_success = true;

	k_sem_take(&rc32k_calib_lock, K_FOREVER);

	start_time = k_cycle_get_32();
	
	if(coarse_flag == 1){
		coarse_level = (CMU_RC32K_COARSE_LEVEL_MAX + 1) / 2;
		accurate_level = (CMU_RC32K_ACCURATE_LEVEL_MAX + 1) / 2;		
		ret = rc32k_coarse_calibrate(coarse_level, accurate_level, &coarse_level, &lfrc_counter, &lfrc_freq);
		if(ret == RC32K_CALIB_NO_BIAS){
			goto out;
		}
	}

	ret = rc32k_accurate_calibrate(accurate_level, coarse_level, &level, &lfrc_counter, &lfrc_freq);
	if(ret != RC32K_CALIB_MISS){
		accurate_level = level;
		coarse_flag = 0;
		goto out;
	}

	coarse_flag = 1;
	calib_success = false;
	printk("rc32k accurate correction failed\n");

out:
	printk("%s %dus\n", __func__, k_cyc_to_us_floor32(k_cycle_get_32() - start_time));

	printk("0x%x-0x%x, rc32k:%d, abs:%d\n", coarse_level, accurate_level, lfrc_freq, ABS(lfrc_freq - RTC_32K_CYCLES_VALUE));

	__ASSERT(ABS(lfrc_freq - RTC_32K_CYCLES_VALUE) * 1000000 \
				/ RTC_32K_CYCLES_VALUE < CMU_RC32K_CALIBRATE_PRECISION_PPM, "rc32k error");

#ifdef CONFIG_RC32K_STATS
	acts_clock_rc32k_stats();
#endif

	acts_rtc_cal_rc_step_msk(RTC_CLKSRC_SEL_RC32K, lfrc_counter, CK32_HIGH_RES_MEASURE_CYCLES);

	k_sem_give(&rc32k_calib_lock);

	if (!calib_success)
		return -1;
	else
		return 0;
}

int acts_rtc_setup_rc32k_step_msk(void)
{
	struct acts_rtc *rtc_base = (struct acts_rtc *)RTC_BASE;
	
	rtc_base->rtc_rc32k_step = rtc_rc_step[0];
	rtc_base->rtc_cmp_msk = rtc_rc_cmp_msk[0];

 	return 0;
 }

int acts_clock_rc32k_measure_then_calibrate(void)
{
	uint32_t rc32k_freq, abs;

	rc32k_freq = acts_clock_rc32k_measure_result_high_res();
	abs = ABS(rc32k_freq - RTC_32K_CYCLES_VALUE);
	printk("rc32k_freq=%d,abs=%d\n", rc32k_freq, abs);

	if (abs > CMU_RC32K_PRECISION_THRESHOLD) {
		acts_clock_rc32k_calibrate();
	}
	return 0;
}


#define RC32K_PARAM_THRESHOLD 10000
static uint8_t rc32k_param_flag;

uint8_t acts_clock_get_rc32k_param(void)
{
	return rc32k_param_flag;
}

static int acts_clock_test_and_set_rc32k_param(void)
{
	uint32_t reg = 0;
	uint32_t lfrc_cnt;

	sys_write32(sys_read32(RC32K_CTL) | 0x1, RC32K_CTL);
	sys_write32(reg, LFRC_COUNTCTL);

	sys_write32(reg | (1 << 1), LFRC_COUNTCTL);
	/* wait calibration done */
	while (!(sys_read32(LFRC_COUNTCTL) & (1 << 4)));

	lfrc_cnt = sys_read32(LFRC_COUNTER);
	/* disable RC calibration */
	sys_write32(0, LFRC_COUNTCTL);
	sys_write32(sys_read32(RC32K_CTL) & ~0x1, RC32K_CTL);

	if (lfrc_cnt > RC32K_PARAM_THRESHOLD) {
		rc32k_param_flag = 1;
	} else {
		rc32k_param_flag = 0;
	}
	return 0;
}

void acts_deep_sleep_clock_peripheral(const char *id_buffer, int buffer_len)
{
    int i;
    int devclk0, devclk1;

    devclk0 = 0;
    devclk1 = 0;

    for(i = 0; i < buffer_len; i++){
        if(id_buffer[i] >= 32){
            devclk1 |= (1 << (id_buffer[i] - 32));
        }else{
            devclk0 |= (1 << id_buffer[i]);
        }
    }

	printk("sleep keep devclk0:0x%x devclk1:0x%x\n", devclk0, devclk1);

    sys_write32(devclk0, CMU_DEVCLKEN0);
    sys_write32(devclk1, CMU_DEVCLKEN1);
}

void acts_clock_rc64m_calibrate(void)
{
	uint32_t reg;

	/* RC64M shall be calibrated by software or will only get about 20MHz */
	sys_write32(0xAC034, RC64M_CTL);
	while (!(sys_read32(RC64M_CTL) & (1 << 24)));

	reg = sys_read32(RC64M_CTL);
	reg = ((reg & (0x3f << 25)) >> 25) - 1;

	sys_write32((reg << 6) | 0xAC004, RC64M_CTL);
}

static int soc_clock_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	acts_clock_rc64m_calibrate();

	if ((sys_read32(CHIPVERSION) & 0xf) != RC32K_FILTER7)
		sys_write32(sys_read32(RC32K_CTL) | (0x7<<4), RC32K_CTL);

	sys_write32(0x8830, CMU_ANADEBUG);
	k_busy_wait(10);
	sys_write32(0, CMU_ANADEBUG);

	printk("soc clock initialized\n");
	printk("RC64M_CTL=0x%x, RC32K_CTL=0x%x\n", sys_read32(RC64M_CTL), sys_read32(RC32K_CTL));
	return 0;
}

static int soc_clock_init_later(const struct device *dev)
{
	int err;
	uint32_t osc_val = 0;
	char str[4] = {0};

	ARG_UNUSED(dev);

	err = nvram_config_get("OSC_CFG", &str, sizeof(str));
	if (err > 0) {
		osc_val = strtoul(str, NULL, 10);
		if (osc_val) goto done;
	}
	printk("get nvram osc failed (err %d)\n", err);

	err = get_efuse_hosc_trim(&osc_val);
	if (err == 0 && osc_val)
		goto done;
	printk("get efuse osc failed (err %d)\n", err);

	osc_val = 8;  /* osc default value */
done:
	sys_write32((sys_read32(HOSC_CTL) & ~(0x3f<<8)) | (osc_val<<8), HOSC_CTL);
	printk("HOSC_CTL: %x\n", sys_read32(HOSC_CTL));

	/* RC32K calibration */
	acts_clock_rc32k_calibrate();

	acts_clock_test_and_set_rc32k_param();

	printk("soc clock later initialized\n");
	return 0;
}

SYS_INIT(soc_clock_init, PRE_KERNEL_1, 20);
SYS_INIT(soc_clock_init_later, PRE_KERNEL_2, 80);
