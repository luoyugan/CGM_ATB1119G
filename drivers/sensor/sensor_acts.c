/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions SoC sensor-temperature driver
 */

#include <errno.h>

#include <kernel.h>

#include <drivers/sensor.h>
#include <soc.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(sensadc, CONFIG_LOG_DEFAULT_LEVEL);

#define SENSOR_CMP_THRESH_CELS       3
#define SENSOR_CMPEN_MIN_EFUSE_VREF  9
#define SENSOR_CMPEN_MAX_EFUSE_VREF  24

/* when big bias of sensor measurement occurs, 
 * need measure three times and get the optimal sensor value.
 * this is the X10 gear, 50 means 5 celsius degree.
 */
#define SENSOR_BIAS_PEAK_X10         (50)

/* sensor measurement interval in S1.
 * after measurement if it reaches SENSOR_CMP_THRESH_CELS, will compense.
 */
#define SENSOR_CMPEN_MEAS_INTV_S     (1*60)

struct acts_sensor_info {
	struct k_delayed_work timer;
	int32_t temperature;
	uint8_t efuse_offset;
	uint8_t efuse_vref;
	uint8_t v06_ref;

	uint8_t enable_flag : 1;
};

static struct acts_sensor_info sensor_acts_ddata;

const int temp_fix_array[16][34] = {
{-9, -8, -8, -7, -6, -5, -4, -4, -3, -3, -2, -2, -1, 0, 1, 1, 1, 2, 2, 3, 3, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7},
{-9, -8, -8, -7, -6, -5, -4, -4, -3, -3, -2, -2, -1, 0, 1, 1, 1, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 6},
{-7, -6, -6, -5, -5, -4, -3, -3, -2, -2, -1, -1,  0, 0, 0, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 5, 5, 6, 6, 6},
{-6, -6, -5, -5, -4, -4, -3, -3, -2, -2, -1, -1,  0, 0, 0, 1, 1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6},	
{-6, -6, -5, -5, -4, -4, -3, -3, -2, -2, -1, -1,  0, 0, 0, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6},
{-6, -6, -5, -5, -4, -4, -3, -3, -2, -2, -1, -1,  0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5},	
{-6, -5, -5, -4, -4, -4, -3, -3, -2, -2, -1, -1,  0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5},
{-5, -5, -4, -4, -4, -3, -3, -2, -2, -1, -1, -1,  0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4},	
{-5, -4, -4, -4, -3, -3, -2, -2, -2, -1, -1, -1,  0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4},
{-4, -4, -4, -3, -3, -2, -2, -2, -1, -1, -1, -1,  0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3},
{-4, -4, -3, -3, -2, -2, -2, -1, -1, -1, -1,  0,  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3},
{-4, -4, -3, -3, -2, -2, -2, -1, -1, -1, -1,  0,  0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},	
{-3, -2, -2, -2, -2, -1, -1, -1, -1, -1,  0,  0,  0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2},
{-2, -2, -1, -1, -1, -1, -1, -1, -1,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
{-1, -1, -1, -1, -1, -1,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
{-1, -1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
};//9~24

static int get_decimal_part(int x) 
{
	if(x < 0) 
		return -x%10;
	else 
		return x%10;
}

static void insert_sort(int array[], int size)
{
	for(int i = 1; i < size; i++){
		if(array[i] < array[i - 1]){
			int temp = array[i];
			int j = i;

			while(j >= 1 && temp < array[j - 1]){
				array[j] = array[j-1];
				j--;
			}

			array[j] = temp;
        }
    }
}

static int sensor_adcval_to_temp(uint16_t adcval, uint8_t offset)
{
	int temp_cels = 0;

	temp_cels = 4400 - ((adcval*1730) >> 14);
	if(offset & 0x10)
		temp_cels -= 10*(0x20 - offset);
	else
		temp_cels += 10*offset;		

	return temp_cels;
}

/* get sensor ADC sampling result */
static int get_saradc_sensor_data(void)
{
	return acts_get_saradc_data(SENSOR_CH);
}

#define MAX_SENSOR_MEAS_TIMES_IN_PEAK_CASE 3
#define TEMP_ABSOLUTE_ZERO                 (-2740)
static int sensor_get_temperature(struct acts_sensor_info *sens)
{
	int adc_val;
	int temp_c;
	static int temp_last_val = TEMP_ABSOLUTE_ZERO;
	int val[MAX_SENSOR_MEAS_TIMES_IN_PEAK_CASE];

	adc_val = get_saradc_sensor_data();

	temp_c = sensor_adcval_to_temp(adc_val, sens->efuse_offset);

	if(((temp_c > (temp_last_val + SENSOR_BIAS_PEAK_X10)) \
		|| (temp_c < (temp_last_val - SENSOR_BIAS_PEAK_X10))) \
		&& (temp_last_val != TEMP_ABSOLUTE_ZERO)){
		LOG_DBG("sensor sample bias, adc_val=0x%x, temp_last=%d, temp_c=%d, efuse_offset=%d\n", adc_val, temp_last_val, temp_c, sens->efuse_offset);
		for(int i = 0; i < MAX_SENSOR_MEAS_TIMES_IN_PEAK_CASE; i++){
			val[i] = get_saradc_sensor_data();
		}

		insert_sort(val, MAX_SENSOR_MEAS_TIMES_IN_PEAK_CASE);
		adc_val = val[1];/* take the middle value */
		temp_c = sensor_adcval_to_temp(adc_val, sens->efuse_offset);
		LOG_DBG("adc_val=0x%x, temp_optimal=%d, efuse_offset=%d\n", adc_val, temp_c, sens->efuse_offset);
	}

	temp_last_val = temp_c;	

	return temp_c;
}

static int sensor_acts_get_property(struct device *dev,
				       enum sensor_property psp,
				       union sensor_propval *val)
{
	struct acts_sensor_info *sens = dev->data;

	switch (psp) {
	case SENSOR_PROP_TEMPERATURE:
		val->intval = sensor_get_temperature(sens);
		return 0;
	default:
		return -EINVAL;
	}
}

static void sensor_acts_enable(struct device *dev)
{
	struct acts_sensor_info *sens = dev->data;

	LOG_DBG("enable sensor detect");

	if (sens->enable_flag)
		return ;

	uint32_t key = irq_lock();
	soc_saradc_sensor_enable();
	sens->enable_flag = 1;
	irq_unlock(key);
}

static void sensor_acts_disable(struct device *dev)
{
	struct acts_sensor_info *sens = dev->data;

	LOG_DBG("disable sensor detect");

	if (!sens->enable_flag)
		return ;

	uint32_t key = irq_lock();
	soc_saradc_sensor_disable();
	sens->enable_flag = 0;
	irq_unlock(key);
}

#if CONFIG_SENSOR_AUTO_CMPEN
static void sensor_acts_compens_event(struct acts_sensor_info *sens)
{
	uint32_t count;
	uint32_t v06_origin = sens->efuse_vref;
	uint32_t v06 = sens->v06_ref;
	int temp_x10, v06_diff[5], temp_diff;

	if (!sens->enable_flag)
		return ;

	#if CONFIG_SENSOR_COMPARE_RESULT
	sys_write32((sys_read32(VREF_CTL) & (~VREF_CTL_VREF06_VOL_MASK)) | v06_origin, VREF_CTL);
	k_busy_wait(300);
	temp_x10 = sensor_get_temperature(sens);
	LOG_INF("no compensation, current temperature: %d.%d, v6=%d\n", temp_x10/10, get_decimal_part(temp_x10), v06_origin);

	sys_write32((sys_read32(VREF_CTL) & (~VREF_CTL_VREF06_VOL_MASK)) | v06, VREF_CTL);
	k_busy_wait(300);
	#endif
	/* check whether current temperature reaches the threshhold */
	temp_x10 = sensor_get_temperature(sens);
	#if CONFIG_SENSOR_COMPARE_RESULT
	LOG_INF("compensation, current temperature: %d.%d, v6=%d\n", temp_x10/10, get_decimal_part(temp_x10), v06);
	#else
	LOG_DBG("C-temp: %d.%d, v6=%d, v6_efuse=%d\n", temp_x10/10, get_decimal_part(temp_x10), v06, v06_origin);
	#endif

	if((temp_x10 < (sens->temperature + SENSOR_CMP_THRESH_CELS*10)) \
		&& (temp_x10 > (sens->temperature - SENSOR_CMP_THRESH_CELS*10))){	
		return ;
	}

	acts_clock_rc64m_calibrate();

	/* in condition of (efuse_vref > SENSOR_CMPEN_MAX_EFUSE_VREF), no need compensation */
	if(sens->efuse_vref > SENSOR_CMPEN_MAX_EFUSE_VREF)
		return ;

	/* temperature compensation */
	count = 0;
	while(count < 5)
	{
		temp_x10 = sensor_get_temperature(sens);
		temp_diff = (temp_x10/10 + 42)/5;
		if(temp_diff > 33)
			temp_diff = 33;
		if(temp_diff < 0)
			temp_diff = 0;

		v06_diff[count] = temp_fix_array[(sens->efuse_vref - SENSOR_CMPEN_MIN_EFUSE_VREF)][temp_diff];
		LOG_DBG("v06diff %d,count %d\n", v06_diff[count], count);
		v06 = sens->efuse_vref + v06_diff[count];
		sys_write32((sys_read32(VREF_CTL) & (~VREF_CTL_VREF06_VOL_MASK)) | v06, VREF_CTL);
		sens->v06_ref = (uint8_t)v06;
		LOG_DBG("VREF_CTL new 0x%x.\n", sys_read32(VREF_CTL));
		k_busy_wait(300);

		if(count >= 1)
		{
			if(v06_diff[count] == v06_diff[count - 1])
				count = 4;
		}

		count++;
	}

	/* update temperature for next threshhold compare */
	sens->temperature = sensor_get_temperature(sens);

	LOG_DBG("C-temp: %d.%d, v6=%d, v6_efuse=%d, compens\n", sens->temperature/10, get_decimal_part(sens->temperature), sens->v06_ref, v06_origin);
}
#endif

#if CONFIG_SENSOR_AUTO_CMPEN
static void sensor_acts_compens_handle(struct k_work *work)
{
	struct acts_sensor_info *sens = CONTAINER_OF(work, struct acts_sensor_info, timer);

	sensor_acts_compens_event(sens);

	/* In S1 we need check temperature compens every SENSOR_CMPEN_MEAS_INTV_S */
	k_delayed_work_submit(&sens->timer, K_SECONDS(SENSOR_CMPEN_MEAS_INTV_S));
}
#endif

static const struct sensor_driver_api sensor_acts_driver_api = {
	.get_property = sensor_acts_get_property,
	.enable = sensor_acts_enable,
	.disable = sensor_acts_disable,
};

static int sensor_acts_init(const struct device *dev)
{
	struct acts_sensor_info *sens = dev->data;

	#if CONFIG_SENSOR_AUTO_CMPEN
	int cur_temp;
	uint8_t efuse_vref;
	uint8_t efuse_offset;
	#endif

	soc_saradc_sensor_enable();
	sens->enable_flag = 1;

	#if CONFIG_SENSOR_AUTO_CMPEN
	if(get_efuse_sensor_trim(&efuse_vref, &efuse_offset) == 0){
		sens->efuse_vref = efuse_vref;
		sens->efuse_offset = efuse_offset;
		LOG_INF("efuse vref=%d, offset=%d\n", sens->efuse_vref, sens->efuse_offset);		
	}
	else{
		sens->efuse_offset = 0;
		sens->efuse_vref = SENSOR_CMPEN_MAX_EFUSE_VREF + 1;
		LOG_ERR("efuse vref get failed, use default value\n");	
	}

	sens->v06_ref = (sys_read32(VREF_CTL) & VREF_CTL_VREF06_VOL_MASK);
	LOG_DBG("default v6=%d\n", sens->v06_ref);	
	cur_temp = sensor_get_temperature(sens);
	LOG_INF("temperature: %d.%d\n", cur_temp/10, get_decimal_part(cur_temp));
	sens->temperature = cur_temp;

	k_delayed_work_init(&sens->timer, sensor_acts_compens_handle);
	k_delayed_work_submit(&sens->timer, K_SECONDS(SENSOR_CMPEN_MEAS_INTV_S));
	#endif

	return 0;
}

#if CONFIG_SENSOR
#ifdef CONFIG_PM_DEVICE
static int sensor_acts_pm_control(const struct device *device,
			       uint32_t command,
			       void *context, device_pm_cb cb, void *arg)
{
	int ret = 0;
	uint32_t state = *((uint32_t*)context);
	struct acts_sensor_info *sens = (struct acts_sensor_info *)(device->data);

	LOG_DBG("command:0x%x state:%d", command, state);

	if (command != DEVICE_PM_SET_POWER_STATE)
		return 0;

	switch (state) {
	case DEVICE_PM_ACTIVE_STATE:
		LOG_DBG("sensor active");
		sensor_acts_enable((struct device *)device);
		#if CONFIG_SENSOR_AUTO_CMPEN
		k_delayed_work_submit(&sens->timer, K_NO_WAIT);
		#endif
		break;
	case DEVICE_PM_SUSPEND_STATE:
		LOG_DBG("sensor suspend");
		sensor_acts_disable((struct device *)device);
		#if CONFIG_SENSOR_AUTO_CMPEN
		/* if delayed_work time is not arrived, but sys wants to enter S3, we should cancel the delayed_work */
		if(k_delayed_work_remaining_ticks(&sens->timer))
			k_delayed_work_cancel(&sens->timer);
		#endif
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
#define sensor_acts_pm_control NULL
#endif

DEVICE_DEFINE(sensor, CONFIG_ACTS_SENSOR_DEV_NAME, sensor_acts_init, sensor_acts_pm_control,
	    &sensor_acts_ddata, NULL, POST_KERNEL,
	    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &sensor_acts_driver_api);
#endif
