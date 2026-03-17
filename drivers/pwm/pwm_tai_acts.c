/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief PWM controller driver for Actions SoC
 */

#include <errno.h>
#include <sys/__assert.h>
#include <stdbool.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/pwm.h>
#include <soc.h>
#include <drivers/dma.h>
#include <errno.h>
#include <soc_regs.h>
#include "pwm_context.h"
#include <drivers/cfg_drv/dev_config.h>
#include <soc.h>

#define LOG_LEVEL CONFIG_LOG_PWM_DEV_LEVEL

#include <logging/log.h>

LOG_MODULE_REGISTER(pwm);

#define DMA_IRQ_TC                           (0) /* DMA completion flag */
#define DMA_IRQ_HF                           (1) /* DMA half-full flag */

enum PWM_GROUP {
	PWM_GROUP0_REG,
	PWM_GROUP1_REG,
	PWM_GROUP2_REG,
	PWM_GROUP3_REG,
	PWM_GROUP4_REG,
	PWM_GROUP5_REG,
	PWM_GROUP_MAX,
};
#define PWM_FIFO_REG                         (6)
#define PWM_IR_REG                           (7)
#define PWM_INTCTL_REG                       (8)
#define PWM_PENDING_REG                      (9)

enum PWM_MODE {
	PWM_DEFAULT_REG,
	PWM_FIX_INIT,
	PWM_BTH_INIT,
	PWM_PRG_INIT,
	PWM_IR_INIT,
	PWM_MODE_MAX,
};

#define PWM_MODE_MASK                        (0x7)
#define PWM_chan(x)                          (1 << (3 + x))
#define PWM_chan_act(x)                      (1 << (9 + x))
#define PWM_chan_act_MASK                    (0x7e00)

#define ir_code_pre_sym(a)			((a&0x8000) >> 15)
#define ir_code_pre_val(a)			((a&0x7f00) >> 8)
#define ir_code_pos_sym(a)			((a&0x80) >> 7)
#define ir_code_pos_val(a)			((a&0x7f) >> 0)

#define PWM_IR_REPEAT_MODE			(0 << 8)
#define PWM_IR_CYCLE_MODE			(1 << 8)

#define PWM_IR_MASK					(0xff)

#define PWM_IR_TX_MARGIN			(1000)

#define PWM_IR_TIMEOUT				(1)

/* IR_RX_ANA_CTL */
#define TX_ANA_EN                       (1)
#define RX_ANA_CTL(base)                (base + 0xf0)
#define TX_ANA_CTL(base)                (base + 0xf4)
#define IR_TX_DINV                      (1 << 8)
#define IR_TX_SR(X)                     (X << 4)
#define IR_TX_POUT(X)                   (X << 1)
#define IR_TX_EN                        (1)

struct pwm_acts_data {
	struct k_sem dma_sync;
	struct k_sem ir_sync;
	struct k_sem ir_transfer_sync;
	struct device *dma_dev;
	int dma_chan;
	int (*program_callback)(void *cb_data, u8_t reason);
	void *cb_data;
	u8_t program_pin;
	u16_t group_init_status[6];
	u32_t pwm_ir_sw;
	u32_t buf_num;
	u32_t pwm_ir_mode;
	struct k_timer timer;
	u32_t ir_event_timeout;
	u32_t pwm_ir_lc[2];
	u32_t pwm_ir_ll[2];
	u32_t pwm_ir_ld[2];
	u32_t pwm_ir_pl[2];
	u32_t pwm_ir_pd0[2];
	u32_t pwm_ir_pd1[2];
	u32_t pwm_ir_sl[2];
	u8_t ir_pout;
	bool manual_stop_flag;
};

struct pwm_acts_config {
	u32_t	base;
	u32_t	pwmclk_reg;
	u32_t	cycle;
	u8_t	clock_id;
	u8_t	reset_id;
	const struct acts_pin_config *pinmux;
	u8_t pinmux_size;
	void (*irq_config_func)(void);
	const char *dma_dev_name;
	u8_t txdma_id;
	u8_t flag_use_dma;
};

void pwm_acts_repeat_event_process(const struct pwm_acts_config *cfg, u32_t pending)
{
	uint32_t pwm_base;

	if(pending & PWM_PENDING_G0REPEAT) {
		sys_write32(~(PWM_PENDING_G0REPEAT) & sys_read32(PWM_INT_CTL(cfg->base)), PWM_INT_CTL(cfg->base));
		pwm_base = PWM0_BASE(cfg->base);
		struct acts_pwm_groupx *pwm = (struct acts_pwm_groupx *)pwm_base;

		pwm->ctrl = pwm->ctrl & ~(PWMx_CTRL_CHx_MODE_SEL(0,0xfff));
	}

	if(pending & PWM_PENDING_G1REPEAT) {
		sys_write32(~(PWM_PENDING_G1REPEAT) & sys_read32(PWM_INT_CTL(cfg->base)), PWM_INT_CTL(cfg->base));
		pwm_base = PWM1_BASE(cfg->base);
		struct acts_pwm_groupx *pwm = (struct acts_pwm_groupx *)pwm_base;

		pwm->ctrl = pwm->ctrl & ~(PWMx_CTRL_CHx_MODE_SEL(0,0xfff));
	}

	if(pending & PWM_PENDING_G2REPEAT) {
		sys_write32(~(PWM_PENDING_G2REPEAT) & sys_read32(PWM_INT_CTL(cfg->base)), PWM_INT_CTL(cfg->base));
		pwm_base = PWM2_BASE(cfg->base);
		struct acts_pwm_groupx *pwm = (struct acts_pwm_groupx *)pwm_base;

		pwm->ctrl = pwm->ctrl & ~(PWMx_CTRL_CHx_MODE_SEL(0,0x3));
	}

	if(pending & PWM_PENDING_G3REPEAT) {
		sys_write32(~(PWM_PENDING_G3REPEAT) & sys_read32(PWM_INT_CTL(cfg->base)), PWM_INT_CTL(cfg->base));
		pwm_base = PWM3_BASE(cfg->base);
		struct acts_pwm_groupx *pwm = (struct acts_pwm_groupx *)pwm_base;

		pwm->ctrl = pwm->ctrl & ~(PWMx_CTRL_CHx_MODE_SEL(0,0x3));
	}

	if(pending & PWM_PENDING_G4REPEAT) {
		sys_write32(~(PWM_PENDING_G4REPEAT) & sys_read32(PWM_INT_CTL(cfg->base)), PWM_INT_CTL(cfg->base));
		pwm_base = PWM4_BASE(cfg->base);
		struct acts_pwm_groupx *pwm = (struct acts_pwm_groupx *)pwm_base;

		pwm->ctrl = pwm->ctrl & ~(PWMx_CTRL_CHx_MODE_SEL(0,0x3));
	}
}

