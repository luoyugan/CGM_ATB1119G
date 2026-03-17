/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPICACHE profile interface for Actions SoC
 */

#include <errno.h>
#include <kernel.h>
#include "soc.h"
#include "spicache.h"

#define SPI1_CACHE_SIZE (1024*32)


#ifdef CONFIG_SOC_SPICACHE_PROFILE
static void spicache_update_profile_data(struct spicache_profile *profile)
{
	profile->hit_cnt = sys_read32(SPI0_CACHE_RANGE_ADDR_HIT_COUNT) * 8;
	profile->miss_cnt = sys_read32(SPI0_CACHE_RANGE_ADDR_MISS_COUNT);
	profile->total_hit_cnt = sys_read32(SPI0_CACHE_TOTAL_HIT_COUNT) * 8;
	profile->total_miss_cnt = sys_read32(SPI0_CACHE_TOTAL_MISS_COUNT);
}

int spicache_profile_get_data(struct spicache_profile *profile)
{
	if (!profile)
		return -EINVAL;

	spicache_update_profile_data(profile);

	return 0;
}

int spicache_profile_stop(struct spicache_profile *profile)
{
	if (!profile)
		return -EINVAL;

	profile->end_time = k_uptime_get();
	spicache_update_profile_data(profile);

	sys_write32(sys_read32(SPI0_CACHE_CTL) & ~(1 << 4), SPI0_CACHE_CTL);

	return 0;
}

int spicache_profile_start(struct spicache_profile *profile)
{
	if (!profile)
		return -EINVAL;

	sys_write32(profile->start_addr, SPI0_CACHE_PROFILE_ADDR_START);
	sys_write32(profile->end_addr, SPI0_CACHE_PROFILE_ADDR_END);

	profile->start_time = k_uptime_get();

	sys_write32(sys_read32(SPI0_CACHE_CTL) | (1 << 4), SPI0_CACHE_CTL);

	return 0;
}
#endif

void *cache_to_uncache(void *vaddr)
{
	void *pvadr = NULL;

	if (buf_is_nor(vaddr)) {
		pvadr = (void *) (SPI0_UNCACHE_ADDR + (((uint32_t)vaddr) - SPI0_BASE_ADDR));
	}

	return pvadr;
}

void *uncache_to_cache(void *paddr)
{
	void *vaddr = NULL;

	if(buf_is_nor_un(paddr))
		vaddr = (void *) (SPI0_BASE_ADDR + (((uint32_t)paddr) - SPI0_UNCACHE_ADDR));

	return vaddr;

}

