/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file peripheral clock configuration macros for Actions SoC
 */

#ifndef	_ACTIONS_SOC_CLOCK_H_
#define	_ACTIONS_SOC_CLOCK_H_


#define	CLOCK_ID_DMA			0
#define	CLOCK_ID_SPI0			1
#define	CLOCK_ID_SPI2			2
#define	CLOCK_ID_SPI3			3
#define	CLOCK_ID_SPI0CACHE		4
#define	CLOCK_ID_EFUSE			5
#define	CLOCK_ID_TRNG			6
#define	CLOCK_ID_CAPTURE		7
#define	CLOCK_ID_PWM0			8
#define	CLOCK_ID_PWM1			9
#define	CLOCK_ID_PWM2			10
#define	CLOCK_ID_PWM3			11
#define	CLOCK_ID_PWM4			12
#define	CLOCK_ID_PWM5			13

#define	CLOCK_ID_LRADC			14
#define	CLOCK_ID_QD				15

#define	CLOCK_ID_I2C0			16
#define	CLOCK_ID_I2C1			17
#define	CLOCK_ID_I2C2			18
#define	CLOCK_ID_UART0			19
#define	CLOCK_ID_UART1			20
#define	CLOCK_ID_UART2			21
#define	CLOCK_ID_UART3			22
#define	CLOCK_ID_UART4			23
#define	CLOCK_ID_ADC			24
#define	CLOCK_ID_PDMTX			25
#define	CLOCK_ID_I2STX			26
#define	CLOCK_ID_I2SRX			27
#define	CLOCK_ID_CPUTIMER		28


#define	CLOCK_ID_RTC			32
#define	CLOCK_ID_KEY			33
#define	CLOCK_ID_GPIO			34

#define	CLOCK_ID_MAX_ID			35

#ifndef _ASMLANGUAGE

void acts_clock_peripheral_enable(int clock_id);
void acts_clock_peripheral_disable(int clock_id);
uint32_t clk_rate_get_corepll(void);
int clk_set_rate(int clock_id,  uint32_t rate_hz);
uint32_t clk_get_rate(int clock_id);
uint32_t acts_clock_rc32k_measure_result_high_res(void);
int acts_clock_rc32k_calibrate(void);
int acts_clock_rc32k_measure_then_calibrate(void);
uint8_t acts_clock_get_rc32k_param(void);
void acts_deep_sleep_clock_peripheral(const char *id_buffer, int buffer_len);
void acts_clock_rc64m_calibrate(void);

#endif /* _ASMLANGUAGE */

#endif /* _ACTIONS_SOC_CLOCK_H_	*/
