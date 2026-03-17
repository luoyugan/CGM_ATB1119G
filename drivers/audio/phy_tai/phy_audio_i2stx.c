/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio I2STX physical implementation
 */

/*
 * Features
 *    - Support slave mode only
 *    - Support I2S TX/RX 5 wires mode
 *	- I2STX FIFO(16 x 16bits level)
 *	- Support 2 format: left-justified format, I2S format
 *	- Support 16 effective data width
 *    - Sample rate support 8k/12k/11.025k/16k/22.05k/24k/32k/44.1k/48k under slaver mode
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
LOG_MODULE_REGISTER(i2stx0, CONFIG_LOG_DEFAULT_LEVEL);

/***************************************************************************************************
 * I2STX_CTL
 */
#define I2STX0_CTL_TXD_DELAY_SHIFT                            (21) /* TX data output position */
#define I2STX0_CTL_TXD_DELAY_MAST                             (0x3 << I2STX0_CTL_TXD_DELAY_SHIFT)
#define I2STX0_CTL_TXD_DELAY(x)                               ((x) << I2STX0_CTL_TXD_DELAY_SHIFT)
#define I2STX0_CTL_TXMODELSEL_SHIFT                           (1)
#define I2STX0_CTL_TXMODELSEL_MASK                            (0x3 << I2STX0_CTL_TXMODELSEL_SHIFT)
#define I2STX0_CTL_TXMODELSEL(x)                              ((x) << I2STX0_CTL_TXMODELSEL_SHIFT)
#define I2STX0_CTL_TXEN                                       BIT(0)

/***************************************************************************************************
 * I2STX0_FIFOCTL
 */
#define I2STX0_FIFOCTL_FIFO_IS_SHIFT                          (4)
#define I2STX0_FIFOCTL_FIFO_IS_MASK                           (0x3 << I2STX0_FIFOCTL_FIFO_IS_SHIFT)
#define I2STX0_FIFOCTL_FIFO_IS(x)                             ((x) << I2STX0_FIFOCTL_FIFO_IS_SHIFT)
#define I2STX0_FIFOCTL_FIFO_OS                                BIT(3) /* I2STX FIFO output selection. 0: I2STX; 1: PDMTX */
#define I2STX0_FIFOCTL_FIFO_IEN                               BIT(2)
#define I2STX0_FIFOCTL_FIFO_DEN                               BIT(1)
#define I2STX0_FIFOCTL_FIFO_RST                               BIT(0)

/***************************************************************************************************
 * I2STX0_FIFOSTA
 */
#define I2STX0_FIFOSTA_TFEP                                   BIT(8) /* I2STX FIFO error pending bit */
#define I2STX0_FIFOSTA_IP                                     BIT(7) /* I2STX FIFO half empty IRQ pending bit */
#define I2STX0_FIFOSTA_TFFU                                   BIT(6) /* I2STX FIFO full flag */
#define I2STX0_FIFOSTA_STA_SHIFT                              (0)
#define I2STX0_FIFOSTA_STA_MASK                               (0x1F << I2STX0_FIFOSTA_STA_SHIFT)

/***************************************************************************************************
 * I2STX0_FIFO_CNT
 */
#define I2STX0_FIFO_CNT_IP                                    BIT(18) /* I2STX FIFO overflow IRQ pending */
#define I2STX0_FIFO_CNT_IE                                    BIT(17) /* I2STX FIFO coutner overflow IRQ pending */
#define I2STX0_FIFO_CNT_EN                                    BIT(16) /* I2STX FIFO counter enable */
#define I2STX0_FIFO_CNT_CNT_SHIFT                             (0)
#define I2STX0_FIFO_CNT_CNT_MASK                              (0xFFFF << I2STX0_FIFO_CNT_CNT_SHIFT)

/***************************************************************************************************
 * CMU_I2SCLK
 */
#define CMU_I2SCLK_TXFIFOCLKEN                                BIT(8)
#define CMU_I2SCLK_TXLRCLKRVS                                 BIT(7)
#define CMU_I2SCLK_TXBCLKRVS                                  BIT(6)
#define CMU_I2SCLK_TXMCLKRVS                                  BIT(4)
#define CMU_I2SCLK_TXLRCLKSRC                                 BIT(3)
#define CMU_I2SCLK_TXBCLKSRC                                  BIT(2)
#define CMU_I2SCLK_TXMCLKSRC_SHIFT                            (0)
#define CMU_I2SCLK_TXMCLKSRC_MASK                             (0x3 << CMU_I2SCLK_TXMCLKSRC_SHIFT)
#define CMU_I2SCLK_TXMCLKSRC(x)                               ((x) << CMU_I2SCLK_TXMCLKSRC_SHIFT)