static void pwm_acts_ir_timeout_event(struct k_timer *timer)
{
	struct pwm_acts_data *data = k_timer_user_data_get(timer);
	struct acts_pwm_ir *pwm_ir = (struct acts_pwm_ir *)PWM_IR(PWM_REG_BASE);

	pwm_ir->ir_ll = data->pwm_ir_ll[data->pwm_ir_sw];
	pwm_ir->ir_ld = data->pwm_ir_ld[data->pwm_ir_sw];
	pwm_ir->ir_pd0 = data->pwm_ir_pd0[data->pwm_ir_sw];
	pwm_ir->ir_pd1 = data->pwm_ir_pd1[data->pwm_ir_sw];
	pwm_ir->ir_sl = data->pwm_ir_sl[data->pwm_ir_sw];
	pwm_ir->ir_pl = data->pwm_ir_pl[data->pwm_ir_sw];
	pwm_ir->ir_lc = data->pwm_ir_lc[data->pwm_ir_sw];

	pwm_ir->ir_ctl |= PWM_IRCTL_CU;

	if(data->pwm_ir_sw < data->buf_num)
		data->pwm_ir_sw++;

	if(data->pwm_ir_sw >= data->buf_num) {
		if(data->pwm_ir_mode & PWM_IR_CYCLE_MODE)
			data->pwm_ir_sw = 0;
		else
			data->pwm_ir_sw = data->buf_num -1;
	}

	k_timer_stop(&data->timer);

}

void pwm_acts_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct pwm_acts_data *data = dev->data;
	const struct pwm_acts_config *cfg = dev->config;
	struct acts_pwm_ir *pwm_ir = (struct acts_pwm_ir *)PWM_IR(cfg->base);
	unsigned int key;

	key = irq_lock();

	if((sys_read32(PWM_PENDING(cfg->base)) & PWM_PENDING_IRSS) && (data->buf_num > 1)) {
		pwm_ir->ir_ctl |= PWM_IRCTL_CU;
		u16_t timeout;

		timeout = (data->ir_event_timeout * pwm_ir->ir_ll)/1000 + PWM_IR_TIMEOUT;
		k_timer_start(&data->timer, K_MSEC(timeout), K_MSEC(timeout));
	}

	irq_unlock(key);

	if(sys_read32(PWM_PENDING(cfg->base)) & PWM_PENDING_IRAE) {
		if (data->manual_stop_flag) {
			k_sem_give(&data->ir_sync);
			data->group_init_status[PWM_GROUP5_REG] = 0;
		} else {
			/* continue to send repeat code or data code */
			pwm_ir->ir_ctl |= PWM_IRCTL_START;
		}
	}

	if(sys_read32(PWM_PENDING(cfg->base)) & (PWM_PENDING_REPEAT_MASK & sys_read32(PWM_INT_CTL(cfg->base)))) {
		pwm_acts_repeat_event_process(cfg, sys_read32(PWM_PENDING(cfg->base)));
	}

	sys_write32(0xffffffff, PWM_PENDING(cfg->base));

}

static void pwm_acts_set_clk(const struct pwm_acts_config *cfg, uint32_t group, uint32_t freq_hz)
{
	clk_set_rate(cfg->clock_id + group, freq_hz);

	k_busy_wait(100);
}

static u32_t pwm_acts_get_group(u32_t pwm)
{
	u32_t group;

	if(pwm > 15)
		return -EINVAL;

	if((pwm) < 6)
		group = PWM_GROUP0_REG;
	else if((pwm) < 12)
		group = PWM_GROUP1_REG;
	else
		group = pwm -10;

	return group;
}

static u32_t pwm_acts_get_reg_base(u32_t base, uint32_t REG)
{
	u32_t controller_reg;

	switch(REG) {
	case PWM_GROUP0_REG:
		controller_reg = PWM0_BASE(base);
		break;
	case PWM_GROUP1_REG:
		controller_reg = PWM1_BASE(base);
		break;
	case PWM_GROUP2_REG:
		controller_reg = PWM2_BASE(base);
		break;
	case PWM_GROUP3_REG:
		controller_reg = PWM3_BASE(base);
		break;
	case PWM_GROUP4_REG:
		controller_reg = PWM4_BASE(base);
		break;
	case PWM_GROUP5_REG:
		controller_reg = PWM5_BASE(base);
		break;
	case PWM_FIFO_REG:
		controller_reg = PWM_FIFO(base);
		break;
	case PWM_IR_REG:
		controller_reg = PWM_IR(base);
		break;
	case PWM_INTCTL_REG:
		controller_reg = PWM_INT_CTL(base);
		break;
	case PWM_PENDING_REG:
		controller_reg = PWM_PENDING(base);
		break;
	default:
		return -EINVAL;
	}

	return controller_reg;
}

/*
 * Set the period and pulse width for a PWM pin.
 *
 * Parameters
 * dev: Pointer to PWM device structure
 * pwm: PWM channel to set
 * period_cycles: Period (in timer count)
 * pulse_cycles: Pulse width (in timer count).
 * @param flags Flags for pin configuration (polarity).
 * return 0, or negative errno code
 */

static void pwm_acts_groupx_fix_init(u32_t base, u32_t period_cycles, u32_t pulse_cycles, u8_t chan, u8_t function, pwm_flags_t flags)
{
	struct acts_pwm_groupx *pwm = (struct acts_pwm_groupx *)base;
	u32_t pol_param;

	pwm->ctrl |= PWMx_CTRL_CHx_MODE_SEL(chan,1);

	if(chan < 4) {
		pol_param = PWMx_CH_CTL0_CHx_POL_SEL(chan);
		if(flags)
			pwm->ch_ctrl0 |= pol_param;
		else
			pwm->ch_ctrl0 = pwm->ch_ctrl0 & (~pol_param);

	} else {
		pol_param = PWMx_CH_CTL1_CHx_POL_SEL(chan);
		if(flags)
			pwm->ch_ctrl1 |= pol_param;
		else
			pwm->ch_ctrl1 = pwm->ch_ctrl1 & (~pol_param);
	}

	if(function) {
		pwm->cntmax = period_cycles;
		pwm->cmp[chan] = pulse_cycles;

		if((pwm->ctrl & PWMx_CTRL_HUA) == 0)
			pwm->ctrl |= PWMx_CTRL_HUA;
		else {
			k_usleep(30);
			pwm->ctrl |= PWMx_CTRL_HUA;
		}

		return;
	}

	pwm->cntmax = period_cycles;
	pwm->cmp[chan] = pulse_cycles;

	pwm->ctrl |= PWMx_CTRL_CNT_EN;//norlmal mode

}

