/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system monitor
 */

#ifndef _SYS_MONITOR_H
#define _SYS_MONITOR_H

#include <kernel.h>

#define CONFIG_MONITOR_PERIOD 150//500

/**
 * @defgroup sys_monitor_apis App system monitor APIs
 * @ingroup system_apis
 * @{
 */
void sys_monitor_init(void);
/**
 * @brief start sysmte monitor
 *
 * @details start system monitor , this rontine make system monitor work
 * can excute in system app context, system monitor used thread timer as
 * executor.
 *
 * @return N/A
 */

void sys_monitor_start(uint32_t ms);
/**
 * @brief stop sysmte monitor
 *
 * @details stop system monitor, all system monitor work will not excute.
 *
 * @return N/A
 */

void sys_monitor_stop(void);

/**
 * @cond INTERNAL_HIDDEN
 */
#define MAX_MONITOR_WORK_NUM 1

/**
 * @brief system monitor work handle
 *
 * @details system monitor work handle, add to system monitor by user.
 * and excute int system monitor context(system app context), excute by
 * thread timer or system work.
 *
 * @return 0 excute success .
 * @return others excute failed .
 */
typedef int (*monitor_work_handle)(void);

/** system monitor structure */
struct sys_monitor_t
{
	/** monitor stoped flag */
	u8_t monitor_stoped:1;
	/** system ready flag */
	u8_t system_ready:1;

	/** monitor works, register by other user */
	monitor_work_handle monitor_work[MAX_MONITOR_WORK_NUM];
	struct k_delayed_work sys_monitor_work;
};

/**
 * @brief add system monitor work to system monitor
 *
 * @details add work to system monitor, when system monitor start ,
 * the new work maybe excute by system monitor
 * @param monitor_work new work want to add to system monitor
 *
 * @return 0 excute success
 * @return others excute failed
 */

int sys_monitor_add_work(monitor_work_handle monitor_work);
/**
 * @brief system monitor init
 *
 * @details init system monitor , malloc system moitor resource
 * init thread timer as sysem monitor executor.
 *
 * @return N/A
 */
/**
 * INTERNAL_HIDDEN @endcond
 */
/**
 * @} end defgroup sys_monitor_apis
 */

#endif /* _SYS_MONITOR_H */

