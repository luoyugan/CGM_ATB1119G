/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TP Keyboard driver for Actions SoC
 */
#include <device.h>
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
#if CONFIG_KEYPAD_SUPPORT_PRO_STUCK_KEY
#include <drivers/pwm.h>
#endif
#include <soc.h>
#include <string.h>
#include <drivers/cfg_drv/dev_config.h>

LOG_MODULE_REGISTER(mxkeypad, CONFIG_SYS_LOG_INPUT_DEV_LEVEL);

/*key ctrl0 reg*/

#define KEY_CTRL0_IRP            (0x1 << 31)
#define KEY_CTRL0_IREN           (0x1 << 30)
#define KEY_CTRL0_KOD            (0x1 << 24)
#define KEY_CTRL0_KPDEN          (0x1 << 23)
#define KEY_CTRL0_KPUEN          (0x1 << 22)
#define KEY_CTRL0_KIE            (0x1 << 21)
#define KEY_CTRL0_KOE            (0x1 << 20)
#define KEY_CTRL0_PENM(X)        (X << 4)
#define KEY_CTRL0_KRS(X)         (X << 2)
#define KEY_CTRL0_KMS            (1 << 1)
#define KEY_CTRL0_KEN            (1 << 0)

/*key ctrl1 reg*/

#define KEY_CTRL0_KST(X)         (X << 16)
#define KEY_CTRL0_DTS(X)         (X << 0)

/* mxkeypad controller */
struct mxkeypad_acts_controller {
	volatile uint32_t ctl0;
	volatile uint32_t ctl1;
	volatile uint32_t reserve[2];
	volatile uint32_t info0;
	volatile uint32_t info1;
	volatile uint32_t info2;
	volatile uint32_t info3;
	volatile uint32_t info4;
	volatile uint32_t info5;
	volatile uint32_t info6;
};

struct mxkeypad_map {
	u32_t scan_key[7];
	u16_t key_code;
};

struct acts_mxkeypad_data {
	input_notify_t notify;
	u16_t prev_stable_keycode;

#if CONFIG_KEYPAD_SUPPORT_PRO_STUCK_KEY
	u32_t keyinfo_data[7];
#endif
};

struct acts_mxkeypad_config {
	struct mxkeypad_acts_controller *base;
	void (*irq_config_func)(void);
	uint8_t clock_id;
	uint8_t reset_id;
	uint16_t penm;
	uint8_t pinmux_size;
	const struct acts_pin_config *pinmux;
	u16_t key_cnt;
	const struct mxkeypad_map *key_maps;
};

static const struct mxkeypad_map acts_mxkeypad_maps[] = {
CONFIG_KEYPAD_PIN_MAP
};

static struct acts_mxkeypad_data mxkeypad_acts_ddata;

static const struct acts_pin_config pins_mxkeypad[] = {
CONFIG_KEYPAD_MFP
};

static void mxkeypad_acts_irq_config(void);

static const struct acts_mxkeypad_config mxkeypad_acts_cdata = {
	.base = (struct mxkeypad_acts_controller *)KEY_REG_BASE,
	.irq_config_func = mxkeypad_acts_irq_config,
	.pinmux = pins_mxkeypad,
	.pinmux_size = ARRAY_SIZE(pins_mxkeypad),
	.penm = CONFIG_KEYPAD_PENM,
	.clock_id = CLOCK_ID_KEY,
	.reset_id = RESET_ID_KEY,
	.key_cnt = ARRAY_SIZE(acts_mxkeypad_maps),
	.key_maps = &acts_mxkeypad_maps[0],
};

 
#define GPIO_MAX_NUM	43
static const uint8_t key_pad_nb[GPIO_MAX_NUM] = {
	0, 1, 13, 14, 4, 12, 11, 7, 8, 9, 
	0xff, 0xff,
	5, 6, 
	0xff, 0xff, 0xff, 0xff, 
	10, 11, 3, 2, 
	9, 15, 0, 11,
	15, 1, 12, 3, 
	0xff,
	4, 9,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
  9, 10, 2
};

static int key_is_out(uint32_t key_num)
{
	if (key_num < 8)
		return 0;
	else 
		return -1;
}

