/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio Driver Shell implementation
 */
#include <shell/shell.h>
#include <string.h>
#include <stdlib.h>
#include <sys/byteorder.h>
#include "../phy_audio_common.h"
#include <drivers/audio/audio_out.h>
#include <drivers/audio/audio_in.h>
#include <board_cfg.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(ashell, CONFIG_LOG_DEFAULT_LEVEL);

#define SHELL_ADC_USE_DIFFERENT_BUFFER
//#define DAC_PCMBUF_DEBUG
#define SHELL_ADC_USE_STATIC_BUFFER

#define SHELL_CMD_CHL_INDEX_KEY "chl_id="
#define SHELL_CMD_CHL_TYPE_KEY "chl_type="
#define SHELL_CMD_CHL_WIDTH_KEY "chl_width="
#define SHELL_CMD_SAMPLE_RATE_KEY "sr="
#define SHELL_CMD_FIFO_TYPE_KEY "fifo_type="
#define SHELL_CMD_LEFT_VOL_KEY "left_vol="
#define SHELL_CMD_RIGHT_VOL_KEY "right_vol="
#define SHELL_CMD_RELOAD_KEY "reload_en"
#define SHELL_CMD_CH0_GAIN_KEY "ch0_gain="
#define SHELL_CMD_CH1_GAIN_KEY "ch1_gain="
#define SHELL_CMD_CH2_GAIN_KEY "ch2_gain="
#define SHELL_CMD_CH3_GAIN_KEY "ch3_gain="
#define SHELL_CMD_INPUT_DEV_KEY "input_dev="
#define SHELL_CMD_DUMP_LEN_KEY "dump_len="
#define SHELL_CMD_IOCTL_PARAM_KEY "param="
#define SHELL_CMD_IOCTL_CMD_KEY "cmd="
#define SHELL_CMD_IOCTL_CMD_SPDIF_CSL_KEY "csl="
#define SHELL_CMD_IOCTL_CMD_SPDIF_CSH_KEY "csh="
#define SHELL_CMD_IOCTL_CMD_BIND_ID_KEY "bind_id="
#define SHELL_CMD_ADDA_KEY "adda_en"
#define SHELL_CMD_PLAY_MONO_KEY "mono_en"
#define SHELL_CMD_PLAY_PERF_DETAIL_KEY "perf_detail"
#define SHELL_CMD_HW_TRIGGER_SRC_KEY "trigger_src="

static const uint16_t L1khz_R2khz_Dat_Sine[] = {
	0x0000, 0x0000,
	0x10B5, 0x2120,
	0x2120, 0x3FFF,
	0x30FB, 0x5A82,
	0x3FFF, 0x6ED9,
	0x4DEB, 0x7BA3,
	0x5A82, 0x7FFF,
	0x658C, 0x7BA3,
	0x6ED9, 0x6ED9,
	0x7641, 0x5A82,
	0x7BA3, 0x3FFF,
	0x7EE7, 0x2120,
	0x7FFF, 0x0000,
	0x7EE7, 0xDEDF,
	0x7BA3, 0xC000,
	0x7641, 0xA57D,
	0x6ED9, 0x9126,
	0x658C, 0x845C,
	0x5A82, 0x8000,
	0x4DEB, 0x845C,
	0x3FFF, 0x9126,
	0x30FB, 0xA57D,
	0x2120, 0xC000,
	0x10B5, 0xDEDF,
	0x0000, 0x0000,
	0xEF4A, 0x2120,
	0xDEDF, 0x3FFF,
	0xCF04, 0x5A82,
	0xC000, 0x6ED9,
	0xB214, 0x7BA3,
	0xA57D, 0x7FFF,
	0x9A73, 0x7BA3,
	0x9126, 0x6ED9,
	0x89BE, 0x5A82,
	0x845C, 0x3FFF,
	0x8118, 0x2120,
	0x8000, 0x0000,
	0x8118, 0xDEDF,
	0x845C, 0xC000,
	0x89BE, 0xA57D,
	0x9126, 0x9126,
	0x9A73, 0x845C,
	0xA57D, 0x8000,
	0xB214, 0x845C,
	0xC000, 0x9126,
	0xCF04, 0xA57D,
	0xDEDF, 0xC000,
	0xEF4A, 0xDEDF
};

static const uint32_t L1khz_R2khz_Dat_Sine_24bits[]= {
	0x00000000,0x00000000,
	0x10B51400,0x2120FB00,
	0x2120FB00,0x3FFFFF00,
	0x30FBC400,0x5A827800,
	0x3FFFFF00,0x6ED9EA00,
	0x4DEBE400,0x7BA37400,
	0x5A827800,0x7FFFFF00,
	0x658C9900,0x7BA37400,
	0x6ED9EA00,0x6ED9EA00,
	0x7641AE00,0x5A827900,
	0x7BA37400,0x3FFFFF00,
	0x7EE7A900,0x2120FB00,
	0x7FFFFF00,0x00000000,
	0x7EE7A900,0xDEDF0600,
	0x7BA37400,0xC0000100,
	0x7641AE00,0xA57D8800,
	0x6ED9EA00,0x91261600,
	0x658C9900,0x845C8D00,
	0x5A827900,0x80000200,
	0x4DEBE400,0x845C8C00,
	0x3FFFFF00,0x91261500,
	0x30FBC500,0xA57D8700,
	0x2120FB00,0xC0000000,
	0x10B51500,0xDEDF0400,
	0x00000000,0x00000000,
	0xEF4AEC00,0x2120FB00,
	0xDEDF0600,0x3FFFFF00,
	0xCF043C00,0x5A827800,
	0xC0000100,0x6ED9EA00,
	0xB2141D00,0x7BA37400,
	0xA57D8800,0x7FFFFF00,
	0x9A736700,0x7BA37400,
	0x91261600,0x6ED9EA00,
	0x89BE5200,0x5A827900,
	0x845C8D00,0x3FFFFF00,
	0x81185700,0x2120FB00,
	0x80000200,0x00000000,
	0x81185700,0xDEDF0600,
	0x845C8C00,0xC0000100,
	0x89BE5200,0xA57D8800,
	0x91261500,0x91261600,
	0x9A736700,0x845C8D00,
	0xA57D8700,0x80000200,
	0xB2141B00,0x845C8C00,
	0xC0000000,0x91261500,
	0xCF043B00,0xA57D8700,
	0xDEDF0400,0xC0000000,
	0xEF4AEB00,0xDEDF0400
};