static int pwm_acts_pin_set(const struct device *dev, uint32_t pwm,
				u32_t period_cycles, u32_t pulse_cycles, pwm_flags_t flags)
{
	const struct pwm_acts_config *cfg = dev->config;
	struct pwm_acts_data *data = dev->data;
	uint32_t base,group;
	u16_t status;

	LOG_INF("PWM@%d set period cycles %d ms, pulse cycles %d ms",
		pwm, period_cycles, pulse_cycles);

//	period_cycles = period_cycles * pwm_normal_clk_rate / 1000;
//	pulse_cycles = pulse_cycles * pwm_normal_clk_rate / 1000;

	if (pulse_cycles > period_cycles) {
		LOG_ERR("pulse cycles %d is biger than period's %d",
			pulse_cycles, period_cycles);
		return -EINVAL;
	}
	group = pwm_acts_get_group(pwm);
	status = data->group_init_status[group];

	if((status&PWM_MODE_MASK) != PWM_DEFAULT_REG && (status&PWM_MODE_MASK) != PWM_FIX_INIT) {
		LOG_ERR("start a fix mode but have  not stop this group bfore!");
		return -EINVAL;
	}

	pwm = (pwm > 5)?pwm-6:pwm;
	pwm = (pwm > 5)?pwm-4-group:pwm;

	base = pwm_acts_get_reg_base(cfg->base, group);

	if((status & PWM_chan(pwm)) == 0)//this group_chan have not initialized yet.
		pwm_acts_groupx_fix_init(base,period_cycles,pulse_cycles,pwm, 0, flags);
	else
		pwm_acts_groupx_fix_init(base,period_cycles,pulse_cycles,pwm, 1, flags);

	if(pulse_cycles == 0) {
		data->group_init_status[group] = data->group_init_status[group] & ~(PWM_chan_act(pwm));
	} else {
		data->group_init_status[group] = PWM_FIX_INIT | PWM_chan(pwm) | status;
		data->group_init_status[group] |= PWM_chan_act(pwm);
	}

	return 0;
}

/*
 * Set the period and pulse width for a PWM pin.
 *
 * Parameters
 * dev: Pointer to PWM device structure
 * pwm: PWM channel to set
 * ctrl: breath mode control
 * return 0, or negative errno code
 */

#ifdef CONFIG_PWM_TAI_FULL_FUNC

static void pwm_acts_groupx_breath_init(u32_t base, u8_t chan, pwm_breath_ctrl_t *ctrl)
{
	struct acts_pwm_groupx *pwm = (struct acts_pwm_groupx *)base;
	struct acts_pwm_breath_mode *breath = (struct acts_pwm_breath_mode *)(PWM_BREATH(base) + chan * PWM_BREATH_REG_SIZE);

	pwm->cntmax = ctrl->pwm_count_max;

	if(ctrl->stage_a_step)
		breath->pwm_bth_a = PWMx_BTHxy_EN | PWMx_BTHxy_XS(ctrl->stage_a_pwm) | PWMx_BTHxy_REPEAT(ctrl->stage_a_repeat) | PWMx_BTHxy_STEP(ctrl->stage_a_step);
	else
		breath->pwm_bth_a &= (~PWMx_BTHxy_EN);

	if(ctrl->stage_b_step)
		breath->pwm_bth_b = PWMx_BTHxy_EN | PWMx_BTHxy_XS(ctrl->stage_b_pwm) | PWMx_BTHxy_REPEAT(ctrl->stage_b_repeat) | PWMx_BTHxy_STEP(ctrl->stage_b_step);
	else
		breath->pwm_bth_b &= (~PWMx_BTHxy_EN);

	if(ctrl->stage_c_step)
		breath->pwm_bth_c = PWMx_BTHxy_EN | PWMx_BTHxy_XS(ctrl->stage_c_pwm) | PWMx_BTHxy_REPEAT(ctrl->stage_c_repeat) | PWMx_BTHxy_STEP(ctrl->stage_c_step);
	else
		breath->pwm_bth_c &= (~PWMx_BTHxy_EN);

	if(ctrl->stage_d_step)
		breath->pwm_bth_d = PWMx_BTHxy_EN | PWMx_BTHxy_XS(ctrl->stage_d_pwm) | PWMx_BTHxy_REPEAT(ctrl->stage_d_repeat) | PWMx_BTHxy_STEP(ctrl->stage_d_step);
	else
		breath->pwm_bth_d &= (~PWMx_BTHxy_EN);

	if(ctrl->stage_e_step)
		breath->pwm_bth_e = PWMx_BTHxy_EN | PWMx_BTHxy_XS(ctrl->stage_e_pwm) | PWMx_BTHxy_REPEAT(ctrl->stage_e_repeat) | PWMx_BTHxy_STEP(ctrl->stage_e_step);
	else
		breath->pwm_bth_e &= (~PWMx_BTHxy_EN);

	if(ctrl->stage_f_step)
		breath->pwm_bth_f = PWMx_BTHxy_EN | PWMx_BTHxy_XS(ctrl->stage_f_pwm) | PWMx_BTHxy_REPEAT(ctrl->stage_f_repeat) | PWMx_BTHxy_STEP(ctrl->stage_f_step);
	else
		breath->pwm_bth_f &= (~PWMx_BTHxy_EN);

	if(ctrl->stage_low_wait)
		breath->pwm_bth_hl = PWMx_BTHx_HL_L(ctrl->stage_low_wait) | PWMx_BTHx_HL_LEN;
	else
		breath->pwm_bth_hl &= (~PWMx_BTHx_HL_LEN);

	if(ctrl->stage_high_wait)
		breath->pwm_bth_hl = PWMx_BTHx_HL_H(ctrl->stage_high_wait) | PWMx_BTHx_HL_HEN;
	else
		breath->pwm_bth_hl &= (~PWMx_BTHx_HL_HEN);

	if(ctrl->start_dir)
		breath->pwm_bth_st = PWMx_BTHx_ST_ST(ctrl->start_pwm) | PWMx_BTHx_ST_DIR;
	else
		breath->pwm_bth_st = PWMx_BTHx_ST_ST(ctrl->start_pwm);

	pwm->ctrl |= PWMx_CTRL_CHx_MODE_SEL(chan,3) | PWMx_CTRL_CNT_EN;//breath mode
}


static int pwm_acts_set_breath_mode(const struct device *dev, uint32_t pwm, pwm_breath_ctrl_t *ctrl)
{
	const struct pwm_acts_config *cfg = dev->config;
	struct pwm_acts_data *data = dev->data;
	uint32_t base,group;
	u16_t status;

	group = pwm_acts_get_group(pwm);

	status = data->group_init_status[group];

	if((status&0x3) != PWM_BTH_INIT && (status&0x3)) {//not brearh mode or default
		LOG_ERR("start a breath mode but have not stop this group bfore!");
		return -EINVAL;
	}

	pwm = (pwm > 5)?pwm-6:pwm;
	pwm = (pwm > 5)?pwm-4-group:pwm;

	base = pwm_acts_get_reg_base(cfg->base, group);

	if(group == PWM_GROUP1_REG || group == PWM_GROUP2_REG)
		pwm_acts_groupx_breath_init(base, pwm, ctrl);
	else {
		LOG_ERR("unsuported channel: %d",pwm);
		return -EINVAL;
	}

	data->group_init_status[group] = PWM_BTH_INIT | PWM_chan(pwm) | status;

	return 0;
}

#endif

/*
 * Set the period and pulse width for a PWM pin.
 *
 * Parameters
 * dev: Pointer to PWM device structure
 * pwm: PWM channel to set
 * ctrl: program mode control
 * return 0, or negative errno code
 */
