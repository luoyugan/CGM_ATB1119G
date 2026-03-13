/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio ADC physical implementation
 */

/*
 * Features
 *    - 16 levels x 16bits FIFO for PDMTX which shared with I2STX
 *    - Support MONO channel audio PDM
 *    - Sample rate support 8k/16k/24k/32k/48k
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <soc.h>
#include <board_cfg.h>
#include <drivers/audio/audio_out.h>

#include "../phy_audio_common.h"

#include <logging/log.h>
 LOG_MODULE_REGISTER(pdmtx0, CONFIG_LOG_DEFAULT_LEVEL);

/***************************************************************************************************
 * PDMTX_CTL
 */
#define PDMTX_CTL_DEBUG_SEL                                   BIT(3) /* debug PCM data(1x or 50x) selection */
#define PDMTX_CTL_DEBUG_EN                                    BIT(2) /* debug enable */
#define PDMTX_CTL_PDM_ARST_EN                                 BIT(1) /* PDM auto reset enable */
#define PDMTX_CTL_PDM_EN                                      BIT(0) /* PDM enable */

/***************************************************************************************************
 * I2STX_FIFO
 */
#define I2STX_FIFO_ADDRESS                                    (0x4005c404)

#define I2STX0_FIFOCTL_FIFO_IS_SHIFT                          (4)
#define I2STX0_FIFOCTL_FIFO_IS_MASK                           (0x3 << I2STX0_FIFOCTL_FIFO_IS_SHIFT)
#define I2STX0_FIFOCTL_FIFO_IS(x)                             ((x) << I2STX0_FIFOCTL_FIFO_IS_SHIFT)
#define I2STX0_FIFOCTL_FIFO_OS                                BIT(3) /* I2STX FIFO output selection. 0: I2STX; 1: PDMTX */
#define I2STX0_FIFOCTL_FIFO_IEN                               BIT(2)
#define I2STX0_FIFOCTL_FIFO_DEN                               BIT(1)
#define I2STX0_FIFOCTL_FIFO_RST                               BIT(0)

/***************************************************************************************************
 * CMU_PDMTXCLK
 */
#define CMU_PDMTXCLK_PDMTXCLKDIV_SHIFT                        (0)
#define CMU_PDMTXCLK_PDMTXCLKDIV_MASK                         (0x7 << CMU_PDMTXCLK_PDMTXCLKDIV_SHIFT)
#define CMU_PDMTXCLK_PDMTXCLKDIV(x)                           ((x) << CMU_PDMTXCLK_PDMTXCLKDIV_SHIFT)

/***************************************************************************************************
 * CMU_I2SCLK
 */
#define CMU_I2SCLK_TXFIFOCLKEN                                BIT(8)

/***************************************************************************************************
 * I2STX0_FIFOSTA
 */
#define I2STX0_FIFOSTA_TFEP                                   BIT(8)

/*
 * @struct acts_audio_pdmtx
 * @brief PDMTX controller hardware register
 */
struct acts_audio_pdmtx {
	volatile uint32_t pdmtx_ctl; /* PMDTX control */
};

/**
 * struct phy_pdmtx_config_data
 * @brief The hardware related data that used by physical pdmtx driver.
 */
struct phy_pdmtx_config_data {
	uint32_t reg_base; /* PMDTX controller register base address */
	uint8_t clk_id; /* PMDTX devclk id */
	uint8_t rst_id; /* PMDTX reset id */
	struct audio_dma_dt dma_fifo0; /* DMA resource for PMDTX */
	const struct acts_pin_config *pinmux; /* PMDTX pinmux array */
	uint8_t pinmux_size; /* PMDTX pinmux array size */
};

/* @brief  Get the PDMTX controller base address */
static inline struct acts_audio_pdmtx *get_pdmtx_reg_base(struct device *dev)
{
	const struct phy_pdmtx_config_data *cfg = dev->config;
	return (struct acts_audio_pdmtx *)cfg->reg_base;
}