void mxkeypad_acts_poweroff(struct device *dev)
{
	const struct acts_mxkeypad_config *cfg = dev->config;
	struct mxkeypad_acts_controller *keyctrl = cfg->base;
	//struct device *gpio_dev;
	uint8_t gpio_num;
	int i;

	/* disable key clock */
	acts_clock_peripheral_disable(cfg->clock_id);
	
	/* disable key module */
	keyctrl->ctl0 = 0;

	//gpio_dev = (struct device *)device_get_binding(CONFIG_GPIO_ACTS_DEV_NAME);
	/* configure gpio of key */
	for (i=0; i<cfg->pinmux_size; i++) {
		gpio_num = cfg->pinmux[i].pin_num;
		if (key_is_out(key_pad_nb[gpio_num]) == 0) {
			/* K0-7  OUT PD10K */
			sys_write32(GPIO_CTL_PULLDOWN_WEAK, GPIO_REG_CTL(GPIO_REG_BASE, gpio_num));
		} else {
			/* K8-15 IN  PU100K falling edge wakeup */
			sys_write32(GPIO_CTL_INTC_MASK | GPIO_CTL_INC_TRIGGER(3) | GPIO_CTL_INTC_EN | GPIO_CTL_PULLUP | GPIO_CTL_GPIO_INEN, GPIO_REG_CTL(GPIO_REG_BASE, gpio_num));
		}
		LOG_DBG("[0x%x]=0x%x\n", GPIO_REG_CTL(GPIO_REG_BASE, i), sys_read32(GPIO_REG_CTL(GPIO_REG_BASE, gpio_num)));
	}
}

static void mxkeypad_acts_report_key( struct acts_mxkeypad_data *keypad,
					 int key_code, int value)
{
	struct input_value val;

	if (keypad->notify) {
		val.keypad.type = EV_KEY;
		val.keypad.code = key_code;
		val.keypad.value = value;

		LOG_DBG("type:0x%x, code:0x%x, value:0x%x\n", val.keypad.type, val.keypad.code, val.keypad.value);

		if(keypad->notify)
			keypad->notify(NULL, &val);
	}
}

#if CONFIG_KEYPAD_SUPPORT_PRO_STUCK_KEY
__unused static u16_t mxkeypad_acts_get_keycode(const struct acts_mxkeypad_config *cfg, struct mxkeypad_acts_controller *keyctrl)
#else
static u16_t mxkeypad_acts_get_keycode(const struct acts_mxkeypad_config *cfg, struct mxkeypad_acts_controller *keyctrl)
#endif
{
	const struct mxkeypad_map *map = cfg->key_maps;

	for(int i = 0; i < cfg->key_cnt; i++) {
		if((map->scan_key[0] == keyctrl->info0) &&
			(map->scan_key[1] == keyctrl->info1) &&
			(map->scan_key[2] == keyctrl->info2) &&
			(map->scan_key[3] == keyctrl->info3) &&
			(map->scan_key[4] == keyctrl->info4) &&
			(map->scan_key[5] == keyctrl->info5) &&
			(map->scan_key[6] == keyctrl->info6))
			return map->key_code;

		map++;
	}

	return KEY_RESERVED;
}

#if CONFIG_KEYPAD_SUPPORT_PRO_STUCK_KEY
#define KEY_INFO_NUM 7
#define FILTER_MODE0 0
#define FILTER_MODE1 1
#define DEBUG_KEYPAD_INFO 1
static u8_t cur_keyinfo_index=0xff;
static u16_t stuck_key_count=0;
static u32_t stuck_keyinfo_data[KEY_INFO_NUM];
u8_t cur_not_zero_bit_cnt;
bool all_key_up=false;
struct k_delayed_work acts_key_stuck_detect_work;

static u16_t mxkeypad_acts_pro_stuck_keycode(struct acts_mxkeypad_data *keypad, const struct acts_mxkeypad_config *cfg)
{
	const struct mxkeypad_map *map = cfg->key_maps;
	for(int i = 0; i < cfg->key_cnt; i++) {
		if((map->scan_key[0] == keypad->keyinfo_data[0]) &&
			(map->scan_key[1] == keypad->keyinfo_data[1]) &&
			(map->scan_key[2] == keypad->keyinfo_data[2]) &&
			(map->scan_key[3] == keypad->keyinfo_data[3]) &&
			(map->scan_key[4] == keypad->keyinfo_data[4]) &&
			(map->scan_key[5] == keypad->keyinfo_data[5]) &&
			(map->scan_key[6] == keypad->keyinfo_data[6]))
			return map->key_code;

		map++;
	}

	return KEY_RESERVED;
}

