/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file register address define for Actions SoC
 */

#ifndef	_ACTIONS_SOC_REGS_H_
#define	_ACTIONS_SOC_REGS_H_
#include "brom_interface.h"

/********************************** REGISTER BASE ADDRESS **********************************/
#define RMU_REG_BASE				0x40000000
#define CMUD_REG_BASE				0x40001000
#define CMUA_REG_BASE				0x40000100
#define PMU_REG_BASE				0x40004000
#define GPIO_REG_BASE				0x40068000
#define DMA_REG_BASE				0x4001c000
#define UART0_REG_BASE				0x40038000
#define UART1_REG_BASE				0x4003C000
#define UART2_REG_BASE				0x40040000
#define UART3_REG_BASE				0x40044000
#define UART4_REG_BASE				0x40048000
#define MEM_REG_BASE                0x40010000
#define SE_REG_BASE                 0x40020000
#define I2C0_REG_BASE				0x40050000
#define I2C1_REG_BASE				0x40054000
#define I2C2_REG_BASE				0x40058000
#define SPI0_REG_BASE				0x40028000
#define SPI2_REG_BASE				0x40030000
#define SPI3_REG_BASE				0x40034000
#define BTC_REG_BASE                0x01100000
#define INTC_BASE					0x40002000
#define RTC_BASE      				0x4000C000
#define AUDIO_ADC_REG_BASE          0x4005C100
#define AUDIO_I2STX0_REG_BASE       0x4005C400
#define AUDIO_I2SRX0_REG_BASE       0x4005C414
#define AUDIO_PDMTX_REG_BASE        0x4005C000

#define PWM_REG_BASE                0x40060000
#define PWM_CLK0_BASE               0x40001054

#define KEY_REG_BASE                0x40064000
#define CAPTURE_REG_BASE            0x4006c000

/* GPIO PIN macro */
#define GPIO_PIN(x)                 (GPIO_REG_BASE + 4 + (x * 4))

/************************************** CMUA_REG_BASE **************************************/
#define HOSC_CTL					(CMUA_REG_BASE + 0x00)
#define RC64M_CTL					(CMUA_REG_BASE + 0x04)
#define LOSC_CTL					(CMUA_REG_BASE + 0x08)
#define RC32K_CTL					(CMUA_REG_BASE + 0x10)
#define LFRC_COUNTCTL               (CMUA_REG_BASE + 0x14)
#define LFRC_COUNTER                (CMUA_REG_BASE + 0x18)
#define CMU_ANADEBUG                (CMUA_REG_BASE + 0x20)

#define HOSC_CTL_HOSC_READY					(24)
/************************************** CMUD_REG_BASE **************************************/
#define CMU_SYSCLK					(CMUD_REG_BASE + 0x00)
#define CMU_DEVCLKEN0				(CMUD_REG_BASE + 0x08)
#define CMU_DEVCLKEN1				(CMUD_REG_BASE + 0x0c)

#define CMU_SPI0CLK                 (CMUD_REG_BASE + 0x10)
#define CMU_SPI2CLK                 (CMUD_REG_BASE + 0x14)
#define CMU_SPI3CLK                 (CMUD_REG_BASE + 0x18)

#define CMU_TRNGCLK                 (CMUD_REG_BASE + 0x20)

#define CMU_CAPCLK                  (CMUD_REG_BASE + 0x24)
#define CMU_PWM0CLK                 (CMUD_REG_BASE + 0x28)

#define CMU_LRADCCLK				(CMUD_REG_BASE + 0x40)

#define CMU_QDCLK                   (CMUD_REG_BASE + 0x44)

#define CMU_ADCCLK                  (CMUD_REG_BASE + 0x50)
#define CMU_PDMTXCLK                (CMUD_REG_BASE + 0x54)
#define CMU_I2SCLK                  (CMUD_REG_BASE + 0x58)

#define CMU_RTCCLK                  (CMUD_REG_BASE + 0x60)

#define CMU_KEYCLK                  (CMUD_REG_BASE + 0x64)
#define CMU_GPIOCLK                 (CMUD_REG_BASE + 0x68)
#define CMU_PMUSCLK                 (CMUD_REG_BASE + 0x70)

