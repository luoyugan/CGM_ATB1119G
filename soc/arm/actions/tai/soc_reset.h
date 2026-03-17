/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file peripheral reset configuration macros for Actions SoC
 */

#ifndef	_ACTIONS_SOC_RESET_H_
#define	_ACTIONS_SOC_RESET_H_


#define	RESET_ID_DMA			0
#define	RESET_ID_SPI0			1
#define	RESET_ID_SPI2			2
#define	RESET_ID_SPI3			3
#define	RESET_ID_SPI0CACHE		4
#define	RESET_ID_TRNG			6
#define	RESET_ID_CAPTURE		7
#define	RESET_ID_PWM0			8
#define	RESET_ID_PWM1			9
#define	RESET_ID_PWM2			10
#define	RESET_ID_PWM3			11
#define	RESET_ID_PWM4			12
#define	RESET_ID_PWM5			13
#define	RESET_ID_QD				15

#define	RESET_ID_I2C0			16
#define	RESET_ID_I2C1			17
#define	RESET_ID_I2C2			18
#define	RESET_ID_UART0			19
#define	RESET_ID_UART1			20
#define	RESET_ID_UART2			21
#define	RESET_ID_UART3			22
#define	RESET_ID_UART4			23
#define	RESET_ID_ADC			24
#define	RESET_ID_PDMTX			25
#define	RESET_ID_I2S			26

#define	RESET_ID_RTC			32
#define	RESET_ID_KEY			33
#define	RESET_ID_BT				36
#define	RESET_ID_CPU1			37


#define	RESET_ID_MAX_ID			38

#ifndef _ASMLANGUAGE

void acts_reset_peripheral_assert(int reset_id);
void acts_reset_peripheral_deassert(int reset_id);
void acts_reset_peripheral(int reset_id);

#endif /* _ASMLANGUAGE */

#endif /* _ACTIONS_SOC_RESET_H_	*/