#ifdef CONFIG_PWM_TAI_FULL_FUNC

static void dma_done_callback(const struct device *dev, void *callback_data, uint32_t ch , int type)
{
	struct pwm_acts_data *data = (struct pwm_acts_data *)callback_data;

	//if (type != DMA_IRQ_TC)
		//return;

	LOG_DBG("pwm dma transfer is done");

	k_sem_give(&data->dma_sync);
}

static int pwm_acts_start_dma(const struct pwm_acts_config *cfg,
				  struct pwm_acts_data *data,
				  uint32_t dma_chan,
				  uint16_t *buf,
				  int32_t len,
				  bool is_tx,
				  void *callback)
{
	struct acts_pwm_fifo *pwm_fifo = (struct acts_pwm_fifo *)PWM_FIFO(cfg->base);
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_block_cfg = {0};

	if (callback) {
		dma_cfg.dma_callback = (dma_callback_t)callback;
		dma_cfg.user_data = data;
		dma_cfg.complete_callback_en = 1;
	}

	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_block_cfg;
	dma_block_cfg.block_size = len;
	if (is_tx) {
		dma_cfg.dma_slot = cfg->txdma_id;
		dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
		dma_block_cfg.source_address = (uint32_t)buf;
		dma_block_cfg.dest_address = (uint32_t)&pwm_fifo->fifodat;
		dma_cfg.dest_data_size = 2;
	} else {
		return -EINVAL;
	}

	dma_cfg.source_burst_length = 4;

	if (dma_config(data->dma_dev, dma_chan, &dma_cfg)) {
		LOG_ERR("dma%d config error", dma_chan);
		return -1;
	}

	if (dma_start(data->dma_dev, dma_chan)) {
		LOG_ERR("dma%d start error", dma_chan);
		return -1;
	}

	return 0;
}

static void pwm_acts_stop_dma(const struct pwm_acts_config *cfg,
					 struct pwm_acts_data *data,
					 uint32_t dma_chan)
{
	  dma_stop(data->dma_dev, dma_chan);
}

static int pwm_acts_wait_fifo_staus(struct acts_pwm_fifo *pwm_fifo, uint16_t timeout)
{
	int start_time;

	start_time = k_uptime_get_32();

	while(!(pwm_fifo->fifosta&PWM_FIFOSTA_EMPTY)) {
		if(k_uptime_get_32() - start_time > 500)
			return 0;
	}

	return -EINVAL;
}

static int pwm_acts_dma_transfer(const struct pwm_acts_config *cfg, struct pwm_acts_data *data, u8_t chan, pwm_program_ctrl_t *ctrl)
{
	struct acts_pwm_groupx *pwm = (struct acts_pwm_groupx *)cfg->base;
	struct acts_pwm_fifo *pwm_fifo = (struct acts_pwm_fifo *)PWM_FIFO(cfg->base);
	int ret;

	pwm->ctrl |= PWMx_CTRL_CHx_MODE_SEL(chan,2) | PWMx_CTRL_CM | PWMx_CTRL_RM;

	if(ctrl->cntmax) {
		pwm->ctrl = pwm->ctrl & ~(PWMx_CTRL_CM);
		pwm->cntmax = ctrl->cntmax;
	}

	if(ctrl->repeat) {
		pwm->ctrl = pwm->ctrl & ~(PWMx_CTRL_RM);
		pwm->repeat = ctrl->repeat;
	}

	if(pwm_acts_wait_fifo_staus(pwm_fifo, 500)) {

		LOG_ERR("time out error, pwm fifo can not be empty!");
		return -EINVAL;
	}

	pwm_fifo->fifoctl = PWM_FIFOCTL_START;

	ret = pwm_acts_start_dma(cfg, data, data->dma_chan, ctrl->ram_buf, ctrl->ram_buf_len, 1, dma_done_callback);

	if (ret) {
		LOG_ERR("faield to start dma chan 0x%x\n", data->dma_chan);
		goto out;
	}

	pwm->ctrl |= PWMx_CTRL_CNT_EN;

	/* wait until dma transfer is done */
	k_sem_take(&data->dma_sync, K_FOREVER);//K_MSEC(500));//

out:
	pwm_acts_stop_dma(cfg, data, data->dma_chan);

	if(pwm_acts_wait_fifo_staus(pwm_fifo, 500)) {

		LOG_ERR("time out error, pwm fifo can not be empty!");
		return -EINVAL;
	}

	pwm->ctrl = pwm->ctrl & ~(PWMx_CTRL_CHx_MODE_SEL(chan,2) | PWMx_CTRL_CM | PWMx_CTRL_RM | PWMx_CTRL_CNT_EN);
	pwm_fifo->fifoctl = pwm_fifo->fifoctl & ~(PWM_FIFOCTL_START);

	return ret;

}

static int pwm_acts_set_program_mode(const struct device *dev, uint32_t pwm, pwm_program_ctrl_t *ctrl)
{
	const struct pwm_acts_config *cfg = dev->config;
	struct pwm_acts_data *data = dev->data;
	uint32_t base,group;
	u16_t status;

	group = pwm_acts_get_group(pwm);

	status = data->group_init_status[group];

	if((status&0x3) != PWM_PRG_INIT && (status&0x3)) {//not program mode or default
		LOG_ERR("start a breath mode but have  not stop this group bfore!");
		return -EINVAL;
	}

	pwm = (pwm > 5)?pwm-6:pwm;
	pwm = (pwm > 5)?pwm-4-group:pwm;

	base = pwm_acts_get_reg_base(cfg->base, group);

	if(group == PWM_GROUP0_REG || group == PWM_GROUP1_REG || group == PWM_GROUP2_REG) {
		pwm_acts_dma_transfer(cfg, data, pwm, ctrl);//这里有问题的需要修改，ctrl无法覆盖到group2和group1
	} else {
		LOG_ERR("unsuported channel: %d",pwm);
		return -EINVAL;
	}

//	data->group_init_status[group] = PWM_BTH_INIT | PWM_chan(pwm) | status;

	return 0;
}

#endif

//#define TX_ANA_CTL(base)                (base + 0xf4)
//#define IR_TX_DINV                      (1 << 8)
//#define IR_TX_SR(X)                     (X << 4)
//#define IR_TX_POUT(X)                   (X << 1)
//#define IR_TX_EN                        (1)

