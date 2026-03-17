/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RTC driver for Actions SoC
 */

#include <errno.h>
#include <device.h>
#include <init.h>
#include <kernel.h>
#include <string.h>
#include <soc.h>
#include <board_cfg.h>
#include <drivers/rtc.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(rtc0, CONFIG_LOG_DEFAULT_LEVEL);

#define RTC_DEFAULT_INIT_TIME	      (1483200000)	/* 2016.12.31  16:00:00 */

/* RTC Control Register */
#define	RTC_CTL_ALIP                  BIT(0)
#define	RTC_CTL_ALIE                  BIT(1)
#define RTC_CTL_4HZ_CLK_SEL_SHIFT     (2)
#define RTC_CTL_4HZ_CLK_SEL_MASK      (3 << RTC_CTL_4HZ_CLK_SEL_SHIFT)
#define RTC_CTL_4HZ_CLK_SEL(x)        ((x) << RTC_CTL_4HZ_CLK_SEL_SHIFT)
#define RTC_CTL_4HZ_CLK_32KHZ         RTC_CTL_4HZ_CLK_SEL(1)
#define RTC_CTL_4HZ_CLK_HOSC          RTC_CTL_4HZ_CLK_SEL(2)

#define	RTC_CTL_CALEN                 BIT(4)
#define	RTC_CTL_LEAP                  BIT(7)
#define RTC_CTL_32KHZ_SEL             BIT(16) /* 0: LOSC 32KHZ; 1 EXT_32KHZ */
#define RTC_CTL_CAL_CLK_SEL           BIT(17) /* 0: 4HZ; 1: 100HZ */
#define	RTC_CTL_TESTEN                BIT(18)

#define RTC_MS_SHIRT                  (0) /* millisecode x 10 */
#define RTC_MS_MASK                   (0x7f << RTC_MS_SHIRT)

#define RTC_MS(x)                     (((x) & RTC_MS_MASK) >> RTC_MS_SHIRT)
#define RTC_MS_MUL10(x)               (RTC_MS(x) * 10)
#define RTC_MS_DIV10(x)               (((x + 9) / 10) << RTC_MS_SHIRT)

#define RTC_YMD_Y_SHIFT		          (16)
#define RTC_YMD_Y_MASK		          (0x7f << RTC_YMD_Y_SHIFT)
#define RTC_YMD_M_SHIFT		          (8)
#define RTC_YMD_M_MASK		          (0xf << RTC_YMD_M_SHIFT)
#define RTC_YMD_D_SHIFT		          (0)
#define RTC_YMD_D_MASK		          (0x1f << RTC_YMD_D_SHIFT)

#define RTC_YMD_Y(ymd)		          (((ymd) & RTC_YMD_Y_MASK) >> RTC_YMD_Y_SHIFT)
#define RTC_YMD_M(ymd)		          (((ymd) & RTC_YMD_M_MASK) >> RTC_YMD_M_SHIFT)
#define RTC_YMD_D(ymd)		          (((ymd) & RTC_YMD_D_MASK) >> RTC_YMD_D_SHIFT)
#define RTC_YMD_VAL(y, m, d)	      (((y) << RTC_YMD_Y_SHIFT) | ((m) << RTC_YMD_M_SHIFT) | ((d) << RTC_YMD_D_SHIFT))

#define RTC_HMS_H_SHIFT		          (16)
#define RTC_HMS_H_MASK		          (0x1f << RTC_HMS_H_SHIFT)
#define RTC_HMS_M_SHIFT		          (8)
#define RTC_HMS_M_MASK		          (0x3f << RTC_HMS_M_SHIFT)
#define RTC_HMS_S_SHIFT		          (0)
#define RTC_HMS_S_MASK		          (0x3f << RTC_HMS_S_SHIFT)