/***************************************************************************************************
 * i2STX FEATURES CONGIURATION
 */

#define I2STX_FIFO_LEVEL                                      (16)

/*
 * @struct acts_audio_i2stx0
 * @brief I2STX controller hardware register
 */
struct acts_audio_i2stx {
	volatile uint32_t tx_ctl; /* I2STX control */
	volatile uint32_t fifoctl; /* I2STX FIFO control */
	volatile uint32_t fifostat; /* I2STX FIFO state */
	volatile uint32_t dat; /* I2STX FIFO data */
	volatile uint32_t fifocnt; /* I2STX out FIFO counter */
};

/*
 * struct phy_i2stx_drv_data
 * @brief The software related data that used by physical i2stx driver.
 */
struct phy_i2stx_drv_data {
	uint32_t fifo_cnt; /* I2STX FIFO hardware counter max value is 0xFFFF */
};

/**
 * union phy_i2stx_features
 * @brief The infomation from DTS to control the I2STX features to enable or nor.
 */
typedef union {
	uint16_t raw;
	struct {
		uint16_t slave_internal_clk : 1; /* slave mode MCLK to use internal clock */
		uint16_t mclk_reverse : 1; /* mclk reverse */
		uint16_t lrclk_reverse : 1; /* lrclk reverse */
		uint16_t bclk_reverse : 1; /* bclk reverse */
		uint16_t lrclk_source : 1; /* 0: I2S_LRCLK_EXT0; 1: I2S_LRCLK_EXT1 */
		uint16_t bclk_source : 1; /* 0: I2S_BCLK_EXT0; 1: I2S_BCLK_EXT1 */
		uint16_t mclk_source : 1; /* 0: I2S_MCLK_EXT0; 1: I2S_MCLK_EXT1 */
		uint16_t format : 2; /* I2S transfer format */
		uint16_t txd_delay : 2; /* I2STX data delay */
	} v;
} phy_i2stx_features;

/**
 * struct phy_i2stx_config_data
 * @brief The hardware related data that used by physical i2stx driver.
 */
struct phy_i2stx_config_data {
	uint32_t reg_base; /* I2STX controller register base address */
	uint8_t clk_id; /* I2STX devclk id */
	uint8_t rst_id; /* I2STX reset id */
	struct audio_dma_dt dma_fifo0; /* DMA resource for I2STX */
	const struct acts_pin_config *pinmux; /* I2STX pinmux array */
	uint8_t pinmux_size; /* I2STX pinmux array size */
	void (*irq_config)(void); /* IRQ configuration function */
	phy_i2stx_features features; /* I2STX features */
};

/* @brief  Get the I2STX controller base address */
static inline struct acts_audio_i2stx *get_i2stx_reg_base(struct device *dev)
{
	const struct phy_i2stx_config_data *cfg = dev->config;
	return (struct acts_audio_i2stx *)cfg->reg_base;
}

/* @brief Dump the I2STX relative registers */
static void i2stx_dump_register(struct device *dev)
{
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);

	LOG_INF("** i2stx contoller regster **");
	LOG_INF("          BASE: %08x", (uint32_t)i2stx_reg);
	LOG_INF("     I2STX_CTL: %08x", i2stx_reg->tx_ctl);
	LOG_INF(" I2STX_FIFOCTL: %08x", i2stx_reg->fifoctl);
	LOG_INF(" I2STX_FIFOSTA: %08x", i2stx_reg->fifostat);
	LOG_INF("     I2STX_DAT: %08x", i2stx_reg->dat);
	LOG_INF(" I2STX_FIFOCNT: %08x", i2stx_reg->fifocnt);
	LOG_INF("    CMU_I2SCLK: %08x", sys_read32(CMU_I2SCLK));
}

