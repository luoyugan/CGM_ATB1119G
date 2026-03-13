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
 *    - 16 levels x 16bits FIFO for ADC
 *    - Support MONO single ended and full differential input
 *    - Support AMIC/DMIC input
 *    - Sample rate support 8k/16k/32k
 */
#include <kernel.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <soc.h>
#include <drivers/cfg_drv/dev_config.h>
#include "../phy_audio_common.h"
#include <drivers/audio/audio_in.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(adc0, CONFIG_LOG_DEFAULT_LEVEL);

/***************************************************************************************************
 * ADC_DIGCTL
 */
#define ADC_DIGCTL_PRE_GAIN_SHIFT                             (8)
#define ADC_DIGCTL_PRE_GAIN_MASK                              (0x3 << ADC_DIGCTL_PRE_GAIN_SHIFT)
#define ADC_DIGCTL_PRE_GAIN(x)                                ((x) << ADC_DIGCTL_PRE_GAIN_SHIFT)
#define ADC_DIGCTL_DIG_EN                                     BIT(4) /* ADC digital enable */
#define ADC_DIGCTL_ADDEN                                      BIT(3)
#define ADC_DIGCTL_AADEN                                      BIT(2)

/***************************************************************************************************
 * CH_DIGCTL
 */
#define CH_DIGCTL_MIC_SEL                                     BIT(12) /* 0: AMIC; 1: DMIC */
#define CH_DIGCTL_HPF_EN                                      BIT(11) /* high pass filter control */
#define CH_DIGCTL_HPF_S                                       BIT(10) /* HPF frequency fc0&s0 select */
#define CH_DIGCTL_HPF_N_SHIFT                                 (4)
#define CH_DIGCTL_HPF_N_MASK                                  (0x3F << CH_DIGCTL_HPF_N_SHIFT)
#define CH_DIGCTL_HPF_N(x)                                    ((x) << CH_DIGCTL_HPF_N_SHIFT)
#define CH_DIGCTL_ADCGC_SHIFT                                 (0)
#define CH_DIGCTL_ADCGC_MASK                                  (0x3 << CH_DIGCTL_ADCGC_SHIFT)
#define CH_DIGCTL_ADCGC(x)                                    ((x) << CH_DIGCTL_ADCGC_SHIFT)

/***************************************************************************************************
 * ADC_FIFOCTL
 */
#define ADC_FIFOCTL_ADFIS                                     BIT(4) /* ADC FIFO input select. 0: ADC; 1:I2SRX */
#define ADC_FIFOCTL_ADF0OS                                    BIT(3) /* ADC FIFO output select 0:CPU 1:DMA */
#define ADC_FIFOCTL_ADF0FIE                                   BIT(2)
#define ADC_FIFOCTL_ADF0FDE                                   BIT(1)
#define ADC_FIFOCTL_ADF0RT                                    BIT(0) /* ADC FIFO reset */

/***************************************************************************************************
 * ADC_STAT
 */
#define ADC_STAT_FIFO0_ER                                     BIT(9) /* FIFO error */
#define ADC_STAT_ADF0EF		                                  BIT(8) /* FIFO empty */
#define ADC_STAT_ADF0IP                                       BIT(7) /* FIFO half full IRQ pending flag */
#define ADC_STAT_ADF0S_SHIFT                                  (0)
#define ADC_STAT_ADF0S_MASK                                   (0x1F << ADC_STAT_ADF0S_SHIFT)

/***************************************************************************************************
 * ADC_CTL
 */