#define CMU_BTCLK                   (CMUD_REG_BASE + 0x74)

#define CMU_S1MCLKCTL               (CMUD_REG_BASE + 0x80)
#define CMU_S1BTCLKCTL              (CMUD_REG_BASE + 0x84)
#define CMU_S3CLKCTL                (CMUD_REG_BASE + 0x88)

#define CMU_MEMCLKEN                (CMUD_REG_BASE + 0x90)
#define CMU_MEMCLKSRC               (CMUD_REG_BASE + 0x98)

/*************************************** RMU_REG_BASE ***************************************/
#define RMU_MRCR0					(RMU_REG_BASE + 0x00)
#define RMU_MRCR1					(RMU_REG_BASE + 0x04)
#define	CHIPVERSION                 (RMU_REG_BASE + 0xA0)

/***************************************** INTC_BASE *****************************************/
#define INT_TO_BT_CPU				(INTC_BASE + 0x00)
#define PENDING_FROM_BT_CPU         (INTC_BASE + 0x04)

#define SPINOR_API_ADDR				(p_brom_api->p_spinor_api)

// Interaction RAM
#define INTER_RAM_ADDR			(CONFIG_SRAM_BASE_ADDRESS + (CONFIG_SRAM_SIZE * 1024) + CONFIG_SRAM_EXTRA_SIZE)
#define INTER_RAM_SIZE			(CONFIG_SRAM_BASE_ADDRESS + (CONFIG_SRAM_TOTAL_SIZE * 1024) - INTER_RAM_ADDR)

/************************************** SPICACHE_CONTROL_REGISTER **************************************/
#define SPICACHE_CONTROL_REGISTER_BASE                              (0x40014000)
#define SPI0_CACHE_CTL                                              (SPICACHE_CONTROL_REGISTER_BASE + 0x0000)
#define SPI0_CACHE_INVALIDATE                                       (SPICACHE_CONTROL_REGISTER_BASE + 0x0004)
#define SPI0_CACHE_TOTAL_MISS_COUNT                                 (SPICACHE_CONTROL_REGISTER_BASE + 0x0010)
#define SPI0_CACHE_TOTAL_HIT_COUNT                                  (SPICACHE_CONTROL_REGISTER_BASE + 0x0014)
#define SPI0_CACHE_PROFILE_INDEX_START                              (SPICACHE_CONTROL_REGISTER_BASE + 0x0018)
#define SPI0_CACHE_PROFILE_INDEX_END                                (SPICACHE_CONTROL_REGISTER_BASE + 0x001C)
#define SPI0_CACHE_RANGE_INDEX_MISS_COUNT                           (SPICACHE_CONTROL_REGISTER_BASE + 0x0020)
#define SPI0_CACHE_RANGE_INDEX_HIT_COUNT                            (SPICACHE_CONTROL_REGISTER_BASE + 0x0024)
#define SPI0_CACHE_PROFILE_ADDR_START                               (SPICACHE_CONTROL_REGISTER_BASE + 0x0028)
#define SPI0_CACHE_PROFILE_ADDR_END                                 (SPICACHE_CONTROL_REGISTER_BASE + 0x002C)
#define SPI0_CACHE_RANGE_ADDR_MISS_COUNT                            (SPICACHE_CONTROL_REGISTER_BASE + 0x0030)
#define SPI0_CACHE_RANGE_ADDR_HIT_COUNT                             (SPICACHE_CONTROL_REGISTER_BASE + 0x0034)
#define MAPPING_MISS_ADDR                                           (SPICACHE_CONTROL_REGISTER_BASE + 0x0060)