/* @brief Dump the PDMTX relative registers */
static void pdmtx_dump_register(struct device *dev)
{
	struct acts_audio_pdmtx *pdmtx_reg = get_pdmtx_reg_base(dev);

	LOG_INF("** pdmtx contoller regster **");
	LOG_INF("          BASE: %08x", (uint32_t)pdmtx_reg);
	LOG_INF("     PDMTX_CTL: %08x", pdmtx_reg->pdmtx_ctl);
	LOG_INF(" I2STX_FIFOCTL: %08x", sys_read32(I2STX_FIFO_ADDRESS));
	LOG_INF("    CMU_I2SCLK: %08x", sys_read32(CMU_I2SCLK));
	LOG_INF("  CMU_PDMTXCLK: %08x", sys_read32(CMU_PDMTXCLK));
}

/* @brief  Disable the PDMTX FIFO (share with I2STX FIFO) */
static void pdmtx_fifo_disable(struct device *dev)
{
	ARG_UNUSED(dev);

	sys_write32(0, I2STX_FIFO_ADDRESS);

	/* I2STX FIFO clock disable */
	sys_write32(sys_read32(CMU_I2SCLK) & ~CMU_I2SCLK_TXFIFOCLKEN, CMU_I2SCLK);
}

/* @brief Check the PDMTX FIFO is working or not */
static bool is_pdmtx_fifo_working(struct device *dev)
{
	ARG_UNUSED(dev);

	uint32_t reg = sys_read32(I2STX_FIFO_ADDRESS);
	return !!(reg & I2STX0_FIFOCTL_FIFO_RST);
}

/* @brief Check the PDMTX FIFO is error */
static bool check_pdmtx_fifo_error(struct device *dev)
{
	ARG_UNUSED(dev);

	uint32_t reg = sys_read32(I2STX_FIFO_ADDRESS);

	if (reg & I2STX0_FIFOSTA_TFEP) {
		sys_write32(reg | I2STX0_FIFOSTA_TFEP, I2STX_FIFO_ADDRESS);
		return true;
	}

	return false;
}

/* @brief  Enable the PDMTX FIFO */
static void pdmtx_fifo_enable(struct device *dev, audio_fifouse_sel_e sel)
{
	uint32_t reg = sys_read32(I2STX_FIFO_ADDRESS);

	ARG_UNUSED(dev);

	if (FIFO_SEL_CPU == sel) {
		reg |= (I2STX0_FIFOCTL_FIFO_IEN | I2STX0_FIFOCTL_FIFO_RST | I2STX0_FIFOCTL_FIFO_OS);
	} else if (FIFO_SEL_DMA == sel) {
		reg |= (I2STX0_FIFOCTL_FIFO_DEN | I2STX0_FIFOCTL_FIFO_RST | I2STX0_FIFOCTL_FIFO_OS
				| I2STX0_FIFOCTL_FIFO_IS(1));
	} else {
		LOG_ERR("invalid fifo sel %d", sel);
	}

	sys_write32(reg, I2STX_FIFO_ADDRESS);

	/* I2STX FIFO clock enable */
	sys_write32(sys_read32(CMU_I2SCLK) | CMU_I2SCLK_TXFIFOCLKEN, CMU_I2SCLK);
}

/* @brief PDMTX sample rate config */
static int pdmtx_sample_rate_set(struct device *dev, uint16_t sr_khz)
{
	uint32_t reg = sys_read32(CMU_PDMTXCLK) & ~CMU_PDMTXCLK_PDMTXCLKDIV_MASK;
	uint8_t div = 0;

	/* PDMTX sample rate only support 8k/16k/24k/32k/48k */
	if ((sr_khz != SAMPLE_RATE_8KHZ)
		&& (sr_khz != SAMPLE_RATE_16KHZ)
		&& (sr_khz != SAMPLE_RATE_24KHZ)
		&& (sr_khz != SAMPLE_RATE_32KHZ)
		&& (sr_khz != SAMPLE_RATE_48KHZ)) {
		LOG_ERR("sample rate invalid %d", sr_khz);
		return -EPERM;
	}

	/* PDMTX clock source is 24Mhz (50fs) */
	if (sr_khz == SAMPLE_RATE_8KHZ) {
		div = 4;
	} else if (sr_khz == SAMPLE_RATE_16KHZ) {
		div = 3;
	} else if (sr_khz == SAMPLE_RATE_24KHZ) {
		div = 2;
	} else if (sr_khz == SAMPLE_RATE_32KHZ) {
		div = 1;
	} else if (sr_khz == SAMPLE_RATE_48KHZ) {
		div = 0;
	}

	reg |= CMU_PDMTXCLK_PDMTXCLKDIV(div);

	sys_write32(reg, CMU_PDMTXCLK);

	return 0;
}

