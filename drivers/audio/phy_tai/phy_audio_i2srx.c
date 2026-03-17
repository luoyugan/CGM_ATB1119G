/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio I2SRX physical implementation
 */

/*
 * Features
 *    - Support slave mode
 *	- I2SRXFIFO(16 x 16bits level)
 *	- Support 2 format: left-justified format, I2S format
 *    - Sample rate support 8k/12k/11.025k/16k/22.05k/24k/32k/44.1k/48k
 */
#include <kernel.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <soc.h>
#include <board_cfg.h>
#include "../phy_audio_common.h"
#include <drivers/audio/audio_in.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(i2srx0, CONFIG_LOG_DEFAULT_LEVEL);

/***************************************************************************************************
 * I2SRX_CTL
 */
#define I2SRX0_CTL_RXMODELSEL_SHIFT                          (1)
#define I2SRX0_CTL_RXMODELSEL_MASK                           (0x3 << I2SRX0_CTL_RXMODELSEL_SHIFT)
#define I2SRX0_CTL_RXMODELSEL(x)                             ((x) << I2SRX0_CTL_RXMODELSEL_SHIFT)

#define I2SRX0_CTL_RXEN                                      BIT(0)

/***************************************************************************************************
 * I2SRX_CTL
 */
#define ADC_FIFOCTL_ADDRESS                                 (0x4005c108)

#define ADC_FIFOCTL_ADFIS                                    BIT(4) /* 0: ADC; 1: I2SRX */
#define ADC_FIFOCTL_ADFOS                                    BIT(3) /* 0: CPU; 1: DMA */
#define ADC_FIFOCTL_ADFFIE                                   BIT(2)
#define ADC_FIFOCTL_ADFFDE                                   BIT(1)
#define ADC_FIFOCTL_ADFRST                                   BIT(0)

/***************************************************************************************************
 * CMU_I2SCLK
 */
#define CMU_I2SCLK_RXLRCLKRVS                                BIT(23)
#define CMU_I2SCLK_RXBCLKRVS                                 BIT(22)
#define CMU_I2SCLK_RXMCLKRVS                                 BIT(20)
#define CMU_I2SCLK_RXLRCLKSRC                                BIT(19)
#define CMU_I2SCLK_RXBCLKSRC                                 BIT(18)
#define CMU_I2SCLK_RXMCLKSRC_SHIFT                           (16)
#define CMU_I2SCLK_RXMCLKSRC_MASK                            ((0x3 << CMU_I2SCLK_RXMCLKSRC_SHIFT))
#define CMU_I2SCLK_RXMCLKSRC(x)                              ((x) << CMU_I2SCLK_RXMCLKSRC_SHIFT)

/***************************************************************************************************
 * CMU_ADCCLK
 */
#define CMU_ADCCLK_ADCFIFOCLKEN                              BIT(16)

#define CMU_DEVCLKEN0_ADCCLKEN                               (24)
#define MRCR0_ADCRESET                                       (24)

/*
 * @struct acts_audio_i2srx
 * @brief I2SRX controller hardware register
 */
struct acts_audio_i2srx {
	volatile uint32_t rx_ctl; /* I2SRX control */
};

/**
 * union phy_i2srx_features
 * @brief The infomation from DTS to control the I2SRX features to enable or nor.
 */
typedef union {
	uint16_t raw;
	struct {
		uint32_t slave_internal_clk : 1; /* slave mode MCLK to use internal clock */
		uint32_t mclk_reverse : 1; /* mclk reverse */
		uint16_t lrclk_reverse : 1; /* lrclk reverse */
		uint16_t bclk_reverse : 1; /* bclk reverse */
		uint16_t lrclk_source : 1; /* 0: I2S_LRCLK_EXT0; 1: I2S_LRCLK_EXT1 */
		uint16_t bclk_source : 1; /* 0: I2S_BCLK_EXT0; 1: I2S_BCLK_EXT1 */
		uint16_t mclk_source : 1; /* 0: I2S_MCLK_EXT0; 1: I2S_MCLK_EXT1 */
		uint32_t format : 2; /* I2S transfer format */
	} v;
} phy_i2srx_features;

/**
 * struct phy_i2srx_config_data
 * @brief The hardware related data that used by physical i2srx driver.
 */