/****************************************** InterruptController_BASE ******************************************/
#define InterruptController_BASE                                    (0xe000e000)
#define NVIC_ISER0                                                  (InterruptController_BASE + 0x100)
#define NVIC_ICER0                                                  (InterruptController_BASE + 0x180)
#define NVIC_ISPR0                                                  (InterruptController_BASE + 0x200)
#define NVIC_ICPR0                                                  (InterruptController_BASE + 0x280)
#define NVIC_IABR0                                                  (InterruptController_BASE + 0x300)
#define NVIC_IPR0                                                   (InterruptController_BASE + 0x400)
#define NVIC_IPR1                                                   (InterruptController_BASE + 0x404)
#define NVIC_IPR2                                                   (InterruptController_BASE + 0x408)
#define NVIC_IPR3                                                   (InterruptController_BASE + 0x40c)
#define NVIC_IPR4                                                   (InterruptController_BASE + 0x410)
#define NVIC_IPR5                                                   (InterruptController_BASE + 0x414)
#define NVIC_IPR6                                                   (InterruptController_BASE + 0x418)
#define NVIC_IPR7                                                   (InterruptController_BASE + 0x41c)

/*********************************************** RTC_REMAIN ***********************************************/
#define RTC_REMAIN0                                                 (RTC_BASE+0x0060)
#define RTC_REMAIN1                                                 (RTC_BASE+0x0064)
#define RTC_REMAIN2                                                 (RTC_BASE+0x0068)
#define RTC_REMAIN3                                                 (RTC_BASE+0x006c)
#define RTC_REMAIN4                                                 (RTC_BASE+0x0070)
#define RTC_REMAIN5                                                 (RTC_BASE+0x0074)

#define WD_CTL        				                                (RTC_BASE + 0x40)

/*********************************************** PMU_REG_BASE ***********************************************/
#define PMUVDDRET_BASE                                              (0x40004000)
#define SYSTEM_CTL                                                  (PMUVDDRET_BASE + 0x00)
#define VOUT_CTL_S1                                                 (PMUVDDRET_BASE + 0x04)
#define VOUT_CTL_S3                                                 (PMUVDDRET_BASE + 0x08)
#define DCDC_CTL                                                    (PMUVDDRET_BASE + 0x0C)
#define VREF_CTL                                                    (PMUVDDRET_BASE + 0x10)
#define POWER_CTL                                                   (PMUVDDRET_BASE + 0x14)
#define RAMPWR_CTL                                                  (PMUVDDRET_BASE + 0x20)
#define RAMPWR                                                      (PMUVDDRET_BASE + 0x24)
#define LPCMP_INTMASK                                               (PMUVDDRET_BASE + 0x28)
#define PMU_PD                                                      (PMUVDDRET_BASE + 0x2C)
#define ADC_CMP                                                     (PMUVDDRET_BASE + 0x30)
#define LPCMP_CTL                                                   (PMUVDDRET_BASE + 0x100)
#define LPCMP_CH(x)                                                 (PMUVDDRET_BASE + 0x104 + (x)*4)

#define SYSTEM_CTL_S3_S1_CYCLEM_E                                   (19)
#define SYSTEM_CTL_S3_S1_CYCLEM_SHIFT                               (16)
#define SYSTEM_CTL_S3_S1_CYCLEM_MASK                                (0xF << 16)
#define SYSTEM_CTL_S3_S1_CYCLEN_E                                   (15)
#define SYSTEM_CTL_S3_S1_CYCLEN_SHIFT                               (12)
#define SYSTEM_CTL_S3_S1_CYCLEN_MASK                                (0xF << 12)
#define SYSTEM_CTL_S1_HOLD                                          (9)
#define SYSTEM_CTL_VDIG_SET_E                                       (6)
#define SYSTEM_CTL_VDIG_SET_SHIFT                                   (5)
#define SYSTEM_CTL_VDIG_SET_MASK                                    (0x3 << 5)
#define SYSTEM_CTL_BDG1V8_EN                                        (4)
#define SYSTEM_CTL_VC18SPD                                          (3)
#define SYSTEM_CTL_VD12SPD                                          (2)
#define SYSTEM_CTL_VDDSPD                                           (1)
#define SYSTEM_CTL_VFLASH_EN                                        (0)

