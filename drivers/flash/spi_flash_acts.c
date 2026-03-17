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
#include <drivers/cfg_drv/dev_config.h>
#include <soc.h>

LOG_MODULE_REGISTER(spi_flash_acts, CONFIG_FLASH_LOG_LEVEL);

#ifdef SPINOR_RESET_FUN_ADDR
typedef void (*spi_reset_func)(struct spi_info *si);
__ramfunc void spi_flash_reset(struct spi_info *si)
{
	spi_reset_func func = (spi_reset_func)(SPINOR_RESET_FUN_ADDR);
	func(si);
}
#else
__ramfunc void spi_flash_reset(struct spi_info *si)
{
	p_spinor_api->continuous_read_reset((struct spinor_info *)si);
}
#endif

__ramfunc  void spi_flash_acts_prepare(struct spi_info *si)
{
	/* wait for spi ready */
	while(!(sys_read32(SPI_STA(si->base)) & SPI_STA_READY));

	spi_flash_reset(si);
}


int spi_flash_acts_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	struct spinor_info *sni = DEV_DATA(dev);
	uint32_t key;
	int ret;
	key = irq_lock();
	ret = p_spinor_api->read(sni, offset, data, len);
	irq_unlock(key);
	return ret;
}

int spi_flash_acts_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	struct spinor_info *sni = DEV_DATA(dev);
	uint32_t key;
	int ret;
	key = irq_lock();
	ret = p_spinor_api->write(sni, offset, data, len);
	irq_unlock(key);

	return ret ;
}

int spi_flash_acts_erase(const struct device *dev, off_t offset, size_t size)
{
	struct spinor_info *sni = DEV_DATA(dev);
	uint32_t key;
	int ret;
	key = irq_lock();
	ret = p_spinor_api->erase(sni, offset, size);
	irq_unlock(key);
	return ret ;
}

static inline void xspi_delay(void)
{
	volatile int i = 100000;

	while (i--)
		;
}

#if (CONFIG_SPI_FLASH_BUS_WIDTH == 4)
__ramfunc void xspi_nor_enable_status_qe(struct spinor_info *sni)
{
	uint16_t status;

	/* MACRONIX's spinor has different QE bit */
	if (XSPI_NOR_MANU_ID_MACRONIX == (sni->chipid & 0xff)) {
		status = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
		if (!(status & 0x40)) {
			/* set QE bit to disable HOLD/WP pin function */
			status |= 0x40;
			p_spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS,
						(u8_t *)&status, 1);
		}

		return;
	}

	/* check QE bit */
	status = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);

	if (!(status & 0x2)) {
		/* set QE bit to disable HOLD/WP pin function, for WinBond */
		status |= 0x2;
		p_spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS2,
					(u8_t *)&status, 1);

		/* check QE bit again */
		status = p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS2);

		if (!(status & 0x2)) {
			/* oh, let's try old write status cmd, for GigaDevice/Berg */
			status = ((status | 0x2) << 8) |
				 p_spinor_api->read_status(sni, XSPI_NOR_CMD_READ_STATUS);
			p_spinor_api->write_status(sni, XSPI_NOR_CMD_WRITE_STATUS,
						(u8_t *)&status, 2);
		}
	}

	xspi_delay();

}
#endif

static inline void xspi_setup_bus_width(struct spinor_info *sni, u8_t bus_width)
{
	struct acts_spi_reg *spi= (struct acts_spi_reg *)sni->spi.base;

	spi->ctrl = (spi->ctrl & ~(0x3 << 10)) | (((bus_width & 0x7) / 2 + 1) << 10);

	xspi_delay();
}

static inline void xspi_setup_delaychain(struct spinor_info *sni, u8_t ns)
{
	struct acts_spi_reg *spi= (struct acts_spi_reg *)sni->spi.base;
	spi->ctrl = (spi->ctrl & ~(0xF << 16)) | (ns << 16);

	xspi_delay();
}

#if IS_ENABLED(CONFIG_SPINOR_TEST_DELAYCHAIN)
extern int nor_test_delaychain(struct device *dev);
#endif

#if IS_ENABLED(CONFIG_NOR_ACTS_DQ_MODE_ENABLE)
extern void nor_dual_quad_read_mode_try(struct spinor_info *sni);
#endif

//static const struct acts_pin_config spinor_pins_state[] = {
//	CONFIG_SPI_FLASH_MFP
//};