#define ADC_CTL_VMIC_VOL_SHIFT                                (30) /* VMIC output voltage control */
#define ADC_CTL_VMIC_VOL_MASK                                 (0x3 << ADC_CTL_VMIC_VOL_SHIFT)
#define ADC_CTL_VMIC_VOL(x)                                   ((x) << ADC_CTL_VMIC_VOL_SHIFT)
#define ADC_CTL_VMIC_EN_SHIFT                                 (28) /* VMIC enable */
#define ADC_CTL_VMIC_EN_MASK                                  (0x3 << ADC_CTL_VMIC_EN_SHIFT)
#define ADC_CTL_VMIC_EN(x)                                    ((x) << ADC_CTL_VMIC_EN_SHIFT)
#define ADC_CTL_VREF_RSEL_SHIFT                               (26)
#define ADC_CTL_VREF_RSEL_MASK                                (0x3 << ADC_CTL_VREF_RSEL_SHIFT)
#define ADC_CTL_VREF_RSEL(x)                                  ((x) << ADC_CTL_VREF_RSEL_SHIFT)
#define ADC_CTL_VREF_FU                                       BIT(25) /* VREF fastup control */
#define ADC_CTL_VREF_EN                                       BIT(24) /* VREF enable control */
#define ADC_CTL_BIASEN                                        BIT(20) /* BIAS enable */
#define ADC_CTL_VRDA_EN                                       BIT(19) /* VRDA OP enable */
#define ADC_CTL_ADC0_BINV                                     BIT(18) /* ADC 0 channel ouput bits invert */
#define ADC_CTL_FDBUF0_IRS_SHIFT                              (16) /* FDBUF0 input resistor */
#define ADC_CTL_FDBUF0_IRS_MASK                               (0x3 << ADC_CTL_FDBUF0_IRS_SHIFT)
#define ADC_CTL_FDBUF0_IRS(x)                                 ((x) << ADC_CTL_FDBUF0_IRS_SHIFT)
#define ADC_CTL_PREAM0_PG_SHIFT                               (12) /* PREAMP0 OP feedback */
#define ADC_CTL_PREAM0_PG_MASK                                (0x7<< ADC_CTL_PREAM0_PG_SHIFT)
#define ADC_CTL_PREAM0_PG(x)                                  ((x) << ADC_CTL_PREAM0_PG_SHIFT)
#define ADC_CTL_ADC_CAPFC_EN                                  BIT(11) /* input cap to ADC channel fast charge enable */
#define ADC_CTL_FDBUF_EN                                      BIT(10) /* FD BUF OP enable for SE mode */
#define ADC_CTL_PREOP_EN                                      BIT(9) /* PREOP enable */
#define ADC_CTL_ADC_EN                                        BIT(8) /* ADC channel enable */
#define ADC_CTL_INPUT0N_EN_SHIFT                              (6) /* INPUT0N pad to ADC channel input enable */
#define ADC_CTL_INPUT0N_EN_MASK                               (0x3 << ADC_CTL_INPUT0N_EN_SHIFT)
#define ADC_CTL_INPUT0N_EN(x)                                 ((x) << ADC_CTL_INPUT0N_EN_SHIFT)
#define ADC_CTL_INPUT0P_EN_SHIFT                              (4)
#define ADC_CTL_INPUT0P_EN_MASK                               (0x3 << ADC_CTL_INPUT0P_EN_SHIFT)
#define ADC_CTL_INPUT0P_EN(x)                                 ((x) << ADC_CTL_INPUT0P_EN_SHIFT)
#define ADC_CTL_INPUT0_IN_MODE                                BIT(2) /* INPUT0 mode select: 0: differential mode; 1: single end mode */
#define ADC_CTL_INPUT0_IRS                                    BIT(0) /* INPUT0 input resistor select */

/***************************************************************************************************
 * CMU_ADCCLK
 */
#define CMU_ADCCLK_ADCDEBUGEN                                 BIT(31)
#define CMU_ADCCLK_ADCFIFOCLKEN                               BIT(16) /* ADC FIFO access clock enable */
#define CMU_ADCCLK_ADCFIREN                                   BIT(14) /* ADC FIR filter clock enable */
#define CMU_ADCCLK_ADCDMICEN                                  BIT(13) /* DMIC clock enable */
#define CMU_ADCCLK_ADCANAEN                                   BIT(12) /* ADC analog clock enable */
#define CMU_ADCCLK_ADCFIRCLKRVS                               BIT(10) /* ADC FIR clock reverse control */
#define CMU_ADCCLK_ADCDMICCLKRVS                              BIT(9) /* ADC DMIC clock reverse control */
#define CMU_ADCCLK_ADCANACLKRVS                               BIT(8) /* ADC analog clock reverse control */
#define CMU_ADCCLK_ADCCLKDIV_SHIFT                            (0)
#define CMU_ADCCLK_ADCCLKDIV_MASK                             (0x7 << CMU_ADCCLK_ADCCLKDIV_SHIFT)
#define CMU_ADCCLK_ADCCLKDIV(x)                               ((x) << CMU_ADCCLK_ADCCLKDIV_SHIFT)

/***************************************************************************************************
 * ADC FEATURES CONGIURATION
 */
#define ADC_DIGITAL_CH_GAIN_MAX                               (0x3) /* 18dB */
#define ADC_DIGITAL_PRE_GAIN_MAX                              (0x3) /* 18dB */

/**
 * @struct acts_audio_adc
 * @brief ADC controller hardware register
 */
struct acts_audio_adc {
	volatile uint32_t adc_digctl;	/* ADC digital control */
	volatile uint32_t ch_digctl;	/* channel digital control */
	volatile uint32_t fifoctl;		/* ADC fifo control */
	volatile uint32_t stat;			/* ADC stat */
	volatile uint32_t fifo0_dat;	/* ADC FIFO0 data */
	volatile uint32_t adc_ctl;		/* ADC control */
	volatile uint32_t bias;			/* ADC bias */
};

/**
 * union phy_adc_features
 * @brief The infomation from DTS to control the ADC features to enable or nor.
 */
