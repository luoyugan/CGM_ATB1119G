/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system standby
 */

#include <kernel.h>
#include <device.h>
#include <power/power.h>
#include <sys_monitor.h>
#include <sys_wakelock.h>
#include <soc.h>

#include <sys_log.h>
LOG_MODULE_REGISTER(standby, CONFIG_LOG_DEFAULT_LEVEL);

#define STANDBY_MIN_TIME_MSEC (100)

#define MSEC_PER_TICK (MSEC_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

enum STANDBY_STATE_E {
	STANDBY_HW_S1,
	STANDBY_SW_S2,
	STANDBY_HW_S3,
};

struct standby_context_t {
	uint32_t standby_threshold_time;
	uint32_t auto_standby_time;
	uint32_t auto_powerdown_time;
	uint32_t wakeup_timestamp;
	uint8_t standby_state:3;
	uint8_t wakeup_flag:1;
};

static struct standby_context_t *standby_context = NULL;

/**
 * System Power State Machine
 * S1: man and bt cores work in normal state (hardware attribution)
 * S2: main core enters light sleep and bt core works in normal state.
 * S3: main and bt cores enter deep sleep state
 *
 * state machine switch: S1 <=> S2 <=> S3
 */

/* exit S1 and enter S2 */
static int _sys_standby_s1_to_s2(void)
{
	LOG_INF("S1 => S2, rtc_cnt=0x%x\n", acts_rtc_get_cnt());

#ifdef CONFIG_STANDBY_POLL_DELAY_WORK
	sys_monitor_stop();
#endif

	pm_early_suspend();

	/* TODO dvfs set S2 */

	/* enter light sleep */
	sys_pm_enter_light_sleep();

	soc_light_sleep();

	standby_context->standby_state = STANDBY_SW_S2;

	return 0;
}

/* exit S2 and enter S1 */
static int _sys_standby_s2_to_s1(void)
{
	LOG_INF("S2 => S1, rtc_cnt=0x%x\n", acts_rtc_get_cnt());

	standby_context->wakeup_timestamp = k_uptime_get_32();

	soc_exit_light_sleep();

	/* TODO dvfs unset S2 */

	pm_late_resume();

#ifdef CONFIG_STANDBY_POLL_DELAY_WORK
	sys_monitor_start(CONFIG_MONITOR_PERIOD);
#endif

	standby_context->standby_state = STANDBY_HW_S1;

	return 0;
}

/* exit S2 and enter S3 */
static int _sys_standby_s2_to_s3(void)
{
	LOG_INF("S2 => S3, rtc_cnt=0x%x\n", acts_rtc_get_cnt());

	standby_context->wakeup_flag = 0;

	/* enter deep sleep */
	sys_pm_enter_deep_sleep();

	standby_context->standby_state = STANDBY_HW_S3;

	return 0;
}

/* exist S3 and enter S2 */
static int _sys_standby_s3_to_s2(void)
{
	LOG_INF("S3 => S2, rtc_cnt=0x%x\n", acts_rtc_get_cnt());

	standby_context->standby_state = STANDBY_SW_S2;

	sys_wake_lock(WAKELOCK_WAKE_UP);
	_sys_standby_s2_to_s1();
	sys_wake_unlock(WAKELOCK_WAKE_UP);

	return 0;
}

/* S1 process */
static int _sys_standby_process_hw_s1(void)
{
	uint32_t wakelocks = sys_wakelocks_check();

	LOG_DBG("process S1");

	if (wakelocks) {
		LOG_DBG("system wakelocks:0x%08x", wakelocks);
		return 0;
	}

	LOG_DBG("system idle time:%d", sys_wakelocks_get_free_time());

	if (sys_wakelocks_get_free_time() > standby_context->auto_standby_time)
		_sys_standby_s1_to_s2();

	return 0;
}

/* S2 process */
static int _sys_standby_process_sw_s2(int32_t ticks)
{
	uint32_t wakelocks = sys_wakelocks_check();

	LOG_DBG("process S2");

	if (wakelocks) {
		LOG_INF("system wakelocks:0x%08x", wakelocks);
		_sys_standby_s2_to_s1();
		return 0;
	}

	//if (sys_wakelocks_get_free_time() > standby_context->auto_standby_time) {
	if (ticks > standby_context->standby_threshold_time / MSEC_PER_TICK) {
		_sys_standby_s2_to_s3();
	} else {
		_sys_standby_s2_to_s1();
	}

	return 0;
}

static int _sys_standby_check_auto_powerdown(void)
{
	int ret = 0;

	if (sys_wakelocks_get_free_time()
		>= standby_context->auto_powerdown_time) {
		LOG_INF("power down!!!");
		sys_pm_poweroff();
		ret = 1;
	}

	return ret;
}

