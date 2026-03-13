/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifdef CONFIG_SEGGER_SYSTEMVIEW

#include <kernel.h>
#include <SEGGER_SYSVIEW.h>
#include "SEGGER_SYSVIEW_Zephyr.h"

#if defined(CONFIG_SOC_LARK)
#define INT_ID_DESC		"I#2=TIM0,I#7=RTC,I#16=SD0,I#17=SD1,I#18=I2C0,I#19=I2C1," \
						"I#20=DSP,I#21=DSP1,I#27=DMA0,I#37=GPIO,I#43=LCD,I#49=DE," \
						"I#51=LRADC,I#56=BT,I#57=ANCDSP0,I#58=TWS,I#59=ANCDSP1"
#else
#define INT_ID_DESC		""
#endif

static void cbSendSystemDesc(void)
{
	SEGGER_SYSVIEW_SendSysDesc("N=ZephyrSysView");
	SEGGER_SYSVIEW_SendSysDesc("D=" CONFIG_BOARD " "
				   CONFIG_SOC_SERIES " " CONFIG_ARCH);
	SEGGER_SYSVIEW_SendSysDesc("O=Zephyr");
	SEGGER_SYSVIEW_SendSysDesc(INT_ID_DESC);
}

void SEGGER_SYSVIEW_Conf(void)
{
	SEGGER_SYSVIEW_Init(sys_clock_hw_cycles_per_sec(),
			    sys_clock_hw_cycles_per_sec(),
			    &SYSVIEW_X_OS_TraceAPI, cbSendSystemDesc);

	SEGGER_SYSVIEW_SetRAMBase(CONFIG_SRAM_BASE_ADDRESS);
}
#endif