typedef union {
	uint32_t raw;
	struct {
		uint32_t adc0_hpf_fc_high: 1; /* ADC0 HPF use high frequency range */
		uint32_t adc0_frequency : 6; /* ADC0 HFP frequency */
	} v;
} phy_adc_features;

/**
 * struct phy_adc_config_data
 * @brief The hardware related data that used by physical adc driver.
 */
struct phy_adc_config_data {
	uint32_t reg_base; /* ADC controller register base address */
	struct audio_dma_dt dma_fifo0; /* DMA resource for FIFO0 */
	uint8_t clk_id; /* ADC devclk id */
	uint8_t rst_id; /* ADC reset id */
	const struct acts_pin_config *pinmux; /* PMDTX pinmux array */
	uint8_t pinmux_size; /* PMDTX pinmux array size */
	phy_adc_features features; /* ADC features */
};

struct adc_amic_gain_setting {
	int16_t gain;
	uint8_t input_res;
	uint8_t feedback_res;
	uint8_t pre_gain;
	uint8_t digital_gain;
};

struct adc_dmic_gain_setting {
	int16_t gain;
	uint8_t pre_gain;
	uint8_t digital_gain;
};

/**
  * @struct adc_amic_aux_gain_setting
  * @brief The gain mapping table of the analog mic and aux.
  * @note By the SD suggestion, it is suitable to ajust the analog gain when below 20dB.
  * Whereas, it is the same effect to ajust the analog or digital gian when above 20dB.
  */
static const struct adc_amic_gain_setting amic_gain_mapping[] = {
	{-60, 1, 0, 0, 0},
	{-30, 1, 1, 0, 0},
	{0, 1, 2, 0, 0},
	{30, 1, 3, 0, 0},
	{60, 1, 4, 0, 0},
	{90, 1, 5, 0, 0},
	{120, 1, 6, 0, 0},
	{150, 1, 7, 0, 0},
	{155, 0, 0, 0, 0},
	{185, 0, 1, 0, 0},
	{215, 0, 2, 0, 0},
	{245, 0, 3, 0, 0},
	{275, 0, 4, 0, 0},
	{305, 0, 5, 0, 0},
	{335, 0, 6, 0, 0},
	{365, 0, 7, 0, 0},
	{425, 0, 7, 1, 0},
	{485, 0, 7, 2, 0},
	{545, 0, 7, 3, 0},
	{605, 0, 7, 3, 1},
	{665, 0, 7, 3, 2},
	{725, 0, 7, 3, 3},
};

#ifdef CONFIG_ADC_DMIC
static const struct adc_dmic_gain_setting dmic_gain_mapping[] = {
	{0, 0, 0},
	{60, 1, 0},
	{120, 2, 0},
	{180, 3, 0},
	{240, 3, 1},
	{300, 3, 2},
	{360, 3, 3},
};
#endif

typedef enum {
	ADC_AMIC = 0, /* analog mic */
	ADC_DMIC /* digital mic */
} a_adc_ch_type_e;

static uint8_t vmic_ctl_array[] = CONFIG_AUDIO_ADC_0_VMIC_CTL_ARRAY;
static uint8_t vmic_voltage_array[] = CONFIG_AUDIO_ADC_0_VMIC_VOLTAGE_ARRAY;

/* @brief  Get the ADC controller base address */
static inline struct acts_audio_adc *get_adc_reg_base(struct device *dev)
{
	const struct phy_adc_config_data *cfg = dev->config;
	return (struct acts_audio_adc *)cfg->reg_base;
}

/* @brief Dump the ADC relative registers */
static void adc_dump_register(struct device *dev)
{
	struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);

	LOG_INF("** adc contoller regster **");
	LOG_INF("          BASE: %08x", (uint32_t)adc_reg);
	LOG_INF("    ADC_DIGCTL: %08x", adc_reg->adc_digctl);
	LOG_INF("     CH_DIGCTL: %08x", adc_reg->ch_digctl);
	LOG_INF("   ADC_FIFOCTL: %08x", adc_reg->fifoctl);
	LOG_INF("      ADC_STAT: %08x", adc_reg->stat);
	LOG_INF(" ADC_FIFO0_DAT: %08x", adc_reg->fifo0_dat);
	LOG_INF("      ADC_CTL0: %08x", adc_reg->adc_ctl);
	LOG_INF("      ADC_BIAS: %08x", adc_reg->bias);
	LOG_INF("    CMU_ADCCLK: %08x", sys_read32(CMU_ADCCLK));
}

const audio_input_map_t board_audio_input_map[] = {BOARD_AUDIO_INPUT_MAP};

int board_audio_device_mapping(audio_input_map_t *input_map)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(board_audio_input_map); i++) {
		if (input_map->audio_dev == board_audio_input_map[i].audio_dev) {
			input_map->ch0_input = board_audio_input_map[i].ch0_input;
			break;
		}
	}

	if (i == ARRAY_SIZE(board_audio_input_map)) {
		printk("can not find out audio dev %d\n", input_map->audio_dev);
		return -ENOENT;
	}

	return 0;
}

