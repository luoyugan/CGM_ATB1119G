/*
 * Copyright (c) 2020 Linaro Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PINCTRL_TAI_H_
#define PINCTRL_TAI_H_

#define PIN_MFP_SET(pin, val)				\
	{.pin_num = pin,		\
	 .mode = val}

#define GPIO_CTL_MFP_SHIFT					(0)
#define GPIO_CTL_MFP_MASK					(0x1f << GPIO_CTL_MFP_SHIFT)
#define GPIO_CTL_MFP_GPIO					(0x0 << GPIO_CTL_MFP_SHIFT)
#define GPIO_CTL_MFP(x)						(x << GPIO_CTL_MFP_SHIFT)
#define GPIO_CTL_SMIT						(0x1 << 5)
#define GPIO_CTL_GPIO_OUTEN					(0x1 << 6)
#define GPIO_CTL_GPIO_INEN					(0x1 << 7)
#define GPIO_CTL_PULL_MASK					(0xf << 8)
#define GPIO_CTL_PULLUP_100K					(0x1 << 8)
#define GPIO_CTL_PULLDOWN					(0x1 << 9)
#define GPIO_CTL_PULLUP_10K					(0x1 << 10)

#define GPIO_CTL_PADDRV_SHIFT				(12)
#define GPIO_CTL_PADDRV_LEVEL(x)			((x) << GPIO_CTL_PADDRV_SHIFT)
#define GPIO_CTL_PADDRV_MASK				GPIO_CTL_PADDRV_LEVEL(0x7)
#define GPIO_CTL_INTC_EN					(0x1 << 24)
#define GPIO_CTL_INC_TRIGGER_SHIFT			(25)
#define GPIO_CTL_INC_TRIGGER(x)				((x) << GPIO_CTL_INC_TRIGGER_SHIFT)
#define GPIO_CTL_INC_TRIGGER_MASK			GPIO_CTL_INC_TRIGGER(0x7)
#define GPIO_CTL_INC_TRIGGER_RISING_EDGE	GPIO_CTL_INC_TRIGGER(2)
#define GPIO_CTL_INC_TRIGGER_FALLING_EDGE	GPIO_CTL_INC_TRIGGER(3)
#define GPIO_CTL_INC_TRIGGER_DUAL_EDGE		GPIO_CTL_INC_TRIGGER(4)
#define GPIO_CTL_INC_TRIGGER_HIGH_LEVEL		GPIO_CTL_INC_TRIGGER(0)
#define GPIO_CTL_INC_TRIGGER_LOW_LEVEL		GPIO_CTL_INC_TRIGGER(1)
#define GPIO_CTL_INTC_MASK					(0x1 << 31)

/* uart2 */
#define UART2_MFP_SEL			4
#define UART2_MFP_CFG (GPIO_CTL_MFP(UART2_MFP_SEL) | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4))
#define gpio0_uart2_rx_node		PIN_MFP_SET(0, UART2_MFP_CFG | GPIO_CTL_PULLUP_10K)
#define gpio1_uart2_tx_node		PIN_MFP_SET(1, UART2_MFP_CFG)

/* uart1 */
#define UART1_MFP_SEL			4
#define UART1_MFP_CFG (GPIO_CTL_MFP(UART1_MFP_SEL) | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4))
#define gpio5_uart1_rx_node		PIN_MFP_SET(5, UART1_MFP_CFG | GPIO_CTL_PULLUP_10K)
#define gpio4_uart1_tx_node		PIN_MFP_SET(4, UART1_MFP_CFG)

/* uart0 */
#define UART0_MFP_SEL			4
#define UART0_MFP_CFG (GPIO_CTL_MFP(UART0_MFP_SEL) | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4))
#define gpio13_uart0_rx_node	PIN_MFP_SET(13, UART0_MFP_CFG | GPIO_CTL_PULLUP_10K)
#define gpio12_uart0_tx_node	PIN_MFP_SET(12, UART0_MFP_CFG)

