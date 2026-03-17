/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system monitor
 */

#include <kernel.h>
#include <sys_monitor.h>
#include <sys_standby.h>

#include <sys_log.h>
LOG_MODULE_REGISTER(monitor, CONFIG_LOG_DEFAULT_LEVEL);

static struct sys_monitor_t g_monitor;

static void _sys_monitor_timer_work(struct k_work *work)
{
	uint8_t i;

	struct sys_monitor_t *sys_monitor =
		CONTAINER_OF(work, struct sys_monitor_t, sys_monitor_work);

	if (!sys_monitor || sys_monitor->monitor_stoped)
		return;

	for (i = 0 ; i < MAX_MONITOR_WORK_NUM; i++) {
		if (!sys_monitor->monitor_work[i]) {
			continue;
		}
		sys_monitor->monitor_work[i]();
	}

	k_delayed_work_submit(&sys_monitor->sys_monitor_work,
							SYS_TIMEOUT_MS(CONFIG_MONITOR_PERIOD));
}

void sys_monitor_init(void)
{
	struct sys_monitor_t *sys_monitor = &g_monitor;

	memset(sys_monitor, 0, sizeof(struct sys_monitor_t));

	sys_monitor->monitor_stoped = 0;

	k_delayed_work_init(&sys_monitor->sys_monitor_work, _sys_monitor_timer_work);
}

int sys_monitor_add_work(monitor_work_handle monitor_work)
{
	int ret = -ESRCH;
	uint8_t i;

	struct sys_monitor_t *sys_monitor = &g_monitor;

	for (i = 0 ; i < MAX_MONITOR_WORK_NUM; i++) {
		if (!sys_monitor->monitor_work[i]) {
			sys_monitor->monitor_work[i] = monitor_work;
			ret = 0;
			break;
		}
	}

	if (ret) {
		LOG_ERR(" err %d\n", ret);
	}

	return ret;
}

void sys_monitor_start(uint32_t ms)
{
	struct sys_monitor_t *sys_monitor = &g_monitor;

	sys_monitor->monitor_stoped = 0;	
	k_delayed_work_submit(&sys_monitor->sys_monitor_work, SYS_TIMEOUT_MS(ms));
}

void sys_monitor_stop(void)
{
	struct sys_monitor_t *sys_monitor = &g_monitor;

	sys_monitor->monitor_stoped = 1;
	k_delayed_work_cancel(&sys_monitor->sys_monitor_work);
}