/* @brief disable ADC FIFO by specified FIFO index */
static void __adc_fifo_disable(struct device *dev)
{
	struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);

	adc_reg->fifoctl &= ~ADC_FIFOCTL_ADF0RT;

	/* disable ADC FIFO0 access clock */
	sys_write32(sys_read32(CMU_ADCCLK) & ~CMU_ADCCLK_ADCFIFOCLKEN, CMU_ADCCLK);
}

/* @brief check whether the ADC FIFO is working now */
static bool __is_adc_fifo_working(struct device *dev)
{
	struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);

	if (adc_reg->fifoctl & ADC_FIFOCTL_ADF0RT)
		return true;

	return false;
}

/* @brief enable ADC FIFO by specified FIFO index */
static void __adc_fifo_enable(struct device *dev, audio_fifouse_sel_e sel)
{
	struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
	uint32_t reg = adc_reg->fifoctl & ~0x1f;

	if (FIFO_SEL_CPU == sel) {
		reg |= (ADC_FIFOCTL_ADF0FIE | ADC_FIFOCTL_ADF0RT);
	} else {
		reg |= (ADC_FIFOCTL_ADF0OS | ADC_FIFOCTL_ADF0FDE | ADC_FIFOCTL_ADF0RT);
	}

	adc_reg->fifoctl = reg;

	/* enable ADC FIFO1 to access ADC CLOCK */
	sys_write32(sys_read32(CMU_ADCCLK) | CMU_ADCCLK_ADCFIFOCLKEN, CMU_ADCCLK);
}

/* @brief ADC channel digital configuration */
static void __adc_digital_channel_cfg(struct device *dev, a_adc_ch_type_e type, bool is_en)
{
	struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
	uint32_t reg0, reg1;

	reg0 = adc_reg->adc_digctl;
	reg1 = adc_reg->ch_digctl;

	if (is_en) { /* channel enable */
		/* enable ADC FIR CLOCK */
		sys_write32(sys_read32(CMU_ADCCLK) | CMU_ADCCLK_ADCFIREN, CMU_ADCCLK);
		if (ADC_AMIC == type) {
			/* enable ADC analog CLOCK */
			sys_write32(sys_read32(CMU_ADCCLK) | CMU_ADCCLK_ADCANAEN, CMU_ADCCLK);
			reg1 &= ~CH_DIGCTL_MIC_SEL; /* select AMIC */
		} else if (ADC_DMIC == type) {
			/* enable ADC DMIC CLOCK */
			sys_write32(sys_read32(CMU_ADCCLK) | CMU_ADCCLK_ADCDMICEN, CMU_ADCCLK);
			reg1 |= CH_DIGCTL_MIC_SEL; /* select DMIC */
		}

		/* channel digital enable */
		reg0 |= ADC_DIGCTL_DIG_EN;
	} else { /* channel disable */
		sys_write32(sys_read32(CMU_ADCCLK) & ~CMU_ADCCLK_ADCDMICEN, CMU_ADCCLK);
		sys_write32(sys_read32(CMU_ADCCLK) & ~CMU_ADCCLK_ADCANAEN, CMU_ADCCLK);
		sys_write32(sys_read32(CMU_ADCCLK) & ~CMU_ADCCLK_ADCFIREN, CMU_ADCCLK);

		/* channel digital disable */
		reg0 &= ~ADC_DIGCTL_DIG_EN;
	}

	adc_reg->adc_digctl = reg0;
	adc_reg->ch_digctl = reg1;
}

/* @brief ADC HPF(High Pass Filter) enable */
static void __adc_hpf_enable(struct device *dev)
{
	struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
	const struct phy_adc_config_data *cfg = dev->config;
	uint32_t reg;
	bool is_high = PHY_DEV_FEATURE(adc0_hpf_fc_high);;
	uint8_t frequency = PHY_DEV_FEATURE(adc0_frequency);

	/* clear HPF_S and HPF_N */
	reg = adc_reg->ch_digctl & ~(0x7f << CH_DIGCTL_HPF_N_SHIFT);
	reg |= CH_DIGCTL_HPF_N(frequency);

	if (is_high)
		reg |= CH_DIGCTL_HPF_S;

	/* enable HPF */
	reg |= CH_DIGCTL_HPF_EN;

	adc_reg->ch_digctl = reg;

	k_sleep(K_MSEC(10));
	reg &= ~CH_DIGCTL_HPF_S;
	adc_reg->ch_digctl = reg;
}