/* @brief Get the sample rate from the PDMTX config */
static int pdmtx_sample_rate_get(struct device *dev)
{
	ARG_UNUSED(dev);
	uint8_t sr;
	uint32_t div = (sys_read32(CMU_PDMTXCLK) & CMU_PDMTXCLK_PDMTXCLKDIV_MASK)
					>> CMU_PDMTXCLK_PDMTXCLKDIV_SHIFT;

	if (4 == div) {
		sr = SAMPLE_RATE_8KHZ;
	} else if (3 == div) {
		sr = SAMPLE_RATE_16KHZ;
	} else if (2 == div) {
		sr = SAMPLE_RATE_24KHZ;
	} else if (1 == div) {
		sr = SAMPLE_RATE_32KHZ;
	} else if (0 == div) {
		sr = SAMPLE_RATE_48KHZ;
	} else {
		LOG_ERR("invalid divisor:%d", div);
		return -EINVAL;
	}

	return sr;
}

/* @brief PDMTX prepare enable */
static int phy_pdmtx_prepare_enable(struct device *dev, aout_param_t *out_param)
{
	const struct phy_pdmtx_config_data *cfg = dev->config;

	if (!out_param) {
		LOG_ERR("Invalid parameters");
		return -EINVAL;
	}

	if (out_param->channel_type != AUDIO_CHANNEL_PDMTX) {
		LOG_ERR("Invalid channel type %d", out_param->channel_type);
		return -EINVAL;
	}

	if (AOUT_FIFO_I2STX0 != out_param->outfifo_type) {
		LOG_ERR("Invalid fifo type %d", out_param->outfifo_type);
		return -EINVAL;
	}

    /* Config the pdmtx pin state */
	acts_pinmux_setup_pins(cfg->pinmux, cfg->pinmux_size);

	acts_clock_peripheral_enable(cfg->clk_id);

	return 0;
}

/* @brief  PDMTX digital function control enable */
static void pdmtx_digital_enable(struct device *dev)
{
	struct acts_audio_pdmtx *pdmtx_reg = get_pdmtx_reg_base(dev);
	uint32_t reg = pdmtx_reg->pdmtx_ctl & ~(PDMTX_CTL_PDM_EN | PDMTX_CTL_PDM_ARST_EN);

	reg |= (PDMTX_CTL_PDM_EN | PDMTX_CTL_PDM_ARST_EN);

	pdmtx_reg->pdmtx_ctl = reg;
}

/* @brief  PDMTX digital function control disable */
static void pdmtx_digital_disable(struct device *dev)
{
	struct acts_audio_pdmtx *pdmtx_reg = get_pdmtx_reg_base(dev);
	uint32_t reg = pdmtx_reg->pdmtx_ctl & ~(PDMTX_CTL_PDM_EN | PDMTX_CTL_PDM_ARST_EN);
	pdmtx_reg->pdmtx_ctl = reg;
}

/* @brief PDMTX physical level enable procedure */
static int phy_pdmtx_enable(struct device *dev, void *param)
{
	aout_param_t *out_param = (aout_param_t *)param;
	int ret;

	if ((!out_param) || (!out_param->sample_rate)) {
		LOG_ERR("Invalid parameters");
		return -EINVAL;
	}

	if (is_pdmtx_fifo_working(dev)) {
		LOG_ERR("PDMTX FIFO now is using ...");
		return -EACCES;
	}

	ret = phy_pdmtx_prepare_enable(dev, out_param);
	if (ret) {
		LOG_ERR("Failed to prepare enable pdmtx err=%d", ret);
		return ret;
	}

	/* PDMTX sample rate set */
	if (pdmtx_sample_rate_set(dev, out_param->sample_rate)) {
		LOG_ERR("Failed to config sample rate %d",
			out_param->sample_rate);
		return -ESRCH;
	}

	pdmtx_fifo_enable(dev, FIFO_SEL_DMA);

	pdmtx_digital_enable(dev);

	/* Clear FIFO ERROR */
	check_pdmtx_fifo_error(dev);

	return 0;
}