#define RTC_HMS_H(ymd)		          (((ymd) & RTC_HMS_H_MASK) >> RTC_HMS_H_SHIFT)
#define RTC_HMS_M(ymd)		          (((ymd) & RTC_HMS_M_MASK) >> RTC_HMS_M_SHIFT)
#define RTC_HMS_S(ymd)		          (((ymd) & RTC_HMS_S_MASK) >> RTC_HMS_S_SHIFT)

#define RTC_HMS_VAL(h, m, s)	      (((h) << RTC_HMS_H_SHIFT) | ((m) << RTC_HMS_M_SHIFT) | ((s) << RTC_HMS_S_SHIFT))

#define RTC_REGS_UPDATE_MAGIC         (0xA596)
#define RTC_REGS_UPDATE_OK	          (0x5A69)

#define RTC_CONFIG_PERIOD_ALARM_TIMEOUT_MS    (1000)

enum acts_rtc_clock_src {
	RTC_CLKSRC_HOSC_4HZ = 0,
	RTC_CLKSRC_HCL_LOSC_100HZ,
};

struct acts_rtc_controller {
	volatile uint32_t ctrl;
	volatile uint32_t regupdata;
	volatile uint32_t ms_alarm;
	volatile uint32_t hms_alarm;
	volatile uint32_t ymd_alarm;
	volatile uint32_t ms; /* millisecond x 10 */
	volatile uint32_t hms;
	volatile uint32_t ymd;
};

struct acts_rtc_data {
	struct acts_rtc_controller *base;
	void (*alarm_cb_fn)(const void *cb_data);
	const void *cb_data;
	struct rtc_alarm_period_config period_config;
	bool alarm_en;
	uint8_t alarm_wakeup : 1; /* System wakeup from RTC alarm indicator */
};

struct acts_rtc_config {
	struct acts_rtc_controller *base;
	uint8_t rtc_clk_source;
	void (*irq_config)(void);
};

static void rtc_acts_dump_regs(struct acts_rtc_controller *base)
{
	LOG_INF("** RTC Controller register **");
	LOG_INF("      BASE: 0x%x", (uint32_t)base);
	LOG_INF("       CTL: 0x%x", base->ctrl);
	LOG_INF(" REGUPDATE: 0x%x", base->regupdata);
	LOG_INF("     MSALM: 0x%x", base->ms_alarm);
	LOG_INF("   DHMSALM: 0x%x", base->hms_alarm);
	LOG_INF("    YMDALM: 0x%x", base->ymd_alarm);
	LOG_INF("        MS: 0x%x", base->ms);
	LOG_INF("      DHMS: 0x%x", base->hms);
	LOG_INF("       YMD: 0x%x", base->ymd);
}

/* RTC associated registers update to take effect */
void rtc_acts_update_regs(void)
{
#if !defined(CONFIG_SOC_SERIES_LARK)
	struct acts_rtc_controller *base = (struct acts_rtc_controller *)RTC_REG_BASE;

	base->regupdata = RTC_REGS_UPDATE_MAGIC;

    while (base->regupdata != RTC_REGS_UPDATE_OK) {
        ; // wait rtc register update
    }
#endif
}

/* Select the LOSC as the clock source which is inside of the HCL controller. */
static inline void rtc_acts_init_clksource(const struct device *dev, struct acts_rtc_data *rtcd)
{
	struct acts_rtc_controller *base = rtcd->base;
	const struct acts_rtc_config *cfg = dev->config;

	if (cfg->rtc_clk_source == RTC_CLKSRC_HOSC_4HZ) {
		base->ctrl &= ~RTC_CTL_4HZ_CLK_SEL_MASK;
		base->ctrl |= RTC_CTL_4HZ_CLK_HOSC;
	} else {
		/* WIO2 => LOSCI; WIO3 => LOSCO */
		sys_write32(1 << 4, WIO2_CTL);
		sys_write32(1 << 4, WIO3_CTL);

		/* enable LOSC */
		sys_write32(0x1a1a71, LOSC_CTL);

		base->ctrl = RTC_CTL_ALIP;

		sys_write32(0, HCL_CTL);

		k_busy_wait(500);

		sys_write32(0x1c8d, HCL_CTL);

		base->ctrl |= RTC_CTL_CAL_CLK_SEL; /* select 100HZ */
	}
	rtc_acts_update_regs();

	LOG_DBG("rtc_ctl:0x%08x", base->ctrl);
}