#define VOUT_CTL_S1_BDG1V2_EN_S1                                    (24)
#define VOUT_CTL_S1_EN_LGBIAS_S1                                    (23)
#define VOUT_CTL_S1_VC18_EN_S1                                      (22)
#define VOUT_CTL_S1_VD12_SW_S1_E                                    (21)
#define VOUT_CTL_S1_VD12_SW_S1_SHIFT                                (20)
#define VOUT_CTL_S1_VD12_SW_S1_MASK                                 (0x3 << 20)
#define VOUT_CTL_S1_VC18FB_SET_S1                                   (19)
#define VOUT_CTL_S1_VD12FB_SET_S1                                   (18)
#define VOUT_CTL_S1_VDD_EN_S1                                       (17)
#define VOUT_CTL_S1_VD12_SET_S1BT_E                                 (15)
#define VOUT_CTL_S1_VD12_SET_S1BT_SHIFT                             (12)
#define VOUT_CTL_S1_VD12_SET_S1BT_MASK                              (0xF << 12)
#define VOUT_CTL_S1_VD12_SET_S1M_E                                  (11)
#define VOUT_CTL_S1_VD12_SET_S1M_SHIFT                              (8)
#define VOUT_CTL_S1_VD12_SET_S1M_MASK                               (0xF << 8)
#define VOUT_CTL_S1_VDD_SET_S1BT_E                                  (7)
#define VOUT_CTL_S1_VDD_SET_S1BT_SHIFT                              (4)
#define VOUT_CTL_S1_VDD_SET_S1BT_MASK                               (0xF << 4)
#define VOUT_CTL_S1_VDD_SET_S1M_E                                   (3)
#define VOUT_CTL_S1_VDD_SET_S1M_SHIFT                               (0)
#define VOUT_CTL_S1_VDD_SET_S1M_MASK                                (0xF << 0)

#define VOUT_CTL_S3_BDG1V2_EN_S3                                    (24)
#define VOUT_CTL_S3_VC18_EN_S3                                      (22)
#define VOUT_CTL_S3_VD12_EN_S3                                      (20)

#define DCDC_CTL_EN_ZERO                                            (30)
#define DCDC_CTL_EN_RING                                            (29)
#define DCDC_CTL_ZERO_IBSEL                                         (28)
#define DCDC_CTL_EN_ZRCL                                            (27)
#define DCDC_CTL_IZSL_E                                             (26)
#define DCDC_CTL_IZSL_SHIFT                                         (21)
#define DCDC_CTL_IZSL_MASK                                          (0x3F << 21)
#define DCDC_CTL_NDT_E                                              (20)
#define DCDC_CTL_NDT_SHIFT                                          (19)
#define DCDC_CTL_NDT_MASK                                           (0x3 << 19)
#define DCDC_CTL_PDT_E                                              (18)
#define DCDC_CTL_PDT_SHIFT                                          (17)
#define DCDC_CTL_PDT_MASK                                           (0x3 << 17)
#define DCDC_CTL_IPK_SEL_E                                          (15)
#define DCDC_CTL_IPK_SEL_SHIFT                                      (14)
#define DCDC_CTL_IPK_SEL_MASK                                       (0x3 << 14)
#define DCDC_CTL_LGBIAS_SEL                                         (13)
#define DCDC_CTL_DB_SEL                                             (12)

#define VREF_CTL_VC18_BIAS_E                                        (31)
#define VREF_CTL_VC18_BIAS_SHIFT                                    (30)
#define VREF_CTL_VC18_BIAS_MASK                                     (0x3 << 30)
#define VREF_CTL_VD12_BIAS_E                                        (29)
#define VREF_CTL_VD12_BIAS_SHIFT                                    (28)
#define VREF_CTL_VD12_BIAS_MASK                                     (0x3 << 28)
#define VREF_CTL_RC8K_SET_E                                         (27)
#define VREF_CTL_RC8K_SET_SHIFT                                     (26)
#define VREF_CTL_RC8K_SET_MASK                                      (0x3 << 26)
#define VREF_CTL_DIAG_HV_SOC_INT_OUT                                (25)
#define VREF_CTL_SENS_BIAS_EFUSE_E                                  (24)
#define VREF_CTL_SENS_BIAS_EFUSE_SHIFT                              (20)
#define VREF_CTL_SENS_BIAS_EFUSE_MASK                               (0x1F << 20)
#define VREF_CTL_SEN_BIAS_TEST                                      (19)
#define VREF_CTL_VREF12_ADC_BUF                                     (18)
#define VREF_CTL_VDDRET_BUF                                         (17)
#define VREF_CTL_VDDRET_VOL                                         (16)
#define VREF_CTL_VDDRET_CAL08_E                                     (15)
#define VREF_CTL_VDDRET_CAL08_SHIFT                                 (12)
#define VREF_CTL_VDDRET_CAL08_MASK                                  (0xF << 12)
#define VREF_CTL_VDDRET_CAL07_E                                     (11)
#define VREF_CTL_VDDRET_CAL07_SHIFT                                 (8)
#define VREF_CTL_VDDRET_CAL07_MASK                                  (0xF << 8)
#define VREF_CTL_VREF18_BUF                                         (7)
#define VREF_CTL_VREF12_BUF                                         (6)
#define VREF_CTL_VREF06_BUF                                         (5)
#define VREF_CTL_VREF06_VOL_E                                       (4)
#define VREF_CTL_VREF06_VOL_SHIFT                                   (0)
#define VREF_CTL_VREF06_VOL_MASK                                    (0x1F << 0)

