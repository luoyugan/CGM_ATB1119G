/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file security engine for Actions SoC
 */

#ifndef SOC_SE_H_
#define SOC_SE_H_

#include <device.h>

#define TRNG_CTRL         (SE_REG_BASE+0x0400)
#define TRNG_LR           (SE_REG_BASE+0x0404)
#define TRNG_MR           (SE_REG_BASE+0x0408)
#define TRNG_ILR          (SE_REG_BASE+0x040c)
#define TRNG_IMR          (SE_REG_BASE+0x0410)

int se_trng_init(void);
int se_trng_deinit(void);
uint32_t se_trng_process(uint32_t *trng_low, uint32_t *trng_high);

#endif /* SOC_SE_H_ */