/* @brief I2STX sample rate config */
static int i2stx_sample_rate_set(struct device *dev, uint16_t sr_khz)
{
	const struct phy_i2stx_config_data *cfg = dev->config;
	uint32_t reg = sys_read32(CMU_I2SCLK) & ~0x7F;

	/* I2STX slave mode does no need to set sample rate, however tai I2S can only support up to 48K */
	if (sr_khz > SAMPLE_RATE_48KHZ) {
		LOG_ERR("sample rate invalid %d", sr_khz);
		return -EPERM;
	}

	/* I2STX LRCLK reverse */
	if (PHY_DEV_FEATURE(lrclk_reverse))
		reg |= CMU_I2SCLK_TXLRCLKRVS;

	/* I2STX BCLK reverse */
	if (PHY_DEV_FEATURE(bclk_reverse))
		reg |= CMU_I2SCLK_TXBCLKRVS;

	/* I2STX MCLK reverse */
	if (PHY_DEV_FEATURE(mclk_reverse))
		reg |= CMU_I2SCLK_TXMCLKRVS;

	/* I2STX LRCLK source */
	if (PHY_DEV_FEATURE(lrclk_source))
		reg |= CMU_I2SCLK_TXLRCLKSRC;

	/* I2STX BCLK source */
	if (PHY_DEV_FEATURE(bclk_source))
		reg |= CMU_I2SCLK_TXBCLKSRC;

	if (PHY_DEV_FEATURE(slave_internal_clk)) {
		/* I2STX MCLK source from HOSC */
		reg |= CMU_I2SCLK_TXMCLKSRC(0);
	} else {
		/* I2STX MCLK source from external PAD */
		if (PHY_DEV_FEATURE(mclk_source))
			reg |= CMU_I2SCLK_TXMCLKSRC(2);
		else
			reg |= CMU_I2SCLK_TXMCLKSRC(1);
	}

	sys_write32(reg, CMU_I2SCLK);

	return 0;
}

/* @brief  Disable the I2STX FIFO */
static void i2stx_fifo_disable(struct device *dev)
{
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
	i2stx_reg->fifoctl = 0;

	/* I2STX FIFO clock disable */
	sys_write32(sys_read32(CMU_I2SCLK) & ~CMU_I2SCLK_TXFIFOCLKEN, CMU_I2SCLK);
}

/* @brief Check the I2STX FIFO is working or not */
static bool is_i2stx_fifo_working(struct device *dev)
{
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
	return !!(i2stx_reg->fifoctl & I2STX0_FIFOCTL_FIFO_RST);
}

/* @brief Check the I2STX FIFO is error */
static bool check_i2stx_fifo_error(struct device *dev)
{
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);

	if (i2stx_reg->fifostat & I2STX0_FIFOSTA_TFEP) {
		i2stx_reg->fifostat |= I2STX0_FIFOSTA_TFEP;
		return true;
	}

	return false;
}

/* @brief Get the I2STX FIFO status which indicates how many samples not filled */
static uint32_t i2stx_fifo_status(struct device *dev)
{
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
	return (i2stx_reg->fifostat & I2STX0_FIFOSTA_STA_MASK) >> I2STX0_FIFOSTA_STA_SHIFT;
}

/* @brief  Enable the I2STX FIFO */
static void i2stx_fifo_enable(struct device *dev, audio_fifouse_sel_e sel)
{
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
	uint32_t reg = 0;

	if (FIFO_SEL_CPU == sel) {
		reg |= (I2STX0_FIFOCTL_FIFO_IEN | I2STX0_FIFOCTL_FIFO_RST);
	} else if (FIFO_SEL_DMA == sel) {
		reg |= (I2STX0_FIFOCTL_FIFO_DEN | I2STX0_FIFOCTL_FIFO_RST
				| I2STX0_FIFOCTL_FIFO_IS(1));
	} else {
		LOG_ERR("invalid fifo sel %d", sel);
	}
	i2stx_reg->fifoctl = reg;

	/* I2STX FIFO clock enable */
	sys_write32(sys_read32(CMU_I2SCLK) | CMU_I2SCLK_TXFIFOCLKEN, CMU_I2SCLK);

	/* clear I2STX FIFO half empty IRQ pending */
	i2stx_reg->fifostat |= I2STX0_FIFOSTA_IP;
}