/* @brief Translate the AMIC/AUX gain from dB fromat to hardware register value */
static int adc_amic_gain_translate(int16_t gain, uint8_t *input_res,
			uint8_t *fd_res, uint8_t *pre_gain, uint8_t *dig_gain)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(amic_gain_mapping); i++) {
		if (gain <= amic_gain_mapping[i].gain) {
			*input_res = amic_gain_mapping[i].input_res;
			*fd_res = amic_gain_mapping[i].feedback_res;
			*pre_gain = amic_gain_mapping[i].pre_gain;
			*dig_gain = amic_gain_mapping[i].digital_gain;
			LOG_INF("gain:%d map [%d %d %d %d]",
				gain, *input_res, *fd_res, *pre_gain, *dig_gain);
			break;
		}
	}

	if (i == ARRAY_SIZE(amic_gain_mapping)) {
		LOG_ERR("can not find out gain map %d", gain);
		return -ENOENT;
	}

	return 0;
}

#ifdef CONFIG_ADC_DMIC
/* @brief Translate the DMIC gain from dB fromat to hardware register value */
static int adc_dmic_gain_translate(int16_t gain, uint8_t *pre_gain, uint8_t *dig_gain)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(dmic_gain_mapping); i++) {
		if (gain <= dmic_gain_mapping[i].gain) {
			*pre_gain = dmic_gain_mapping[i].pre_gain;
			*dig_gain = dmic_gain_mapping[i].digital_gain;
			LOG_DBG("gain:%d map [%d %d]",
				gain, *pre_gain, *dig_gain);
			break;
		}
	}

	if (i == ARRAY_SIZE(dmic_gain_mapping)) {
		LOG_ERR("can not find out gain map %d", gain);
		return -ENOENT;
	}

	return 0;
}
#endif

/* @brief ADC digital pre gain setting */
static void __adc_digital_pre_gain_set(struct device *dev, uint8_t gain)
{
	struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
	uint32_t reg;

	if (gain > ADC_DIGITAL_PRE_GAIN_MAX)
		gain = ADC_DIGITAL_PRE_GAIN_MAX;

	reg = adc_reg->adc_digctl & ~ADC_DIGCTL_PRE_GAIN_MASK;
	reg |= ADC_DIGCTL_PRE_GAIN(gain);

	adc_reg->adc_digctl = reg;

	LOG_DBG("digital pre gain:0x%x", adc_reg->adc_digctl);
}

/* @brief ADC channel digital gain setting */
static void __adc_digital_gain_set(struct device *dev, uint8_t gain)
{
	struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
	uint32_t reg;

	if (gain > ADC_DIGITAL_CH_GAIN_MAX)
		gain = ADC_DIGITAL_CH_GAIN_MAX;

	reg = adc_reg->ch_digctl & ~CH_DIGCTL_ADCGC_MASK;
	reg |= CH_DIGCTL_ADCGC(gain);

	adc_reg->ch_digctl = reg;

	LOG_DBG("ch digital gain:0x%x", adc_reg->ch_digctl);
}

/* @brief ADC input analog gain setting */
static int __adc_input_gain_set(struct device *dev, uint8_t input_res, uint8_t feedback_res, bool is_fd)
{
	struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
	uint32_t reg = adc_reg->adc_ctl;

	/* clear FDBUF0 input resistor / OP feedback resistor / INPUT0 input resistor */
	reg &= ~(ADC_CTL_FDBUF0_IRS_MASK | ADC_CTL_PREAM0_PG_MASK | ADC_CTL_INPUT0_IRS);

	/* full differential mode no need to set FDBUF0 input resistor */
	if (!is_fd)
		reg |= (ADC_CTL_FDBUF0_IRS(input_res) | (0x2 << ADC_CTL_FDBUF0_IRS_SHIFT));

	if (input_res)
		reg |= ADC_CTL_INPUT0_IRS;
	else
		reg &= ~ADC_CTL_INPUT0_IRS;

	reg |= ADC_CTL_PREAM0_PG(feedback_res);

	adc_reg->adc_ctl = reg;

	return 0;
}


