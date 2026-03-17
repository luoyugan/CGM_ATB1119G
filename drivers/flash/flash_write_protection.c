/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/flash.h>
#include <drivers/spi.h>
#include <logging/log.h>
#include <soc.h>
#include "spi_flash.h"

#define NOR_STATUS1_MASK (0x1f << 2)  /* bp4-bp0 status1 bit6-bit2 */
#define NOR_STATUS2_MASK (0x1 << 6)   /* cmp status2 bit6 */

#define NOR_STATUS1_SRP0 (1 << 7)  /* srp0 bit7 */
#define NOR_STATUS2_SRP1 (1 << 0)  /* srp1 bit0 */

/**********P25D40SL 512KB****************/
#define P25D40SL_NOR_CHIPID	0x136085  	/* P25D40SL */
#define P25D40SL_CMP1_VAL			1
#define P25D40SL_CMP1_PROTECT_512KB	0x0  /* 0-07FFFFH */
#define P25D40SL_CMP1_PROTECT_448KB	0x1  /* 0-06FFFFH */
#define P25D40SL_CMP1_PROTECT_256KB	0x3  /* 0-03FFFFH */

#define P25D40SL_NOR_BP_STATUS1  (P25D40SL_CMP1_PROTECT_448KB << 2)
#define P25D40SL_NOR_CMP_STATUS2 (P25D40SL_CMP1_VAL << 6)

/**********GD25WD40C 512KB****************/
#define GD25WD40C_NOR_CHIPID	 0x1364c8  	/* GD25WD40C */
#define GD25WD40C_PROTECT_512KB	 0x7  /* 0-07FFFFH */
#define GD25WD40C_PROTECT_448KB	 0x4  /* 0-06FFFFH */
#define GD25WD40C_PROTECT_256KB	 0x6  /* 0-03FFFFH */

#define GD25WD40C_NOR_BP_STATUS1 (GD25WD40C_PROTECT_448KB << 2)
#define GD25WD40C_NOR_STATUS2  	 (0 << 6)

struct nor_wp_info {
	unsigned int chipid;
	unsigned char wp_status1_val;
	unsigned char wp_status1_mask;
	unsigned char wp_status2_val;
	unsigned char wp_status2_mask;
};

const struct nor_wp_info g_nor_wp_info[] = {
	/* Puya P25D40SL */
	{
		P25D40SL_NOR_CHIPID,
		P25D40SL_NOR_BP_STATUS1,
		NOR_STATUS1_MASK,
		P25D40SL_NOR_CMP_STATUS2,
		NOR_STATUS2_MASK,
	},
	/* GigaDevice GD25WD40C */
	{
		GD25WD40C_NOR_CHIPID,
		GD25WD40C_NOR_BP_STATUS1,
		NOR_STATUS1_MASK,
		GD25WD40C_NOR_STATUS2,
		NOR_STATUS2_MASK,
	},
};

static void xspi_nor_write_status(struct spinor_info *sni, u8_t status1, u8_t status2)
{
	u8_t status[2];
	unsigned int flag;

	status[0] = status1;
	status[1] = status2;

	flag = sni->spi.flag;
	sni->spi.flag &= ~SPI_FLAG_NO_IRQ_LOCK;  // wait ready
	if (P25D40SL_NOR_CHIPID == (sni->chipid & 0xffffff)) {
		p_spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS2, &status2, 1);
	}
	p_spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS, &status1, 1);
	p_spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS, status, 2);

	sni->spi.flag = flag;
}

void xspi_nor_read_status(struct spinor_info *sni, u8_t *status1, u8_t *status2)
{
	*status1 = *status2 = 0;
	*status1 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
	if (P25D40SL_NOR_CHIPID == (sni->chipid & 0xffffff)) {
		*status2 = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);
	}
}

static void xspi_nor_protect_handle(struct spinor_info *sni, const struct nor_wp_info *wp, bool enable)
{
	u32_t flags;
	u8_t status1, status2;

	flags = irq_lock();

	/* read status */
	xspi_nor_read_status(sni, &status1, &status2);
	printk("status1-2=0x%x,0x%x\n", status1, status2);

	if (enable) {
		printk("wp_status1_val=0x%x, wp_status2_val=0x%x\n", wp->wp_status1_val, wp->wp_status2_val);
		status1 = ((~wp->wp_status1_mask) & status1) | wp->wp_status1_val;
		status2 = ((~wp->wp_status2_mask) & status2) | wp->wp_status2_val;
		printk("enable status1-2=0x%x,0x%x\n", status1, status2);
		xspi_nor_write_status(sni, status1 & ~NOR_STATUS1_SRP0, status2 & ~NOR_STATUS2_SRP1);
	} else {
		if (P25D40SL_NOR_CHIPID == (sni->chipid & 0xffffff)) {
			status1 = wp->wp_status1_mask | ((~wp->wp_status1_mask) & status1);
			status2 = wp->wp_status2_mask | ((~wp->wp_status2_mask) & status2);
		} else {
			status1 = (~wp->wp_status1_mask) & status1;
			status2 = (~wp->wp_status2_mask) & status2;
		}

		printk("disable status1-2=0x%x,0x%x\n", status1, status2);
		xspi_nor_write_status(sni, status1 & ~NOR_STATUS1_SRP0, status2 & ~NOR_STATUS2_SRP1);
	}

	status1 = status2 = 0;
	xspi_nor_read_status(sni, &status1, &status2);
	printk("read again status1-2=0x%x,0x%x\n", status1, status2);

	irq_unlock(flags);
}

int nor_write_protection(const struct device *dev, bool enable)
{
	struct spinor_info *sni = (struct spinor_info *)(dev)->data;
	const struct nor_wp_info *wp;

	for(int i = 0; i < ARRAY_SIZE(g_nor_wp_info); i++){
		wp = &g_nor_wp_info[i];
		if (wp->chipid == (sni->chipid & 0xffffff)) {
			xspi_nor_protect_handle(sni, wp, enable);
			break;
		}
	}

	return 0;
}