/**
 * struct audio_play_attr
 * @brief Audio play attribute.
 */
struct audio_play_attr {
	uint8_t chl_type; /* channel type */
	uint8_t chl_width; /* channel width */
	uint8_t sr; /* sample rate */
	uint8_t fifo_type; /* fifo type */
	int32_t left_vol; /* left volume */
	int32_t right_vol; /* right volume */
	uint8_t id; /* channel index */
	uint8_t hw_trigger_src; /* hw irq sources to trigger DAC start */
	uint8_t reload_en : 1; /* reload enable flag */
	uint8_t adda_en : 1; /* ADC => DAC enable */
	uint8_t mono_en : 1; /* mono mode enable  */
	uint8_t perf_detail_en : 1; /* performance detail information enable */
	uint8_t reserved : 4;
};

/**
 * struct audio_play_objects
 * @brief The resource that belongs to audio player.
 */
struct audio_play_object {
#define AUDIO_PLAY_START_FLAG			(1 << 0)
#define AUDIO_PLAY_SAMPLES_CNT_EN_FLAG	(1 << 1)
#define AUDIO_PLAY_SDM_CNT_EN_FLAG		(1 << 2)
	struct device *aout_dev;
	uint8_t *audio_buffer;
	uint32_t audio_buffer_size;
	void *aout_handle;
	uint32_t play_total_size;
	uint32_t flags;
#ifdef DAC_PCMBUF_DEBUG
	uint32_t pcmbuf_avail_origin;
	uint32_t pcmbuf_avail;
#endif
	struct audio_play_attr attribute;
};

/* @brief audio play handler */
typedef struct audio_play_object *audio_play_handle;

/* audio player instance */
#define AUDIO_PLAY_MAX_HANLDER (2)
static audio_play_handle audio_player[AUDIO_PLAY_MAX_HANLDER];

/* statistics audio play total size */
static uint64_t audio_play_total_size;

/**
 * struct audio_record_attr
 * @brief Audio record attribute.
 */
struct audio_record_attr {
	uint8_t chl_type; /* channel type */
	uint8_t chl_width; /* channel width */
    uint8_t sr; /* sample rate */
	uint8_t dump_len; /* the length of data that per-second dump */
	uint8_t id; /* channel id */
    int16_t ch_gain[ADC_CH_NUM_MAX]; /* ADC left gain */
	uint16_t input_dev; /* input audio device */
	uint8_t hw_trigger_src; /* hw irq sources to trigger ADC start */
};

/**
 * struct audio_record_object
 * @brief The resource that belongs to audio recorder.
 */
struct audio_record_object {
    struct device *ain_dev;
    uint8_t *audio_buffer;
    uint32_t audio_buffer_size;
    void *ain_handle;
    struct audio_record_attr attribute;
};

/* @brief audio record handler */
typedef struct audio_record_object *audio_record_handle;

/* audio recorder instance */
#define AUDIO_RECORD_MAX_HANLDER (2)
static audio_record_handle audio_recorder[AUDIO_RECORD_MAX_HANLDER];

#define SHELL_AUDIO_BUFFER_SIZE (1024)

#ifdef SHELL_ADC_USE_STATIC_BUFFER
static uint8_t shell_audio_buffer[SHELL_AUDIO_BUFFER_SIZE];
#else
extern char __share_ram_start[];
static uint8_t *shell_audio_buffer = __share_ram_start;
#endif

#ifdef SHELL_ADC_USE_DIFFERENT_BUFFER
#ifdef SHELL_ADC_USE_STATIC_BUFFER
static uint8_t shell_audio_buffer1[SHELL_AUDIO_BUFFER_SIZE];
#else
static uint8_t *shell_audio_buffer1 = __share_ram_start + SHELL_AUDIO_BUFFER_SIZE;
#endif
#endif

/* @brief find the value according to the key */
static int cmd_search_value_by_key(size_t argc, char **argv,
				const char *key, uint32_t *value)
{
	int i;
	char *p;

	for (i = 1; i < argc; i++) {
		if (strstr(argv[i], key)) {
			LOG_DBG("found string:%s", argv[i]);
			/* found key */
			p = strchr(argv[i], '=');
			if (!p) {
				*value = 1;
			} else {
				LOG_DBG("found key value:%s", p);
				*value = strtoul(++p, NULL, 0);
			}
			break;
		}
	}

	if (i == argc)
		return -ENXIO;

	return 0;
}

/* @brief prepare audio PCM data */
static void audio_prepare_data(audio_play_handle hdl, uint8_t *buf, uint32_t len, uint8_t width)
{
	uint32_t fill_len;
	uint16_t count;
	uint16_t i, j;

	if (hdl)
		hdl->audio_buffer = buf;

	if (width == CHANNEL_WIDTH_16BITS) {
		fill_len = len / sizeof(L1khz_R2khz_Dat_Sine) * sizeof(L1khz_R2khz_Dat_Sine);
		if (hdl)
			hdl->audio_buffer_size = fill_len;
		count = fill_len / sizeof(L1khz_R2khz_Dat_Sine);
		const uint16_t *src = L1khz_R2khz_Dat_Sine;
		uint16_t *dst = (uint16_t *)buf;
		for (i = 0; i < count; i++) {
			for (j = 0; j < sizeof(L1khz_R2khz_Dat_Sine)/2; j++)
				*dst++ = src[j];
		}
	} else {
		/* 32bits width */
		fill_len = len / sizeof(L1khz_R2khz_Dat_Sine_24bits) * sizeof(L1khz_R2khz_Dat_Sine_24bits);
		if (hdl)
			hdl->audio_buffer_size = fill_len;
		count = fill_len / sizeof(L1khz_R2khz_Dat_Sine_24bits);
		const uint32_t *src = L1khz_R2khz_Dat_Sine_24bits;
		uint32_t *dst = (uint32_t *)buf;
		for (i = 0; i < count; i++) {
			for (j = 0; j < sizeof(L1khz_R2khz_Dat_Sine)/4; j++) {
				*dst++ = src[j];
			}
		}
	}
}