#if 0
static void pwm_acts_ir_reg_dump(struct acts_pwm_ir *pwm_ir)
{
	printk("pwm_ir->ir_asc		:%d\n", pwm_ir->ir_asc			);
	printk("pwm_ir->ir_duty		:%d\n", pwm_ir->ir_duty			);
	printk("pwm_ir->ir_lc		:%d\n", pwm_ir->ir_lc			);
	printk("pwm_ir->ir_ld		:%d\n", pwm_ir->ir_ld			);
	printk("pwm_ir->ir_ll		:%d\n", pwm_ir->ir_ll			);
	printk("pwm_ir->ir_pd0		:%d\n", pwm_ir->ir_pd0			);
	printk("pwm_ir->ir_pd1		:%d\n", pwm_ir->ir_pd1			);
	printk("pwm_ir->ir_period	:%d\n", pwm_ir->ir_period		);
	printk("pwm_ir->ir_pl		:%d\n", pwm_ir->ir_pl			);
	printk("pwm_ir->ir_pl0_post	:%d\n", pwm_ir->ir_pl0_post		);
	printk("pwm_ir->ir_pl0_pre	:%d\n", pwm_ir->ir_pl0_pre		);
	printk("pwm_ir->ir_pl1_post	:%d\n", pwm_ir->ir_pl1_post		);
	printk("pwm_ir->ir_pl1_pre	:%d\n", pwm_ir->ir_pl1_pre		);
	printk("pwm_ir->ir_sl		:%d\n", pwm_ir->ir_sl			);
}

#endif

static void pwm_acts_ir_tx(const struct pwm_acts_config *cfg, struct pwm_acts_data *data, u8_t chan, struct pwm_ir_mode_param_t *ctrl, uint32_t base)
{
	struct acts_pwm_groupx *pwm = (struct acts_pwm_groupx *)base;
	struct acts_pwm_ir *pwm_ir = (struct acts_pwm_ir *)PWM_IR(cfg->base);
	u16_t mode;

	acts_pinmux_set(cfg->pinmux[cfg->pinmux_size - 1].pin_num, cfg->pinmux[cfg->pinmux_size - 1].mode);

#if TX_ANA_EN
	sys_write32(0, RX_ANA_CTL(GPIO_REG_BASE));
	sys_write32(IR_TX_EN | IR_TX_POUT(data->ir_pout), TX_ANA_CTL(GPIO_REG_BASE));
#endif

	while(pwm_ir->ir_ctl&PWM_IRCTL_START);

	data->pwm_ir_mode = ctrl[0].mode;

	mode = ctrl[0].mode & PWM_IR_MASK;

	pwm->ctrl |= PWMx_CTRL_CHx_MODE_SEL(chan,2);

	pwm_ir->ir_asc		 = ctrl[0].ir_asc;
	pwm_ir->ir_duty		 = ctrl[0].ir_duty;
	pwm_ir->ir_lc		 = ctrl[0].ir_lc;
	pwm_ir->ir_ld		 = ctrl[0].ir_ld;
	pwm_ir->ir_ll		 = ctrl[0].ir_ll;
	pwm_ir->ir_pd0		 = ctrl[0].ir_pd0;
	pwm_ir->ir_pd1		 = ctrl[0].ir_pd1;
	pwm_ir->ir_period	 = ctrl[0].ir_period;
	pwm_ir->ir_pl		 = ctrl[0].ir_pl;
	pwm_ir->ir_pl0_post	 = ctrl[0].ir_pl0_post;
	pwm_ir->ir_pl0_pre	 = ctrl[0].ir_pl0_pre;
	pwm_ir->ir_pl1_post	 = ctrl[0].ir_pl1_post;
	pwm_ir->ir_pl1_pre	 = ctrl[0].ir_pl1_pre;
	pwm_ir->ir_sl		 = ctrl[0].ir_sl;

//	pwm_acts_ir_reg_dump(pwm_ir);

	if(ctrl[0].buf_num > 1) {
		data->pwm_ir_sw = 1;

		data->pwm_ir_lc[0] = ctrl[0].ir_lc;
		data->pwm_ir_ld[0] = ctrl[0].ir_ld;
		data->pwm_ir_ll[0] = ctrl[0].ir_ll;
		data->pwm_ir_pd0[0] = ctrl[0].ir_pd0;
		data->pwm_ir_pd1[0] = ctrl[0].ir_pd1;
		data->pwm_ir_pl[0] = ctrl[0].ir_pl;
		data->pwm_ir_sl[0] = ctrl[0].ir_sl;

		data->pwm_ir_lc[1] = ctrl[1].ir_lc;
		data->pwm_ir_ld[1] = ctrl[1].ir_ld;
		data->pwm_ir_ll[1] = ctrl[1].ir_ll;
		data->pwm_ir_pd0[1] = ctrl[1].ir_pd0;
		data->pwm_ir_pd1[1] = ctrl[1].ir_pd1;
		data->pwm_ir_pl[1] = ctrl[1].ir_pl;
		data->pwm_ir_sl[1] = ctrl[1].ir_sl;
	}

	data->manual_stop_flag = false;
	data->buf_num = ctrl[0].buf_num;

	sys_write32(0xffffffff, PWM_PENDING(cfg->base));
	sys_write32(PWM_INTCTL_IRAE | PWM_INTCTL_IRSS, PWM_INT_CTL(cfg->base));
	pwm_ir->ir_ctl = PWM_IRCTL_PLED | PWM_IRCTL_START;
	pwm->ctrl |= PWMx_CTRL_CNT_EN;

}

static u32_t pwm_acts_data_cal(u32_t data, u32_t buf_num)
{
	u32_t cal_val = 0;
	u32_t cal_dat = data;

	while(buf_num > 0) {
		if(cal_dat & 0x1)
			cal_val++;
		buf_num--;
		cal_dat = cal_dat >> 1;
	}

	return cal_val;
}

static u32_t pwm_acts_ir_get_len(u32_t ld)
{
	u32_t len = 0;

	for(int i = 0; i < 32; i++) {
		if(ld & 0x80000000)
			break;

		len++;

		ld = ld << 1;
	}

	len = 32 - len;

	return len;
}

static void pwm_acts_ir_param_sw(struct ir_tx_data_param *ctrl, struct pwm_ir_mode_param_t *param, u32_t loc, struct ir_tx_protocol_param *protocol_param)
{
	u32_t val, sym, ld_len, protocol;
	u16_t cr_rate;

	protocol = ctrl->mode;

	if(ctrl->rate)
		cr_rate = ctrl->rate/100;
	else
		cr_rate = protocol_param->ir_cr_rate;

	param->ir_period = pwm_clk_rate/(cr_rate * 1000);

	if(ctrl->duty)
		param->ir_duty = param->ir_period *10/ctrl->duty;
	else
		param->ir_duty = param->ir_period/3;

	param->ir_lc = cr_rate * protocol_param->ir_lc_bit_length/1000;

	sym = ir_code_pre_sym(protocol_param->ir_0_code) << 16;
	val = ir_code_pre_val(protocol_param->ir_0_code) * protocol_param->code_bit_length;
	val = val * cr_rate/1000;

	if(sym)
		val = val + 1;//process remainder

	param->ir_pl0_pre = sym | val;

	sym = ir_code_pos_sym(protocol_param->ir_0_code) << 16;
	val = ir_code_pos_val(protocol_param->ir_0_code) * protocol_param->code_bit_length;
	val = val * cr_rate/1000;

	if(sym)
		val = val + 1;//process remainder

	param->ir_pl0_post = sym | val;

	sym = ir_code_pre_sym(protocol_param->ir_1_code) << 16;
	val = ir_code_pre_val(protocol_param->ir_1_code) * protocol_param->code_bit_length;
	val = val * cr_rate/1000;

	if(sym)
		val = val + 1;//process remainder

	param->ir_pl1_pre = sym | val;

	sym = ir_code_pos_sym(protocol_param->ir_1_code) << 16;
	val = ir_code_pos_val(protocol_param->ir_1_code) * protocol_param->code_bit_length;
	val = val * cr_rate/1000;

	if(sym)
		val = val + 1;//process remainder

	param->ir_pl1_post = sym | val;

	param->ir_ll = pwm_acts_ir_get_len(protocol_param->ir_lc_code);
	param->ir_ld = protocol_param->ir_lc_code;//01b

	if(protocol & PWM_IR_CYCLE_MODE) {
		ld_len = pwm_acts_ir_get_len(protocol_param->ir_trc_loc);
		param->ir_pl = (loc == 1)?(protocol_param->ir_dc_length - ld_len):(ld_len -1);

		param->ir_asc = protocol_param->ir_asc;
		param->ir_Tf = (loc == 1)? 0 : (protocol_param->ir_Tf_length + PWM_IR_TX_MARGIN) * 10;

		val = ir_code_pos_val(protocol_param->ir_0_code) * protocol_param->code_bit_length;
		val = val * cr_rate/1000;

		if(protocol_param->ir_stop_bit && loc != 1)
			param->ir_lc = param->ir_lc - val;

	} else {
		param->ir_pl = protocol_param->ir_dc_length;
		param->ir_asc = protocol_param->ir_asc;
		param->ir_Tf = (protocol_param->ir_Tf_length + PWM_IR_TX_MARGIN) * 10;
	}
	param->ir_stop_bit = protocol_param->ir_stop_bit;


}