__ramfunc int spi_flash_acts_init(const struct device *dev)
{
	struct spinor_info *sni = DEV_DATA(dev);
	uint32_t key;

	sni->spi.prepare_hook = spi_flash_acts_prepare;

	/* setup spi nor pins state */
	// acts_pinmux_setup_pins(spinor_pins_state, ARRAY_SIZE(spinor_pins_state));

	key = irq_lock();
	sni->chipid = p_spinor_api->read_chipid(sni);

	printk("read spi nor chipid:0x%x\n", sni->chipid);
#if IS_ENABLED(CONFIG_NOR_ACTS_DQ_MODE_ENABLE)
	nor_dual_quad_read_mode_try(sni);
	printk("bus width : %d, and cache read use ", sni->spi.bus_width);

#else
	if(sni->spi.bus_width == 4) {
		printk("nor is 4 line mode\n");
#if (CONFIG_SPI_FLASH_NXIO_ENABLE == 1)
		sni->spi.flag = SPI_FLAG_SPI_NXIO;
#endif

#if (CONFIG_SPI_FLASH_BUS_WIDTH == 4)
		xspi_nor_enable_status_qe(sni);
#endif
		/* enable 4x mode */
		xspi_setup_bus_width(sni, 4);
	} else if(sni->spi.bus_width == 2) {
		printk("nor is 2 line mode\n");
#if (CONFIG_SPI_FLASH_NXIO_ENABLE == 1)
		sni->spi.flag = SPI_FLAG_SPI_NXIO;
#endif
		/* enable 2x mode */
		xspi_setup_bus_width(sni, 2);
	} else {
		sni->spi.bus_width = 1;
		printk("nor is 1 line mode\n");
		/* enable 1x mode */
		xspi_setup_bus_width(sni, 1);
	}
#endif

	/* setup SPI clock rate */
	clk_set_rate(CLOCK_ID_SPI0, CONFIG_SPI_FLASH_CLK_HZ);

	/* configure delay chain */
	xspi_setup_delaychain(sni, sni->spi.delay_chain);

	/* check delay chain workable */
	sni->chipid = p_spinor_api->read_chipid(sni);

	printk("read again spi nor chipid:0x%x\n", sni->chipid);

#if IS_ENABLED(CONFIG_SPINOR_TEST_DELAYCHAIN)
	nor_test_delaychain(dev);
#endif

#if IS_ENABLED(CONFIG_NOR_ACTS_DATA_PROTECTION_ENABLE)
	flash_write_protection_set(dev, true);
#endif

	irq_unlock(key);

	return 0;
}

/* system XIP spinor */
static struct spinor_info spi_flash_acts_data = {
	.spi = {
		.base = SPI0_REG_BASE,
		.bus_width = CONFIG_SPI_FLASH_BUS_WIDTH,
		.delay_chain = CONFIG_SPI_FLASH_DELAY_CHAIN,
		//.dma_base= 0x4001C600, //DMA5
		.flag = 0,
	},
	.flag = 0,
};

#if 0
__ramfunc void  sys_norflash_power_ctrl(uint32_t is_powerdown)

{
	if (is_powerdown){
		p_spinor_api->write_status(NULL, 0xB9, NULL, 0);
	} else {
		p_spinor_api->write_status(NULL, 0xAB, NULL, 0);
	}
}
#endif

#if IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT)
static void spi_flash_acts_pages_layout(
				const struct device *dev,
				const struct flash_pages_layout **layout,
				size_t *layout_size)
{
	*layout = &(DEV_CFG(dev)->pages_layout);
	*layout_size = 1;
}
#endif /* IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT) */

#if IS_ENABLED(CONFIG_NOR_ACTS_DATA_PROTECTION_ENABLE)
extern int nor_write_protection(const struct device *dev, bool enable);
#endif

int spi_flash_acts_write_protection(const struct device *dev, bool enable)
{
#if IS_ENABLED(CONFIG_NOR_ACTS_DATA_PROTECTION_ENABLE)
	nor_write_protection(dev, enable);
#endif
	return 0;
}

static const struct flash_driver_api spi_flash_nor_api = {
	.read = spi_flash_acts_read,
	.write = spi_flash_acts_write,
	.erase = spi_flash_acts_erase,
	.write_protection = spi_flash_acts_write_protection,
#if IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = spi_flash_acts_pages_layout,
#endif
};

static const struct spi_flash_acts_config spi_acts_config = {
#if IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT)
	.pages_layout = {
			.pages_count = CONFIG_SPI_FLASH_CHIP_SIZE/0x1000,
			.pages_size  = 0x1000,
		},
#endif
	.chip_size	 = CONFIG_SPI_FLASH_CHIP_SIZE,
	.page_size	 = 0x1000,
};

DEVICE_DEFINE(spi_flash_acts, CONFIG_SPI_FLASH_NAME, &spi_flash_acts_init,
			device_pm_control_nop, &spi_flash_acts_data, &spi_acts_config, PRE_KERNEL_2,
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &spi_flash_nor_api);