static bool _sys_standby_wakeup_from_s3(void)
{
	uint32_t wakelocks = sys_wakelocks_check();

	if (_sys_standby_check_auto_powerdown()) {
		return true;
	}

	if ((wakelocks) || (sys_wakelocks_get_free_time()
			< standby_context->auto_standby_time)) {
		LOG_INF("s3 wakelock:0x%08x free:0x%x",
				wakelocks, sys_wakelocks_get_free_time());
		return true;
	}

#ifndef CONFIG_STANDBY_HANDLE_IN_DELAY_WORK
	if (standby_context->wakeup_flag)
		return true;
#endif

	return false;
}

static int _sys_standby_process_hw_s3(void)
{
	LOG_DBG("process S3");

	while (1) {
		/* TODO clear watchdog */

		if (_sys_standby_wakeup_from_s3()) {
			_sys_standby_s3_to_s2();
			break;
		}

#ifdef CONFIG_STANDBY_HANDLE_IN_DELAY_WORK
		k_yield();

		while (!standby_context->wakeup_flag)
			k_sleep(K_MSEC(2));
#endif
	}

	return 0;
}

int _sys_standby_work_handle(int32_t ticks)
{
	int ret = _sys_standby_check_auto_powerdown();

	if (ret)
		return ret;

	switch (standby_context->standby_state) {
		case STANDBY_HW_S1:
			ret = _sys_standby_process_hw_s1();
			break;
		case STANDBY_SW_S2:
			ret = _sys_standby_process_sw_s2(ticks);
			break;
		case STANDBY_HW_S3:
			ret = _sys_standby_process_hw_s3();
			break;
		default:
			LOG_ERR("invalid standby state:%d", standby_context->standby_state);
			ret = -EINVAL;
	}

	return ret;
}

static void _sys_standby_enter_sleep(enum pm_state state)
{
	LOG_DBG("enter pm state:%d", state);
}

static void _sys_standby_exit_sleep(enum pm_state state)
{
	LOG_DBG("exit pm state:%d", state);

	if (state == PM_STATE_SUSPEND_TO_RAM) {
		standby_context->wakeup_flag = 1;
#ifndef CONFIG_STANDBY_HANDLE_IN_DELAY_WORK
		_sys_standby_process_hw_s3();
#endif
	}
}

static struct pm_notifier notifier = {
	.state_entry = _sys_standby_enter_sleep,
	.state_exit = _sys_standby_exit_sleep,
};

int sys_standby_init(void)
{
	static struct standby_context_t global_standby_context;
	standby_context = &global_standby_context;

	memset(standby_context, 0, sizeof(struct standby_context_t));
	standby_context->standby_state = STANDBY_HW_S1;

#ifdef CONFIG_STANDBY_THRESHOLD_MSEC
	if (0 == CONFIG_STANDBY_THRESHOLD_MSEC) {
		standby_context->standby_threshold_time = -1;
	} else {
		standby_context->standby_threshold_time = CONFIG_STANDBY_THRESHOLD_MSEC;
	}
#endif

#ifdef CONFIG_AUTO_STANDBY_TIME_MSEC
	if (0 == CONFIG_AUTO_STANDBY_TIME_MSEC) {
		standby_context->auto_standby_time = -1;
	} else if (CONFIG_AUTO_STANDBY_TIME_MSEC < STANDBY_MIN_TIME_MSEC) {
		standby_context->auto_standby_time = STANDBY_MIN_TIME_MSEC;
		LOG_WRN("auto standby time too small and used default:%dms",
				standby_context->auto_standby_time);
	} else {
		standby_context->auto_standby_time = CONFIG_AUTO_STANDBY_TIME_MSEC;
	}
#else
	standby_context->auto_standby_time = -1;
#endif

#ifdef CONFIG_AUTO_POWEDOWN_TIME_SEC
	if (CONFIG_AUTO_POWEDOWN_TIME_SEC) {
		standby_context->auto_powerdown_time = CONFIG_AUTO_POWEDOWN_TIME_SEC * 1000;
	} else {
		standby_context->auto_powerdown_time = -1;
	}
#else
	standby_context->auto_powerdown_time = -1;
#endif

	if (standby_context->auto_powerdown_time == -1)
		LOG_INF("disable auto powerdown");

	sys_wakelocks_init();

#ifdef CONFIG_STANDBY_HANDLE_IN_DELAY_WORK
	if (sys_monitor_add_work(_sys_standby_work_handle)) {
		LOG_ERR("add work failed");
		return -EFAULT;
	}
#endif

	pm_notifier_register(&notifier);

	LOG_INF("standby time : %dms", standby_context->auto_standby_time);

	set_deepsleep_mode(DEEPSLEEP_MODE);

	set_deepsleep_log_level(CONFIG_DEEPSLEEP_LOG_LEVEL);

	return 0;
}