static void pwm_acts_ir_param_cal(struct ir_tx_data_param *ctrl, struct pwm_ir_mode_param_t *result, struct ir_tx_protocol_param *protocol_param)
{
	u32_t num_0, num_1, sum_cycle, ir_sl, pwm_rate, mode, ld_len;
	u32_t data[2] = {0};
	struct pwm_ir_mode_param_t param;

	mode = ctrl->mode & PWM_IR_MASK;

	pwm_acts_ir_param_sw(ctrl, &param , 1, protocol_param);

	pwm_rate = pwm_clk_rate/1000/param.ir_period;

	result[0].ir_period = param.ir_period;
	result[0].ir_duty = param.ir_duty;
	result[0].ir_lc = param.ir_lc;
	result[0].ir_pl0_pre = param.ir_pl0_pre;
	result[0].ir_pl0_post = param.ir_pl0_post;
	result[0].ir_pl1_pre = param.ir_pl1_pre;
	result[0].ir_pl1_post = param.ir_pl1_post;
	result[0].ir_ll = param.ir_ll;
	result[0].ir_ld = param.ir_ld;//01b

	if(param.ir_stop_bit) {
		param.ir_pl = param.ir_pl + 1;//patload 24bit+endflag
		ctrl->data[1] = (ctrl->data[1] << 1) | (ctrl->data[0] >> 31);
		ctrl->data[0] = ctrl->data[0] << 1;
	}

	ld_len = pwm_acts_ir_get_len(protocol_param->ir_trc_loc);

	data[0] = ctrl->data[0] >> ld_len;
	data[1] = ctrl->data[1] >> ld_len;

	result[0].ir_pl = param.ir_pl;

	if(param.ir_pl > 32) {
		result[0].ir_pd0 = data[0];
		num_1 = pwm_acts_data_cal(data[0], 32);
		result[0].ir_pd1 = data[1];
		num_1 += pwm_acts_data_cal(data[0], result[0].ir_pl - 32);
	} else {
		result[0].ir_pd0 = data[0];
		num_1 = pwm_acts_data_cal(data[0], result[0].ir_pl);
	}

	num_0 = result[0].ir_pl - num_1;

	sum_cycle = result[0].ir_lc*result[0].ir_ll;
	sum_cycle += num_0*((result[0].ir_pl0_pre&0xff) + (result[0].ir_pl0_post&0xff));
	sum_cycle += num_1*((result[0].ir_pl1_pre&0xff) + (result[0].ir_pl1_post&0xff));

	if(param.ir_Tf) {
		ir_sl = (param.ir_Tf/1000 - (sum_cycle/pwm_rate));

		ir_sl = ir_sl * pwm_rate / result[0].ir_lc;

		result[0].ir_sl = ir_sl;
	} else
		result[0].ir_sl = 0;

	if(ctrl->buf_num > 1 && (ctrl->mode & PWM_IR_CYCLE_MODE))
		result[0].ir_asc = param.ir_asc * 2;
	else if(ctrl->buf_num > 1)
		result[0].ir_asc = param.ir_asc + 1;
	else
		result[0].ir_asc = param.ir_asc;

	if(ctrl->buf_num > 1) {

		if(ctrl->mode & PWM_IR_CYCLE_MODE)
			pwm_acts_ir_param_sw(ctrl, &param, 2, protocol_param);
		else
			pwm_acts_ir_param_sw(ctrl, &param, 2, protocol_param + 1);

		result[1].ir_lc = param.ir_lc;

		if(ctrl->lead == NULL) {
			result[1].ir_ld = param.ir_ld;
			result[1].ir_ll = param.ir_ll;
		} else {
			result[1].ir_ld = ctrl->lead->ld;
			result[1].ir_ll = ctrl->lead->ll;
		}

		if(param.ir_stop_bit)
			param.ir_pl = param.ir_pl + 1;//patload 24bit+endflag

		result[1].ir_pl = param.ir_pl;

		data[0] = data[1] = 0;
		ld_len = (1 << ld_len) - 1;
		data[0] = ctrl->data[0] & ld_len;
		data[1] = ctrl->data[1] & ld_len;

		if(param.ir_pl > 32) {
			result[1].ir_pd0 = data[0];
			num_1 = pwm_acts_data_cal(data[0], 32);
			result[1].ir_pd1 = data[1];
			num_1 += pwm_acts_data_cal(data[0], result[1].ir_pl - 32);
		} else {
			result[1].ir_pd0 = data[0];
			num_1 = pwm_acts_data_cal(data[0], result[1].ir_pl);
		}
		result[1].ir_sl = 0;

		num_0 = result[1].ir_pl - num_1;

		if(ctrl->mode & PWM_IR_CYCLE_MODE)
			sum_cycle += param.ir_lc * result[1].ir_ll;
		else
			sum_cycle = param.ir_lc * result[1].ir_ll;

		sum_cycle += num_0*((param.ir_pl0_pre&0xff) + (param.ir_pl0_post&0xff));
		sum_cycle += num_1*((param.ir_pl1_pre&0xff) + (param.ir_pl1_post&0xff));

		if(param.ir_Tf) {

			ir_sl = (param.ir_Tf/1000 - (sum_cycle/pwm_rate));

			ir_sl = ir_sl * pwm_rate / param.ir_lc;

			result[1].ir_sl = ir_sl;
		}

	}

	result[0].buf_num = ctrl->buf_num;
	result[0].mode = ctrl->mode;

}

