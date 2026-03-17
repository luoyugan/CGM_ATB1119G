/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/slist.h>
#include <arch/arm/aarch32/cortex_m/mpu/arm_mpu.h>

#include "arm_mpu_mem_cfg.h"

static const struct arm_mpu_region mpu_regions[] = {
	/* Region 0 */
	MPU_REGION_ENTRY("FLASH_0",
			 0x01100000,
			 REGION_FLASH_ATTR(REGION_256K)),
	/* Region 1 */
	MPU_REGION_ENTRY("ROM",
			 0,
			 REGION_FLASH_ATTR(REGION_16K)),
};

const struct arm_mpu_config mpu_config = {
	.num_regions = ARRAY_SIZE(mpu_regions),
	.mpu_regions = mpu_regions,
};