/* @brief audio i2stx SRD callback */
static int audio_i2stx_srd_cb(void *cb_data, uint32_t cmd, void *param)
{
	switch (cmd) {
		case I2STX_SRD_FS_CHANGE:
			LOG_INF("New sample rate %d", *(audio_sr_sel_e *)param);
			break;
		case I2STX_SRD_WL_CHANGE:
			LOG_INF("New width length %d", *(audio_i2s_srd_wl_e *)param);
			break;
		case I2STX_SRD_TIMEOUT:
			LOG_INF("SRD timeout");
			break;
		default:
			LOG_ERR("Error command %d", cmd);
			return -EFAULT;
	}

	return 0;
}

/* @brief dump the detail informatioin of audio play  */
static void audio_play_perf_detail(void)
{
	uint8_t i;
	int ret;
	uint32_t count = 0, sdm_count = 0, sdm_stable_count = 0;

	for (i = 0; i < AUDIO_PLAY_MAX_HANLDER; i++) {
		if (audio_player[i]->flags & AUDIO_PLAY_START_FLAG) {
			LOG_INF("** play[%d] performance %dB/s **",
					i, audio_player[i]->play_total_size);

			audio_play_total_size += audio_player[i]->play_total_size;

			audio_player[i]->play_total_size = 0;

			if (audio_player[i]->flags & AUDIO_PLAY_SAMPLES_CNT_EN_FLAG) {
				ret = audio_out_control(audio_player[i]->aout_dev,
					audio_player[i]->aout_handle, AOUT_CMD_GET_SAMPLE_CNT, &count);
			}

			if (audio_player[i]->flags & AUDIO_PLAY_SDM_CNT_EN_FLAG) {
				ret = audio_out_control(audio_player[i]->aout_dev,
					audio_player[i]->aout_handle, AOUT_CMD_GET_DAC_SDM_SAMPLE_CNT, &sdm_count);
				ret = audio_out_control(audio_player[i]->aout_dev,
					audio_player[i]->aout_handle, AOUT_CMD_GET_DAC_SDM_STABLE_SAMPLE_CNT, &sdm_stable_count);
			}

			if ((audio_player[i]->flags & AUDIO_PLAY_SAMPLES_CNT_EN_FLAG)
				|| audio_player[i]->flags & AUDIO_PLAY_SDM_CNT_EN_FLAG)
				LOG_INF("   play[%d] counter samples:%d sdm:%d sdm lock:%d ", i, count, sdm_count, sdm_stable_count);

#ifdef DAC_PCMBUF_DEBUG
			LOG_INF("   play[%d] pcmbuf avail origin 0x%x   ", i, audio_player[i]->pcmbuf_avail_origin);
			LOG_INF("   play[%d] pcmbuf avail 0x%x   ", i, audio_player[i]->pcmbuf_avail);
#endif
		}
	}

	LOG_INF("   play total size %lld   ", audio_play_total_size);
}

/* @brief The callback from the low level audio play device for the request to write more data */
static int audio_play_write_data_cb(void *handle, uint32_t reason)
{
	audio_play_handle hdl = (audio_play_handle)handle;
	uint32_t len, delta;
	uint8_t *buf = NULL;
	int ret;
	static uint32_t play_timestamp = 0;

	LOG_DBG("handle %p, cb reason %d", handle, reason);

	if (hdl->attribute.reload_en) {
		len = hdl->audio_buffer_size / 2;
		if (AOUT_DMA_IRQ_HF == reason) {
			buf = hdl->audio_buffer;
			LOG_DBG("DMA IRQ HF");
		} else if (AOUT_DMA_IRQ_TC == reason) {
			buf = hdl->audio_buffer + len;
			LOG_DBG("DMA IRQ TC");
		} else {
			LOG_ERR("invalid reason %d", reason);
			return -1;
		}
	} else {
		buf = hdl->audio_buffer;
		len = hdl->audio_buffer_size;
	}

#ifdef CONFIG_AUDIO_OUT_DAC_PCMBUF_SUPPORT
	if ((AOUT_FIFO_DAC0 == hdl->attribute.fifo_type)
		|| (AOUT_FIFO_DAC1 == hdl->attribute.fifo_type))
	if (AOUT_DMA_IRQ_TC == reason) {
		LOG_ERR("*");
	}
#endif

	delta = (uint32_t)k_cyc_to_ns_floor64(k_cycle_get_32() - play_timestamp);
	if (hdl->flags & AUDIO_PLAY_START_FLAG)
		hdl->play_total_size += len;

	if (!hdl->attribute.reload_en) {
#ifdef DAC_PCMBUF_DEBUG
		audio_out_control(hdl->aout_dev,
						hdl->aout_handle, AOUT_CMD_GET_FIFO_AVAILABLE_LEN,
						&hdl->pcmbuf_avail_origin);
#endif
		ret = audio_out_write(hdl->aout_dev, hdl->aout_handle, buf, len);
		if (ret) {
			LOG_ERR("write data error:%d", ret);
			return ret;
		}

#ifdef DAC_PCMBUF_DEBUG
		audio_out_control(hdl->aout_dev,
						hdl->aout_handle, AOUT_CMD_GET_FIFO_AVAILABLE_LEN,
						&hdl->pcmbuf_avail);
#endif
	}

	if (delta > 1000000000UL) {
		play_timestamp = k_cycle_get_32();
		audio_play_perf_detail();
	}

	return 0;
}

/* @brief Configure the extral infomation to the low level audio play device */
static void audio_play_ext_config(audio_play_handle handle)
{
	int ret = 0;
	uint32_t count;

	if (!handle) {
		LOG_ERR("null handle");
		return;
	}

	LOG_DBG("perf_detail_en %d", handle->attribute.perf_detail_en);

	if (handle->attribute.hw_trigger_src) {
		--handle->attribute.hw_trigger_src;
		ret = audio_out_control(handle->aout_dev,
					handle->aout_handle, AOUT_CMD_SET_DAC_TRIGGER_SRC,
					&handle->attribute.hw_trigger_src);
		if (!ret) {
			LOG_INF("set DAC external IRQ trigger source %d ok",
				handle->attribute.hw_trigger_src);
		}
	}

	if (handle->attribute.perf_detail_en) {
		ret = audio_out_control(handle->aout_dev,
					handle->aout_handle, AOUT_CMD_RESET_SAMPLE_CNT, NULL);
		if (!ret) {
			handle->flags |= AUDIO_PLAY_SAMPLES_CNT_EN_FLAG;
			LOG_INF("enable samples counter function");
		}

		ret = audio_out_control(handle->aout_dev,
				handle->aout_handle, AOUT_CMD_GET_SAMPLE_CNT, &count);
		if (!ret) {
			LOG_INF("orign samples counter:%d", count);
		}

		if ((handle->attribute.fifo_type == AOUT_FIFO_DAC0)
			&& (handle->attribute.chl_type == AUDIO_CHANNEL_DAC)) {
			ret = audio_out_control(handle->aout_dev,
						handle->aout_handle, AOUT_CMD_RESET_DAC_SDM_SAMPLE_CNT, NULL);

			if (!ret) {
				handle->flags |= AUDIO_PLAY_SDM_CNT_EN_FLAG;
				LOG_INF("enable sdm counter function");
			}

			ret = audio_out_control(handle->aout_dev,
					handle->aout_handle, AOUT_CMD_GET_DAC_SDM_SAMPLE_CNT, &count);
			if (!ret) {
				LOG_INF("origin sdm samples counter:%d", count);
			}
		}
	}
}