#define POWER_CTL_LB_EN                                             (1)
#define POWER_CTL_ENTER_S5                                          (0)

#define RAMPWR_CTL_CACHE_CTL_E                                      (31)
#define RAMPWR_CTL_CACHE_CTL_SHIFT                                  (30)
#define RAMPWR_CTL_CACHE_CTL_MASK                                   (0x3 << 30)
#define RAMPWR_CTL_RAM6_01_CTL_E                                    (29)
#define RAMPWR_CTL_RAM6_01_CTL_SHIFT                                (28)
#define RAMPWR_CTL_RAM6_01_CTL_MASK                                 (0x3 << 28)
#define RAMPWR_CTL_RAM5_CTL_E                                       (27)
#define RAMPWR_CTL_RAM5_CTL_SHIFT                                   (26)
#define RAMPWR_CTL_RAM5_CTL_MASK                                    (0x3 << 26)
#define RAMPWR_CTL_RAM4_01_CTL_E                                    (25)
#define RAMPWR_CTL_RAM4_01_CTL_SHIFT                                (24)
#define RAMPWR_CTL_RAM4_01_CTL_MASK                                 (0x3 << 24)
#define RAMPWR_CTL_RAM3_1_CTL_E                                     (23)
#define RAMPWR_CTL_RAM3_1_CTL_SHIFT                                 (22)
#define RAMPWR_CTL_RAM3_1_CTL_MASK                                  (0x3 << 22)
#define RAMPWR_CTL_RAM3_0_CTL_E                                     (21)
#define RAMPWR_CTL_RAM3_0_CTL_SHIFT                                 (20)
#define RAMPWR_CTL_RAM3_0_CTL_MASK                                  (0x3 << 20)
#define RAMPWR_CTL_RAM2_CTL_E                                       (19)
#define RAMPWR_CTL_RAM2_CTL_SHIFT                                   (18)
#define RAMPWR_CTL_RAM2_CTL_MASK                                    (0x3 << 18)
#define RAMPWR_CTL_RAM1_CTL_E                                       (17)
#define RAMPWR_CTL_RAM1_CTL_SHIFT                                   (16)
#define RAMPWR_CTL_RAM1_CTL_MASK                                    (0x3 << 16)
#define RAMPWR_CTL_RAM0_01_CTL                                      (15)