static int rtc_acts_get_datetime(struct acts_rtc_data *rtcd, struct rtc_time *tm)
{
	struct acts_rtc_controller *base = rtcd->base;
	uint32_t ymd, hms;
	uint8_t ms;

	ymd = base->ymd;
	hms = base->hms;
	ms = base->ms;

	tm->tm_ms = RTC_MS_MUL10(ms);		/* rtc milisecond 0 ~ 999 */
	tm->tm_sec = RTC_HMS_S(hms);		/* rtc second 0-59 */
	tm->tm_min = RTC_HMS_M(hms);		/* rtc minute 0-59 */
	tm->tm_hour = RTC_HMS_H(hms);		/* rtc hour 0-23 */
	tm->tm_mday = RTC_YMD_D(ymd);		/* rtc day 1-31 */
	tm->tm_wday = 0;
	tm->tm_mon = RTC_YMD_M(ymd) - 1;	/* rtc mon 1-12 */
	tm->tm_year = 100 + RTC_YMD_Y(ymd);	/* rtc year 0-99 */

	LOG_DBG("read date time: %04d-%02d-%02d  %02d:%02d:%02d:%03d",
		1900 + tm->tm_year, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_ms);

	/* the clock can give out invalid datetime, but we cannot return
	 * -EINVAL otherwise hwclock will refuse to set the time on bootup.
	 */
	if (rtc_valid_tm(tm) < 0) {
		LOG_ERR("rtc: retrieved date/time is not valid.\n");
		rtc_acts_dump_regs(base);
	}
	return 0;
}

static int rtc_acts_set_datetime(struct acts_rtc_data *rtcd, struct rtc_time *tm)
{
	struct acts_rtc_controller *base = rtcd->base;

	LOG_INF("set datetime: %04d-%02d-%02d  %02d:%02d:%02d:%03d",
               1900 + tm->tm_year, tm->tm_mon + 1, tm->tm_mday,
               tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_ms);

	/* disable calendar when update the hms register
	 *  From the specification discription, the CAL_EN bit must be disable  when
	 *  the RTC_DHMS/RTC_YMD registers being written. And RTC_DHMS/RTC_YMD
	 *  registers must be written before CAL_EN is enabled.
	 */
	base->ctrl &= ~RTC_CTL_CALEN;
	rtc_acts_update_regs();
	base->ymd = RTC_YMD_VAL(tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday);
	base->hms = RTC_HMS_VAL(tm->tm_hour, tm->tm_min, tm->tm_sec);
	base->ms = RTC_MS_DIV10(tm->tm_ms);
	rtc_acts_update_regs();
	base->ctrl |= RTC_CTL_CALEN;
	rtc_acts_update_regs();

	return 0;
}

static void rtc_acts_set_alarm_interrupt(struct acts_rtc_controller *base, bool enable)
{
	if (enable)
		base->ctrl |= RTC_CTL_ALIE;
	else
		base->ctrl &= ~RTC_CTL_ALIE;

	rtc_acts_update_regs();
}

/* get & clear alarm irq pending */
static uint32_t rtc_acts_get_pending_int(const struct device *dev)
{
	struct acts_rtc_data *rtcd = dev->data;
	struct acts_rtc_controller *base = rtcd->base;
	int pending;

	pending = base->ctrl & RTC_CTL_ALIP;
	if (pending) {
		/* clear pending */
		base->ctrl |= RTC_CTL_ALIP;
		LOG_INF("Clear old RTC alarm pending");
		rtc_acts_update_regs();
	}

	return pending;
}

