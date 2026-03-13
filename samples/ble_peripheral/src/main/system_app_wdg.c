/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include "soc.h"
#include <drivers/watchdog.h>
#include "system_app_wdg.h"

#include <sys_log.h>
LOG_MODULE_REGISTER(watchdog, CONFIG_LOG_DEFAULT_LEVEL);

#define WD_CTL_EN				(1 << 4)
#define WD_CTL_CLKSEL_MASK		(0x7 << 1)
#define WD_CTL_CLKSEL_SHIFT		(1)
#define WD_CTL_CLKSEL(n)		((n) << WD_CTL_CLKSEL_SHIFT)
#define WD_CTL_CLR				(1 << 0)
#define WD_CTL_IRQ_PD			(1 << 6)
#define WD_CTL_IRQ_EN			(1 << 5)

#ifdef CONFIG_WDT_ACTS_NAME
#define CONFIG_WDG_DEVICE_NAME CONFIG_WDT_ACTS_NAME
#else
#define CONFIG_WDG_DEVICE_NAME "WDT_0"
#endif

const struct device *wdt_dev;

/********************************
Watch Dog timer Clock Select:
000   256hz 703 ms
001   128hz 1.4s
010   64hz  2.8s
011   32hz  5.6 s
100   16hz  11.2s
101   8hz   22.4 s
110   4hz   44.8s
111         10ms
**********************************/
#define CONFIG_WDG_ISR_FEED_UPDATE_TIMEOUT 0x2  // 2.8s
static int wdt_timeout_to_clksel(int ms)
{
	if (ms <= 10) {
		return 0x7;
	} else if (ms <= 703) {
		return 0x0;
	} else if (ms <= 1400) {
		return 0x1;
	} else if (ms <= 2800) {
		return 0x2;
	} else if (ms <= 5600) {
		return 0x3;
	} else if (ms <= 11200) {
		return 0x4;
	} else if (ms <= 22400) {
		return 0x5;
	} else if (ms <= 44800) {
		return 0x6;
	}
	return 0;
}

void watchdog_disable(void)
{
	if (wdt_dev) {
		wdt_disable(wdt_dev);
	} else {
		sys_write32(0x0, WD_CTL);
#ifdef CONFIG_DEEPSLEEP_WDG_ENABLE
		/* CPU Deepsleep WatchDog Clock Disable */
		sys_write32(sys_read32(CMU_RTCCLK) & ~(1 << 12), CMU_RTCCLK);
#endif
	}
	SYS_LOG_INF("watchdog_disable");
}

void watchdog_clear_pending(void)
{
	sys_write32(sys_read32(WD_CTL), WD_CTL);
}

void watchdog_feed(void)
{
	if (wdt_dev) {
		wdt_feed(wdt_dev, 0);
	} else {
		sys_write32(sys_read32(WD_CTL) | WD_CTL_CLR, WD_CTL);
	}
}

/* feed watchdog in isr */
void watchdog_isr_feed(void)
{
#ifndef CONFIG_WDG_ISR_FEED_UPDATE_TIMEOUT
	sys_write32(sys_read32(WD_CTL) | WD_CTL_CLR, WD_CTL);
#else
	sys_write32((sys_read32(WD_CTL) & ~WD_CTL_CLKSEL_MASK)
		| WD_CTL_CLKSEL(CONFIG_WDG_ISR_FEED_UPDATE_TIMEOUT) | WD_CTL_CLR, WD_CTL);
#endif
}

int watchdog_enable(int timeout) // unit: ms
{
	struct wdt_timeout_cfg wdt_config = { 0 };
	int err = 0;

	if (!timeout)
		return -EINVAL;

	if (wdt_dev) {
		/* Reset SoC when watchdog timer expires. */
		wdt_config.flags = WDT_FLAG_RESET_SOC;

		/* Expire watchdog after timeout milliseconds. */
		wdt_config.window.max = timeout;

		/* Set up watchdog callback. Jump into it when watchdog expired. */
		wdt_config.callback = NULL;

		err = wdt_install_timeout(wdt_dev, &wdt_config);
		if (err < 0) {
			SYS_LOG_ERR("Watchdog install error\n");
			return err;
		}

		err = wdt_setup(wdt_dev, 0);
		if (err < 0) {
			SYS_LOG_ERR("Watchdog setup error\n");
			return err;
		}
	} else {
		int clksel = wdt_timeout_to_clksel(timeout);

		sys_write32((sys_read32(WD_CTL) & ~WD_CTL_CLKSEL_MASK)
			| WD_CTL_CLKSEL(clksel), WD_CTL);

		sys_write32((sys_read32(WD_CTL) & ~WD_CTL_IRQ_EN) | WD_CTL_EN | WD_CTL_CLR, WD_CTL);

#ifdef CONFIG_DEEPSLEEP_WDG_ENABLE
		/* CPU Deepsleep WatchDog Clock Enable */
		sys_write32(sys_read32(CMU_RTCCLK) | (1 << 12), CMU_RTCCLK);
#endif
	}

	SYS_LOG_INF("enable watchdog (%d ms)", timeout);

	return 0;
}

void watchdog_init(void)
{
	wdt_dev = device_get_binding(CONFIG_WDG_DEVICE_NAME);
	if (wdt_dev) {
		SYS_LOG_INF("get watchdog device success");
	}
}