/* @brief ADC INPUT configuration */
static int adc_input_config(struct device *dev, uint16_t input_dev)
{
	struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
	audio_input_map_t adc_input_map = {.audio_dev = input_dev,};
	uint8_t ch0_input;
	uint32_t reg;

	if (board_audio_device_mapping(&adc_input_map))
		return -ENOENT;

	LOG_INF("ADC channel {dev:0x%x, [0x%x]}",
			input_dev, adc_input_map.ch0_input);

	ch0_input = adc_input_map.ch0_input;
	reg = adc_reg->adc_ctl;

	reg &= ~ADC_CTL_INPUT0_IN_MODE;

	/* disable INPUT0P_EN / INPUT0N_EN / ADC_EN / PREOP_EN / FBUF_EN */
	reg &= ~(0x7f << ADC_CTL_INPUT0P_EN_SHIFT);

	reg |= ADC_CTL_VRDA_EN; /* VARD OP enable */
	reg |= ADC_CTL_BIASEN; /* BIAS enable */
	reg |= ADC_CTL_VREF_EN; /* VREF enable */

	reg |= ADC_CTL_VREF_FU; /* VREF fast up enable */
	reg |= ADC_CTL_ADC_CAPFC_EN; /* enable ADC input-cap fast charge */
	reg &= ~ADC_CTL_PREAM0_PG_MASK;

	reg |= ADC_CTL_FDBUF_EN; /* FD BUF OP enable for SE mode */
	reg |= ADC_CTL_PREOP_EN; /* PREOP enable */
	reg |= ADC_CTL_ADC_EN; /* ADC channel enable */

	reg |= ADC_CTL_INPUT0_IN_MODE; /* default SE mode */

	if (ADC_CH_INPUT0NP_DIFF == ch0_input) {
		reg &= ~ADC_CTL_INPUT0_IN_MODE; /* differential input mode */
		reg &= ~ADC_CTL_FDBUF_EN; /* disable FDBUF0 when differential mode */
		reg &= ~ADC_CTL_FDBUF0_IRS_MASK; /* not connect FDBUF0 input resistor when differential mode */
		reg |= (ADC_CTL_INPUT0N_EN(3) | ADC_CTL_INPUT0P_EN(3)); /* enable INPUT0N/INPUT0P channels */
	} else {
		reg |= ADC_CTL_INPUT0P_EN(3); /* enable INPUT0P channels */
	}

	adc_reg->adc_ctl = reg;

	k_sleep(K_MSEC(50)); /* 1uF+62.5K,RC==62.5ms */

	reg &= ~ADC_CTL_VREF_FU;
	reg &= ~ADC_CTL_ADC_CAPFC_EN;
	adc_reg->adc_ctl = reg;

	return 0;
}

/* @brief ADC VMIC control */
static void __adc_vmic_ctl_enable(struct device *dev)
{
	struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
	uint32_t reg = adc_reg->adc_ctl;

	reg &= ~ADC_CTL_VMIC_VOL_MASK;
	reg |= ADC_CTL_VMIC_VOL(vmic_voltage_array[0]);

	reg &= ~ADC_CTL_VMIC_EN_MASK;
	reg |= ADC_CTL_VMIC_EN(vmic_ctl_array[0]);

	adc_reg->adc_ctl = reg;
}

/* @brief ADC VMIC control */
static void __adc_vmic_ctl_disable(struct device *dev)
{
	struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
	uint32_t reg = adc_reg->adc_ctl;

	/* disable VMIC OP */
	reg &= ~ADC_CTL_VMIC_EN_MASK;

	adc_reg->adc_ctl = reg;
}

/* @brief ADC gain configuration */
static int adc_gain_config(struct device *dev, uint16_t input_dev, adc_gain *gain)
{
	uint8_t ch_input;
	uint8_t pre_gain = 0, dig_gain = 0;
	int16_t ch_gain = gain->ch_gain[0];

	audio_input_map_t adc_input_map = {.audio_dev = input_dev,};

	if (board_audio_device_mapping(&adc_input_map))
		return -ENOENT;

	ch_input = adc_input_map.ch0_input;

#ifdef CONFIG_ADC_DMIC
	if (ADC_CH_DMIC != ch_input) {
		LOG_ERR("DMIC invalid input:%d", ch_input);
		return -EINVAL;
	}

	if (ADC_GAIN_INVALID != (uint16_t)ch_gain) {
		if (adc_dmic_gain_translate(ch_gain, &pre_gain, &dig_gain)) {
			LOG_ERR("invalid gain:%d", ch_gain);
			return -EINVAL;
		}
	}

	__adc_digital_pre_gain_set(dev, pre_gain);
	__adc_digital_gain_set(dev, dig_gain);

	return 0;
#endif
	bool is_fd = false;

	if (ADC_CH_INPUT0NP_DIFF != ch_input)
		ch_gain += 60;
	else
		is_fd = true;

	if ((ADC_CH_INPUT0P != ch_input) && (ADC_CH_INPUT0NP_DIFF != ch_input)) {
		LOG_ERR("AMIC invalid input:%d", ch_input);
		return -EINVAL;
	}

	uint8_t input_res, fd_res;
	if (adc_amic_gain_translate(ch_gain, &input_res, &fd_res, &pre_gain, &dig_gain)) {
		LOG_ERR("invalid gain:%d", ch_gain);
		return -EINVAL;
	}

	__adc_digital_pre_gain_set(dev, pre_gain);
	__adc_digital_gain_set(dev, dig_gain);
	__adc_input_gain_set(dev, input_res, fd_res, is_fd);

	return 0;
}