/* @brief Configure the low level audio play device by specified handler with necessary attribute */
static int audio_play_config(audio_play_handle handle)
{
	aout_param_t aout_param = {0};
	dac_setting_t dac_setting = {0};
	i2stx_setting_t i2stx_setting = {0};
	spdiftx_setting_t spdiftx_setting = {0};
	audio_reload_t reload_setting = {0};

	if (!handle) {
		LOG_ERR("null handle");
		return -EINVAL;
	}

	aout_param.sample_rate = handle->attribute.sr;
	aout_param.channel_type = handle->attribute.chl_type;
	aout_param.outfifo_type = handle->attribute.fifo_type;
	aout_param.channel_width = handle->attribute.chl_width;

	LOG_INF("sr:%d channel type:0x%x fifo type:%d width:%d",
			aout_param.sample_rate, aout_param.channel_type,
			aout_param.outfifo_type, aout_param.channel_width);

	aout_param.callback = audio_play_write_data_cb;
	aout_param.cb_data = handle;

	/* DMA reload mode */
	if (handle->attribute.reload_en) {
		reload_setting.reload_addr = handle->audio_buffer;
		reload_setting.reload_len = handle->audio_buffer_size;
		aout_param.reload_setting = &reload_setting;
	}

	/* DAC interface setting */
	if (handle->attribute.chl_type & AUDIO_CHANNEL_DAC) {
		dac_setting.volume.left_volume = handle->attribute.left_vol;
		dac_setting.volume.right_volume = handle->attribute.right_vol;
		dac_setting.channel_mode = handle->attribute.mono_en ? MONO_MODE : STEREO_MODE;
		aout_param.dac_setting = &dac_setting;
	}

	/* I2S interface setting */
	if (handle->attribute.chl_type & AUDIO_CHANNEL_I2STX) {
		/* SRD function is only used in slave mode */
		i2stx_setting.srd_callback = audio_i2stx_srd_cb;
		i2stx_setting.cb_data = handle;
		aout_param.i2stx_setting = &i2stx_setting;
		if (AOUT_FIFO_DAC0 == aout_param.outfifo_type) {
			aout_param.dac_setting = &dac_setting;
			dac_setting.volume.left_volume = handle->attribute.left_vol;
			dac_setting.volume.right_volume = handle->attribute.right_vol;
			dac_setting.channel_mode = handle->attribute.mono_en ? MONO_MODE : STEREO_MODE;
		}
	}

	/* SPDIF interface setting */
	if (handle->attribute.chl_type & AUDIO_CHANNEL_SPDIFTX) {
		if (AOUT_FIFO_DAC0 == aout_param.outfifo_type) {
			aout_param.dac_setting = &dac_setting;
			dac_setting.volume.left_volume = handle->attribute.left_vol;
			dac_setting.volume.right_volume = handle->attribute.right_vol;
		}
		aout_param.spdiftx_setting = &spdiftx_setting;
	}

	handle->aout_handle = audio_out_open(handle->aout_dev, (void *)&aout_param);
	if (!handle->aout_handle)
		return -EFAULT;

	audio_play_ext_config(handle);

	return 0;
}

/* @brief create an audio play instance and return the play handler */
static audio_play_handle audio_play_create(struct audio_play_attr *attr)
{
	struct device *dev;
	audio_play_handle hdl = NULL;
	static struct audio_play_object play_object[AUDIO_PLAY_MAX_HANLDER];

	if (!attr) {
		LOG_ERR("Invalid attr");
		return NULL;
	}

	dev = (struct device *)device_get_binding(CONFIG_AUDIO_OUT_ACTS_DEV_NAME);
	if (!dev) {
		LOG_ERR("find to bind dev %s", CONFIG_AUDIO_OUT_ACTS_DEV_NAME);
		return NULL;
	}

	hdl = &play_object[attr->id];
	memset(hdl, 0, sizeof(struct audio_play_object));

#ifdef SHELL_ADC_USE_DIFFERENT_BUFFER
	if (!attr->id)
		hdl->audio_buffer = shell_audio_buffer;
	else
		hdl->audio_buffer = shell_audio_buffer1;
#else
	hdl->audio_buffer = shell_audio_buffer;
#endif

	hdl->audio_buffer_size = SHELL_AUDIO_BUFFER_SIZE;

	if (!attr->adda_en) {
		audio_prepare_data(hdl, hdl->audio_buffer,
				SHELL_AUDIO_BUFFER_SIZE, attr->chl_width);
	} else {
		memset(hdl->audio_buffer, 0, SHELL_AUDIO_BUFFER_SIZE);
	}

	hdl->aout_dev = (struct device *)dev;
	memcpy(&hdl->attribute, attr, sizeof(struct audio_play_attr));

	return hdl;
}


/* @brief start to play stream */
static int audio_play_start(audio_play_handle handle)
{
	int ret;
	if (handle->attribute.reload_en) {
		ret = audio_out_start(handle->aout_dev, handle->aout_handle);
	} else {
		ret = audio_out_write(handle->aout_dev, handle->aout_handle,
						handle->audio_buffer, handle->audio_buffer_size);
		audio_play_total_size += handle->audio_buffer_size;
	}

	if (ret) {
		LOG_ERR("player[%d] start error:%d", handle->attribute.id, ret);
	} else {
		handle->flags |= AUDIO_PLAY_START_FLAG;
		LOG_INF("player[%d] start successfully", handle->attribute.id);
	}

	return ret;
}

