/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file timer for Actions SoC
 */

#ifndef SOC_TIMER_H_
#define SOC_TIMER_H_

#include <soc.h>

/* CMU_RTCCLK */
#define CMU_RTCCLK_CPUDS_WDCLKEN                                (12)
#define CMU_RTCCLK_RTC32KSRC_SHIFT                              (8)
#define CMU_RTCCLK_RTC32KSRC_MASK                               (0x3 << CMU_RTCCLK_RTC32KSRC_SHIFT)
#define CMU_RTCCLK_RTC32KSRC(x)                                 ((x) << CMU_RTCCLK_RTC32KSRC_SHIFT)
#define CMU_RTCCLK_RTCHCLKDIV_SHIFT                             (0)
#define CMU_RTCCLK_RTCHCLKDIV_MASK                              (0x3 << CMU_RTCCLK_RTCHCLKDIV_SHIFT)
#define CMU_RTCCLK_RTCHCLKDIV(x)                                ((x) << CMU_RTCCLK_RTCHCLKDIV_SHIFT)

/* RTC_CTL */
#define RTC_CTL_CK_SRC_STA                                      BIT(31)
#define RTC_CTL_RTC_VAL3_IRQ_PD                                 BIT(30)
#define RTC_CTL_RTC_VAL2_IRQ_PD                                 BIT(29)
#define RTC_CTL_RTC_VAL1_IRQ_PD                                 BIT(28)
#define RTC_CTL_RTC_VAL0_IRQ_PD                                 BIT(27)
#define RTC_CTL_RTC_VAL3_IRQ_EN                                 BIT(26)
#define RTC_CTL_RTC_VAL2_IRQ_EN                                 BIT(25)
#define RTC_CTL_RTC_VAL1_IRQ_EN                                 BIT(24)
#define RTC_CTL_RTC_VAL0_IRQ_EN                                 BIT(23)
#define RTC_CTL_RTC_CMP3_EN                                     BIT(19)
#define RTC_CTL_RTC_CMP2_EN                                     BIT(18)
#define RTC_CTL_RTC_CMP1_EN                                     BIT(17)
#define RTC_CTL_RTC_CMP0_EN                                     BIT(16)
#define RTC_CTL_CNT_UD_MODE                                     BIT(9)
#define RTC_CTL_RTC_CK_SRC                                      BIT(8)
#define RTC_CTL_RTC_MODE                                        BIT(1)
#define RTC_CTL_RTC_EN                                          BIT(0)

#define RTC_CTL_RTC_VAL0_IRQ_SHIFT                              (23)
#define RTC_CTL_RTC_VAL0_IRQ_MASK                               (0xF << RTC_CTL_RTC_VAL0_IRQ_SHIFT)

#define RTC_CTL_RTC_VALX_IRQ_PD_SHIFT                           (27)
#define RTC_CTL_RTC_VALX_IRQ_PD_MASK                            (0xF << RTC_CTL_RTC_VALX_IRQ_PD_SHIFT)

#define RTC_CTL_RTC_CMPX_EN_SHIFT                               (16)
#define RTC_CTL_RTC_CMPX_EN_MASK                                (0xF << RTC_CTL_RTC_CMPX_EN_SHIFT)

#define TIMER_MAX_CYCLES_VALUE                                  (0xfffffffful)
#define HOSC_DIV2_CYCLES_VALUE                                  (MHZ(CONFIG_HOSC_CLK_MHZ) / 2)
#define RTC_32K_CYCLES_VALUE                                    (32768)

#define RTC_MAX_CMP_INDEX                                       (4)

typedef void (*acts_rtc_timer_isr)(void *arg);

/* rtc register */
struct acts_rtc {
	volatile uint32_t rtc_ctl;
	volatile uint32_t rtc_cnt;
	volatile uint32_t rtc_cmp0;
	volatile uint32_t rtc_cmp1;
	volatile uint32_t rtc_cmp2;
	volatile uint32_t rtc_cmp3;
	volatile uint32_t rtc_cmp_msk;
	volatile uint32_t rtc_rc32k_step;
	volatile uint32_t rtc_cnt_tmp0;
	volatile uint32_t rtc_cnt_tmp1;
	volatile uint32_t rtc_cnt_update;
};

/* rtc clock source selection */
typedef enum {
	RTC_CLKSRC_SEL_RC32K = 0,
	RTC_CLKSRC_SEL_LOSC,
	RTC_CLKSRC_SEL_RC8K, /* Only when BT disconnected and the system is going to S3, can set this clock */
	RTC_CLKSRC_SEL_HOSC
} acts_rtc_clksource_sel;