static void rtc_acts_enable(const struct device *dev)
{
	struct acts_rtc_data *rtcd = dev->data;
	struct acts_rtc_controller *base = rtcd->base;

	base->ctrl |= RTC_CTL_CALEN;
	rtc_acts_update_regs();
}

static void rtc_acts_disable(const struct device *dev)
{
	struct acts_rtc_data *rtcd = dev->data;
	struct acts_rtc_controller *base = rtcd->base;

	base->ctrl &= ~RTC_CTL_CALEN;
	rtc_acts_update_regs();
}

static int rtc_acts_get_time(const struct device *dev, struct rtc_time *tm)
{
	struct acts_rtc_data *rtcd = dev->data;

	if (!tm)
		return -EINVAL;

	if (rtc_acts_get_datetime(rtcd, tm)) {
		LOG_ERR("failed to get datetime");
		return -EACCES;
	}

	return 0;
}

static int rtc_acts_set_time(const struct device *dev, const struct rtc_time *tm)
{
	struct acts_rtc_data *rtcd = dev->data;

	if (!tm)
		return -EINVAL;

	if (rtc_valid_tm((struct rtc_time *)tm)) {
		LOG_ERR("Bad time structure");
		print_rtc_time((struct rtc_time *)tm);
		return -ENOEXEC;
	}

	return rtc_acts_set_datetime(rtcd, (struct rtc_time *)tm);
}

static int rtc_acts_set_alarm_time(const struct acts_rtc_data *rtcd, struct rtc_time *tm)
{
	struct acts_rtc_controller *base = rtcd->base;

	/* disable alarm interrupt */
	rtc_acts_set_alarm_interrupt(base, false);

	base->ymd_alarm = RTC_YMD_VAL(tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday);

	base->hms_alarm = RTC_HMS_VAL(tm->tm_hour, tm->tm_min, tm->tm_sec);

	base->ms_alarm = RTC_MS_DIV10(tm->tm_ms);

	rtc_acts_update_regs();

	/* enable alarm interrupt */
	rtc_acts_set_alarm_interrupt(base, true);

	LOG_DBG("set alarm: %04d-%02d-%02d  %02d:%02d:%02d:%03d",
		1900 + tm->tm_year, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_ms);

	return 0;
}

static int rtc_acts_set_alarm(const struct device *dev, struct rtc_alarm_config *config, bool enable)
{
	struct acts_rtc_data *rtcd = dev->data;
	int ret = 0;

	/* clear old alarm pending */
	rtc_acts_get_pending_int(dev);

	if (enable) {
		if (!config) {
			LOG_ERR("no alarm configuration");
			return -EINVAL;
		}

		if (rtc_valid_tm(&config->alarm_time)) {
			LOG_ERR("Bad time structure");
			print_rtc_time(&config->alarm_time);
			return -ENOEXEC;
		}

		rtcd->alarm_cb_fn = config->cb_fn;
		rtcd->cb_data = config->cb_data;
		rtcd->alarm_en = true;
		ret = rtc_acts_set_alarm_time(rtcd, &config->alarm_time);
	} else {
		rtc_acts_set_alarm_interrupt(rtcd->base, false);
		rtcd->alarm_en = false;
		rtcd->alarm_cb_fn = NULL;
		rtcd->cb_data = NULL;
	}

	return ret;
}

