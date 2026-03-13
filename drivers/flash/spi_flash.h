/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SPI_FLASH_H__
#define __SPI_FLASH_H__
#include <drivers/flash.h>

#define SPI_FLAG_NO_IRQ_LOCK				    (1 << 0)

struct acts_spi_reg
{
	volatile uint32_t ctrl;
	volatile uint32_t status;
	volatile uint32_t txdat;
	volatile uint32_t rxdat;
	volatile uint32_t bc;
} ;

struct spi_info
{
	/* spi controller address */
	unsigned int base;
	unsigned int flag;

	unsigned short cs_gpio;
	unsigned char bus_width;
	unsigned char delay_chain;

	unsigned int freq_khz;
	unsigned int dma_base;

	void (*set_cs)(struct spi_info *si, int value);
	void (*set_clk)(struct spi_info *si, unsigned int freq_khz);
	void (*prepare_hook)(struct spi_info *si);
};

struct spinor_info {
	struct spi_info spi;
	unsigned int chipid;
	unsigned int flag;
};

struct spinor_operation_api {
	int (*init)(struct spinor_info *sni);
	void (*set_clk)(unsigned int clk_mhz);

	unsigned int (*read_chipid) (struct spinor_info *sni);

	unsigned int (*read_status) (struct spinor_info *sni, unsigned char cmd);
	void (*write_status)(struct spinor_info *sni, unsigned char cmd,
			unsigned char *status, int len);

	int (*read) (struct spinor_info *sni, unsigned int addr, void *buf, int len);
	int (*write) (struct spinor_info *sni, unsigned int addr, const void *buf, int len);
	int (*erase) (struct spinor_info *sni, unsigned int addr, int len);
	void (*continuous_read_reset) (struct spinor_info *sni);/*new add by lark*/
};
// brom api address
#define p_spinor_api	((struct spinor_operation_api *)SPINOR_API_ADDR)

/* NOR Flash vendors ID */
#define XSPI_NOR_MANU_ID_MACRONIX	0xc2	/* Macronix (MX) */

/* spinor commands */
#define	XSPI_NOR_CMD_READ_STATUS	0x05	/* read status1 */
#define	XSPI_NOR_CMD_READ_STATUS2	0x35	/* read status2 */
#define	XSPI_NOR_CMD_READ_STATUS3	0x15	/* read status3 */
#define	XSPI_NOR_CMD_WRITE_STATUS	0x01	/* write status1 */
#define	XSPI_NOR_CMD_WRITE_STATUS2	0x31	/* write status2 */
#define	XSPI_NOR_CMD_WRITE_STATUS3	0x11	/* write status3 */

#define SPI_FLAG_SPI_NXIO			(1 << 2)
//#define SPI_FLAG_USE_MIO_ADDR_DATA	(1<<12)


struct spi_flash_acts_config {
#if IS_ENABLED(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout pages_layout;
#endif
	uint32_t chip_size;
	uint32_t page_size;
};

#define DEV_CFG(dev) \
	((const struct spi_flash_acts_config *const)(dev)->config)
#define DEV_DATA(dev) \
	((struct spinor_info *)(dev)->data)

#define SPI_CTL(base)				(base+0x0000)
#define SPI_STA(base)				(base+0x0004)
#define SPI_TXDAT(base)		    	(base+0x0008)
#define SPI_RXDAT(base)			    (base+0x000c)
#define SPI_BC(base)				(base+0x0010)

#define SPI_STA_READY                       (0x1 << 8)

#endif	/* __SPI_FLASH_H__ */

