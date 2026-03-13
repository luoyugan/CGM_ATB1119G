/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <drivers/timer/system_timer.h>
#include <sys_clock.h>
#include <spinlock.h>
#include <soc.h>

#define USE_T2_FOR_CYCLE

#define CYC_PER_TICK (sys_clock_hw_cycles_per_sec()	\
		      / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define MAX_TICKS ((TIMER_MAX_CYCLES_VALUE / CYC_PER_TICK) - 1)
#define MAX_CYCLES (MAX_TICKS * CYC_PER_TICK)

#define TICKLESS (IS_ENABLED(CONFIG_TICKLESS_KERNEL))

#define RTC_TIMER_SYSTEM_CMP_INDEX (0)

static struct k_spinlock lock;

static uint32_t last_load;

/*
 * This local variable holds the amount of SysTick HW cycles elapsed
 * and it is updated in z_clock_isr() and z_clock_set_timeout().
 *
 * Note:
 *  At an arbitrary point in time the "current" value of the SysTick
 *  HW timer is calculated as:
 *
 * t = cycle_counter + elapsed();
 */
static uint64_t cycle_count;

static uint64_t last_elapsed;

#if defined(CONFIG_PM_DEVICE)
static uint32_t sleep_cycle_count, sleep_st_cycle;
#endif

/*
 * This local variable holds the amount of elapsed SysTick HW cycles
 * that have been announced to the kernel.
 */
static uint64_t announced_cycles;

static uint32_t acts_timer_elapsed(uint32_t cur_cycle)
{
	uint32_t val1, ret = 0;
	val1 = cur_cycle;

	if (val1 > last_elapsed) {
		ret = val1 - last_elapsed;
	} else {
		ret = (TIMER_MAX_CYCLES_VALUE - last_elapsed) + val1;
	}

	return ret;
}

/* Callout out of platform assembly, not hooked via IRQ_CONNECT... */
void acts_timer_isr(void *arg)
{
	ARG_UNUSED(arg);
	uint32_t dticks, cur_cycle;

	/* Increment the amount of HW cycles elapsed (complete counter
	 * cycles) and announce the progress to the kernel.
	 */
	acts_rtc_setup_hosc_msk();
	acts_rtc_cmp_clear_pending(RTC_TIMER_SYSTEM_CMP_INDEX);

	cur_cycle = acts_rtc_get_cnt();
	uint32_t pending = acts_timer_elapsed(cur_cycle);
	last_elapsed = cur_cycle;

	cycle_count += pending;

	// TODO: rtc cnt is always timed with the HOSC clock. 
	// Compensation is required only when rc32K is significantly different between entering and exiting deepsleep, for example, temperature change.
	// if (sleep_cycle_count) {
	// 	printk("sleep_cycle_count-%d\n",sleep_cycle_count);
	// 	cycle_count += sleep_cycle_count;
	// 	sleep_cycle_count = 0;
	// }

	if (TICKLESS) {
		dticks = (cycle_count - announced_cycles) / CYC_PER_TICK;
		announced_cycles += ((uint64_t)dticks) * ((uint64_t)CYC_PER_TICK);
		if (dticks)
			z_clock_announce(dticks);
	} else {
		z_clock_announce(1);
	}
}

int z_clock_driver_init(const struct device *device)
{
	uint32_t cmp_val;

	acts_rtc_init(RTC_CLKSRC_SEL_HOSC, CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);

	last_load = CYC_PER_TICK;
	cmp_val = acts_rtc_new_cmp_val(last_load - 1);

	acts_rtc_cmp_enable(RTC_TIMER_SYSTEM_CMP_INDEX, cmp_val, true, acts_timer_isr);

	return 0;
}

void z_clock_set_timeout(int32_t ticks, bool idle)
{
#if defined(CONFIG_TICKLESS_KERNEL)
	uint32_t delay, cmp_val, cur_cycle;

	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : ticks;
	if(!ticks)
		ticks = 1;
	ticks = CLAMP(ticks - 1, 0, (int32_t)MAX_TICKS - 5000);

	k_spinlock_key_t key = k_spin_lock(&lock);

	cur_cycle = acts_rtc_get_cnt();
	uint32_t pending = acts_timer_elapsed(cur_cycle);
	last_elapsed = cur_cycle;

	cycle_count += pending;

	uint32_t unannounced = cycle_count - announced_cycles;

	if ((int32_t)unannounced < 0) {
		/* We haven't announced for more than half the 32-bit
		 * wrap duration, because new timeouts keep being set
		 * before the existing one fires.  Force an announce
		 * to avoid loss of a wrap event, making sure the
		 * delay is at least the minimum delay possible.
		 */
		//last_load = MIN_DELAY;
		last_load = CYC_PER_TICK;
	} else {
		/* Desired delay in the future */
		delay = ticks * CYC_PER_TICK;
		/* Round delay up to next tick boundary */
		delay += unannounced;
		delay =
		 ((delay + CYC_PER_TICK - 1) / CYC_PER_TICK) * CYC_PER_TICK;
		delay -= unannounced;
		delay = MAX(delay, CYC_PER_TICK);
		if (delay > MAX_CYCLES) {
			last_load = MAX_CYCLES;
		} else {
			last_load = delay;
		}
	}

	cmp_val = acts_rtc_new_cmp_val(last_load - 1);
	acts_rtc_cmp_update(RTC_TIMER_SYSTEM_CMP_INDEX, cmp_val);

	k_spin_unlock(&lock, key);
#endif
}

#if defined(CONFIG_PM_DEVICE)
int z_clock_device_ctrl(const struct device *device,
			       uint32_t command,
			       void *context, device_pm_cb cb, void *arg)
{
	k_spinlock_key_t key;
	int ret = 0;
	uint32_t state = *((uint32_t*)context);

	if (command != DEVICE_PM_SET_POWER_STATE)
		return 0;

	key = k_spin_lock(&lock);
	switch (state) {
	case DEVICE_PM_ACTIVE_STATE:
		if (sleep_st_cycle) {
			acts_rtc_setup(RTC_CLKSRC_SEL_HOSC);
			sleep_cycle_count = acts_rtc_get_cnt_in_rc32k();
			sleep_st_cycle = 0;
		}
		break;
	case DEVICE_PM_SUSPEND_STATE:
		sleep_st_cycle = 1;
		acts_rtc_setup(RTC_CLKSRC_SEL_RC32K);
		break;
	case DEVICE_PM_EARLY_SUSPEND_STATE:
	case DEVICE_PM_LATE_RESUME_STATE:
	case DEVICE_PM_LOW_POWER_STATE:
	case DEVICE_PM_OFF_STATE:
		break;
	default:
		ret = -ESRCH;
	}

	k_spin_unlock(&lock, key);
	return ret;
}

#endif

uint32_t z_clock_elapsed(void)
{
	if (!TICKLESS)
		return 0;
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t cyc = acts_timer_elapsed(acts_rtc_get_cnt()) + cycle_count - announced_cycles;
	k_spin_unlock(&lock, key);
	return cyc / CYC_PER_TICK;
}

uint32_t z_timer_cycle_get_32(void)
{
	return acts_rtc_get_cnt();
}

void z_clock_idle_exit(void)
{

}
void sys_clock_disable(void)
{

}

