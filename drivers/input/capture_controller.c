/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TP Keyboard driver for Actions SoC
 */

#include <errno.h>
#include <kernel.h>
#include <string.h>
#include <init.h>
#include <irq.h>
#include <drivers/adc.h>
#include <drivers/input/input_dev.h>
#include <sys/util.h>
#include <sys/byteorder.h>
#include <logging/log.h>
#include <device.h>
#include <drivers/gpio.h>
#include <soc.h>
#include <string.h>
#include <drivers/cfg_drv/dev_config.h>
#include <soc.h>
#include "capture_hal.h"

LOG_MODULE_REGISTER(quad_decoder, CONFIG_SYS_LOG_INPUT_DEV_LEVEL);

#define DT_DRV_COMPAT actions_capture
/* capture ctl reg */
#define CAP_CTL_FIFO_LEV_IRQ_SEL_(X)                 (X << 31)
#define CAP_CTL_FIFO_LEV_IRQ_SEL_4BYTE               CAP_CTL_FIFO_LEV_IRQ_SEL_(1)
#define CAP_CTL_FIFO_LEV_IRQ_SEL_1BYTE               CAP_CTL_FIFO_LEV_IRQ_SEL_(0)

#define CAP_CTL_FIFO_LEV_IRQ_EN                      (1 << 30)
#define CAP_CTL_CC_IRQ_EN                            (1 << 26)
#define CAP_CTL_INLEVEL                              (1 << 24)
#define CAP_CTL_SS                                   (1 << 5)

#define CAP_CTL_ES_SEL(X)                            (X << 3)
#define CAP_CTL_RISE_EDGE                            CAP_CTL_ES_SEL(0)
#define CAP_CTL_FALLING_EDGE                         CAP_CTL_ES_SEL(1)
#define CAP_CTL_BOTH_EDGE                            CAP_CTL_ES_SEL(2)

#define CAP_CTL_MS_SEL(X)                            (X << 1)
#define CAP_CTL_COUNTER_MODE                         CAP_CTL_MS_SEL(0)
#define CAP_CTL_CAPTURE_MODE                         CAP_CTL_MS_SEL(1)
#define CAP_CTL_TIMER_MODE                           CAP_CTL_MS_SEL(2)

#define CAP_CTL_EN                                   (1 << 0)

/* capture cnt reg */
#define CAP_CNT_RE_SEL_(X)                           (X << 15)
#define CAP_CNT_FALLING_EDGE                         CAP_CNT_RE_SEL_(0)
#define CAP_CNT_RISEING_EDGE                         CAP_CNT_RE_SEL_(1)

#define CAP_CNT_CNT(X)                               (X << 0)

/* capture sta reg */
#define CAP_STA_FIFO1_ERR                            (1 << 24)
#define CAP_STA_C1_OVF                               (1 << 23)
#define CAP_STA_RXFL1                                (20)//20~22 READ
#define CAP_STA_RXFF1                                (1 << 19)
#define CAP_STA_RXFE1                                (1 << 18)
#define CAP_STA_FF_LEV_PD1                           (1 << 17)
#define CAP_STA_CT_PD1                               (1 << 16)
#define CAP_STA_FIFO0_ERR                            (1 << 8)
#define CAP_STA_C0_OVF                               (1 << 7)
#define CAP_STA_RXFL0                                (4)//4~6 READ
#define CAP_STA_RXFF0                                (1 << 3)
#define CAP_STA_RXFE0                                (1 << 2)
#define CAP_STA_FF_LEV_PD0                           (1 << 1)
#define CAP_STA_CT_PD0                               (1 << 0)

/* capture dbc ctl */
#define CAP_DBC_CTL_DBC1(X)                          (X << 24)
#define CAP_DBC_CTL_DB_EN1                           (1 << 16)
#define CAP_DBC_CTL_DBC0(X)                          (X << 8)
#define CAP_DBC_CTL_DB_EN0                           (1 << 0)

/* capture demod ctl */
#define CAP_DEM_CTL_INPUT_INV                        (1 << 10)
#define CAP_DEM_CTL_DC_IRQ_EN                        (1 << 9)
#define CAP_DEM_CTL_SPACE_ERR_IRQ_EN                 (1 << 8)
#define CAP_DEM_CTL_MC(X)                            (X << 3)
#define CAP_DEM_CTL_MC_VAL(val)                      ((val & 0x78) >> 3)
#define CAP_DEM_CTL_LC(X)                            (X << 1)
#define CAP_DEM_CTL_EN                               (1 << 0)