struct phy_i2srx_config_data {
	uint32_t reg_base; /* I2STX controller register base address */
	uint8_t clk_id; /* I2STX devclk id */
	uint8_t rst_id; /* I2STX reset id */
	struct audio_dma_dt dma_fifo0; /* DMA resource for SPDIFRX */
	const struct acts_pin_config *pinmux; /* I2STX pinmux array */
	uint8_t pinmux_size; /* I2STX pinmux array size */
	phy_i2srx_features features; /* I2STX features */
};

/* @brief  Get the I2SRX controller base address */
static inline struct acts_audio_i2srx *get_i2srx_reg_base(struct device *dev)
{
	const struct phy_i2srx_config_data *cfg = dev->config;
	return (struct acts_audio_i2srx *)cfg->reg_base;
}

/* @brief Dump the I2SRX relative registers */
static void i2srx_dump_register(struct device *dev)
{
	struct acts_audio_i2srx *i2srx_reg = get_i2srx_reg_base(dev);

	LOG_INF("** i2srx contoller regster **");
	LOG_INF("          BASE: %08x", (uint32_t)i2srx_reg);
	LOG_INF("     I2SRX_CTL: %08x", i2srx_reg->rx_ctl);
	LOG_INF("   ADC_FIFOCTL: %08x", sys_read32(ADC_FIFOCTL_ADDRESS));
	LOG_INF("    CMU_I2SCLK: %08x", sys_read32(CMU_I2SCLK));
}

/* @brief I2SRX sample rate config */
static int i2srx_sample_rate_set(struct device *dev, uint16_t sr_khz)
{
	const struct phy_i2srx_config_data *cfg = dev->config;
	uint32_t reg = sys_read32(CMU_I2SCLK) & ~(0xFF << 16);

	/* I2SRX slave mode does no need to set sample rate, however tai I2S can only support up to 48K */
	if (sr_khz > SAMPLE_RATE_48KHZ) {
		LOG_ERR("sample rate invalid %d", sr_khz);
		return -EPERM;
	}

	/* I2SRX LRCLK reverse */
	if (PHY_DEV_FEATURE(lrclk_reverse))
		reg |= CMU_I2SCLK_RXLRCLKRVS;

	/* I2SRX BCLK reverse */
	if (PHY_DEV_FEATURE(bclk_reverse))
		reg |= CMU_I2SCLK_RXBCLKRVS;

	/* I2SRX MCLK reverse */
	if (PHY_DEV_FEATURE(mclk_reverse))
		reg |= CMU_I2SCLK_RXMCLKRVS;

	/* I2SRX LRCLK source */
	if (PHY_DEV_FEATURE(lrclk_source))
		reg |= CMU_I2SCLK_RXLRCLKSRC;

	/* I2SRX BCLK source */
	if (PHY_DEV_FEATURE(bclk_source))
		reg |= CMU_I2SCLK_RXBCLKSRC;

	if (PHY_DEV_FEATURE(slave_internal_clk)) {
		/* I2SRX MCLK source from HOSC */
		reg |= CMU_I2SCLK_RXMCLKSRC(0);
	} else {
		/* I2SRX MCLK source from external PAD */
		if (PHY_DEV_FEATURE(mclk_source))
			reg |= CMU_I2SCLK_RXMCLKSRC(2);
		else
			reg |= CMU_I2SCLK_RXMCLKSRC(1);
	}

	sys_write32(reg, CMU_I2SCLK);

	return 0;
}

/* @brief Disable the I2SRX FIFO which shared with ADC */
static void i2srx_fifo_disable(struct device *dev)
{
	ARG_UNUSED(dev);

	sys_write32(0, ADC_FIFOCTL_ADDRESS);

	/* ADC FIFO clock disable */
	sys_write32(sys_read32(CMU_ADCCLK) & ~CMU_ADCCLK_ADCFIFOCLKEN, CMU_ADCCLK);
}

/* @brief Check the ADC FIFO is working or not */
static bool is_i2srx_fifo_working(struct device *dev)
{
	ARG_UNUSED(dev);

	uint32_t reg = sys_read32(ADC_FIFOCTL_ADDRESS);
	return !!(reg & ADC_FIFOCTL_ADFRST);
}