static u8_t mxkeypad_acts_get_notzero_bit(uint32_t reg_data)
{
	u8_t i, bit_dat=0;
	for(i=0; i<32; i++) {
		bit_dat = (reg_data & 0x01);
		if(bit_dat)
			break;
		reg_data=(reg_data>>1);
	}

	return i;
}

static u8_t mxkeypad_acts_count_notzero_bit(uint32_t reg_data)
{
	u8_t i, no_zero_bit_cnt=0;
	uint32_t bit_dat;
	for(i=0; i<32; i++) {
		bit_dat = (reg_data & 0x01);
		if(bit_dat) {
			no_zero_bit_cnt++;
		}
		reg_data=(reg_data>>1);
	}

	return no_zero_bit_cnt;
}

static u8_t mxkeypad_acts_get_low_bit_index(uint32_t *reg_data)
{
	u8_t i;
	uint32_t bit_dat, tmp_dat;
	tmp_dat=*reg_data;
	for(i=0; i<32; i++) {
		bit_dat = (tmp_dat & 0x01);
		if(bit_dat)
			break;
		tmp_dat=(tmp_dat>>1);
	}

	return i;
}

#if DEBUG_KEYPAD_INFO
static void mxkeypad_acts_get_cur_keycoord(struct acts_mxkeypad_data *keypad, u8_t keyinfo_index)
{
	u8_t cur_k_num=0;
	u8_t cur_p_num=0;
	u8_t not_zero_bit;

	not_zero_bit=mxkeypad_acts_get_notzero_bit(keypad->keyinfo_data[keyinfo_index]);
	LOG_DBG("cur_not_zero_bit:%d\n", not_zero_bit);

	if (0<=keyinfo_index && keyinfo_index<=3) { /* info0~3 */
		cur_p_num=not_zero_bit%16;
		if (not_zero_bit<=15) {
			cur_k_num=keyinfo_index*2;
		} else {
			cur_k_num=keyinfo_index*2+1;
		}
	} else if (keyinfo_index==4||keyinfo_index==5) { /* info4~5 */
		cur_p_num=not_zero_bit%8+8;
		if (not_zero_bit<=7) {
			cur_k_num=4*keyinfo_index-8;
		} else if (8<=not_zero_bit && not_zero_bit<=15) {
			cur_k_num=4*keyinfo_index-7;
		} else if (16<=not_zero_bit && not_zero_bit<=23) {
			cur_k_num=4*keyinfo_index-6;
		} else if (24<=not_zero_bit && not_zero_bit<=31) {
			cur_k_num=4*keyinfo_index-5;
		}
	} else if (keyinfo_index==6) { /* info6 */
		cur_p_num=not_zero_bit%16;
		if (not_zero_bit<=15) {
			cur_k_num=keyinfo_index*2+4;
		} else {
			cur_k_num=keyinfo_index*2+5;
		}
	}

	printk("cur_down_k_coord:k%d, p%d\n\n", cur_k_num, cur_p_num);
}
#endif

static void mxkeypad_acts_expiry_function(struct k_work *timer)
{
		u8_t i;
		struct device *dev = (struct device *)device_get_binding(CONFIG_KEYPAD_NAME);
		const struct acts_mxkeypad_config *cfg = dev->config;
		struct mxkeypad_acts_controller *keyctrl = cfg->base;
		stuck_keyinfo_data[0]=keyctrl->info0;
		stuck_keyinfo_data[1]=keyctrl->info1;
		stuck_keyinfo_data[2]=keyctrl->info2;
		stuck_keyinfo_data[3]=keyctrl->info3;
		stuck_keyinfo_data[4]=keyctrl->info4;
		stuck_keyinfo_data[5]=keyctrl->info5;
		stuck_keyinfo_data[6]=keyctrl->info6;
		stuck_key_count=0;

		for (i=0; i<KEY_INFO_NUM; i++) {
			stuck_key_count += mxkeypad_acts_count_notzero_bit(stuck_keyinfo_data[i]);
		}

#if DEBUG_KEYPAD_INFO
		for (i=0; i<KEY_INFO_NUM; i++) {
			printk("stuck_keyinfo_data[%d]:0x%x\n", i, stuck_keyinfo_data[i]);
		}

		printk("save stuck-key info!\n");
		printk("stuck_key_count:0x%x\n\n", stuck_key_count);
#endif

		/* stop ir tx */
		struct device * pwm_ir_dev=(struct device *)device_get_binding(CONFIG_PWM_NAME);
		pwm_ir_tx_stop(pwm_ir_dev);

		/* turn off led */
		struct device * gpio_led_dev=(struct device *)device_get_binding(CONFIG_GPIO_ACTS_DEV_NAME);
		gpio_pin_set(gpio_led_dev, CONFIG_GPIO_LED_PIN, CONFIG_GPIO_LED_OFF);
}