/* @brief  Enable the I2STX FIFO counter function */
static void i2stx_fifocount_enable(struct device *dev)
{
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
	uint32_t reg = 0;

	/* enable I2STX FIFO counter */
	reg = I2STX0_FIFO_CNT_EN;

	/* overflow irq enable */
	reg |= I2STX0_FIFO_CNT_IE;

	/* clear overflow pending */
	reg |= I2STX0_FIFO_CNT_IP;

	i2stx_reg->fifocnt = reg;

	LOG_INF("I2STX sample counter enable");
}

/* @brief  Disable the I2STX FIFO counter function */
static void i2stx_fifocount_disable(struct device *dev)
{
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
	i2stx_reg->fifocnt &= ~(I2STX0_FIFO_CNT_EN | I2STX0_FIFO_CNT_IE);
}

/* @brief  Reset the I2STX FIFO counter function and by default to enable IRQ */
static void i2stx_fifocount_reset(struct device *dev)
{
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
	i2stx_reg->fifocnt &= ~(I2STX0_FIFO_CNT_EN | I2STX0_FIFO_CNT_IE);

	i2stx_reg->fifocnt |= I2STX0_FIFO_CNT_EN;

	i2stx_reg->fifocnt |= I2STX0_FIFO_CNT_IE;
}

/* @brief  Enable the I2STX FIFO counter function */
static uint32_t i2stx_read_fifocount(struct device *dev)
{
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
	return i2stx_reg->fifocnt & I2STX0_FIFO_CNT_CNT_MASK;
}

/* @brief  I2STX digital function control enable */
static void i2stx_digital_enable(struct device *dev)
{
	const struct phy_i2stx_config_data *cfg = dev->config;
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
	uint32_t reg = 0;
	uint8_t fmt = PHY_DEV_FEATURE(format);

	if ((fmt != I2S_FORMAT) && (fmt != LEFT_JUSTIFIED_FORMAT)) {
		LOG_ERR("invalid fmt:%d\n", fmt);
		return ;
	}

	/* By default TXD = 2MCLK */
	reg |= I2STX0_CTL_TXD_DELAY(PHY_DEV_FEATURE(txd_delay));

	reg |= (I2STX0_CTL_TXMODELSEL(fmt) | I2STX0_CTL_TXEN);

	i2stx_reg->tx_ctl = reg;
}

/* @brief  I2STX digital function control disable */
static void i2stx_digital_disable(struct device *dev)
{
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
	i2stx_reg->tx_ctl = 0;
}

/* @brief I2STX prepare enable */
static int phy_i2stx_prepare_enable(struct device *dev, aout_param_t *out_param)
{
	const struct phy_i2stx_config_data *cfg = dev->config;

	if (!out_param) {
		LOG_ERR("Invalid parameters");
		return -EINVAL;
	}

	if (!(out_param->channel_type & AUDIO_CHANNEL_I2STX)) {
		LOG_ERR("Invalid channel type %d", out_param->channel_type);
		return -EINVAL;
	}

	if (AOUT_FIFO_I2STX0 != out_param->outfifo_type) {
		LOG_ERR("Invalid fifo type %d", out_param->outfifo_type);
		return -EINVAL;
	}

    /* Config the i2stx pin state */
	acts_pinmux_setup_pins(cfg->pinmux, cfg->pinmux_size);

	acts_clock_peripheral_enable(cfg->clk_id);

	return 0;
}

/* @brief physical I2STX device enable */
static int phy_i2stx_enable(struct device *dev, void *param)
{
	aout_param_t *out_param = (aout_param_t *)param;
	int ret;

	ret = phy_i2stx_prepare_enable(dev, out_param);
	if (ret) {
		LOG_ERR("Failed to prepare enable i2stx err=%d", ret);
		return ret;
	}

	/* I2STX sample rate set */
	if (i2stx_sample_rate_set(dev, out_param->sample_rate)) {
		LOG_ERR("Failed to config sample rate %d",
			out_param->sample_rate);
		return -ESRCH;
	}

	if (is_i2stx_fifo_working(dev)) {
		LOG_ERR("I2STX FIFO now is using ...");
		return -EACCES;
	}

	i2stx_fifo_enable(dev, FIFO_SEL_DMA);

	i2stx_digital_enable(dev);

	/* Clear FIFO ERROR */
	check_i2stx_fifo_error(dev);

	return 0;
}

/* @brief physical I2STX device disable */
static int phy_i2stx_disable(struct device *dev, void *param)
{
	struct phy_i2stx_drv_data *data = dev->data;

	i2stx_fifo_disable(dev);
	i2stx_digital_disable(dev);
	data->fifo_cnt = 0;

	return 0;
}