/* @brief  Enable the ADC FIFO */
static void i2srx_fifo_enable(struct device *dev, audio_fifouse_sel_e sel)
{
	uint32_t reg = 0;

	if (FIFO_SEL_CPU == sel) {
		reg |= (ADC_FIFOCTL_ADFFIE | ADC_FIFOCTL_ADFRST | ADC_FIFOCTL_ADFIS);
	} else if (FIFO_SEL_DMA == sel) {
		reg |= (ADC_FIFOCTL_ADFFDE | ADC_FIFOCTL_ADFRST
				| ADC_FIFOCTL_ADFOS | ADC_FIFOCTL_ADFIS);
	} else {
		LOG_ERR("invalid fifo sel %d", sel);
	}
	sys_write32(reg, ADC_FIFOCTL_ADDRESS);

	/* ADC FIFO clock enable */
	sys_write32(sys_read32(CMU_ADCCLK) | CMU_ADCCLK_ADCFIFOCLKEN, CMU_ADCCLK);
}

/* @brief  I2SRX digital function control */
static void i2srx_digital_enable(struct device *dev)
{
	const struct phy_i2srx_config_data *cfg = dev->config;
	struct acts_audio_i2srx *i2srx_reg = get_i2srx_reg_base(dev);
	uint32_t reg = 0;
	uint8_t fmt = PHY_DEV_FEATURE(format);

	if ((fmt != I2S_FORMAT) && (fmt != LEFT_JUSTIFIED_FORMAT)) {
		LOG_ERR("invalid fmt:%d\n", fmt);
		return ;
	}

	reg |= I2SRX0_CTL_RXMODELSEL(fmt);

	i2srx_reg->rx_ctl = reg;

	/* I2S RX enable */
	i2srx_reg->rx_ctl |= I2SRX0_CTL_RXEN;
}

/* @brief  Disable I2SRX digital function */
static void i2srx_digital_disable(struct device *dev)
{
	struct acts_audio_i2srx *i2srx_reg = get_i2srx_reg_base(dev);
	i2srx_reg->rx_ctl = 0;
}

/* @brief physical I2SRX device enable */
static int phy_i2srx_enable(struct device *dev, void *param)
{
	const struct phy_i2srx_config_data *cfg = dev->config;
	ain_param_t *in_param = (ain_param_t *)param;

	if (!in_param) {
		LOG_ERR("Invalid parameters");
		return -EINVAL;
	}

	if (in_param->channel_type != AUDIO_CHANNEL_I2SRX) {
		LOG_ERR("Invalid channel type %d", in_param->channel_type);
		return -EINVAL;
	}

	if (is_i2srx_fifo_working(dev)) {
		LOG_ERR("ADC fifo now is using");
		return -EBUSY;
	}

    /* Config the i2srx pin state */
	acts_pinmux_setup_pins(cfg->pinmux, cfg->pinmux_size);

	/* enable i2srx clock */
	acts_clock_peripheral_enable(cfg->clk_id);

	/* enable adc clock as I2SRX use ADC FIFO */
	acts_clock_peripheral_enable(CMU_DEVCLKEN0_ADCCLKEN);

	/* I2SRX sample rate set */
	if (i2srx_sample_rate_set(dev, in_param->sample_rate)) {
		LOG_ERR("Failed to config sample rate %d",
			in_param->sample_rate);
		return -ESRCH;
	}

	i2srx_fifo_enable(dev, FIFO_SEL_DMA);

	i2srx_digital_enable(dev);

	return 0;
}

/* @brief physical I2SRX device disable */
static int phy_i2srx_disable(struct device *dev, void *param)
{
	const struct phy_i2srx_config_data *cfg = dev->config;

	i2srx_fifo_disable(dev);

	i2srx_digital_disable(dev);

	acts_clock_peripheral_disable(cfg->clk_id);
	acts_clock_peripheral_disable(CMU_DEVCLKEN0_ADCCLKEN);

	return 0;
}

/* @brief physical I2SRX IO commands */
static int phy_i2srx_ioctl(struct device *dev, uint32_t cmd, void *param)
{
	const struct phy_i2srx_config_data *cfg = dev->config;
	int ret = 0;

	switch (cmd) {
	case PHY_CMD_DUMP_REGS:
	{
		i2srx_dump_register(dev);
		break;
	}
	case PHY_CMD_GET_AIN_DMA_INFO:
	{
		struct audio_out_dma_info *info = (struct audio_out_dma_info *)param;
		info->dma_info.dma_chan = cfg->dma_fifo0.dma_chan;
		info->dma_info.dma_dev_name = cfg->dma_fifo0.dma_dev_name;
		info->dma_info.dma_id = cfg->dma_fifo0.dma_id;
		break;
	}
	default:
		LOG_ERR("Unsupport command %d", cmd);
		return -ENOTSUP;
	}

	return ret;
}