void mxkeypad_acts_filter_stuck_key(u8_t mode, u32_t *stuck_key, u32_t *cur_key)
{
	u8_t cur_nozero_bit=0;

	switch (mode) {
		case FILTER_MODE0:
			for (u8_t i=0; i<KEY_INFO_NUM; i++) {
				cur_key[i]=stuck_key[i]^cur_key[i];
			}
		break;

		case FILTER_MODE1:
			cur_nozero_bit=mxkeypad_acts_get_low_bit_index(cur_key);
			LOG_DBG("cur_nozero_bit:%d", cur_nozero_bit);
			*cur_key &= (1<<cur_nozero_bit);
		break;

		default:
			break;
	}
}

static bool mxkeypad_acts_detect_stuck_key_up(u32_t *stuck_key, u32_t *cur_key)
{
	u8_t i, j=0;
	for (i=0; i<KEY_INFO_NUM; i++) {
			cur_key[i]=stuck_key[i]&cur_key[i];
			if (cur_key[i]!=0x00) {
				for (j=0; j<KEY_INFO_NUM; j++) {
					cur_key[j]=0x00;
				}
				return true;
			}
	}

	return false;
}

void mxkeypad_acts_process_key_stuck(struct acts_mxkeypad_data *keypad, struct mxkeypad_acts_controller *keyctrl)
{
	u8_t i;
	u8_t cur_not_zero_bit=0;
	u32_t data_tmp;

	keypad->keyinfo_data[0]=keyctrl->info0;
	keypad->keyinfo_data[1]=keyctrl->info1;
	keypad->keyinfo_data[2]=keyctrl->info2;
	keypad->keyinfo_data[3]=keyctrl->info3;
	keypad->keyinfo_data[4]=keyctrl->info4;
	keypad->keyinfo_data[5]=keyctrl->info5;
	keypad->keyinfo_data[6]=keyctrl->info6;

	for (i=0; i<KEY_INFO_NUM; i++) {
			cur_not_zero_bit += mxkeypad_acts_count_notzero_bit(keypad->keyinfo_data[i]);
	}

	if(!all_key_up) {
		k_delayed_work_submit(&acts_key_stuck_detect_work, K_SECONDS(CONFIG_STUCK_KEY_LONG_TIMEOUT));
		printk("cur_not_zero_bit:%d, stuck_key_count:%d\n", cur_not_zero_bit, stuck_key_count);
	}

	if (stuck_key_count) {
#if DEBUG_KEYPAD_INFO
		for (i=0; i<KEY_INFO_NUM; i++) {
			printk("current->keyinfo_data[%d]:0x%x\n", i, keypad->keyinfo_data[i]);
		}
		printk("\n\n");
#endif
		if(cur_not_zero_bit < stuck_key_count) {
			if(stuck_key_count>1)  {
				if(mxkeypad_acts_detect_stuck_key_up(stuck_keyinfo_data, keypad->keyinfo_data)){
					printk("stuck key up\n");
					stuck_key_count=0;
					k_delayed_work_cancel(&acts_key_stuck_detect_work);
					k_delayed_work_submit(&acts_key_stuck_detect_work, K_SECONDS(CONFIG_STUCK_KEY_SHORT_TIME_OUT));
					return;
				}
			}
		}

		mxkeypad_acts_filter_stuck_key(FILTER_MODE0, stuck_keyinfo_data, keypad->keyinfo_data);

		/* parse the filter results */
		for (i=0; i<KEY_INFO_NUM; i++) {
			if (keypad->keyinfo_data[i]!=0) { /* save low bit */
				cur_keyinfo_index=i;
				data_tmp=keypad->keyinfo_data[i];
				break;
			}
		}

		printk("cur_keyinfo_index:%d\n", cur_keyinfo_index);
		for (i=0; i<KEY_INFO_NUM; i++) {
			keypad->keyinfo_data[i]=0;
		}
		keypad->keyinfo_data[cur_keyinfo_index]=data_tmp;

		cur_not_zero_bit_cnt = mxkeypad_acts_count_notzero_bit(keypad->keyinfo_data[cur_keyinfo_index]);
		printk("cur_not_zero_bit_cnt:%d\n", cur_not_zero_bit_cnt);

		if (cur_not_zero_bit_cnt==1) {
#if DEBUG_KEYPAD_INFO
			mxkeypad_acts_get_cur_keycoord(keypad, cur_keyinfo_index);
#endif
		} else if (cur_not_zero_bit_cnt>1) {	/* save low bit */
			mxkeypad_acts_filter_stuck_key(FILTER_MODE1, &stuck_keyinfo_data[cur_keyinfo_index], &keypad->keyinfo_data[cur_keyinfo_index]);
#if DEBUG_KEYPAD_INFO
			mxkeypad_acts_get_cur_keycoord(keypad, cur_keyinfo_index);
#endif
		}

#if DEBUG_KEYPAD_INFO
		printk("after filter:\n");
		for (i=0; i<KEY_INFO_NUM; i++) {
			printk("filter->keyinfo_data[%d]:0x%x\n", i, keypad->keyinfo_data[i]);
		}
		printk("\n\n");
#endif
	}
}
#endif