static int rtc_acts_get_alarm(const struct device *dev, struct rtc_alarm_status *sts)
{
	struct acts_rtc_data *rtcd = dev->data;
	struct acts_rtc_controller *base = rtcd->base;
	struct rtc_time *tm = &sts->alarm_time;
	uint32_t hms;
	uint32_t ymd;
	uint8_t ms;

	if (!sts)
		return -EINVAL;

	memset(sts, 0, sizeof(struct rtc_alarm_status));

	sts->is_on = rtcd->alarm_en;

	hms = base->hms_alarm;
	tm->tm_sec = RTC_HMS_S(hms);		/* rtc second 0-59 */
	tm->tm_min = RTC_HMS_M(hms);		/* rtc minute 0-59 */
	tm->tm_hour = RTC_HMS_H(hms);		/* rtc hour 0-23 */

	ymd = base->ymd_alarm;
	tm->tm_mday = RTC_YMD_D(ymd);		/* rtc day 1-31 */
	tm->tm_wday = 0;
	tm->tm_mon = RTC_YMD_M(ymd) - 1;	/* rtc mon 1-12 */
	tm->tm_year = 100 + RTC_YMD_Y(ymd);	/* rtc year 0-99 */

	ms = base->ms_alarm;
	tm->tm_ms = RTC_MS_MUL10(ms);		/* rtc millisecond 0-999 */

	return 0;
}

static void rtc_acts_period_alarm_cb_fn(const void *cb_data);
static int __rtc_acts_set_alarm_period(const struct device *dev)
{
	struct acts_rtc_data *rtcd = dev->data;
	struct rtc_alarm_config alarm_config = {0};
	struct rtc_time tm = {0}, _tm = {0};
	uint32_t time, ms, _time;
	int ret = -1;
	struct rtc_alarm_period_config *config = &rtcd->period_config;

	uint32_t timestamp = k_uptime_get_32();

	while (1) {
		rtc_acts_get_time(dev, &tm);
		rtc_tm_to_time(&tm, &time);
		if ((tm.tm_ms + config->tm_msec) >= 1000) {
			time += 1;
			ms = tm.tm_ms + config->tm_msec - 1000;
		} else {
			ms = tm.tm_ms + config->tm_msec;
		}
		time += config->tm_sec;

		/**
		 * FIXME: HW issue
		 * LARK chipset rtc alarm IRQ will be missed if alrarm time setting too short.
		 */

		rtc_time_to_tm(time, &tm);
		tm.tm_ms = ms;
		memcpy(&alarm_config.alarm_time, &tm, sizeof(struct rtc_time));
		alarm_config.cb_fn = rtc_acts_period_alarm_cb_fn;
		alarm_config.cb_data = (const void *)dev;

		ret = rtc_acts_set_alarm(dev, &alarm_config, true);
		if (ret)
			return ret;

		rtc_acts_get_time(dev, &_tm);
		rtc_tm_to_time(&_tm, &_time);
		//print_rtc_time(&_tm);

		if ((_tm.tm_ms <= tm.tm_ms) && (_time == time))
			break;

		if ((_tm.tm_ms > tm.tm_ms) && (time == (_time + 1)))
			break;

		if (_time < time)
			break;

		if ((k_uptime_get_32() - timestamp)
			< RTC_CONFIG_PERIOD_ALARM_TIMEOUT_MS) {
			LOG_ERR("set period alarm timeout");
			return -ETIMEDOUT;
		}

	}

	return 0;
}

static void rtc_acts_period_alarm_cb_fn(const void *cb_data)
{
	const struct device *dev = (const struct device *)cb_data;
	struct acts_rtc_data *rtcd = dev->data;

	if (rtcd->period_config.cb_fn)
		rtcd->period_config.cb_fn(rtcd->period_config.cb_data);

	__rtc_acts_set_alarm_period(dev);
}

static int rtc_acts_set_period_alarm(const struct device *dev,
				struct rtc_alarm_period_config *config, bool enable)
{
	struct acts_rtc_data *rtcd = dev->data;
	int ret;

	if (enable) {
		memcpy(&rtcd->period_config, config, sizeof(struct rtc_alarm_period_config));
		ret = __rtc_acts_set_alarm_period(dev);
	} else {
		memset(&rtcd->period_config, 0, sizeof(struct rtc_alarm_period_config));
		ret = rtc_acts_set_alarm(dev, NULL, false);
	}