/* @brief stop playing stream */
static void audio_play_stop(audio_play_handle handle)
{
	uint8_t i;
	bool all_stop_flag = true;

	if (handle && handle->aout_handle) {
		audio_out_stop(handle->aout_dev, handle->aout_handle);
		audio_out_close(handle->aout_dev, handle->aout_handle);
		audio_player[handle->attribute.id & (AUDIO_PLAY_MAX_HANLDER - 1)] = NULL;
		handle->flags = 0;
	}

	for (i = 0; i < AUDIO_PLAY_MAX_HANLDER; i++) {
		if (audio_player[i]) {
			all_stop_flag = false;
			break;
		}
	}

	if (all_stop_flag)
		audio_play_total_size = 0;
}

/* @brief command to dump the audio controller registers */
static int cmd_dump_register(const struct shell *shell,
			      size_t argc, char **argv)
{
	struct device *dev = NULL;

	if (argc != 2) {
		shell_error(shell, "Invalid parameter");
		shell_fprintf(shell, SHELL_NORMAL,
				"Usage: dump_reg [dac|i2stx|spdiftx|adc|i2srx|spdifrx|anc]\n");
		return -1;
	}

	if (!strcmp(argv[1], "dac")) {
#ifdef CONFIG_AUDIO_OUT_DAC_SUPPORT
		dev = (struct device *)device_get_binding(CONFIG_AUDIO_DAC_0_NAME);
#endif
	} else if (!strcmp(argv[1], "i2stx")) {
#ifdef CONFIG_AUDIO_OUT_I2STX_SUPPORT
		dev = (struct device *)device_get_binding(CONFIG_AUDIO_I2STX_0_NAME);
#endif
	} else if (!strcmp(argv[1], "spdiftx")) {
#ifdef CONFIG_AUDIO_OUT_SPDIFTX_SUPPORT
		dev = (struct device *)device_get_binding(CONFIG_AUDIO_SPDIFTX_0_NAME);
#endif
	} else if (!strcmp(argv[1], "adc")) {
#ifdef CONFIG_AUDIO_IN_ADC_SUPPORT
		dev = (struct device *)device_get_binding(CONFIG_AUDIO_ADC_0_NAME);
#endif
	} else if (!strcmp(argv[1], "i2srx")) {
#ifdef CONFIG_AUDIO_IN_I2SRX_SUPPORT
		dev = (struct device *)device_get_binding(CONFIG_AUDIO_I2SRX_0_NAME);
#endif
	} else if (!strcmp(argv[1], "spdifrx")) {
#ifdef CONFIG_AUDIO_IN_SPDIFRX_SUPPORT
		dev = (struct device *)device_get_binding(CONFIG_AUDIO_SPDIFRX_0_NAME);
#endif
	} else if (!strcmp(argv[1], "anc")) {
#ifdef CONFIG_AUDIO_IN_ANC_SUPPORT
		dev = (struct device *)device_get_binding(PHY_ANC_DEV_NAME);
#endif
	} else if (!strcmp(argv[1], "pdmtx")) {
#ifdef CONFIG_AUDIO_OUT_PDMTX_SUPPORT
		dev = (struct device *)device_get_binding(CONFIG_AUDIO_PDMTX_0_NAME);
#endif
	} else
		shell_error(shell, "invalid %s", argv[1]);

	if (dev)
		phy_audio_control(dev, PHY_CMD_DUMP_REGS, NULL);

	return 0;
}

/* @brief command to start playing */
static int cmd_play_start(const struct shell *shell,
			      size_t argc, char **argv)
{
	struct audio_play_attr play_attr = {0};
	uint32_t val;
	audio_play_handle handle;
	int ret;

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_CHL_TYPE_KEY, &val))
		play_attr.chl_type = val;
	else
		play_attr.chl_type = AUDIO_CHANNEL_DAC;

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_CHL_WIDTH_KEY, &val))
		play_attr.chl_width = val;
	else
		play_attr.chl_width = CHANNEL_WIDTH_16BITS;

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_SAMPLE_RATE_KEY, &val))
		play_attr.sr = val;
	else
		play_attr.sr = SAMPLE_RATE_48KHZ;

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_FIFO_TYPE_KEY, &val))
		play_attr.fifo_type = val;
	else
		play_attr.fifo_type = AOUT_FIFO_DAC0;

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_LEFT_VOL_KEY, &val))
		play_attr.left_vol = (int32_t)val;
	else
		play_attr.left_vol = 0; /* 0dB */

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_RIGHT_VOL_KEY, &val))
		play_attr.right_vol = (int32_t)val;
	else
		play_attr.right_vol = 0; /* 0dB */

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_RELOAD_KEY, &val))
		play_attr.reload_en = val;
	else
		play_attr.reload_en = 0;

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_CHL_INDEX_KEY, &val))
		play_attr.id = val;
	else
		play_attr.id = 0;

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_ADDA_KEY, &val))
		play_attr.adda_en = val;
	else
		play_attr.adda_en = 0;

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_PLAY_MONO_KEY, &val))
		play_attr.mono_en = val;
	else
		play_attr.mono_en = 0;

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_PLAY_PERF_DETAIL_KEY, &val))
		play_attr.perf_detail_en = val;
	else
		play_attr.perf_detail_en = 0;

	if (play_attr.id >= AUDIO_PLAY_MAX_HANLDER) {
		LOG_ERR("invalid channel id %d", play_attr.id);
		return -EINVAL;
	}

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_HW_TRIGGER_SRC_KEY, &val))
		play_attr.hw_trigger_src = val + 1;
	else
		play_attr.hw_trigger_src = 0;

	if (audio_player[play_attr.id])
		audio_play_stop(audio_player[play_attr.id]);

	handle = audio_play_create(&play_attr);
	if (!handle)
		return -EFAULT;

	audio_player[play_attr.id] = handle;

	ret = audio_play_config(handle);
	if (ret)
		return ret;

	ret = audio_play_start(handle);
	if (ret)
		return ret;

	return ret;
}

/* @brief command to stop playing */
static int cmd_play_stop(const struct shell *shell,
			      size_t argc, char **argv)
{
	uint32_t val;
	uint8_t id;

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_CHL_INDEX_KEY, &val))
		id = val;
	else
		id = 0;

	if (id >= AUDIO_PLAY_MAX_HANLDER) {
		LOG_ERR("invalid channel id %d", id);
		return -EINVAL;
	}

	if (audio_player[id])
		audio_play_stop(audio_player[id]);

	return 0;
}