static void mxkeypad_acts_scan_key(const struct acts_mxkeypad_config *cfg,
					 struct acts_mxkeypad_data *keypad)
{
	struct mxkeypad_acts_controller *keyctrl = cfg->base;
	u16_t key_code;

#if CONFIG_KEYPAD_SUPPORT_PRO_STUCK_KEY
	mxkeypad_acts_process_key_stuck(keypad, keyctrl);
	key_code = mxkeypad_acts_pro_stuck_keycode(keypad, cfg);
#else
	key_code = mxkeypad_acts_get_keycode(cfg, keyctrl);
#endif

	/* previous key is released? */
	if (keypad->prev_stable_keycode != KEY_RESERVED) {
		if (key_code != keypad->prev_stable_keycode) {
			mxkeypad_acts_report_key(keypad, keypad->prev_stable_keycode, 0);
		}
	}

	/* a new key press? */
	if (key_code != KEY_RESERVED)
		mxkeypad_acts_report_key(keypad, key_code, 1);

	keypad->prev_stable_keycode = key_code;

}

void mxkeypad_acts_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct acts_mxkeypad_data *mxkeypad = dev->data;
	const struct acts_mxkeypad_config *cfg = dev->config;
	struct mxkeypad_acts_controller *keyctrl = cfg->base;

#if CONFIG_KEYPAD_REG

	printk("info0:0x%x, info1:0x%x, info2:0x%x, info3:0x%x, info4:0x%x, info5:0x%x, info6:0x%x\n",
			keyctrl->info0, keyctrl->info1, keyctrl->info2, keyctrl->info3, keyctrl->info4, keyctrl->info5, keyctrl->info6);

#endif

#if CONFIG_KEYPAD_SUPPORT_PRO_STUCK_KEY
	all_key_up=false;
	if (keyctrl->info0==keyctrl->info1&&keyctrl->info1==keyctrl->info2&&keyctrl->info2==keyctrl->info3\
	   &&keyctrl->info3==keyctrl->info4&&keyctrl->info4==keyctrl->info5&&keyctrl->info5==keyctrl->info6\
	   &&keyctrl->info6==0x00) {
		k_delayed_work_cancel(&acts_key_stuck_detect_work);
		stuck_key_count=0;
		all_key_up=true;
		printk("all key up\n");
	}
#endif

	/* disable irq */
	keyctrl->ctl0 &= ~KEY_CTRL0_IREN;

	/* scan key */
	mxkeypad_acts_scan_key(cfg, mxkeypad);

	keyctrl->ctl0 |= KEY_CTRL0_IREN | KEY_CTRL0_IRP;


}

static void mxkeypad_acts_set_clk(const struct acts_mxkeypad_config *cfg, uint32_t freq_khz)
{
#if 1
	clk_set_rate(cfg->clock_id, freq_khz);
	k_busy_wait(100);
#endif
}