	return ret;
}

void rtc_acts_isr(const struct device *dev)
{
	struct acts_rtc_data *rtcd = dev->data;
	struct acts_rtc_controller *base = rtcd->base;
	int pending = base->ctrl & RTC_CTL_ALIP;

	LOG_DBG("alarm isr ctl:0x%x", base->ctrl);

	if (pending)
		base->ctrl |= RTC_CTL_ALIP; /* clear pending */

	/* disable alarm irq */
	rtc_acts_set_alarm_interrupt(rtcd->base, false);

	if (pending && rtcd->alarm_en && rtcd->alarm_cb_fn) {
		LOG_DBG("call alarm_cb_fn %p", rtcd->alarm_cb_fn);
		rtcd->alarm_cb_fn(rtcd->cb_data);
	}
}

static int rtc_acts_check_datetime(struct acts_rtc_data *rtcd)
{
	struct rtc_time tm;
	int ret;

	ret = rtc_acts_get_datetime(rtcd, &tm);
	if (ret)
		return ret;

	if (rtc_valid_tm(&tm) < 0) {
		LOG_ERR("Invalid RTC date/time, set to default!");

		rtc_time_to_tm(RTC_DEFAULT_INIT_TIME, &tm);
		rtc_acts_set_datetime(rtcd, &tm);
	}

	return 0;
}

static bool rtc_acts_is_alarm_wakeup(const struct device *dev)
{
	struct acts_rtc_data *rtcd = dev->data;
	if (rtcd->alarm_wakeup)
		return true;
	else
		return false;
}

const struct rtc_driver_api rtc_acts_driver_api = {
	.enable = rtc_acts_enable,
	.disable = rtc_acts_disable,
	.get_time = rtc_acts_get_time,
	.set_time = rtc_acts_set_time,
	.set_alarm = rtc_acts_set_alarm,
	.set_period_alarm = rtc_acts_set_period_alarm,
	.get_alarm = rtc_acts_get_alarm,
	.is_alarm_wakeup = rtc_acts_is_alarm_wakeup,
};

int rtc_acts_init(const struct device *dev)
{
	const struct acts_rtc_config *cfg = dev->config;
	struct acts_rtc_data *rtcd = dev->data;
	struct acts_rtc_controller *base = cfg->base;

	rtcd->base = base;
	rtcd->alarm_wakeup = false;

	if (soc_pmu_is_alarm_wakeup())
		rtcd->alarm_wakeup = true;

	/* By default to disable RTC alarm IRQ */
	if (base->ctrl & RTC_CTL_ALIE)
		base->ctrl |= RTC_CTL_ALIE;

	rtc_acts_init_clksource(dev, rtcd);

	/* By default to enable the rtc function */
	rtc_acts_enable(dev);

	rtc_acts_check_datetime(rtcd);

	cfg->irq_config();

	printk("rtc initialized\n");

	return 0;
}

static void rtc_acts_irq_config(void);
struct acts_rtc_data rtc_acts_ddata;

static const struct acts_rtc_config rtc_acts_cdata = {
	.base = (struct acts_rtc_controller *)RTC_REG_BASE,
	.rtc_clk_source = CONFIG_RTC_CLK_SOURCE,
	.irq_config = rtc_acts_irq_config,
};

DEVICE_DEFINE(rtc0, CONFIG_RTC_0_NAME, rtc_acts_init, device_pm_control_nop,
		    &rtc_acts_ddata, &rtc_acts_cdata,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &rtc_acts_driver_api);

static void rtc_acts_irq_config(void)
{
	IRQ_CONNECT(IRQ_ID_RTC, CONFIG_RTC_IRQ_PRI,
		    rtc_acts_isr, DEVICE_GET(rtc0), 0);
	irq_enable(IRQ_ID_RTC);
}