#define RAMPWR_CACHE_RET_EN                                         (23)
#define RAMPWR_CACHE_PD_EN                                          (22)
#define RAMPWR_RAM6_1_RET_EN                                        (21)
#define RAMPWR_RAM6_1_PD_EN                                         (20)
#define RAMPWR_RAM6_0_RET_EN                                        (19)
#define RAMPWR_RAM6_0_PD_EN                                         (18)
#define RAMPWR_RAM5_RET_EN                                          (17)
#define RAMPWR_RAM5_PD_EN                                           (16)
#define RAMPWR_RAM4_1_RET_EN                                        (15)
#define RAMPWR_RAM4_1_PD_EN                                         (14)
#define RAMPWR_RAM4_0_RET_EN                                        (13)
#define RAMPWR_RAM4_0_PD_EN                                         (12)
#define RAMPWR_RAM3_1_RET_EN                                        (11)
#define RAMPWR_RAM3_1_PD_EN                                         (10)
#define RAMPWR_RAM3_0_RET_EN                                        (9)
#define RAMPWR_RAM3_0_PD_EN                                         (8)
#define RAMPWR_RAM2_RET_EN                                          (7)
#define RAMPWR_RAM2_PD_EN                                           (6)
#define RAMPWR_RAM1_RET_EN                                          (5)
#define RAMPWR_RAM1_PD_EN                                           (4)
#define RAMPWR_RAM0_1_RET_EN                                        (3)
#define RAMPWR_RAM0_1_PD_EN                                         (2)
#define RAMPWR_RAM0_0_RET_EN                                        (1)
#define RAMPWR_RAM0_0_PD_EN                                         (0)

#define LPCMP_INTMASK_CMP_CH_INTEN(x)                               (x)

#define PMU_PD_SYSRESET_PD                                          (13)
#define PMU_PD_POWEROK_PD                                           (12)
#define PMU_PD_S3_PD                                                (11)
#define PMU_PD_EXS1M_PD                                             (10)
#define PMU_PD_EXS1BT_PD                                            (9)
#define PMU_PD_S1M_REQ                                              (8)
#define PMU_PD_S1BT_REQ                                             (7)

#define PMU_PD_CMP_CH_PD(x)                                         (x)

#define ADC_CMP_SENS_HIGH_E                                         (31)
#define ADC_CMP_SENS_HIGH_SHIFT                                     (22)
#define ADC_CMP_SENS_HIGH_MASK                                      (0x3FF << 22)
#define ADC_CMP_SENS_LOW_E                                          (21)
#define ADC_CMP_SENS_LOW_SHIFT                                      (12)
#define ADC_CMP_SENS_LOW_MASK                                       (0x3FF << 12)
#define ADC_CMP_SENS_CMP_S1EN                                       (11)
#define ADC_CMP_IOVCC_CMP_S1EN                                      (10)
#define ADC_CMP_IOVCC_LOW_E                                         (9)
#define ADC_CMP_IOVCC_LOW_SHIFT                                     (0)
#define ADC_CMP_IOVCC_LOW_MASK                                      (0x3FF << 0)

#define LPCMP_CTL_DEBOUNCE_SET_E                                    (9)
#define LPCMP_CTL_DEBOUNCE_SET_SHIFT                                (8)
#define LPCMP_CTL_DEBOUNCE_SET_MASK                                 (0x3 << 8)
#define LPCMP_CTL_LPCMP_EN                                          (7)

#define LPCMP_CTL_CH_EN(x)											(x)

#define LPCMP_CH_TRIG_SET_E                                        (19)
#define LPCMP_CH_TRIG_SET_SHIFT                                    (18)
#define LPCMP_CH_TRIG_SET_MASK                                     (0x3 << 18)

#define LPCMP_CH_READY                                             (17)
#define LPCMP_CH_RESULT_HIGH                                       (16)
#define LPCMP_CH_RESULT_LOW                                        (15)
#define LPCMP_CH_INPUT_SEL_E                                       (14)
#define LPCMP_CH_INPUT_SEL_SHIFT                                   (12)
#define LPCMP_CH_INPUT_SEL_MASK                                    (0x7 << 12)
#define LPCMP_CH_REF_TH_HIGH_E                                     (11)
#define LPCMP_CH_REF_TH_HIGH_SHIFT                                 (7)
#define LPCMP_CH_REF_TH_HIGH_MASK                                  (0x1F << 7)
#define LPCMP_CH_REF_TH_LOW_E                                      (6)
#define LPCMP_CH_REF_TH_LOW_SHIFT                                  (2)
#define LPCMP_CH_REF_TH_LOW_MASK                                   (0x1F << 2)

#define LPCMP_CH_REF_SEL_E                                         (1)
#define LPCMP_CH_REF_SEL_SHIFT                                     (0)
#define LPCMP_CH_REF_SEL_MASK                                      (0x3 << 0)
#define LPCMP_CH_REF_VREF18                                        (0)
#define LPCMP_CH_REF_IOVCC                                         (1)
#define LPCMP_CH_REF_IO2                                           (2)
#define LPCMP_CH_REF_IO4                                           (3)

