/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system wakelock
 */

#include <kernel.h>
#include <irq.h>
#include <sys_wakelock.h>

static uint32_t wakelocks_bitmaps;
static uint32_t wakelocks_free_timestamp;

static int _sys_wakelock_lookup(int wakelock_holder)
{
	if (wakelocks_bitmaps & (1 << wakelock_holder)) {
		return wakelock_holder;
	} else {
		return 0;
	}
}

int sys_wake_lock(int wakelock_holder)
{
	int res = 0;
	int wakelock = 0;
	uint32_t flags;

	flags = irq_lock();

	wakelock = _sys_wakelock_lookup(wakelock_holder);
	if (wakelock) {
		res = -EEXIST;
		goto exit;
	}

	wakelocks_bitmaps |= (1 << wakelock_holder);
	wakelocks_free_timestamp = 0;

exit:
	irq_unlock(flags);
	return res;
}

int sys_wake_unlock(int wakelock_holder)
{
	int res = 0;
	int wakelock = 0;
	uint32_t flags;

	flags = irq_lock();

	wakelock = _sys_wakelock_lookup(wakelock_holder);
	if (!wakelock) {
		res = -ESRCH;
		goto exit;
	}

	wakelocks_bitmaps &= (~(1 << wakelock_holder));

	if (!wakelocks_bitmaps)
		wakelocks_free_timestamp = k_uptime_get_32();

exit:
	irq_unlock(flags);
	return res;
}

int sys_wakelocks_init(void)
{
	wakelocks_free_timestamp = k_uptime_get_32();
	return 0;
}

int sys_wakelocks_check(void)
{
	return wakelocks_bitmaps;
}

uint32_t sys_wakelocks_get_free_time(void)
{
	if ((wakelocks_bitmaps) || (!wakelocks_free_timestamp))
		return 0;

	return k_uptime_get_32() - wakelocks_free_timestamp;
}