static int pwm_acts_ir_transfer(const struct device *dev, u32_t pwm, pwm_ir_ctrl_t *ctrl)
{
	const struct pwm_acts_config *cfg = dev->config;
	struct pwm_acts_data *data = dev->data;
	uint32_t base, group;
	u16_t status;
	struct pwm_ir_mode_param_t result[2] = {0};
	struct ir_tx_protocol_param *ir_protocol = ctrl->protocol;
	struct ir_tx_data_param *ir_data = ctrl->data;

	k_sem_take(&data->ir_transfer_sync, K_FOREVER);

	data->ir_event_timeout = ir_protocol->ir_lc_bit_length;

	pwm_acts_ir_param_cal(ir_data, result, ir_protocol);

	group = pwm_acts_get_group(pwm);

	status = data->group_init_status[group];

	if((status&PWM_MODE_MASK) != PWM_IR_INIT && (status&PWM_MODE_MASK) != PWM_DEFAULT_REG) {//not ir mode or default
		LOG_ERR("start a ir mode but have  not stop this group bfore!");
		return -EINVAL;
	}

	pwm = (pwm > 5)?pwm-6:pwm;
	pwm = (pwm > 5)?pwm-4-group:pwm;

	base = pwm_acts_get_reg_base(cfg->base, group);

	if(group == PWM_GROUP5_REG) {//only group 5 support ir
		pwm_acts_ir_tx(cfg, data, pwm, result, base);
	} else {
		LOG_ERR("unsuported channel: %d",pwm);
		return -EINVAL;
	}

		data->group_init_status[group] = PWM_IR_INIT | PWM_chan(pwm) | status;


	return 0;
}

static int pwm_acts_ir_stop_transfer(const struct device *dev)
{
	const struct pwm_acts_config *cfg = dev->config;
	struct pwm_acts_data *data = dev->data;
	struct acts_pwm_ir *pwm_ir = (struct acts_pwm_ir *)PWM_IR(cfg->base);

	if (pwm_ir->ir_ctl & PWM_IRCTL_START) {
		data->manual_stop_flag = true;
		pwm_ir->ir_ctl |= PWM_IRCTL_STOP;
	}

	k_sem_take(&data->ir_sync, K_FOREVER);
	k_sem_give(&data->ir_transfer_sync);

	return 0;
}

#ifdef CONFIG_PWM_TAI_FULL_FUNC

static int pwm_acts_pin_stop(const struct device *dev, uint32_t pwm)
{

	return 0;
}

#endif

static int pwm_acts_pin_repeat(const struct device *dev, uint32_t pwm, uint8_t repeat)
{
	const struct pwm_acts_config *cfg = dev->config;
	struct pwm_acts_data *data = dev->data;
	uint32_t base,group;
	u16_t status;
	unsigned int key;

	if(repeat == 0)
		return -EINVAL;


	key = irq_lock();

	group = pwm_acts_get_group(pwm);
	status = data->group_init_status[group];

	pwm = (pwm > 5)?pwm-6:pwm;
	pwm = (pwm > 5)?pwm-4-group:pwm;

	base = pwm_acts_get_reg_base(cfg->base, group);

	switch(group){
		case PWM_GROUP0_REG:
			sys_write32(PWM_INTCTL_G0REPEAT, PWM_INT_CTL(cfg->base));
			break;
		case PWM_GROUP1_REG:
			sys_write32(PWM_INTCTL_G1REPEAT, PWM_INT_CTL(cfg->base));
			break;
		case PWM_GROUP2_REG:
			sys_write32(PWM_INTCTL_G2C0 | PWM_INTCTL_G2REPEAT, PWM_INT_CTL(cfg->base));
			break;
		case PWM_GROUP3_REG:
			sys_write32(PWM_INTCTL_G3C0 | PWM_INTCTL_G3REPEAT, PWM_INT_CTL(cfg->base));
			break;
		case PWM_GROUP4_REG:
			sys_write32(PWM_INTCTL_G4C0 | PWM_INTCTL_G4REPEAT, PWM_INT_CTL(cfg->base));
			break;
		case PWM_GROUP5_REG:
			sys_write32(PWM_INTCTL_G5C0 | PWM_INTCTL_G5REPEAT, PWM_INT_CTL(cfg->base));
			break;
		default:
			return -EINVAL;
	}

	struct acts_pwm_groupx *pwm_reg = (struct acts_pwm_groupx *)base;

	pwm_reg->repeat = repeat;
	pwm_reg->ctrl |= PWMx_CTRL_HUA;
//	k_sleep(K_USEC(32));
//	pwm_reg->ctrl |= PWMx_CTRL_CU;

	sys_write32(0xffffffff, PWM_PENDING(cfg->base));

	irq_unlock(key);

	return 0;
}

/*
 * Get the clock rate (cycles per second) for a PWM pin.
 *
 * Parameters
 * dev: Pointer to PWM device structure
 * pwm: PWM port number
 * cycles: Pointer to the memory to store clock rate (cycles per second)
 *
 * return 0, or negative errno code
 */

#ifdef CONFIG_PWM_TAI_FULL_FUNC

static int pwm_acts_get_cycles_per_sec(const struct device *dev, uint32_t pwm,
					u64_t *cycles)
{
	const struct pwm_acts_config *cfg = dev->config;
	struct pwm_acts_data *data = dev->data;
	u32_t group;

	group = pwm_acts_get_group(pwm);

	if(group == PWM_GROUP5_REG)
		*cycles = pwm_clk_rate;
	else
		*cycles = pwm_normal_clk_rate;

	return 0;
}

#endif

static int pwm_acts_reset_peripheral(int reset_id)
{
	sys_write32((sys_read32(RMU_MRCR0) & ~(0x3f << reset_id)), RMU_MRCR0);

	sys_write32(((0x3f << reset_id) | sys_read32(RMU_MRCR0)), RMU_MRCR0);

	return 0;
}

static int pwm_acts_ir_tx_power(const struct device *dev, uint8_t level)
{
	struct pwm_acts_data *data = dev->data;

	if(level > 7) {
		LOG_ERR("error param level:%d", level);
		return -1;
	}

	data->ir_pout = level;

	return 0;
}

#ifdef CONFIG_KEYPAD_SUPPORT_PRO_STUCK_KEY
static void pwm_acts_ir_tx_stop(const struct device *dev)
{
	const struct pwm_acts_config *cfg = dev->config;
	struct pwm_acts_data *data = dev->data;
	struct acts_pwm_ir *pwm_ir = (struct acts_pwm_ir *)PWM_IR(cfg->base);

	if (pwm_ir->ir_ctl & PWM_IRCTL_START) {
		data->manual_stop_flag = true;
		pwm_ir->ir_ctl |= PWM_IRCTL_STOP;
		k_sem_give(&data->ir_sync);
	}
}
#endif