/* SPI NOR */
#define SPINOR_MFP_SEL						(7)
#define SPINOR_MFP_CFG						(GPIO_CTL_MFP(SPINOR_MFP_SEL)| GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3))
#define SPINOR_MFP_PU_CFG					(GPIO_CTL_MFP(SPINOR_MFP_SEL)| GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3) | GPIO_CTL_PULLUP_100K)
#define gpio14_spinor0_miso_node			PIN_MFP_SET(14,   SPINOR_MFP_CFG)
#define gpio15_spinor0_cs_node				PIN_MFP_SET(15,   SPINOR_MFP_CFG)
#define gpio16_spinor0_clk_node				PIN_MFP_SET(16,   SPINOR_MFP_CFG)
#define gpio17_spinor0_mosi_node			PIN_MFP_SET(17,   SPINOR_MFP_CFG)
#define gpio10_spinor0_io2_node				PIN_MFP_SET(10,   SPINOR_MFP_PU_CFG)
#define gpio11_spinor0_io3_node				PIN_MFP_SET(11,   SPINOR_MFP_PU_CFG)

/* ADC */
#define AMIC_MFP_SEL						25
#define AMIC_MFP_CFG						(GPIO_CTL_MFP(AMIC_MFP_SEL))
#define gpio40_amic_micinp_node				PIN_MFP_SET(40, AMIC_MFP_CFG)
#define gpio41_amic_micinn_node				PIN_MFP_SET(41, AMIC_MFP_CFG)
#define gpio42_amic_adcvref_node			PIN_MFP_SET(42, AMIC_MFP_CFG)

#define VMIC_MFP_SEL                        26
#define gpio19_vmic_node					PIN_MFP_SET(19, VMIC_MFP_SEL)

#define DMIC_MFP_SEL              			13
#define DMIC_MFP_CFG 						(GPIO_CTL_MFP(DMIC_MFP_SEL)| GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3))
#define gpio6_dmic_clk_node					PIN_MFP_SET(6, DMIC_MFP_CFG)
#define gpio7_dmic_dat_node					PIN_MFP_SET(7, DMIC_MFP_CFG)
#define gpio42_dmic_clk_node				PIN_MFP_SET(42, DMIC_MFP_CFG)
#define gpio40_dmic_dat_node				PIN_MFP_SET(40, DMIC_MFP_CFG)

/* PWM */
#define PWM0_MFP_SEL             0x1
#define PWM1_MFP_SEL             0x2
#define PWMX_MFP_SEL             0x3
#define GPIO_CTL_MFP(x)					(x << GPIO_CTL_MFP_SHIFT)

#define PWM_MFP_CFG(X) (GPIO_CTL_MFP(X) | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3))

#define gpio18_pwm_g0c0p_node				PIN_MFP_SET(18, PWM_MFP_CFG(PWM0_MFP_SEL))
#define gpio2_pwm_g0c1p_node				PIN_MFP_SET(2, PWM_MFP_CFG(PWM0_MFP_SEL))
#define gpio29_pwm_g0c2p_node				PIN_MFP_SET(29, PWM_MFP_CFG(PWM0_MFP_SEL))
#define gpio24_pwm_g0c3p_node				PIN_MFP_SET(24, PWM_MFP_CFG(PWM0_MFP_SEL))
#define gpio22_pwm_g0c4p_node				PIN_MFP_SET(22, PWM_MFP_CFG(PWM0_MFP_SEL))
#define gpio20_pwm_g0c5p_node				PIN_MFP_SET(20, PWM_MFP_CFG(PWM0_MFP_SEL))
#define gpio26_pwm_g1c0_node				PIN_MFP_SET(26, PWM_MFP_CFG(PWM1_MFP_SEL))
#define gpio28_pwm_g1c1_node				PIN_MFP_SET(28, PWM_MFP_CFG(PWM1_MFP_SEL))
#define gpio31_pwm_g1c2_node				PIN_MFP_SET(31, PWM_MFP_CFG(PWM1_MFP_SEL))
#define gpio25_pwm_g1c3_node				PIN_MFP_SET(25, PWM_MFP_CFG(PWM1_MFP_SEL))
#define gpio23_pwm_g1c4_node				PIN_MFP_SET(23, PWM_MFP_CFG(PWM1_MFP_SEL))
#define gpio21_pwm_g1c5_node				PIN_MFP_SET(21, PWM_MFP_CFG(PWM1_MFP_SEL))
#define gpio41_pwm_g2_node					PIN_MFP_SET(41, PWM_MFP_CFG(PWMX_MFP_SEL))
#define gpio42_pwm_g3_node					PIN_MFP_SET(42, PWM_MFP_CFG(PWMX_MFP_SEL))
#define gpio9_pwm_irtx_node					PIN_MFP_SET(9, PWM_MFP_CFG(PWMX_MFP_SEL))

