/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <soc.h>
#include "arm_core_mpu_dev.h"
#include <linker/linker-defs.h>
#include <kernel_arch_data.h>
#include <arch/arm/aarch32/cortex_m/mpu/arm_mpu.h>

#define LOG_LEVEL CONFIG_MPU_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(mpu);

/**
 *  Get the number of supported MPU regions.
 */
static inline uint8_t get_num_regions(void)
{
#if defined(CONFIG_CPU_CORTEX_M0PLUS) || \
	defined(CONFIG_CPU_CORTEX_M3) || \
	defined(CONFIG_CPU_CORTEX_M4)
	/* Cortex-M0+, Cortex-M3, and Cortex-M4 MCUs may
	 * have a fixed number of 8 MPU regions.
	 */
	return 8;
#endif /* CPU_CORTEX_M0PLUS | CPU_CORTEX_M3 | CPU_CORTEX_M4 */
}


/* ARM Core MPU Driver API Implementation for ARM MPU */
/**
 * @brief enable the MPU
 */
void arm_core_mpu_enable(void)
{
	/* Enable MPU and use the default memory map as a
	 * background region for privileged software access.
	 */
	MPU->CTRL = MPU_CTRL_ENABLE_Msk | MPU_CTRL_PRIVDEFENA_Msk;

	/* Make sure that all the registers are set before proceeding */
	__DSB();
	__ISB();
}

/**
 * @brief disable the MPU
 */
void arm_core_mpu_disable(void)
{
	/* Force any outstanding transfers to complete before disabling MPU */
	__DMB();

	/* Disable MPU */
	MPU->CTRL = 0;
}

/* This internal function performs MPU region initialization.
 *
 * Note:
 *   The caller must provide a valid region index.
 */
static void region_init(const uint32_t index,
	const struct arm_mpu_region *region_conf)
{
	/* Select the region you want to access */
	MPU->RNR = index;
	/* Configure the region */
	MPU->RBAR = (region_conf->base & MPU_RBAR_ADDR_Msk)
				| MPU_RBAR_VALID_Msk | index;
	MPU->RASR = region_conf->attr.rasr | MPU_RASR_ENABLE_Msk;
	LOG_DBG("[%d] 0x%08x 0x%08x",
		index, region_conf->base, region_conf->attr.rasr);
}

int arm_core_mpu_init(void)
{
	uint32_t r_index;

	arm_core_mpu_disable();

	/* Program fixed regions configured at SOC definition. */
	for (r_index = 0U; r_index < mpu_config.num_regions; r_index++) {
		region_init(r_index, &mpu_config.mpu_regions[r_index]);
	}

	arm_core_mpu_enable();

	return 0;
}

/*
 * @brief MPU default configuration
 *
 * This function provides the default configuration mechanism for the Memory
 * Protection Unit (MPU).
 */
int arm_mpu_init(const struct device *arg)
{
	uint32_t r_index;

	LOG_INF("total region count: %d, config region count: %d", get_num_regions(), mpu_config.num_regions);

	arm_core_mpu_init();

	return 0;
}

SYS_INIT(arm_mpu_init, POST_KERNEL/*PRE_KERNEL_1*/,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
