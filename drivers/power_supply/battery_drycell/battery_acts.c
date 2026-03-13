/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions SoC battery driver
 */

#include <errno.h>
#include <kernel.h>
#include <drivers/power_supply.h>
#include <soc.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(batadc, CONFIG_LOG_DEFAULT_LEVEL);

#define CONFIG_BATTERY_POLL_INTERVAL_MS (500)
#define CONFIG_BATTERY_REPORT_BY_CAP_PERCENT (5)

//#define CONFIG_BATTERY_AUTOMATIC_REPORT

struct battery_cap {
	uint16_t cap;
	uint16_t volt;
};

struct acts_battery_info {
#ifdef CONFIG_BATTERY_AUTOMATIC_REPORT
	struct k_delayed_work timer;
	bat_charge_callback_t notify;
	uint8_t last_cap_report : 7;
#endif
	uint8_t enable_flag : 1;
};

#define BATTERY_CAP_TABLE_CNT		12
static const struct battery_cap battery_cap_tbl[BATTERY_CAP_TABLE_CNT] = {
	BOARD_BATTERY_CAP_MAPPINGS
};

#define BAT_ADC_SAMPLE_BUF_COUNT 10
typedef struct {
	u16_t  buf[BAT_ADC_SAMPLE_BUF_COUNT];
	u8_t   index;
} bat_adc_sample_t;

static bat_adc_sample_t bat_adc_sample;

static int get_battery_voltage(void)
{
	int index = bat_adc_sample.index;
	int i;
	int adc_count = 0;
	int bat_adc = 0;

	for (i = 0; i < BAT_ADC_SAMPLE_BUF_COUNT; i++) {
		if (index > 0)
			index -= 1;
		else
			index = BAT_ADC_SAMPLE_BUF_COUNT - 1;

		if (bat_adc_sample.buf[index] == 0)
			break;

		bat_adc += bat_adc_sample.buf[index];
		adc_count++;
	}
	if (adc_count)
		bat_adc = ((bat_adc * 10 / adc_count) + 5) / 10;

	LOG_DBG("adc_count: %d, bat_adc: %d", adc_count, bat_adc);

	return bat_adc;
}

static void bat_adc_sample_buf_put(int bat_adc)
{
	bat_adc_sample.buf[bat_adc_sample.index] = (u16_t)bat_adc;
	bat_adc_sample.index += 1;

	if (bat_adc_sample.index >= BAT_ADC_SAMPLE_BUF_COUNT)
		bat_adc_sample.index = 0;
}

static int battery_adcval_to_voltage(int adcval)
{
	int real_adcval;

	if (adcval)
		bat_adc_sample_buf_put(adcval);

	real_adcval = get_battery_voltage();

	return SARADC_IOVCC2VOL(real_adcval);
}

#ifdef CONFIG_BATTERY_AUTOMATIC_REPORT
bool bat_charge_callback(bat_charge_event_t event, bat_charge_event_para_t *para, struct acts_battery_info *bat)
{
	LOG_DBG("event %d notify %p\n", event, bat->notify);

	if (bat->notify) {
		bat->notify(event, para);
	} else {
		return false;
	}

	return true;
}
#endif

/* get IOVCC ADC sampling result */
static int get_saradc_iovcc_data(void)
{
	return acts_get_saradc_data(IOVCC_CH);
}

static int battery_get_voltage(struct acts_battery_info *bat)
{
	int adc_val, volt_uv;

	adc_val = get_saradc_iovcc_data();

	volt_uv = battery_adcval_to_voltage(adc_val);

	return volt_uv;
}

static int battery_get_voltage_now(struct acts_battery_info *bat)
{
	int adc_val, volt_uv;

	adc_val = get_saradc_iovcc_data();

	volt_uv = SARADC_IOVCC2VOL(adc_val);

	return volt_uv;
}

static uint32_t voltage2capacit(uint32_t volt, struct acts_battery_info *bat)
{
	const struct battery_cap *bc, *bc_prev;
	uint32_t  i, cap = 0;

	/* %0 */
	if (volt <= battery_cap_tbl[0].volt)
		return 0;

	/* %100 */
	if (volt >= battery_cap_tbl[BATTERY_CAP_TABLE_CNT - 1].volt)
		return 100;

	for (i = 1; i < BATTERY_CAP_TABLE_CNT; i++) {
		bc = &battery_cap_tbl[i];
		if (volt < bc->volt) {
			bc_prev = &battery_cap_tbl[i - 1];
			/* linear fitting */
			cap = bc_prev->cap + (((bc->cap - bc_prev->cap) *
				(volt - bc_prev->volt)*10 + 5) / (bc->volt - bc_prev->volt)) / 10;
			break;
		}
	}

	return cap;
}

static int battery_get_capacity(struct acts_battery_info *bat)
{
	int volt;

	volt = battery_get_voltage(bat);

	return voltage2capacit(volt, bat);
}

static int battery_get_capacity_now(struct acts_battery_info *bat)
{
	int volt;

	volt = battery_get_voltage_now(bat);

	return voltage2capacit(volt, bat);
}