/* set the rtc clock source and the HOSC clock source frequency */
static inline int acts_rtc_set_clk(uint8_t clksrc, uint32_t hosc_hz)
{
	if (RTC_CLKSRC_SEL_RC32K == clksrc) {
		sys_write32(sys_read32(CMU_RTCCLK) & ~CMU_RTCCLK_RTC32KSRC_MASK, CMU_RTCCLK);
	} else if (RTC_CLKSRC_SEL_LOSC == clksrc) {
		/* LOSC enable */
		sys_write32(sys_read32(LOSC_CTL) | (1 << 0), LOSC_CTL);
		sys_write32((sys_read32(CMU_RTCCLK) & ~CMU_RTCCLK_RTC32KSRC_MASK)
					| CMU_RTCCLK_RTC32KSRC(1), CMU_RTCCLK);
	} else if (RTC_CLKSRC_SEL_RC8K == clksrc) {
		sys_write32((sys_read32(CMU_RTCCLK) & ~CMU_RTCCLK_RTC32KSRC_MASK)
					| CMU_RTCCLK_RTC32KSRC(2), CMU_RTCCLK);
	} else if (RTC_CLKSRC_SEL_HOSC == clksrc) {
		/* rtc clock source from HOSC/2
		 * 0: /2
		 * 1: /4
		 * 2: /8
		 * 3: /16
		 */
		if (!hosc_hz) {
			printk("invlaid hosc hz:%d\n", hosc_hz);
			return -EINVAL;
		}

		uint8_t div = HOSC_DIV2_CYCLES_VALUE / hosc_hz;

		if ((div > 0) && (div <= 2)) {
			div = 0;
		} else if ((div > 2) && (div <= 4)) {
			div = 1;
		} else if ((div > 4) && (div <= 8)) {
			div = 2;
		} else if (div > 8) {
			div = 3;
		}
		sys_write32((sys_read32(CMU_RTCCLK) & ~CMU_RTCCLK_RTCHCLKDIV_MASK)
					| CMU_RTCCLK_RTCHCLKDIV(div), CMU_RTCCLK);
	} else {
		printk("invalid rtc clksource:%d\n", clksrc);
		return -EINVAL;
	}

	return 0;
}

int acts_rtc_setup_rc32k_step_msk(void);

static inline int acts_rtc_setup_hosc_msk(void)
{
	struct acts_rtc *rtc_base = (struct acts_rtc *)RTC_BASE;

	rtc_base->rtc_cmp_msk = 0xffffffff;

 	return 0;
 }

static inline int acts_rtc_setup(uint8_t clksrc)
{
	struct acts_rtc *rtc_base = (struct acts_rtc *)RTC_BASE;
	int ret = 0;

	printk("setup system timer:%d\n", clksrc);

	if (RTC_CLKSRC_SEL_HOSC == clksrc) {
		// rtc_base->rtc_cmp_msk = 0xffffffff;

		rtc_base->rtc_ctl &= ~RTC_CTL_RTC_CK_SRC;
		/* wait rtc clock source status change to HOSC */
		while (rtc_base->rtc_ctl & RTC_CTL_CK_SRC_STA);
	} else {
		acts_rtc_setup_rc32k_step_msk();

		rtc_base->rtc_ctl |= RTC_CTL_RTC_CK_SRC; /* LPCK */
		/* wait rtc clock source status change to LPCK */
		while (!(rtc_base->rtc_ctl & RTC_CTL_CK_SRC_STA));
	}

	rtc_base->rtc_ctl |= RTC_CTL_RTC_EN;

	return ret;
}

static inline int acts_rtc_init(uint8_t clksrc, uint32_t hosc_hz)
{
	struct acts_rtc *rtc_base = (struct acts_rtc *)RTC_BASE;
	int ret;

	acts_reset_peripheral(RESET_ID_RTC);
	acts_clock_peripheral_enable(CLOCK_ID_RTC);

	/* clear rtc valx irq pending */
	rtc_base->rtc_ctl |= RTC_CTL_RTC_VALX_IRQ_PD_MASK;

	/* disable rtc CMPx */
	rtc_base->rtc_ctl &= ~RTC_CTL_RTC_CMPX_EN_MASK;

	ret = acts_rtc_set_clk(clksrc, hosc_hz);
	if (ret)
		return ret;

	return acts_rtc_setup(clksrc);
}