/* capture demod sta */
#define CAP_DEM_STA_DC_PD0                           (1 << 1)
#define CAP_DEM_STA_SPACE_ERR_PD0                    (1 << 0)

/* IR_RX_ANA_CTL */
#define RX_ANA_EN                       (1)

#define RX_ANA_CTL(base)                             (base + 0xf0)
#define TX_ANA_CTL(base)                             (base + 0xf4)

#define IR_RX_ODSC                                   (1 << 31)
#define IR_RX_IQS                                    (1 << 30)
#define IR_RX_IBOP(X)                                (X << 28)
#define IR_RX_HYS(X)                                 (X << 26)
#define IR_RX_GAIN(X)                                (X << 24)
#define IR_RX_VOL_CMP(X)                             (X << 20)
#define IR_RX_VOL_GAI_NOP(X)                         (X << 16)
#define IR_RX_VOL_INBUF(X)                           (X << 12)
#define IR_RX_INR(X)                                 (X << 9)
#define IR_RX_DINV                                   (1 << 8)
#define IR_RX_EN                                     (1 << 0)

#define IR_DEMOD_TIMEOIUT                            (0xf0)//(6000)

struct capture_acts_controller {
	volatile uint32_t capture0_ctl;
	volatile uint32_t capture0_cnt;
	volatile uint32_t capture0_val;
	volatile uint32_t reserve_1[5];
	volatile uint32_t capture1_ctl;
	volatile uint32_t capture1_cnt;
	volatile uint32_t capture1_val;
	volatile uint32_t reserve_2[1];
	volatile uint32_t capture_sta;
	volatile uint32_t reserve_3[3];
	volatile uint32_t capture_dbc_ctl;
	volatile uint32_t capture_demod_ctl;
	volatile uint32_t capture_demod_sta;
	volatile uint32_t capture_demod_timeout;
	volatile uint32_t capture_measure_cnt;
	volatile uint32_t capture_measure_high_cnt;
};

struct acts_capture_data {
	input_notify_t notify;
	struct k_delayed_work timer;
	const struct device *this_dev;
	u16_t capture_data[IR_MAX_CAPTURE_CNT];
	u8_t byte_cnt;
	u32_t conf_param;
	u16_t carrier_rate;//khz
	u8_t carrier_duty;
	bool abnormal_inter;
};

struct acts_capture_config {
	struct capture_acts_controller *base;
	void (*irq_config_func)(void);
	uint8_t clock_id;
	uint8_t reset_id;
	uint8_t pinmux_size;
	const struct acts_pin_config *pinmux;
};

static struct acts_capture_data capture_acts_ddata;

static const struct acts_pin_config pins_capture[] = {CONFIG_CAPTURE_MFP};

static void capture_acts_irq_config(void);

static const struct acts_capture_config capture_acts_cdata = {
	.base = (struct capture_acts_controller *)CAPTURE_REG_BASE,
	.irq_config_func = capture_acts_irq_config,
	.pinmux = pins_capture,
	.pinmux_size = ARRAY_SIZE(pins_capture),
	.clock_id = CLOCK_ID_CAPTURE,
	.reset_id = RESET_ID_CAPTURE,
};

static void capture_acts_disable(const struct device *dev);

static void capture_acts_enable(const struct device *dev);

static void capture_acts_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct acts_capture_data *data = dev->data;
	const struct acts_capture_config *cfg = dev->config;
	struct capture_acts_controller *capture = cfg->base;
	u32_t m_cnt, h_cnt;
	data->abnormal_inter=false;

	m_cnt = capture->capture_measure_cnt;
	h_cnt = capture->capture_measure_high_cnt;

	if ((capture->capture_demod_sta & CAP_DEM_STA_DC_PD0) !=0) {
		data->abnormal_inter=true;
		capture->capture_demod_ctl |= (CAP_DEM_STA_DC_PD0| CAP_DEM_CTL_SPACE_ERR_IRQ_EN);
		capture_acts_disable(dev);
		k_delayed_work_submit(&data->timer, K_NO_WAIT);
		return;
	} else {
		data->abnormal_inter=false;
	}

	if(h_cnt == 0) {
		//printk("capture_acts_isr: error param!\n");
		goto past;
	}

	if(m_cnt == 0) {
		//printk("capture_acts_isr: error param!\n");
		goto past;
	}

	if(m_cnt == h_cnt) {
		//printk("capture_acts_isr: error param!\n");
		goto past;
	}

	/* compatible with some special ir protocal format, such as RC6 */
	if(m_cnt<IR_MIN_FREQ || m_cnt>IR_MAX_FREQ){
		goto past;
	}

	data->carrier_rate = 6000000 * CAP_DEM_CTL_MC_VAL(capture->capture_demod_ctl)/m_cnt/10;

	if(CAP_DEM_CTL_INPUT_INV & capture->capture_demod_ctl)
		data->carrier_duty = 10*m_cnt/(m_cnt - h_cnt);
	else
		data->carrier_duty = m_cnt *10/h_cnt;