static void mxkeypad_acts_enable(const struct device *dev)
{
	const struct acts_mxkeypad_config *cfg = dev->config;
	struct mxkeypad_acts_controller *keyctrl = cfg->base;

	keyctrl->ctl1 = KEY_CTRL0_KST(0x140) | KEY_CTRL0_DTS(0x280);//time=ClockTime*count  1/32K*0x280=19.5ms
	keyctrl->ctl0 = KEY_CTRL0_IREN | KEY_CTRL0_KPDEN | KEY_CTRL0_PENM(cfg->penm) | KEY_CTRL0_KRS(3) | KEY_CTRL0_KEN;

#if CONFIG_KEYPAD_KMS
	keyctrl->ctl0 |= KEY_CTRL0_KMS;
#endif

#if CONFIG_KEYPAD_SUPPORT_PRO_STUCK_KEY
	k_delayed_work_init(&acts_key_stuck_detect_work, mxkeypad_acts_expiry_function);
#endif

	LOG_DBG("enable mxkeypad");

}

static void mxkeypad_acts_disable(const struct device *dev)
{
	const struct acts_mxkeypad_config *cfg = dev->config;
	struct mxkeypad_acts_controller *keyctrl = cfg->base;

	keyctrl->ctl1 = 0;
	keyctrl->ctl0 = 0;

	LOG_DBG("disable mxkeypad");

}

static void mxkeypad_acts_inquiry(const struct device *dev, struct input_value *val)
{

	LOG_DBG("inquiry mxkeypad");

}

static void mxkeypad_acts_register_notify(const struct device *dev, input_notify_t notify)
{
	struct acts_mxkeypad_data *mxkeypad = dev->data;

	LOG_DBG("register notify 0x%x", (uint32_t)notify);

	mxkeypad->notify = notify;

}

static void mxkeypad_acts_unregister_notify(const struct device *dev, input_notify_t notify)
{
	struct acts_mxkeypad_data *mxkeypad = dev->data;

	LOG_DBG("unregister notify 0x%x", (uint32_t)notify);

	mxkeypad->notify = NULL;

}

const struct input_dev_driver_api mxkeypad_acts_driver_api = {
	.enable = mxkeypad_acts_enable,
	.disable = mxkeypad_acts_disable,
	.inquiry = mxkeypad_acts_inquiry,
	.register_notify = mxkeypad_acts_register_notify,
	.unregister_notify = mxkeypad_acts_unregister_notify,

};

int mxkeypad_acts_init(const struct device *dev)
{
	struct acts_mxkeypad_data *mxkeypad = dev->data;
	const struct acts_mxkeypad_config *cfg = dev->config;

	acts_pinmux_setup_pins(cfg->pinmux, cfg->pinmux_size);

	mxkeypad->prev_stable_keycode = KEY_RESERVED;

	mxkeypad_acts_set_clk(cfg, 32);

	/* enable key controller clock */
	acts_clock_peripheral_enable(cfg->clock_id);

	/* reset key controller */
	acts_reset_peripheral(cfg->reset_id);

	cfg->irq_config_func();

	return 0;

}

#if CONFIG_KEYPAD

static int mxkeypad_acts_pm_control(const struct device *device,
					uint32_t command,
					void *context, device_pm_cb cb, void *arg)
{
	int ret = 0;
	uint32_t state = *((uint32_t*)context);

	LOG_DBG("command:0x%x state:%d", command, state);

	if (command != DEVICE_PM_SET_POWER_STATE)
		return 0;

	switch (state) {
	case DEVICE_PM_ACTIVE_STATE:
	case DEVICE_PM_SUSPEND_STATE:
	case DEVICE_PM_EARLY_SUSPEND_STATE:
	case DEVICE_PM_LATE_RESUME_STATE:
	case DEVICE_PM_LOW_POWER_STATE:
		break;
	case DEVICE_PM_OFF_STATE:
		mxkeypad_acts_poweroff((struct device *)device);
		break;
	default:
		ret = -ESRCH;
	}

	return ret;
}

DEVICE_DEFINE(mxkeypad, CONFIG_KEYPAD_NAME,
		    mxkeypad_acts_init,
		    mxkeypad_acts_pm_control,
		    &mxkeypad_acts_ddata, &mxkeypad_acts_cdata,
		    POST_KERNEL, 60,
		    &mxkeypad_acts_driver_api);

static void mxkeypad_acts_irq_config(void)
{
	IRQ_CONNECT(IRQ_ID_KEY, CONFIG_KEYPAD_IRQ_PRI,
			mxkeypad_acts_isr,
			DEVICE_GET(mxkeypad), 0);
	irq_enable(IRQ_ID_KEY);
}

#endif // DT_NODE_HAS_STATUS