/* update rtc compare value by specified index */
static inline int acts_rtc_cmp_update(uint8_t cmp_index, uint32_t cmp_val)
{
	struct acts_rtc *rtc_base = (struct acts_rtc *)RTC_BASE;

	if (cmp_index > RTC_MAX_CMP_INDEX) {
		printk("invalid cmp index:%d\n", cmp_index);
		return -EINVAL;
	}

	if (cmp_index == 0) {
		if (rtc_base->rtc_ctl & RTC_CTL_RTC_VAL0_IRQ_PD) {
			rtc_base->rtc_ctl |= RTC_CTL_RTC_VAL0_IRQ_PD;
			while (rtc_base->rtc_ctl & RTC_CTL_RTC_VAL0_IRQ_PD);
		}
		rtc_base->rtc_ctl &= ~RTC_CTL_RTC_CMP0_EN;
		rtc_base->rtc_cmp0 = cmp_val;
		rtc_base->rtc_ctl |= RTC_CTL_RTC_CMP0_EN;
	} else if (cmp_index == 1) {
		if (rtc_base->rtc_ctl & RTC_CTL_RTC_VAL1_IRQ_PD) {
			rtc_base->rtc_ctl |= RTC_CTL_RTC_VAL1_IRQ_PD;
			while (rtc_base->rtc_ctl & RTC_CTL_RTC_VAL1_IRQ_PD);
		}
		rtc_base->rtc_ctl &= ~RTC_CTL_RTC_CMP1_EN;
		rtc_base->rtc_cmp1 = cmp_val;
		rtc_base->rtc_ctl |= RTC_CTL_RTC_CMP1_EN;
	} else if (cmp_index == 2) {
		if (rtc_base->rtc_ctl & RTC_CTL_RTC_VAL2_IRQ_PD) {
			rtc_base->rtc_ctl |= RTC_CTL_RTC_VAL2_IRQ_PD;
			while (rtc_base->rtc_ctl & RTC_CTL_RTC_VAL2_IRQ_PD);
		}
		rtc_base->rtc_ctl &= ~RTC_CTL_RTC_CMP2_EN;
		rtc_base->rtc_cmp2 = cmp_val;
		rtc_base->rtc_ctl |= RTC_CTL_RTC_CMP2_EN;
	} else if (cmp_index == 3) {
		if (rtc_base->rtc_ctl & RTC_CTL_RTC_VAL3_IRQ_PD) {
			rtc_base->rtc_ctl |= RTC_CTL_RTC_VAL3_IRQ_PD;
			while (rtc_base->rtc_ctl & RTC_CTL_RTC_VAL3_IRQ_PD);
		}
		rtc_base->rtc_ctl &= ~RTC_CTL_RTC_CMP3_EN;
		rtc_base->rtc_cmp3 = cmp_val;
		rtc_base->rtc_ctl |= RTC_CTL_RTC_CMP3_EN;
	}

	return 0;
}

/* clear rtc compare pending by specified index */
static inline int acts_rtc_cmp_clear_pending(uint8_t cmp_index)
{
	struct acts_rtc *rtc_base = (struct acts_rtc *)RTC_BASE;

	if (cmp_index > RTC_MAX_CMP_INDEX) {
		printk("invalid cmp index:%d\n", cmp_index);
		return -EINVAL;
	}

	if (cmp_index == 0) {
		if (rtc_base->rtc_ctl & RTC_CTL_RTC_VAL0_IRQ_PD) {
			rtc_base->rtc_ctl |= RTC_CTL_RTC_VAL0_IRQ_PD;
			while (rtc_base->rtc_ctl & RTC_CTL_RTC_VAL0_IRQ_PD) {
				rtc_base->rtc_ctl |= RTC_CTL_RTC_VAL0_IRQ_PD;
			}
		}
	} else if (cmp_index == 1) {
		if (rtc_base->rtc_ctl & RTC_CTL_RTC_VAL1_IRQ_PD) {
			rtc_base->rtc_ctl |= RTC_CTL_RTC_VAL1_IRQ_PD;
			while (rtc_base->rtc_ctl & RTC_CTL_RTC_VAL1_IRQ_PD) {
				rtc_base->rtc_ctl |= RTC_CTL_RTC_VAL1_IRQ_PD;
			}
		}
	} else if (cmp_index == 2) {
		if (rtc_base->rtc_ctl & RTC_CTL_RTC_VAL2_IRQ_PD) {
			rtc_base->rtc_ctl |= RTC_CTL_RTC_VAL2_IRQ_PD;
			while (rtc_base->rtc_ctl & RTC_CTL_RTC_VAL2_IRQ_PD) {
				rtc_base->rtc_ctl |= RTC_CTL_RTC_VAL2_IRQ_PD;
			}
		}
	} else if (cmp_index == 3) {
		if (rtc_base->rtc_ctl & RTC_CTL_RTC_VAL3_IRQ_PD) {
			rtc_base->rtc_ctl |= RTC_CTL_RTC_VAL3_IRQ_PD;
			while (rtc_base->rtc_ctl & RTC_CTL_RTC_VAL3_IRQ_PD) {
				rtc_base->rtc_ctl |= RTC_CTL_RTC_VAL3_IRQ_PD;
			}
		}
	}

	return 0;
}