#define SARADC_BASE                                                 (0x40004200)
#define SARADC_CTL                                                  (SARADC_BASE + 0x00)
#define SARADC_INTMASK                                              (SARADC_BASE + 0x04)
#define SARADC_PD                                                   (SARADC_BASE + 0x08)
#define SARADCDIG_CTL                                               (SARADC_BASE + 0x0C)
#define IOVCCADC_DATA                                               (SARADC_BASE + 0x10)
#define SENSADC_DATA                                                (SARADC_BASE + 0x14)
#define LRADC0_DATA                                                 (SARADC_BASE + 0x18)
#define LRADC1_DATA                                                 (SARADC_BASE + 0x1C)
#define LRADC2_DATA                                                 (SARADC_BASE + 0x20)
#define LRADC3_DATA                                                 (SARADC_BASE + 0x24)
#define LRADC4_DATA                                                 (SARADC_BASE + 0x28)
#define LRADC5_DATA                                                 (SARADC_BASE + 0x2C)

#define SARADC_CTL_SARADC_EN                                        (23)
#define SARADC_CTL_TEST_SARAD                                       (22)
#define SARADC_CTL_REG_IBIAS_BUF_E                                  (21)
#define SARADC_CTL_REG_IBIAS_BUF_SHIFT                              (20)
#define SARADC_CTL_REG_IBIAS_BUF_MASK                               (0x3 << 20)
#define SARADC_CTL_REG_IBIAS_ADC_E                                  (19)
#define SARADC_CTL_REG_IBIAS_ADC_SHIFT                              (18)
#define SARADC_CTL_REG_IBIAS_ADC_MASK                               (0x3 << 18)
#define SARADC_CTL_LRADC5_SCAL                                      (17)
#define SARADC_CTL_LRADC4_SCAL                                      (16)
#define SARADC_CTL_LRADC3_SCAL                                      (15)
#define SARADC_CTL_LRADC2_SCAL                                      (14)
#define SARADC_CTL_LRADC1_SCAL                                      (13)
#define SARADC_CTL_LRADC0_SCAL                                      (12)
#define SARADC_CTL_VREF_SEL                                         (11)
#define SARADC_CTL_LRADC5_CHEN                                      (7)
#define SARADC_CTL_LRADC4_CHEN                                      (6)
#define SARADC_CTL_LRADC3_CHEN                                      (5)
#define SARADC_CTL_LRADC2_CHEN                                      (4)
#define SARADC_CTL_LRADC1_CHEN                                      (3)
#define SARADC_CTL_LRADC0_CHEN                                      (2)
#define SARADC_CTL_SENSOR_CHEN                                      (1)
#define SARADC_CTL_IOVCC_CHEN                                       (0)

#define SARADC_INTMASK_LRADC5_INTEN                                 (7)
#define SARADC_INTMASK_LRADC4_INTEN                                 (6)
#define SARADC_INTMASK_LRADC3_INTEN                                 (5)
#define SARADC_INTMASK_LRADC2_INTEN                                 (4)
#define SARADC_INTMASK_LRADC1_INTEN                                 (3)
#define SARADC_INTMASK_LRADC0_INTEN                                 (2)
#define SARADC_INTMASK_SENSOR_INTEN                                 (1)
#define SARADC_INTMASK_IOVCC_INTEN                                  (0)

#define SARADC_PD_SENS_HIGH_PD                                      (10)
#define SARADC_PD_SENS_LOW_PD                                       (9)
#define SARADC_PD_IOVCC_LOW_PD                                      (8)
#define SARADC_PD_LRADC5_PD                                         (7)
#define SARADC_PD_LRADC4_PD                                         (6)
#define SARADC_PD_LRADC3_PD                                         (5)
#define SARADC_PD_LRADC2_PD                                         (4)
#define SARADC_PD_LRADC1_PD                                         (3)
#define SARADC_PD_LRADC0_PD                                         (2)
#define SARADC_PD_SENSOR_PD                                         (1)
#define SARADC_PD_IOVCC_PD                                          (0)