/* KEYPAD */
#define KEY_PAD_MFP_SEL             0xe
#define KEY_PAD_MFP_CFG (GPIO_CTL_MFP(KEY_PAD_MFP_SEL)| GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3))

#define gpio0_keypad_key0_node				PIN_MFP_SET(0, KEY_PAD_MFP_CFG)
#define gpio1_keypad_key1_node				PIN_MFP_SET(1, KEY_PAD_MFP_CFG)
#define gpio21_keypad_key2_node				PIN_MFP_SET(21, KEY_PAD_MFP_CFG)
#define gpio20_keypad_key3_node				PIN_MFP_SET(20, KEY_PAD_MFP_CFG)
#define gpio7_keypad_key7_node				PIN_MFP_SET(7, KEY_PAD_MFP_CFG)
#define gpio8_keypad_key8_node				PIN_MFP_SET(8, KEY_PAD_MFP_CFG)
#define gpio22_keypad_key9_node				PIN_MFP_SET(22, KEY_PAD_MFP_CFG)
#define gpio18_keypad_key10_node			PIN_MFP_SET(18, KEY_PAD_MFP_CFG)
#define gpio6_keypad_key11_node				PIN_MFP_SET(6, KEY_PAD_MFP_CFG)
#define gpio2_keypad_key13_node				PIN_MFP_SET(2, KEY_PAD_MFP_CFG)
#define gpio3_keypad_key14_node				PIN_MFP_SET(3, KEY_PAD_MFP_CFG)
#define gpio26_keypad_key15_node			PIN_MFP_SET(26, KEY_PAD_MFP_CFG)

/* IR capture */
#define CAPTURE_MFP_SEL             0xf
#define CAPTURE_MFP_CFG (GPIO_CTL_MFP(CAPTURE_MFP_SEL)| GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(3))

#define gpio9_ir_capture0_node	PIN_MFP_SET(9, CAPTURE_MFP_CFG)

/* I2C */
#define MFP_I2C			9
#define I2C_MFP_CFG(x)		(GPIO_CTL_MFP(x)|GPIO_CTL_PULLUP_10K|GPIO_CTL_PADDRV_LEVEL(3))
/* I2C0 */
#define gpio0_i2c0_clk_node	PIN_MFP_SET(0, I2C_MFP_CFG(MFP_I2C))
#define gpio1_i2c0_data_node	PIN_MFP_SET(1, I2C_MFP_CFG(MFP_I2C))
/* I2C1 */
#define gpio6_i2c1_clk_node PIN_MFP_SET(6, I2C_MFP_CFG(MFP_I2C))
#define gpio7_i2c1_data_node PIN_MFP_SET(7, I2C_MFP_CFG(MFP_I2C))

/* SPI */
#define MFP_SPI2		8
#define SPI_MFP_CFG(x)		(GPIO_CTL_MFP(x)|GPIO_CTL_PADDRV_LEVEL(3))
#define gpio20_spi2_ss_node	PIN_MFP_SET(20,  SPI_MFP_CFG(MFP_SPI2))
#define gpio21_spi2_clk_node	PIN_MFP_SET(21,  SPI_MFP_CFG(MFP_SPI2))
#define gpio22_spi2_miso_node	PIN_MFP_SET(22,  SPI_MFP_CFG(MFP_SPI2))
#define gpio1_spi2_mosi_node	PIN_MFP_SET(1,  SPI_MFP_CFG(MFP_SPI2))

#endif /* PINCTRL_TAI_H_ */
