/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _SYSTEM_APP_WDG_H_
#define _SYSTEM_APP_WDG_H_

void watchdog_disable(void);

void watchdog_clear_pending(void);

void watchdog_feed(void);

void watchdog_isr_feed(void);

int watchdog_enable(int timeout);

void watchdog_init(void);

#endif    /* _SYSTEM_APP_WDG_H_ */