#define SARADCDIG_CTL_LRADC5_AVG_E                                  (15)
#define SARADCDIG_CTL_LRADC5_AVG_SHIFT                              (14)
#define SARADCDIG_CTL_LRADC5_AVG_MASK                               (0x3 << 14)
#define SARADCDIG_CTL_LRADC4_AVG_E                                  (13)
#define SARADCDIG_CTL_LRADC4_AVG_SHIFT                              (12)
#define SARADCDIG_CTL_LRADC4_AVG_MASK                               (0x3 << 12)
#define SARADCDIG_CTL_LRADC3_AVG_E                                  (11)
#define SARADCDIG_CTL_LRADC3_AVG_SHIFT                              (10)
#define SARADCDIG_CTL_LRADC3_AVG_MASK                               (0x3 << 10)
#define SARADCDIG_CTL_LRADC2_AVG_E                                  (9)
#define SARADCDIG_CTL_LRADC2_AVG_SHIFT                              (8)
#define SARADCDIG_CTL_LRADC2_AVG_MASK                               (0x3 << 8)
#define SARADCDIG_CTL_LRADC1_AVG_E                                  (7)
#define SARADCDIG_CTL_LRADC1_AVG_SHIFT                              (6)
#define SARADCDIG_CTL_LRADC1_AVG_MASK                               (0x3 << 6)
#define SARADCDIG_CTL_LRADC0_AVG_E                                  (5)
#define SARADCDIG_CTL_LRADC0_AVG_SHIFT                              (4)
#define SARADCDIG_CTL_LRADC0_AVG_MASK                               (0x3 << 4)
#define SARADCDIG_CTL_SENS_AVG_E                                    (3)
#define SARADCDIG_CTL_SENS_AVG_SHIFT                                (2)
#define SARADCDIG_CTL_SENS_AVG_MASK                                 (0x3 << 2)
#define SARADCDIG_CTL_IOVCC_AVG_E                                   (1)
#define SARADCDIG_CTL_IOVCC_AVG_SHIFT                               (0)
#define SARADCDIG_CTL_IOVCC_AVG_MASK                                (0x3 << 0)

#define IOVCCADC_DATA_IOVCC_E                                       (15)
#define IOVCCADC_DATA_IOVCC_SHIFT                                   (0)
#define IOVCCADC_DATA_IOVCC_MASK                                    (0xFFFF << 0)

#define SENSADC_DATA_SENSOR_E                                       (15)
#define SENSADC_DATA_SENSOR_SHIFT                                   (0)
#define SENSADC_DATA_SENSOR_MASK                                    (0xFFFF << 0)

#define LRADC0_DATA_LRADC0_E                                        (15)
#define LRADC0_DATA_LRADC0_SHIFT                                    (0)
#define LRADC0_DATA_LRADC0_MASK                                     (0xFFFF << 0)

#define LRADC1_DATA_LRADC1_E                                        (15)
#define LRADC1_DATA_LRADC1_SHIFT                                    (0)
#define LRADC1_DATA_LRADC1_MASK                                     (0xFFFF << 0)

#define LRADC2_DATA_LRADC2_E                                        (15)
#define LRADC2_DATA_LRADC2_SHIFT                                    (0)
#define LRADC2_DATA_LRADC2_MASK                                     (0xFFFF << 0)

#define LRADC3_DATA_LRADC3_E                                        (15)
#define LRADC3_DATA_LRADC3_SHIFT                                    (0)
#define LRADC3_DATA_LRADC3_MASK                                     (0xFFFF << 0)

#define LRADC4_DATA_LRADC4_E                                        (15)
#define LRADC4_DATA_LRADC4_SHIFT                                    (0)
#define LRADC4_DATA_LRADC4_MASK                                     (0xFFFF << 0)

#define LRADC5_DATA_LRADC5_E                                        (15)
#define LRADC5_DATA_LRADC5_SHIFT                                    (0)
#define LRADC5_DATA_LRADC5_MASK                                     (0xFFFF << 0)

#endif /* _ACTIONS_SOC_REGS_H_	*/