/* @brief ADC sample rate config */
static int adc_sample_rate_set(struct device *dev, audio_sr_sel_e sr_khz)
{
	uint8_t div;
	uint32_t reg = sys_read32(CMU_ADCCLK) & ~CMU_ADCCLK_ADCCLKDIV_MASK;

	if ((sr_khz != SAMPLE_RATE_32KHZ)
		&& (sr_khz != SAMPLE_RATE_8KHZ)
		&& (sr_khz != SAMPLE_RATE_16KHZ)) {
		LOG_ERR("unsupport sr:%d", sr_khz);
		return -ENOTSUP;
	}

	/* ADC main clock source is 24MHz (250FS). */
	if (SAMPLE_RATE_8KHZ == sr_khz)
		div = 6;
	else if (SAMPLE_RATE_16KHZ == sr_khz)
		div = 4;
	else
		div = 2;

	reg |= CMU_ADCCLK_ADCCLKDIV(div);

	sys_write32(reg, CMU_ADCCLK);

	return 0;
}

/* @brief Get the sample rate from the ADC config */
static int adc_sample_rate_get(struct device *dev)
{
	ARG_UNUSED(dev);
	uint8_t sr;
	uint32_t div = (sys_read32(CMU_ADCCLK) & CMU_ADCCLK_ADCCLKDIV_MASK)
					>> CMU_ADCCLK_ADCCLKDIV_SHIFT;

	if (6 == div) {
		sr = SAMPLE_RATE_8KHZ;
	} else if (4 == div) {
		sr = SAMPLE_RATE_16KHZ;
	} else if (2 == div) {
		sr = SAMPLE_RATE_32KHZ;
	} else {
		LOG_ERR("invalid divisor:%d", div);
		return -EINVAL;
	}

	return sr;
}

/* @brief Get the ADC DMA information */
static int adc_get_dma_info(struct device *dev, struct audio_in_dma_info *info)
{
	const struct phy_adc_config_data *cfg = dev->config;

	/* use ADC FIFO0 */
	info->dma_info.dma_chan = cfg->dma_fifo0.dma_chan;
	info->dma_info.dma_dev_name = cfg->dma_fifo0.dma_dev_name;
	info->dma_info.dma_id = cfg->dma_fifo0.dma_id;

	return 0;
}

/* @brief  Enable the ADC digital function */
static int adc_digital_enable(struct device *dev, uint16_t input_dev)
{
	audio_input_map_t adc_input_map = {.audio_dev = input_dev,};
	uint8_t type = ADC_AMIC;

	if (board_audio_device_mapping(&adc_input_map))
		return -ENOENT;

	if (adc_input_map.ch0_input == ADC_CH_DMIC)
		type = ADC_DMIC;

	__adc_digital_channel_cfg(dev, type, true);

	__adc_hpf_enable(dev);

	return 0;
}

/* @brief  Disable the ADC digital function */
static void adc_digital_disable(struct device *dev)
{
	__adc_digital_channel_cfg(dev, 0, false);
}

/* @brief ADC physical level enable procedure */
static int phy_adc_enable(struct device *dev, void *param)
{
	const struct phy_adc_config_data *cfg = dev->config;
	ain_param_t *in_param = (ain_param_t *)param;
	adc_setting_t *adc_setting = in_param->adc_setting;
	int ret;

	if ((!in_param) || (!adc_setting)
		|| (!in_param->sample_rate)) {
		LOG_ERR("Invalid parameters");
		return -EINVAL;
	}

	if (in_param->channel_type != AUDIO_CHANNEL_ADC) {
		LOG_ERR("Invalid channel type %d", in_param->channel_type);
		return -EINVAL;
	}

	if (__is_adc_fifo_working(dev)) {
		LOG_ERR("ADC fifo now is using");
		return -EBUSY;
	}

    /* Config the adc pin state */
	acts_pinmux_setup_pins(cfg->pinmux, cfg->pinmux_size);

	/* enable adc clock */
	acts_clock_peripheral_enable(cfg->clk_id);

	__adc_vmic_ctl_enable(dev);

	/* audio_pll and adc clock setting */
	if (adc_sample_rate_set(dev, in_param->sample_rate)) {
		LOG_ERR("Failed to config sample rate %d", in_param->sample_rate);
		return -ESRCH;
	}

	/* ADC FIFO enable */
	__adc_fifo_enable(dev, FIFO_SEL_DMA);

	/* ADC analog input enable */
	ret = adc_input_config(dev, adc_setting->device);
	if (ret) {
		LOG_ERR("ADC input config error %d", ret);
		goto err;
	}

	/* ADC digital enable */
	ret = adc_digital_enable(dev, adc_setting->device);
	if (ret) {
		LOG_ERR("ADC digital enable error %d", ret);
		goto err;
	}

	/* ADC gain setting */
	ret = adc_gain_config(dev, adc_setting->device, &adc_setting->gain);
	if (ret) {
		LOG_ERR("ADC gain config error %d", ret);
		goto err;
	}

	return ret;

err:
	__adc_fifo_disable(dev);
	return ret;
}