int pwm_acts_init(const struct device *dev)
{
	const struct pwm_acts_config *cfg = dev->config;
	struct pwm_acts_data *data = dev->data;

	acts_pinmux_setup_pins(cfg->pinmux, cfg->pinmux_size - 1);

	/* reset pwm controller */
	pwm_acts_reset_peripheral(cfg->reset_id);

	cfg->irq_config_func();

	for(int i = 0;i < 5;i++) {
		/* enable pwm controller clock */
		acts_clock_peripheral_enable(cfg->clock_id + i);
		/* 32kHZ */
		pwm_acts_set_clk(cfg, i, pwm_normal_clk_rate);
	}

	acts_clock_peripheral_enable(cfg->clock_id + 5);

	pwm_acts_set_clk(cfg, 5, pwm_clk_rate);//pwm_normal_clk_rate);//

	memset(data->group_init_status, 0 ,6);

	k_sem_init(&data->ir_sync, 0, 1);
	k_sem_init(&data->ir_transfer_sync, 0, 1);
	k_sem_give(&data->ir_transfer_sync);

	if (cfg->dma_dev_name != NULL) {

		data->dma_dev = (struct device *)device_get_binding(cfg->dma_dev_name);
		if (!data->dma_dev) {
			LOG_ERR("Bind DMA device %s error", cfg->dma_dev_name);
			return -ENOENT;
		}

		k_sem_init(&data->dma_sync, 0, 1);

		data->dma_chan = dma_request(data->dma_dev, 0xff);
		if(data->dma_chan < 0){
			LOG_ERR("dma-dev rxchan config err chan=%d\n", data->dma_chan);
			return -ENODEV;
		}
	}

	data->ir_pout = 0;
	data->manual_stop_flag = false;
	k_timer_init(&data->timer, pwm_acts_ir_timeout_event, NULL);
	k_timer_user_data_set(&data->timer, (void *)data);

	return 0;
}

const struct pwm_driver_api pwm_acts_driver_api = {
	.pin_set = pwm_acts_pin_set,
	.ir_transfer = pwm_acts_ir_transfer,
	.ir_stop_transfer = pwm_acts_ir_stop_transfer,
	.pin_repeat = pwm_acts_pin_repeat,
	.ir_tx_power_set = pwm_acts_ir_tx_power,
#ifdef CONFIG_PWM_TAI_FULL_FUNC
	.get_cycles_per_sec = pwm_acts_get_cycles_per_sec,
	.set_breath = pwm_acts_set_breath_mode,
	.set_program = pwm_acts_set_program_mode,
	.pin_stop = pwm_acts_pin_stop,
#endif
#ifdef CONFIG_KEYPAD_SUPPORT_PRO_STUCK_KEY
	.ir_tx_stop = pwm_acts_ir_tx_stop,
#endif
};

static struct pwm_acts_data pwm_acts_data;

static const struct acts_pin_config pins_pwm[] = {CONFIG_PWM_MFP};

static void pwm_acts_irq_config(void);

static const struct pwm_acts_config pwm_acts_config = {
	.base = PWM_REG_BASE,
	.cycle = CONFIG_PWM_CYCLE,
	.clock_id = CLOCK_ID_PWM0,
	.reset_id = RESET_ID_PWM0,
	.dma_dev_name = CONFIG_DMA_0_NAME,
	.txdma_id = CONFIG_PWM_DMA_ID,
	.pinmux = pins_pwm,
	.irq_config_func = pwm_acts_irq_config,
	.pinmux_size = ARRAY_SIZE(pins_pwm),

};

#if CONFIG_PWM

static void pwm_acts_pinmux_setup_pins(const struct acts_pin_config *pinconf, int pins, u8_t flag)
{
	int i;

	for (i = 0; i < pins; i++) {
		if(flag)
			acts_pinmux_set(pinconf[i].pin_num, pinconf[i].mode);
		else
			acts_pinmux_set(pinconf[i].pin_num, 0x1000);
	}
}

static int pwm_acts_active(const struct device *dev)
{
	const struct pwm_acts_config *cfg = dev->config;

	pwm_acts_pinmux_setup_pins(cfg->pinmux, cfg->pinmux_size - 1, 1);

	pwm_acts_reset_peripheral(cfg->reset_id);

	cfg->irq_config_func();

	for(int i = 0;i < 5;i++) {
		acts_clock_peripheral_enable(cfg->clock_id + i);
		pwm_acts_set_clk(cfg, i, pwm_normal_clk_rate);
	}

	acts_clock_peripheral_enable(cfg->clock_id + 5);

	pwm_acts_set_clk(cfg, 5, pwm_clk_rate);//pwm_normal_clk_rate);//

	return 0;

}

static int pwm_acts_suspend(const struct device *dev)
{
	struct pwm_acts_data *data = dev->data;
	const struct pwm_acts_config *cfg = dev->config;
	u16_t status;

	for(int i = 0; i < PWM_GROUP_MAX; i++) {
		status = data->group_init_status[i] & PWM_MODE_MASK;
		if((status != PWM_DEFAULT_REG) &&(status != PWM_FIX_INIT)) {
			return -ESRCH;
		}
		status = data->group_init_status[i];
		if((PWM_FIX_INIT == (status & PWM_MODE_MASK))
			&& ((status & PWM_chan_act_MASK) != 0)) {
			return -ESRCH;
		}
	}

	pwm_acts_pinmux_setup_pins(cfg->pinmux, cfg->pinmux_size, 0);

	memset(data->group_init_status, 0 ,6);

	sys_write32((sys_read32(RMU_MRCR0) & ~(0x3f << cfg->reset_id)), RMU_MRCR0);

	for(int i = 0; i < PWM_GROUP_MAX; i++)
		acts_clock_peripheral_disable(cfg->clock_id + i);

	return 0;
}

static int pwm_acts_pm_control(const struct device *dev,
					uint32_t command,
					void *context, device_pm_cb cb, void *arg)
{
	int ret = 0;
	uint32_t state = *((uint32_t*)context);
	static u8_t sleep_status = 1;

	LOG_DBG("command:0x%x state:%d", command, state);

	if (command != DEVICE_PM_SET_POWER_STATE)
		return 0;

	switch (state) {
	case DEVICE_PM_ACTIVE_STATE:
		if(sleep_status == 0)
			pwm_acts_active(dev);
		sleep_status = 1;
		break;
	case DEVICE_PM_SUSPEND_STATE:
		ret = pwm_acts_suspend(dev);
		if(ret == 0)
			sleep_status = 0;
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

DEVICE_DEFINE(pwm_acts, CONFIG_PWM_NAME,
			pwm_acts_init,
			pwm_acts_pm_control,
			&pwm_acts_data, &pwm_acts_config,
			POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
			&pwm_acts_driver_api);

static void pwm_acts_irq_config(void)
{
	IRQ_CONNECT(IRQ_ID_PWM, CONFIG_PWM_IRQ_PRI,
			pwm_acts_isr,
			DEVICE_GET(pwm_acts), 0);
	irq_enable(IRQ_ID_PWM);

	//IRQ_CONNECT(IRQ_ID_KEY_WAKEUP, 1,
	//	mxkeypad_acts_wakeup_isr, DEVICE_GET(mxkeypad_acts), 0);
}
#endif