const struct phy_audio_driver_api phy_i2srx_drv_api = {
	.audio_enable = phy_i2srx_enable,
	.audio_disable = phy_i2srx_disable,
	.audio_ioctl = phy_i2srx_ioctl,
};

/* dump i2srx device tree infomation */
static void __i2srx_dt_dump_info(const struct phy_i2srx_config_data *cfg)
{
#if (PHY_DEV_SHOW_DT_INFO == 1)
	LOG_INF("**     I2SRX BASIC INFO     **");
	LOG_INF("     BASE: %08x", cfg->reg_base);
	LOG_INF("   CLK-ID: %08x", cfg->clk_id);
	LOG_INF("   RST-ID: %08x", cfg->rst_id);
	LOG_INF("DMA0-NAME: %s", cfg->dma_fifo0.dma_dev_name);
	LOG_INF("  DMA0-ID: %08x", cfg->dma_fifo0.dma_id);
	LOG_INF("  DMA0-CH: %08x", cfg->dma_fifo0.dma_chan);

	LOG_INF("** 	I2SRX FEATURES	 **");
	LOG_INF("  INTERNAL-CLK: %d", PHY_DEV_FEATURE(slave_internal_clk));
	LOG_INF("  MCLK-REVERSE: %d", PHY_DEV_FEATURE(mclk_reverse));
	LOG_INF("  BCLK-REVERSE: %d", PHY_DEV_FEATURE(bclk_reverse));
	LOG_INF(" LRCLK-REVERSE: %d", PHY_DEV_FEATURE(lrclk_reverse));
	LOG_INF("   MCLK-SOURCE: %d", PHY_DEV_FEATURE(mclk_source));
	LOG_INF("   BCLK-SOURCE: %d", PHY_DEV_FEATURE(bclk_source));
	LOG_INF("  LRCLK-SOURCE: %d", PHY_DEV_FEATURE(lrclk_source));
#endif
}

static int phy_i2srx_init(const struct device *dev)
{
	const struct phy_i2srx_config_data *cfg = dev->config;

	__i2srx_dt_dump_info(cfg);

	/* reset I2SRX controller */
	acts_reset_peripheral(cfg->rst_id);

	/* reset ADC controller as I2SRX use ADC FIFO */
	acts_reset_peripheral(MRCR0_ADCRESET);

	printk("I2SRX init successfully\n");

	return 0;
}

static const struct acts_pin_config i2srx_pins_state[] = {
	CONFIG_AUDIO_I2SRX_0_MFP
};

/* physical i2stx config data */
static const struct phy_i2srx_config_data phy_i2srx_config_data0 = {
	.reg_base = AUDIO_I2SRX0_REG_BASE,
	.clk_id = CLOCK_ID_I2SRX,
	.rst_id = RESET_ID_I2S,
	AUDIO_DMA_FIFO_DEF(I2SRX, 0),
	.pinmux = i2srx_pins_state,
	.pinmux_size = ARRAY_SIZE(i2srx_pins_state),

	PHY_DEV_FEATURE_DEF(slave_internal_clk) = CONFIG_AUDIO_I2SRX_0_SLAVE_INTERNAL_CLK,
	PHY_DEV_FEATURE_DEF(mclk_reverse) = CONFIG_AUDIO_I2SRX_0_MCLK_REVERSE,
	PHY_DEV_FEATURE_DEF(bclk_reverse) = CONFIG_AUDIO_I2SRX_0_BCLK_REVERSE,
	PHY_DEV_FEATURE_DEF(lrclk_reverse) = CONFIG_AUDIO_I2SRX_0_LRCLK_REVERSE,
	PHY_DEV_FEATURE_DEF(mclk_source) = CONFIG_AUDIO_I2SRX_0_MCLK_SOURCE,
	PHY_DEV_FEATURE_DEF(bclk_source) = CONFIG_AUDIO_I2SRX_0_BCLK_SOURCE,
	PHY_DEV_FEATURE_DEF(lrclk_source) = CONFIG_AUDIO_I2SRX_0_LRCLK_SOURCE,
};

DEVICE_DEFINE(i2srx0, CONFIG_AUDIO_I2SRX_0_NAME, phy_i2srx_init, device_pm_control_nop,
	    NULL, &phy_i2srx_config_data0,
	    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, &phy_i2srx_drv_api);