/* @brief ADC physical level disable procedure */
static int phy_adc_disable(struct device *dev, void *param)
{
	struct acts_audio_adc *adc_reg = get_adc_reg_base(dev);
	const struct phy_adc_config_data *cfg = dev->config;
	uint16_t input_dev = *(uint16_t *)param;

	LOG_INF("disable input device:0x%x", input_dev);

	/* DAC FIFO reset */
	__adc_fifo_disable(dev);

	adc_digital_disable(dev);
	adc_reg->adc_ctl = 0;
	acts_clock_peripheral_disable(cfg->clk_id);

	__adc_vmic_ctl_disable(dev);

	return 0;
}

/* @brief ADC ioctl commands */
static int phy_adc_ioctl(struct device *dev, uint32_t cmd, void *param)
{
	int ret = 0;

	switch (cmd) {
	case PHY_CMD_DUMP_REGS:
	{
		adc_dump_register(dev);
		break;
	}
	case PHY_CMD_GET_AIN_DMA_INFO:
	{
		ret = adc_get_dma_info(dev, (struct audio_in_dma_info *)param);
		break;
	}
	case PHY_CMD_ADC_DIGITAL_ENABLE:
	{
		break;
	}
	case PHY_CMD_ADC_GAIN_CONFIG:
	{
		adc_setting_t *setting = (adc_setting_t *)param;
		ret = adc_gain_config(dev, setting->device, &setting->gain);
		break;
	}
	case AIN_CMD_GET_SAMPLERATE:
	{
		ret = adc_sample_rate_get(dev);
		if (ret < 0) {
			LOG_ERR("Failed to get ADC sample rate err=%d", ret);
			return ret;
		}
		*(audio_sr_sel_e *)param = (audio_sr_sel_e)ret;
		ret = 0;
		break;
	}
	case AIN_CMD_SET_SAMPLERATE:
	{
		audio_sr_sel_e val = *(audio_sr_sel_e *)param;
		ret = adc_sample_rate_set(dev, val);
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

const struct phy_audio_driver_api phy_adc_drv_api = {
	.audio_enable = phy_adc_enable,
	.audio_disable = phy_adc_disable,
	.audio_ioctl = phy_adc_ioctl
};

/* dump dac device tree infomation */
static void __adc_dt_dump_info(const struct phy_adc_config_data *cfg)
{
#if (PHY_DEV_SHOW_DT_INFO == 1)
	LOG_INF("**     ADC BASIC INFO     **");
	LOG_INF("     BASE: %08x", cfg->reg_base);
	LOG_INF("   CLK-ID: %08x", cfg->clk_id);
	LOG_INF("   RST-ID: %08x", cfg->rst_id);
	LOG_INF("DMA0-NAME: %s", cfg->dma_fifo0.dma_dev_name);
	LOG_INF("  DMA0-ID: %08x", cfg->dma_fifo0.dma_id);
	LOG_INF("  DMA0-CH: %08x", cfg->dma_fifo0.dma_chan);

	LOG_INF("** 	ADC FEATURES	 **");
	LOG_INF("ADC0-HPF-FC-HIGH: %d", PHY_DEV_FEATURE(adc0_hpf_fc_high));
	LOG_INF("  ADC0-FREQUENCY: %d", PHY_DEV_FEATURE(adc0_frequency));
	LOG_INF("     VMIC-CTL-EN: <%d>", vmic_ctl_array[0]);
	LOG_INF("    VMIC-CTL-VOL: <%d>", vmic_voltage_array[0]);
#endif
}

static int phy_adc_init(const struct device *dev)
{
	const struct phy_adc_config_data *cfg = dev->config;

	__adc_dt_dump_info(cfg);

	/* reset ADC controller */
	acts_reset_peripheral(cfg->rst_id);

	printk("ADC init successfully\n");

	return 0;
}

static const struct acts_pin_config adc_pins_state[] = {
	CONFIG_AUDIO_ADC_0_MFP
};

/* physical adc config data */
static const struct phy_adc_config_data phy_adc_config_data0 = {
	.reg_base = AUDIO_ADC_REG_BASE,
	AUDIO_DMA_FIFO_DEF(ADC, 0),
	.clk_id = CLOCK_ID_ADC,
	.rst_id = RESET_ID_ADC,
	.pinmux = adc_pins_state,
	.pinmux_size = ARRAY_SIZE(adc_pins_state),

	PHY_DEV_FEATURE_DEF(adc0_hpf_fc_high) = CONFIG_AUDIO_ADC_0_CH0_HPF_FC_HIGH,
	PHY_DEV_FEATURE_DEF(adc0_frequency) = CONFIG_AUDIO_ADC_0_CH0_FREQUENCY,
};

DEVICE_DEFINE(adc0, CONFIG_AUDIO_ADC_0_NAME, phy_adc_init, device_pm_control_nop,
	    NULL, &phy_adc_config_data0,
	    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS, &phy_adc_drv_api);

