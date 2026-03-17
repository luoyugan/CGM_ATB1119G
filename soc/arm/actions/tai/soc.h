/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the atb111x family processors.
 *
 */

#ifndef _TAI_SOC_H_
#define _TAI_SOC_H_

#include <sys/util.h>

#ifndef _ASMLANGUAGE

#include <soc_regs.h>
#include "soc_clock.h"
#include "soc_reset.h"
#include "soc_irq.h"
#include "soc_gpio.h"
#include "soc_pinmux.h"
#include "soc_pm.h"
#include "soc_boot.h"
#include "soc_memctrl.h"
#include "soc_se.h"
#include "soc_timer.h"
#include "soc_saradc.h"
#include "soc_efuse.h"

#define __MPU_PRESENT           1

#endif /* !_ASMLANGUAGE */

#endif /* _TAI_SOC_H_ */