past:
	while((capture->capture_sta & CAP_STA_RXFE0) == 0) {
		data->capture_data[data->byte_cnt] = capture->capture0_cnt;
		data->byte_cnt++;
		if(data->byte_cnt >= IR_MAX_CAPTURE_CNT)
			data->byte_cnt = IR_MAX_CAPTURE_CNT-1;
	}

	while((capture->capture_sta & CAP_STA_RXFE1) == 0) {
		data->capture_data[data->byte_cnt] = capture->capture1_cnt;
		data->byte_cnt++;
		if(data->byte_cnt >= IR_MAX_CAPTURE_CNT)
			data->byte_cnt = IR_MAX_CAPTURE_CNT-1;
	}

	capture->capture_sta = capture->capture_sta;
	while(capture->capture_sta & (CAP_STA_CT_PD1 | CAP_STA_CT_PD0));

	if(data->byte_cnt == IR_MAX_CAPTURE_CNT-1) {
		capture_acts_disable(dev);
		k_delayed_work_submit(&data->timer, K_NO_WAIT);
	}
}

static void capture_acts_set_clk(const struct acts_capture_config *cfg, uint32_t freq_hz)
{
#if 1
	clk_set_rate(cfg->clock_id, freq_hz);
	k_busy_wait(100);
#endif
}

#if 0
static void capture_data_dump(u32_t *data)
{

	for(int i = 0; i < 100; i++) {
		data[i] = (data[i] & 0x8000) | ((data[i] & 0x7fff)/3);//cycle to us
		if(i%10 == 9) {
			printk("%d-%d\n", ((data[i] & 0x8000) >> 15), (data[i] & 0x7fff));
			continue;
		}
		printk("%d-%d ", ((data[i] & 0x8000) >> 15), (data[i] & 0x7fff));
	}
}
#endif

static void capture_acts_poll(struct k_work *work)
{
	struct acts_capture_data *data = CONTAINER_OF(work, struct acts_capture_data, timer);
	struct input_value val;

	val.ir.protocol.carry_rate = data->carrier_rate;
	val.ir.protocol.carry_duty = data->carrier_duty;
	val.ir.protocol.data = data->capture_data;
	val.ir.data.abnormal_inter=data->abnormal_inter;

	if(data->notify != NULL)
		data->notify(NULL, &val);
}

static void capture_acts_set_param(const struct device *dev, u32_t *conf_para)
{
	struct acts_capture_data *data = dev->data;
	data->conf_param = *conf_para;
}

static void capture_acts_enable(const struct device *dev)
{
	const struct acts_capture_config *cfg = dev->config;
	struct capture_acts_controller *capture = cfg->base;
	struct acts_capture_data *data = dev->data;

	LOG_DBG("enable capture");

	acts_reset_peripheral(cfg->reset_id);
	acts_pinmux_setup_pins(cfg->pinmux, cfg->pinmux_size);
	data->byte_cnt = 0;
	capture->capture_demod_timeout = IR_DEMOD_TIMEOIUT;
#if RX_ANA_EN
	sys_write32(0, TX_ANA_CTL(GPIO_REG_BASE));
	sys_write32(data->conf_param, RX_ANA_CTL(GPIO_REG_BASE));
	capture->capture_dbc_ctl = CAP_DBC_CTL_DB_EN0 | CAP_DBC_CTL_DBC0(6);
#endif
	capture->capture_demod_ctl = capture->capture_demod_ctl & (~(CAP_DEM_CTL_MC(0xf)));
	capture->capture_demod_ctl |= CAP_DEM_CTL_EN | CAP_DEM_CTL_MC(3) | CAP_DEM_CTL_LC(1)|CAP_DEM_CTL_DC_IRQ_EN;
	capture->capture0_ctl = CAP_CTL_FIFO_LEV_IRQ_EN | CAP_CTL_CC_IRQ_EN | CAP_CTL_SS | CAP_CTL_CAPTURE_MODE | CAP_CTL_EN;
}