/* get current rtc counter */
static inline int acts_rtc_get_cnt(void)
{
	struct acts_rtc *rtc_base = (struct acts_rtc *)RTC_BASE;
	return rtc_base->rtc_cnt;
}

/* get the clock count of the RTC running on a 32K clock. This count has been converted from 32KRC to HOSC. */
static inline uint32_t acts_rtc_get_cnt_in_rc32k(void)
{
	uint32_t cnt, cnt_start, cnt_end;
	struct acts_rtc *rtc_base = (struct acts_rtc *)RTC_BASE;

	cnt_start = rtc_base->rtc_cnt_tmp0;
	cnt_end = rtc_base->rtc_cnt_tmp1;

	if (cnt_end >= cnt_start) {
		cnt = cnt_end - cnt_start;
	} else {
		cnt = TIMER_MAX_CYCLES_VALUE - cnt_start + cnt_end;
	}
	return cnt;
}

static inline int acts_rtc_new_cmp_val(uint32_t cmp_val)
{
	int rtc_cnt = acts_rtc_get_cnt();
	int new_cmp_val;

	/* in case of overflow */
	if (cmp_val > (TIMER_MAX_CYCLES_VALUE - rtc_cnt)) {
		new_cmp_val = cmp_val + (rtc_cnt - TIMER_MAX_CYCLES_VALUE);
	} else {
		new_cmp_val = cmp_val + rtc_cnt;
	}

	return new_cmp_val;
}

/* enable rtc compare by specified index */
#define acts_rtc_cmp_enable(index, val, irq_en, func) { \
	struct acts_rtc *rtc_base = (struct acts_rtc *)RTC_BASE; \
	if (index > RTC_MAX_CMP_INDEX) { \
		printk("invalid cmp index:%d\n", index); \
		return -EINVAL; \
	} \
	if (index == 0) { \
		rtc_base->rtc_ctl |= RTC_CTL_RTC_VAL0_IRQ_PD; \
		rtc_base->rtc_cmp0 = val; \
		rtc_base->rtc_ctl |= RTC_CTL_RTC_CMP0_EN; \
		if (irq_en) { \
			IRQ_CONNECT(IRQ_ID_CMP0, 1, func, 0, 0); \
			irq_enable(IRQ_ID_CMP0); \
			rtc_base->rtc_ctl |= RTC_CTL_RTC_VAL0_IRQ_EN; \
		} \
	} else if (index == 1) { \
		rtc_base->rtc_ctl |= RTC_CTL_RTC_VAL1_IRQ_PD; \
		rtc_base->rtc_cmp1 = val; \
		rtc_base->rtc_ctl |= RTC_CTL_RTC_CMP1_EN; \
		if (irq_en) { \
			IRQ_CONNECT(IRQ_ID_CMP1, 1, func, 0, 0); \
			irq_enable(IRQ_ID_CMP1); \
			rtc_base->rtc_ctl |= RTC_CTL_RTC_VAL1_IRQ_EN; \
		} \
	} else if (index == 2) { \
		rtc_base->rtc_ctl |= RTC_CTL_RTC_VAL2_IRQ_PD; \
		rtc_base->rtc_cmp2 = val; \
		rtc_base->rtc_ctl |= RTC_CTL_RTC_CMP2_EN; \
		if (irq_en) { \
			IRQ_CONNECT(IRQ_ID_CMP2, 1, func, 0, 0); \
			irq_enable(IRQ_ID_CMP2); \
			rtc_base->rtc_ctl |= RTC_CTL_RTC_VAL2_IRQ_EN; \
		} \
	} else if (index == 3) { \
		rtc_base->rtc_ctl |= RTC_CTL_RTC_VAL3_IRQ_PD; \
		rtc_base->rtc_cmp3 = val; \
		rtc_base->rtc_ctl |= RTC_CTL_RTC_CMP3_EN; \
		if (irq_en) { \
			IRQ_CONNECT(IRQ_ID_CMP3, 1, func, 0, 0); \
			irq_enable(IRQ_ID_CMP3); \
			rtc_base->rtc_ctl |= RTC_CTL_RTC_VAL3_IRQ_EN; \
		} \
	} \
	return 0; \
}

#endif /* SOC_TIMER_H_ */