/* @brief physical I2STX IO control and commands */
static int phy_i2stx_ioctl(struct device *dev, uint32_t cmd, void *param)
{
	const struct phy_i2stx_config_data *cfg = dev->config;
	struct phy_i2stx_drv_data *data = dev->data;
	int ret = 0;

	switch (cmd) {
	case PHY_CMD_DUMP_REGS:
	{
		i2stx_dump_register(dev);
		break;
	}
	case AOUT_CMD_GET_SAMPLE_CNT:
	{
		uint32_t val;
		val = i2stx_read_fifocount(dev);
		*(uint32_t *)param = val + data->fifo_cnt;
		LOG_DBG("I2STX FIFO counter: %d", *(uint32_t *)param);
		break;
	}
	case AOUT_CMD_RESET_SAMPLE_CNT:
	{
		data->fifo_cnt = 0;
		i2stx_fifocount_reset(dev);
		break;
	}
	case AOUT_CMD_ENABLE_SAMPLE_CNT:
	{
		i2stx_fifocount_enable(dev);
		break;
	}
	case AOUT_CMD_DISABLE_SAMPLE_CNT:
	{
		i2stx_fifocount_disable(dev);
		break;
	}
	case AOUT_CMD_GET_CHANNEL_STATUS:
	{
		uint8_t status = 0;
		if (check_i2stx_fifo_error(dev)) {
			status = AUDIO_CHANNEL_STATUS_ERROR;
			LOG_DBG("I2STX FIFO ERROR");
		}

		if (i2stx_fifo_status(dev) < (I2STX_FIFO_LEVEL - 1))
			status |= AUDIO_CHANNEL_STATUS_BUSY;

		*(uint8_t *)param = status;
		break;
	}
	case AOUT_CMD_GET_FIFO_LEN:
	{
		*(uint32_t *)param = I2STX_FIFO_LEVEL;
		break;
	}
	case AOUT_CMD_GET_FIFO_AVAILABLE_LEN:
	{
		*(uint32_t *)param = i2stx_fifo_status(dev);
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
	case PHY_CMD_I2STX_IS_MCLK_128FS:
	{
		*(uint8_t *)param = 1;
		break;
	}
	default:
		LOG_ERR("Unsupport command %d", cmd);
		return -ENOTSUP;
	}

	return ret;
}

const struct phy_audio_driver_api phy_i2stx_drv_api = {
	.audio_enable = phy_i2stx_enable,
	.audio_disable = phy_i2stx_disable,
	.audio_ioctl = phy_i2stx_ioctl,
};

/* dump i2stx device tree infomation */
static void __i2stx_dt_dump_info(const struct phy_i2stx_config_data *cfg)
{
#if (PHY_DEV_SHOW_DT_INFO == 1)
	LOG_INF("**     I2STX BASIC INFO     **");
	LOG_INF("     BASE: %08x", cfg->reg_base);
	LOG_INF("   CLK-ID: %08x", cfg->clk_id);
	LOG_INF("   RST-ID: %08x", cfg->rst_id);
	LOG_INF("DMA0-NAME: %s", cfg->dma_fifo0.dma_dev_name);
	LOG_INF("  DMA0-ID: %08x", cfg->dma_fifo0.dma_id);
	LOG_INF("  DMA0-CH: %08x", cfg->dma_fifo0.dma_chan);

	LOG_INF("** 	I2STX FEATURES	 **");
	LOG_INF(" INTERNAL-CLK: %d", PHY_DEV_FEATURE(slave_internal_clk));
	LOG_INF(" MCLK-REVERSE: %d", PHY_DEV_FEATURE(mclk_reverse));
	LOG_INF(" BCLK-REVERSE: %d", PHY_DEV_FEATURE(bclk_reverse));
	LOG_INF("LRCLK-REVERSE: %d", PHY_DEV_FEATURE(lrclk_reverse));
	LOG_INF("  MCLK-SOURCE: %d", PHY_DEV_FEATURE(mclk_source));
	LOG_INF("  BCLK-SOURCE: %d", PHY_DEV_FEATURE(bclk_source));
	LOG_INF(" LRCLK-SOURCE: %d", PHY_DEV_FEATURE(lrclk_source));
	LOG_INF("    TDM-FRAME: %d", PHY_DEV_FEATURE(txd_delay));
#endif
}

static void phy_i2stx_isr(const void *arg)
{
	struct device *dev = (struct device *)arg;
	struct acts_audio_i2stx *i2stx_reg = get_i2stx_reg_base(dev);
	struct phy_i2stx_drv_data *data = dev->data;

	LOG_DBG("fifocnt 0x%x", i2stx_reg->fifocnt);

	if (i2stx_reg->fifocnt & I2STX0_FIFO_CNT_IP) {
		data->fifo_cnt += (AOUT_FIFO_CNT_MAX + 1);
		/* Here we need to wait 100us for the synchronization of audio clock fields */
		k_busy_wait(100);
		i2stx_reg->fifocnt |= I2STX0_FIFO_CNT_IP;
	}
}

static int phy_i2stx_init(const struct device *dev)
{
	const struct phy_i2stx_config_data *cfg = dev->config;
	struct phy_i2stx_drv_data *data = dev->data;

	/* clear driver data */
	memset(data, 0, sizeof(struct phy_i2stx_drv_data));

	__i2stx_dt_dump_info(cfg);

	/* reset I2STX controller */
	acts_reset_peripheral(cfg->rst_id);

	if (cfg->irq_config)
		cfg->irq_config();

	printk("I2STX init successfully\n");

	return 0;
}

static void phy_i2stx_irq_config(void);

/* physical i2stx driver data */
static struct phy_i2stx_drv_data phy_i2stx_drv_data0;

static const struct acts_pin_config i2stx_pins_state[] = {
	CONFIG_AUDIO_I2STX_0_MFP
};

/* physical i2stx config data */
static const struct phy_i2stx_config_data phy_i2stx_config_data0 = {
	.reg_base = AUDIO_I2STX0_REG_BASE,
	.clk_id = CLOCK_ID_I2STX,
	.rst_id = RESET_ID_I2S,
	AUDIO_DMA_FIFO_DEF(I2STX, 0),
	.irq_config = phy_i2stx_irq_config,
	.pinmux = i2stx_pins_state,
	.pinmux_size = ARRAY_SIZE(i2stx_pins_state),

	PHY_DEV_FEATURE_DEF(format) = CONFIG_AUDIO_I2STX_0_FORMAT,
	PHY_DEV_FEATURE_DEF(slave_internal_clk) = CONFIG_AUDIO_I2STX_0_SLAVE_INTERNAL_CLK,
	PHY_DEV_FEATURE_DEF(mclk_reverse) = CONFIG_AUDIO_I2STX_0_MCLK_REVERSE,
	PHY_DEV_FEATURE_DEF(bclk_reverse) = CONFIG_AUDIO_I2STX_0_BCLK_REVERSE,
	PHY_DEV_FEATURE_DEF(lrclk_reverse) = CONFIG_AUDIO_I2STX_0_LRCLK_REVERSE,
	PHY_DEV_FEATURE_DEF(mclk_source) = CONFIG_AUDIO_I2STX_0_MCLK_SOURCE,
	PHY_DEV_FEATURE_DEF(bclk_source) = CONFIG_AUDIO_I2STX_0_BCLK_SOURCE,
	PHY_DEV_FEATURE_DEF(lrclk_source) = CONFIG_AUDIO_I2STX_0_LRCLK_SOURCE,
	PHY_DEV_FEATURE_DEF(txd_delay) = CONFIG_AUDIO_I2STX_0_TX_DELAY,
};

DEVICE_DEFINE(i2stx0, CONFIG_AUDIO_I2STX_0_NAME, phy_i2stx_init, device_pm_control_nop,
	    &phy_i2stx_drv_data0, &phy_i2stx_config_data0,
	    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, &phy_i2stx_drv_api);

/*
 * @brief Enable I2STX IRQ
 * @note I2STX IRQ source as shown below:
 *	- I2STX FIFO Half Empty IRQ
 *	- I2STX FIFO CNT IRQ
 */
static void phy_i2stx_irq_config(void)
{
	IRQ_CONNECT(IRQ_ID_ADC, CONFIG_AUDIO_I2STX_0_IRQ_PRI,
			phy_i2stx_isr,
			DEVICE_GET(i2stx0), 0);
	irq_enable(IRQ_ID_ADC);
}