static void capture_acts_disable(const struct device *dev)
{
	const struct acts_capture_config *cfg = dev->config;
	struct acts_capture_data *data = dev->data;
//	sys_write32(0x3824, 0x40068034);
	data->byte_cnt = 0;
	LOG_DBG("disable capture");
	sys_write32(0, RX_ANA_CTL(GPIO_REG_BASE));
	acts_pinmux_set(cfg->pinmux[0].pin_num, 0x1000);//0x1000 is gpio default value

	acts_reset_peripheral(cfg->reset_id);
}

static void capture_acts_inquiry(const struct device *dev, struct input_value *val)
{

	LOG_DBG("inquiry capture");

}

static void capture_acts_register_notify(const struct device *dev, input_notify_t notify)
{

	LOG_DBG("register notify 0x%x", (uint32_t)notify);

	struct acts_capture_data *capture = dev->data;

	capture->notify = notify;

}

static void capture_acts_unregister_notify(const struct device *dev, input_notify_t notify)
{

	LOG_DBG("unregister notify 0x%x", (uint32_t)notify);

	struct acts_capture_data *capture = dev->data;

	capture->notify = NULL;

}

const struct input_dev_driver_api capture_acts_driver_api = {
	.set_param = capture_acts_set_param,
	.enable = capture_acts_enable,
	.disable = capture_acts_disable,
	.inquiry = capture_acts_inquiry,
	.register_notify = capture_acts_register_notify,
	.unregister_notify = capture_acts_unregister_notify,

};

static int capture_acts_init(const struct device *dev)
{
	struct acts_capture_data *data = dev->data;
	const struct acts_capture_config *cfg = dev->config;

	acts_pinmux_setup_pins(cfg->pinmux, cfg->pinmux_size);

	capture_acts_set_clk(cfg, 3000000);

	/* enable capture controller clock */
	acts_clock_peripheral_enable(cfg->clock_id);

	/* reset capture controller */
	acts_reset_peripheral(cfg->reset_id);

	data->this_dev = dev;
	data->byte_cnt = 0;

	cfg->irq_config_func();

	k_delayed_work_init(&data->timer, capture_acts_poll);

//	sys_write32(0x110001, 0x400680f0);
//	capture_acts_reg_dump(dev);
//	sys_write32(8, 0x40068200);
//	sys_write32(0x2b, 0x40068208);

//	acts_clock_peripheral_disable(cfg->clock_id);
//	acts_reset_peripheral_assert(cfg->clock_id);
//	acts_pinmux_set(cfg->pinmux[0].pin_num, 0x1000);//0x1000 is gpio default value

//	acts_clock_peripheral_enable(cfg->clock_id);
//	acts_reset_peripheral_deassert(cfg->clock_id);

//	capture_acts_enable(dev);

//	printk("1\n");
//	while(1);
	return 0;

}

#if CONFIG_CAPTURE

static int capture_acts_pm_control(const struct device *device,
					uint32_t command,
					void *context, device_pm_cb cb, void *arg)
{
	int ret = 0;
	uint32_t state = *((uint32_t*)context);
	const struct acts_capture_config *cfg = device->config;

	LOG_DBG("command:0x%x state:%d", command, state);

	if (command != DEVICE_PM_SET_POWER_STATE)
		return 0;

	switch (state) {
	case DEVICE_PM_ACTIVE_STATE:
		LOG_INF("capture active");
		acts_clock_peripheral_enable(cfg->clock_id);
		acts_reset_peripheral_deassert(cfg->clock_id);
		capture_acts_set_clk(cfg, 3000000);
		break;
	case DEVICE_PM_SUSPEND_STATE:
		LOG_INF("capture suspend");
		acts_clock_peripheral_disable(cfg->clock_id);
		acts_reset_peripheral_assert(cfg->clock_id);
		break;
	case DEVICE_PM_EARLY_SUSPEND_STATE:
	case DEVICE_PM_LATE_RESUME_STATE:
	case DEVICE_PM_LOW_POWER_STATE:
	case DEVICE_PM_OFF_STATE:
		break;
	default:
		ret = -ESRCH;
	}

	return ret;
}

DEVICE_DEFINE(capture, CONFIG_CAPTURE_NAME,
		    capture_acts_init,
		    capture_acts_pm_control,//device_pm_control_nop
		    &capture_acts_ddata, &capture_acts_cdata,
		    POST_KERNEL, 60,
		    &capture_acts_driver_api);

static void capture_acts_irq_config(void)
{
	IRQ_CONNECT(IRQ_ID_CAPTURE0, CONFIG_CAPTURE_IRQ_PRI,
			capture_acts_isr,
			DEVICE_GET(capture), 0);
	irq_enable(IRQ_ID_CAPTURE0);
}

#endif // DT_NODE_HAS_STATUS