/* @brief command to control playing dynamically */
static int cmd_play_ioctl(const struct shell *shell,
			      size_t argc, char **argv)
{
	uint32_t val, param = 0;
	uint8_t cmd, id = 0;
	int ret = 0;
	bool is_in = false;

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_IOCTL_CMD_KEY, &val)) {
		cmd = val;
	} else {
		LOG_ERR("invalid cmd");
		return -EINVAL;
	}

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_IOCTL_PARAM_KEY, &val)) {
		param = val;
		is_in = true;
	}

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_CHL_INDEX_KEY, &val))
		id = (val < AUDIO_PLAY_MAX_HANLDER) ? val : 0;

	if (!audio_player[id]) {
		LOG_ERR("audio player[%d] does not start yet", id);
		return -ENXIO;
	}

	if (cmd == AOUT_CMD_GET_VOLUME) {
		volume_setting_t volume = {0};

		ret = audio_out_control(audio_player[id]->aout_dev,
				audio_player[id]->aout_handle, cmd, (void *)&volume);
		if (!ret) {
			LOG_INF("Left volume: %d", volume.left_volume);
			LOG_INF("Right_volume: %d", volume.right_volume);
		} else {
			LOG_ERR("Get volume error:%d", ret);
		}
	} else if (cmd == AOUT_CMD_SET_VOLUME) {
		volume_setting_t volume = {0};

		if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_LEFT_VOL_KEY, &val))
			volume.left_volume = val;
		if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_RIGHT_VOL_KEY, &val))
			volume.right_volume = val;

		ret = audio_out_control(audio_player[id]->aout_dev,
				audio_player[id]->aout_handle, cmd, (void *)&volume);
		if (ret) {
			LOG_ERR("Set volume error:%d", ret);
		}
	} else if (cmd == AOUT_CMD_SPDIF_SET_CHANNEL_STATUS) {
		audio_spdif_ch_status_t ch_status = {0};

		if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_IOCTL_CMD_SPDIF_CSL_KEY, &val))
			ch_status.csl = val;
		if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_IOCTL_CMD_SPDIF_CSH_KEY, &val))
			ch_status.csh = val;

		ret = audio_out_control(audio_player[id]->aout_dev,
				audio_player[id]->aout_handle, cmd, (void *)&ch_status);
		if (ret) {
			LOG_ERR("Set SPDIF status error:%d", ret);
		}
	} else if (cmd == AOUT_CMD_SPDIF_GET_CHANNEL_STATUS) {
		audio_spdif_ch_status_t ch_status = {0};

		ret = audio_out_control(audio_player[id]->aout_dev,
				audio_player[id]->aout_handle, cmd, (void *)&ch_status);
		if (ret) {
			LOG_ERR("Get SPDIF status error:%d", ret);
		} else {
			LOG_INF("SPDIF CSL: 0x%x", ch_status.csl);
			LOG_INF("SPDIF CSH: 0x%x", ch_status.csh);
		}
	} else {
		ret = audio_out_control(audio_player[id]->aout_dev,
				audio_player[id]->aout_handle, cmd, (void *)&param);
		if (ret) {
			LOG_ERR("execute cmd:%d error:%d", cmd, ret);
		} else {
			if (!is_in) {
				LOG_INF("cmd:%d result:%d", cmd, param);
			}
		}
	}

	return ret;
}

/* @brief audio i2srx SRD(sample rate detection) callback */
static int audio_i2srx_srd_cb(void *cb_data, uint32_t cmd, void *param)
{
	switch (cmd) {
		case I2SRX_SRD_FS_CHANGE:
			LOG_INF("New sample rate %d", *(audio_sr_sel_e *)param);
			break;
		case I2SRX_SRD_WL_CHANGE:
			LOG_INF("New width length %d", *(audio_i2s_srd_wl_e *)param);
			break;
		case I2SRX_SRD_TIMEOUT:
			LOG_INF("SRD timeout");
			break;
		default:
			LOG_ERR("Error command %d", cmd);
			return -EFAULT;
	}

	return 0;
}

/* @brief audio spdifrx SRD(sample rate detection) callback */
static int audio_spdifrx_srd_cb(void *cb_data, uint32_t cmd, void *param)
{
	switch (cmd) {
		case SPDIFRX_SRD_FS_CHANGE:
			LOG_INF("New sample rate %d", *(audio_sr_sel_e *)param);
			break;
		case SPDIFRX_SRD_TIMEOUT:
			LOG_INF("SRD timeout");
			break;
		default:
			LOG_ERR("Error command %d", cmd);
			return -EFAULT;
	}

	return 0;
}

/* @brief read data callback from the low level audio record device */
static int audio_rec_read_data_cb(void *callback_data, uint32_t reason)
{
	static uint32_t rec_timestamp = 0;
	static uint32_t rec_total_size = 0; /* per-second total size  */
	audio_record_handle handle = (audio_record_handle)callback_data;
	uint32_t delta, len = handle->audio_buffer_size / 2;
	uint8_t *buf = NULL;

	if (AIN_DMA_IRQ_HF == reason) {
		buf = handle->audio_buffer;
	} else if (AIN_DMA_IRQ_TC == reason) {
		buf = handle->audio_buffer + len;
	} else {
		LOG_ERR("invalid reason:%d", reason);
		return 0;
	}

	delta = (uint32_t)k_cyc_to_ns_floor64(k_cycle_get_32() - rec_timestamp);
	rec_total_size += len;

	if (delta > 1000000000UL) {
		if (handle->attribute.dump_len)
			AUDIO_DUMP_MEM(buf, handle->attribute.dump_len);

		rec_timestamp = k_cycle_get_32();
		printk("** record[%d] performance %dB/s ** \n",
				handle->attribute.id, rec_total_size);
		rec_total_size = 0;
	}

	return 0;
}

/* @brief Configure the extral infomation to the low level audio record device */
static void audio_record_ext_config(audio_record_handle handle)
{
	int ret = 0;

	if (!handle) {
		LOG_ERR("null handle");
		return;
	}

	if (handle->attribute.hw_trigger_src) {
		--handle->attribute.hw_trigger_src;
		ret = audio_in_control(handle->ain_dev,
					handle->ain_handle, AIN_CMD_SET_ADC_TRIGGER_SRC,
					&handle->attribute.hw_trigger_src);
		if (!ret) {
			LOG_INF("set ADC external IRQ trigger source %d ok",
				handle->attribute.hw_trigger_src);
		}
	}
}