#ifdef CONFIG_BATTERY_AUTOMATIC_REPORT
void battery_acts_voltage_poll(struct acts_battery_info *bat)
{
	int volt_mv;
	bat_charge_event_para_t para = {0};
	uint32_t cap = 0;

	volt_mv = battery_get_voltage(bat);

	cap = voltage2capacit(volt_mv, bat);

	LOG_DBG("volt_mv:%d cap:%d%% last_cap_report:%d%%",
			volt_mv, cap, bat->last_cap_report);

	if (!bat->last_cap_report) {
		bat->last_cap_report = cap;
	} else {
		if ((bat->last_cap_report - cap)
			<= CONFIG_BATTERY_REPORT_BY_CAP_PERCENT) {
			para.cap = cap;
			para.voltage_val = volt_mv;
			bat_charge_callback(BAT_CHG_EVENT_CAP_CHANGE, &para, bat);
			bat->last_cap_report = cap;
		}
	}
}

static void battery_acts_poll(struct k_work *work)
{
	struct acts_battery_info *bat =
			CONTAINER_OF(work, struct acts_battery_info, timer);

	if (!bat->enable_flag)
		return ;

	battery_acts_voltage_poll(bat);

	k_delayed_work_submit(&bat->timer, K_MSEC(CONFIG_BATTERY_POLL_INTERVAL_MS));
}
#endif

static int battery_acts_get_property(struct device *dev,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	struct acts_battery_info *bat = dev->data;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = 1;
		return 0;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = battery_get_voltage(bat);
		return 0;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = battery_get_capacity(bat);
		return 0;
	case POWER_SUPPLY_PROP_CAPACITY_NOW:
		val->intval = battery_get_capacity_now(bat);
		return 0;
	default:
		return -EINVAL;
	}
}

static void battery_acts_set_property(struct device *dev,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	/* do nothing */
	ARG_UNUSED(dev);
	ARG_UNUSED(psp);
	ARG_UNUSED(val);
}

static void battery_acts_register_notify(struct device *dev, bat_charge_callback_t cb)
{
#ifdef CONFIG_BATTERY_AUTOMATIC_REPORT
	struct acts_battery_info *bat = dev->data;
	int flag;

	flag = irq_lock();

	if ((!bat->notify) && (cb))
		bat->notify = cb;
	else
		LOG_ERR("notify func already exist!\n");

	irq_unlock(flag);
#endif
}

static void battery_acts_enable(struct device *dev)
{
	struct acts_battery_info *bat = dev->data;

	LOG_DBG("enable battery detect");

	if (bat->enable_flag)
		return ;

	uint32_t key = irq_lock();
	soc_saradc_reset();
	soc_saradc_iovcc_enable();
	bat->enable_flag = 1;
	irq_unlock(key);

#ifdef CONFIG_BATTERY_AUTOMATIC_REPORT
	k_delayed_work_submit(&bat->timer, K_MSEC(CONFIG_BATTERY_POLL_INTERVAL_MS));
#endif
}

static void battery_acts_disable(struct device *dev)
{
	struct acts_battery_info *bat = dev->data;

	LOG_DBG("disable battery detect");

	if (!bat->enable_flag)
		return ;

	uint32_t key = irq_lock();
	soc_saradc_iovcc_disable();
	bat->enable_flag = 0;
	irq_unlock(key);

#ifdef CONFIG_BATTERY_AUTOMATIC_REPORT
	k_delayed_work_cancel(&bat->timer);
#endif
}

static const struct power_supply_driver_api battery_acts_driver_api = {
	.get_property = battery_acts_get_property,
	.set_property = battery_acts_set_property,
	.register_notify = battery_acts_register_notify,
	.enable = battery_acts_enable,
	.disable = battery_acts_disable,
};

static int battery_acts_init(const struct device *dev)
{
	struct acts_battery_info *bat = dev->data;

	soc_saradc_iovcc_enable();
	bat->enable_flag = 1;

#ifdef CONFIG_BATTERY_AUTOMATIC_REPORT
	bat->last_cap_report = 0;

	k_delayed_work_init(&bat->timer, battery_acts_poll);
	k_delayed_work_submit(&bat->timer, K_MSEC(CONFIG_BATTERY_POLL_INTERVAL_MS));
#endif

	return 0;
}

static struct acts_battery_info battery_acts_ddata;

#ifdef CONFIG_PM_DEVICE
static int battery_acts_pm_control(const struct device *device,
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
		LOG_DBG("battery active");
		battery_acts_enable((struct device *)device);
		break;
	case DEVICE_PM_SUSPEND_STATE:
		LOG_DBG("battery suspend");
		battery_acts_disable((struct device *)device);
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
#else
#define battery_acts_pm_control NULL
#endif

DEVICE_DEFINE(battery, CONFIG_ACTS_BATTERY_DEV_NAME, battery_acts_init, battery_acts_pm_control,
	    &battery_acts_ddata, NULL, POST_KERNEL,
	    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &battery_acts_driver_api);