/* @brief physical PDMTX device disable */
static int phy_pdmtx_disable(struct device *dev, void *param)
{
	ARG_UNUSED(param);

	pdmtx_digital_disable(dev);

	pdmtx_fifo_disable(dev);

	return 0;
}

/* @brief physical PDMTX IO control and commands */
static int phy_pdmtx_ioctl(struct device *dev, uint32_t cmd, void *param)
{
	const struct phy_pdmtx_config_data *cfg = dev->config;
	int ret = 0;

	switch (cmd) {
	case PHY_CMD_DUMP_REGS:
	{
		pdmtx_dump_register(dev);
		break;
	}
	case PHY_CMD_GET_AOUT_DMA_INFO:
	{
		struct audio_out_dma_info *info = (struct audio_out_dma_info *)param;
		info->dma_info.dma_chan = cfg->dma_fifo0.dma_chan;
		info->dma_info.dma_dev_name = cfg->dma_fifo0.dma_dev_name;
		info->dma_info.dma_id = cfg->dma_fifo0.dma_id;
		break;
	}
	case AOUT_CMD_GET_SAMPLERATE:
	{
		ret = pdmtx_sample_rate_get(dev);
		if (ret < 0) {
			LOG_ERR("Failed to get ADC sample rate err=%d", ret);
			return ret;
		}
		*(audio_sr_sel_e *)param = (audio_sr_sel_e)ret;
		ret = 0;
		break;
	}
	case AOUT_CMD_SET_SAMPLERATE:
	{
		audio_sr_sel_e val = *(audio_sr_sel_e *)param;
		ret = pdmtx_sample_rate_set(dev, val);
		if (ret) {
			LOG_ERR("Failed to set ADC sample rate err=%d", ret);
			return ret;
		}
		break;
	}
	default:
		LOG_ERR("Unsupport command %d", cmd);
		return -ENOTSUP;
	}

	return ret;
}

const struct phy_audio_driver_api phy_pdmtx_drv_api = {
	.audio_enable = phy_pdmtx_enable,
	.audio_disable = phy_pdmtx_disable,
	.audio_ioctl = phy_pdmtx_ioctl,
};

/* dump i2stx device tree infomation */
static void __pdmtx_dt_dump_info(const struct phy_pdmtx_config_data *cfg)
{
#if (PHY_DEV_SHOW_DT_INFO == 1)
	LOG_INF("**     PDMTX BASIC INFO     **");
	LOG_INF("     BASE: %08x", cfg->reg_base);
	LOG_INF("   CLK-ID: %08x", cfg->clk_id);
	LOG_INF("   RST-ID: %08x", cfg->rst_id);
	LOG_INF("DMA0-NAME: %s", cfg->dma_fifo0.dma_dev_name);
	LOG_INF("  DMA0-ID: %08x", cfg->dma_fifo0.dma_id);
	LOG_INF("  DMA0-CH: %08x", cfg->dma_fifo0.dma_chan);
#endif
}

static int phy_pdmtx_init(const struct device *dev)
{
	const struct phy_pdmtx_config_data *cfg = dev->config;

	__pdmtx_dt_dump_info(cfg);

	/* reset I2STX controller */
	acts_reset_peripheral(cfg->rst_id);

	printk("PDMTX init successfully\n");

	return 0;
}

static const struct acts_pin_config pdmtx_pins_state[] = {
	CONFIG_AUDIO_PDMTX_0_MFP
};

/* physical i2stx config data */
static const struct phy_pdmtx_config_data phy_pdmtx_config_data0 = {
	.reg_base = AUDIO_PDMTX_REG_BASE,
	.clk_id = CLOCK_ID_PDMTX,
	.rst_id = RESET_ID_PDMTX,
	AUDIO_DMA_FIFO_DEF(PDMTX, 0),
	.pinmux = pdmtx_pins_state,
	.pinmux_size = ARRAY_SIZE(pdmtx_pins_state),
};

DEVICE_DEFINE(pdmtx0, CONFIG_AUDIO_PDMTX_0_NAME, phy_pdmtx_init, device_pm_control_nop,
	    NULL, &phy_pdmtx_config_data0,
	    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, &phy_pdmtx_drv_api);

