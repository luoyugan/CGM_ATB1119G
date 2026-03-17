/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief board init functions
 */

#include <kernel.h>
#include <init.h>
#include <device.h>
#include <soc.h>

#if 0
static const struct acts_pin_config board_pin_config[] = {
	/* uart0 */
	{10, 5 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4) },	/* UART0_TX */
	{11, 5 | GPIO_CTL_SMIT | GPIO_CTL_PADDRV_LEVEL(4) },	/* UART0_RX */
};
#endif

static int board_early_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	return 0;
}

static int board_later_init(const struct device *arg)
{
	ARG_UNUSED(arg);
	printk("%s %d: \n", __func__, __LINE__);

	return 0;
}

#ifdef CONFIG_EARLY_CONSOLE
/* UART registers struct */
struct acts_uart_reg {
    volatile uint32_t ctrl;

    volatile uint32_t rxdat;

    volatile uint32_t txdat;

    volatile uint32_t stat;

    volatile uint32_t br;
} ;

void uart_poll_out_ch(int c)
{
	struct acts_uart_reg *uart = (struct acts_uart_reg*)UART0_REG_BASE;
	/* Wait for transmitter to be ready */
	while (uart->stat &  BIT(6));
	/* send a character */
	uart->txdat = (uint32_t)c;

}

/*for early printk*/
int arch_printk_char_out(int c)
{
	if ('\n' == c)
		uart_poll_out_ch('\r');
	uart_poll_out_ch(c);
	return 0;
}
#endif

SYS_INIT(board_early_init, PRE_KERNEL_1, 5);

SYS_INIT(board_later_init, POST_KERNEL, 5);