/* @brief Configure the low level audio device to record */
static int audio_record_config(audio_record_handle handle)
{
	ain_param_t ain_param = {0};
	adc_setting_t adc_setting = {0};
	i2srx_setting_t i2srx_setting = {0};
	spdifrx_setting_t spdifrx_setting = {0};

	if (!handle)
		return -EINVAL;

	memset(&ain_param, 0, sizeof(ain_param_t));

	ain_param.sample_rate = handle->attribute.sr;
	ain_param.callback = audio_rec_read_data_cb;
	ain_param.cb_data = (void *)handle;
	ain_param.reload_setting.reload_addr = handle->audio_buffer;
	ain_param.reload_setting.reload_len = handle->audio_buffer_size;
	ain_param.channel_type = handle->attribute.chl_type;
	ain_param.channel_width = handle->attribute.chl_width;

	switch (handle->attribute.chl_type) {
		case AUDIO_CHANNEL_ADC:
			adc_setting.device = handle->attribute.input_dev;
			uint8_t i;
			for (i = 0; i < ADC_CH_NUM_MAX; i++) {
				adc_setting.gain.ch_gain[i] = handle->attribute.ch_gain[i];
			}
			ain_param.adc_setting = &adc_setting;
			break;
		case AUDIO_CHANNEL_I2SRX:
			/* SRD is only used in i2s slave mode */
			i2srx_setting.srd_callback = audio_i2srx_srd_cb;
			i2srx_setting.cb_data = handle;
			ain_param.i2srx_setting = &i2srx_setting;
			break;
		case AUDIO_CHANNEL_SPDIFRX:
			spdifrx_setting.srd_callback = audio_spdifrx_srd_cb;
			spdifrx_setting.cb_data = handle;
			ain_param.spdifrx_setting = &spdifrx_setting;
			break;
		default:
			LOG_ERR("Unsupport channel type: %d", handle->attribute.chl_type);
			return -ENOTSUP;
	}

	handle->ain_handle = audio_in_open(handle->ain_dev, &ain_param);
	if (!handle->ain_handle)
		return -EFAULT;

	audio_record_ext_config(handle);

    return 0;
}

/* @brief create an audio record instance */
static audio_record_handle audio_record_create(struct audio_record_attr *attr)
{
	struct device *dev;
	audio_record_handle hdl = NULL;
	static struct audio_record_object record_object[AUDIO_RECORD_MAX_HANLDER];

	dev = (struct device *)device_get_binding(CONFIG_AUDIO_IN_ACTS_DEV_NAME);
	if (!dev) {
		LOG_ERR("Failed to get the audio in device %s", CONFIG_AUDIO_IN_ACTS_DEV_NAME);
		return NULL;
	}

	hdl = &record_object[attr->id];
	memset(hdl, 0, sizeof(struct audio_record_object));

    hdl->audio_buffer_size = SHELL_AUDIO_BUFFER_SIZE;

#ifdef SHELL_ADC_USE_DIFFERENT_BUFFER
	if (!attr->id)
		hdl->audio_buffer = shell_audio_buffer;
	else
		hdl->audio_buffer = shell_audio_buffer1;
#else
    hdl->audio_buffer = shell_audio_buffer;
#endif

	memcpy(&hdl->attribute, attr, sizeof(struct audio_record_attr));
    hdl->ain_dev = dev;

    return hdl;
}

/* @brief start record */
static int audio_record_start(audio_record_handle handle)
{
	return audio_in_start(handle->ain_dev, handle->ain_handle);
}

/* @brief stop record */
static void audio_record_stop(audio_record_handle handle)
{
	if (handle && handle->ain_handle) {
		audio_in_stop(handle->ain_dev, handle->ain_handle);
		audio_in_close(handle->ain_dev, handle->ain_handle);
		audio_recorder[handle->attribute.id & (AUDIO_RECORD_MAX_HANLDER - 1)] = NULL;
	}
}

/* @brief command to creat a recorder channel instance */
static int cmd_record_create(const struct shell *shell,
			      size_t argc, char **argv)
{
	struct audio_record_attr record_attr = {0};
	uint32_t val;
	audio_record_handle handle;
	int ret;

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_CHL_TYPE_KEY, &val))
		record_attr.chl_type = val;
	else
		record_attr.chl_type = AUDIO_CHANNEL_ADC;

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_CHL_WIDTH_KEY, &val))
		record_attr.chl_width = val;
	else
		record_attr.chl_width = CHANNEL_WIDTH_16BITS;

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_SAMPLE_RATE_KEY, &val))
		record_attr.sr = val;
	else
		record_attr.sr = SAMPLE_RATE_48KHZ;

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_INPUT_DEV_KEY, &val))
		record_attr.input_dev = val;
	else
#ifdef CONFIG_SOC_SERIES_TAI
		record_attr.input_dev = AUDIO_ANALOG_MIC0;
#else
		record_attr.input_dev = AUDIO_LINE_IN0;
#endif

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_CH0_GAIN_KEY, &val))
		record_attr.ch_gain[0] = (int16_t)val;
	else
		record_attr.ch_gain[0] = 0; /* 0dB */

#if (ADC_CH_NUM_MAX == 2)
	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_CH1_GAIN_KEY, &val))
		record_attr.ch_gain[1] = (int16_t)val;
	else
		record_attr.ch_gain[1] = 0; /* 0dB */
#endif

#if (ADC_CH_NUM_MAX == 4)
	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_CH2_GAIN_KEY, &val))
		record_attr.ch_gain[2] = (int16_t)val;
	else
		record_attr.ch_gain[2] = 0; /* 0dB */

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_CH3_GAIN_KEY, &val))
		record_attr.ch_gain[3] = (int16_t)val;
	else
		record_attr.ch_gain[3] = 0; /* 0dB */
#endif

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_DUMP_LEN_KEY, &val)) {
		record_attr.dump_len = val;
		LOG_INF("Dump length: %d", record_attr.dump_len);
	} else {
		record_attr.dump_len = 16;
	}

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_CHL_INDEX_KEY, &val))
		record_attr.id = val;
	else
		record_attr.id = 0;

	if (record_attr.id >= AUDIO_RECORD_MAX_HANLDER) {
		LOG_ERR("invalid channel id %d", record_attr.id);
		return -EINVAL;
	}

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_HW_TRIGGER_SRC_KEY, &val))
		record_attr.hw_trigger_src = val + 1;
	else
		record_attr.hw_trigger_src = 0;

	if (audio_recorder[record_attr.id])
		audio_record_stop(audio_recorder[record_attr.id]);

	handle = audio_record_create(&record_attr);
	if (!handle)
		return -EFAULT;

	ret = audio_record_config(handle);
	if (ret)
		return ret;

	audio_recorder[record_attr.id] = handle;

	return ret;

}

/* @brief command to start recording */
static int cmd_record_start(const struct shell *shell,
			      size_t argc, char **argv)
{
	uint32_t val;
	uint8_t id;
	int ret;

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_CHL_INDEX_KEY, &val))
		id = val;
	else
		id = 0;

	if (!audio_recorder[id]) {
		LOG_ERR("audio recorder[%d] does not create yet", id);
		return -ENXIO;
	}

	ret = audio_record_start(audio_recorder[id]);
	if (ret) {
		LOG_ERR("recorder[id:%d] start error:%d", id, ret);
	} else {
		LOG_INF("recorder[id:%d] start successfully", id);
	}

	return ret;
}

/* @brief command to stop recording */
static int cmd_record_stop(const struct shell *shell,
			      size_t argc, char **argv)
{
	uint32_t val;
	uint8_t id;

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_CHL_INDEX_KEY, &val))
		id = val;
	else
		id = 0;

	if (id >= AUDIO_RECORD_MAX_HANLDER) {
		LOG_ERR("invalid channel id %d", id);
		return -EINVAL;
	}

	if (audio_recorder[id])
		audio_record_stop(audio_recorder[id]);

	return 0;
}

/* @brief command to control recording dynamically */
static int cmd_record_ioctl(const struct shell *shell,
			      size_t argc, char **argv)
{
	uint32_t val, param = 0;
	uint8_t cmd, id = 0;
	int ret = 0;
	bool is_in = false;

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_IOCTL_CMD_KEY, &val)) {
		cmd = val;
	} else {
		LOG_ERR("invalid cmd");
		return -EINVAL;
	}

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_IOCTL_PARAM_KEY, &val)) {
		param = val;
		is_in = true;
	}

	if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_CHL_INDEX_KEY, &val))
		id = (val < AUDIO_RECORD_MAX_HANLDER) ? val : 0;

	if (!audio_recorder[id]) {
		LOG_ERR("audio recorder[%d] does not create yet", id);
		return -ENXIO;
	}

	if (cmd == AIN_CMD_SET_ADC_GAIN) {
		adc_gain gain = {0};

		if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_CH0_GAIN_KEY, &val))
			gain.ch_gain[0] = val;

#if (ADC_CH_NUM_MAX == 2)
		if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_CH1_GAIN_KEY, &val))
			gain.ch_gain[1] = val;
#endif

#if (ADC_CH_NUM_MAX == 4)
		if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_CH2_GAIN_KEY, &val))
			gain.ch_gain[2] = val;
		if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_CH2_GAIN_KEY, &val))
			gain.ch_gain[3] = val;
#endif
		ret = audio_in_control(audio_recorder[id]->ain_dev,
				audio_recorder[id]->ain_handle, cmd, (void *)&gain);
		if (ret) {
			LOG_ERR("Set gain error:%d", ret);
		}
	} else if ((cmd == AIN_CMD_GET_ADC_LEFT_GAIN_RANGE)
			|| (cmd == AIN_CMD_GET_ADC_RIGHT_GAIN_RANGE)) {
		adc_gain_range range = {0};

		ret = audio_in_control(audio_recorder[id]->ain_dev,
				audio_recorder[id]->ain_handle, cmd, (void *)&range);
		if (ret) {
			LOG_ERR("Get gain range error:%d", ret);
		} else {
			LOG_INF("gain range [%d..%d]", range.min, range.max);
		}
	} else if (cmd == AIN_CMD_SPDIF_GET_CHANNEL_STATUS) {
		audio_spdif_ch_status_t ch_status = {0};

		ret = audio_in_control(audio_recorder[id]->ain_dev,
				audio_recorder[id]->ain_handle, cmd, (void *)&ch_status);
		if (ret) {
			LOG_ERR("Get SPDIF status error:%d", ret);
		} else {
			LOG_INF("SPDIF CSL: 0x%x", ch_status.csl);
			LOG_INF("SPDIF CSH: 0x%x", ch_status.csh);
		}
	} else if (cmd == AIN_CMD_BIND_CHANNEL) {
		uint8_t bind_id;
		if (!cmd_search_value_by_key(argc, argv, SHELL_CMD_IOCTL_CMD_BIND_ID_KEY, &val)) {
			bind_id = val;
			if (bind_id > AUDIO_RECORD_MAX_HANLDER) {
				LOG_ERR("invalid bind id %d", bind_id);
				return -EINVAL;
			}
		} else {
			LOG_ERR("no bind id");
			return -ENOTSUP;
		}
		LOG_INF("bind session id %d", bind_id);

		param = (uint32_t)audio_recorder[bind_id]->ain_handle;

		ret = audio_in_control(audio_recorder[id]->ain_dev,
				audio_recorder[id]->ain_handle, cmd, (void *)param);
	} else {
		ret = audio_in_control(audio_recorder[id]->ain_dev,
				audio_recorder[id]->ain_handle, cmd, (void *)&param);
		if (ret) {
			LOG_ERR("execute cmd:%d error:%d", cmd, ret);
		} else {
			if (!is_in) {
				LOG_INF("cmd:%d result:%d", cmd, param);
			}
		}
	}

	return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_audio_play,
	SHELL_CMD(start, NULL, "Play start.", cmd_play_start),
	SHELL_CMD(stop, NULL, "Play stop.", cmd_play_stop),
	SHELL_CMD(ioctl, NULL, "Play io control and comands.", cmd_play_ioctl),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_audio_record,
	SHELL_CMD(create, NULL, "Record create an instance.", cmd_record_create),
	SHELL_CMD(start, NULL, "Record start.", cmd_record_start),
	SHELL_CMD(stop, NULL, "Record stop.", cmd_record_stop),
	SHELL_CMD(ioctl, NULL, "Play io control and comands.", cmd_record_ioctl),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_acts_audio,
	SHELL_CMD(dump_reg, NULL, "Dump register.", cmd_dump_register),
	SHELL_CMD(play, &sub_audio_play, "Audio play.", NULL),
	SHELL_CMD(record, &sub_audio_record, "Audio record.", NULL),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(audio, &sub_acts_audio, "Actions audio commands", NULL);
