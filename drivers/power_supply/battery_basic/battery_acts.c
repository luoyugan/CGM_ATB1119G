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
#include <sys/byteorder.h>
#include <drivers/adc.h>
#include <drivers/power_supply.h>
#include <soc.h>
#include <board.h>
#include <board_cfg.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(batadc, CONFIG_LOG_DEFAULT_LEVEL);

#define V_LSB_MULT1000                        (48*100 * 1000 / 4096) /* BAT ADC sample result coverts to voltage value */
#define C_LSB_MULT10000                       (625 * 3) /* CHARGE ADC sample result coverts to current value */

#define CHG_STATUS_DETECT_DEBOUNCE_BUF_SIZE   (3)
#define BAT_MIN_VOLTAGE                       (2400)
#define BAT_FULL_VOLTAGE                      (4150)
#define BAT_VOL_LSB_MV                        (3)
#define BAT_VOLTAGE_RESERVE                   (0x1fff)
#define BAT_CAP_RESERVE                       (101)

/**
 * BAT_VOLTAGE_TBL_DCNT * BAT_VOLTAGE_TBL_ACNT : the samples number
 * BAT_VOLTAGE_TBL_DCNT + BAT_VOLTAGE_TBL_ACNT : Save space occupied by sample points
 */
#define BAT_VOLTAGE_TBL_DCNT                  (10) /* counter to collect voltage */
#define BAT_CURRENT_TBL_DCNT                  (10) /* counter to collect current */

#define BAT_VOLTAGE_IDEAL_INDEX               (BAT_VOLTAGE_TBL_DCNT * 2 / 3)
#define BAT_CURRENT_IDEAL_INDEX               (BAT_CURRENT_TBL_DCNT * 2 / 3)

#define INDEX_VOL                             (0)
#define INDEX_CAP                             (INDEX_VOL + 1)

#define CAPACITY_CHECK_INTERVAL_1MINUTE       (600)

#define BAT_VOL_SAMPLE_ONE_TIMEOUT_MS         (100)

#define BAT_PRE_CHARGE_VOL                    CONFIG_BATTERY_PRECHARGE_VOLTAGE_MV
#define BAT_TRICKLE_CHARGE_CURRENT            CONFIG_BATTERY_TRICKLE_CHARGE_CURRENT
#define BAT_VOL_VARP_LIMIT                    CONFIG_BATTERY_VOL_VARP_LIMIT
#define BAT_MAX_VARP_ERROR                    CONFIG_BATTERY_MAX_VARP_ERROR /* if counter reaches the max VARP error number, just collect the sample whatever */
#define BAT_CHARGE_FULL_CONST_VOL             CONFIG_BATTERY_CHARGE_FULL_CV /* 4.2V */
#define BAT_CHARGE_NORMAL_CONST_VOL           CONFIG_BATTERY_CHARGE_NORMAL_VOL /* 4.1V */
#define BAT_MORMAL_CHARGE_CURRENT_MA          CONFIG_BATTERY_CHARGER_MAX_CURRENT
#define BAT_CHARGE_FULL_DURATION_SEC          CONFIG_BATTERY_CHARGE_FULL_SEC
#define BAT_DC5V_POLL_TIME_MSEC               CONFIG_BATTERY_DC5V_POLL_SEC

/** Battery Charge State
 * Pre-charge state: set a lower current that can maintain the system power supply and trickle charge to protect the battery.
 * Normal charge state: set a higher current for battery fast charge.
 */
enum battery_charge_state {
	BATTERY_PRE_CHARGE_STATE = 1,
	BATTERY_CC_CHARGE_STATE, /* const current charge state */
	BATTERY_CV_CHARGE_STATE, /* const voltage charge state */
	BATTERY_TRICKLE_CHARGE_STATE, /* trickle charge state */
	BATTERY_FULL_CHARGE_STATE, /* full charge state */
};

/* battery poll state when DC5V existed */
enum battery_dc5v_poll_state {
	BATTERY_DC5V_POLL_INIT = 0,
	BATTERY_DC5V_POLL_DISABLE_CHARGE,
	BATTERY_DC5V_POLL_ENABLE_CHARGE,
};

struct battery_cap {
	uint16_t cap;
	uint16_t volt;
};

struct report_deb_ctr {
	uint16_t rise;
	uint16_t decline;
	uint16_t times;
	uint8_t step;
};

struct voltage_tbl {
	uint16_t vol_data[BAT_VOLTAGE_TBL_DCNT];
	uint8_t d_index;
	uint8_t data_collecting;   /* indicator whether the voltage data is on collecting */
};

struct current_tbl {
	uint16_t cur_data[BAT_CURRENT_TBL_DCNT];
	uint8_t d_index;
	uint8_t data_collecting;   /* indicator whether the current data is on collecting */
};

struct acts_battery_info {
	struct device *adc_dev;
	struct device *this_dev;
	struct k_delayed_work timer;
	struct adc_sequence bat_sequence;
	struct adc_sequence charge_sequence;
	uint8_t bat_adcval[2];
	uint8_t charge_adcval[2];
	struct voltage_tbl vol_tbl;
	struct current_tbl cur_tbl;
	uint32_t cur_voltage       : 13;
	uint32_t cur_cap           : 7;
	uint32_t cur_dc5v_status   : 1;
	uint32_t cur_chg_status    : 3;
	uint32_t last_cap_report     : 7;
	uint32_t last_voltage_report : 13;
	uint32_t last_dc5v_report    : 1;
	uint32_t last_chg_report     : 3;

	uint32_t cur_charge_state    : 3;

	uint32_t cur_ma              : 13;

	uint8_t vol_varp_err_cnt;
	uint8_t cur_varp_err_cnt;

	uint8_t poll_state;

	struct report_deb_ctr report_ctr[2];

	const struct battery_cap *cap_tbl;
	bat_charge_callback_t notify;

	uint32_t debug_timestamp;
	uint32_t state_timestamp;

	uint8_t chg_debounce_buf[CHG_STATUS_DETECT_DEBOUNCE_BUF_SIZE];

};

struct acts_battery_config {
	char *adc_name;
	uint16_t batadc_chan;
	uint16_t chargeadc_chan;
	uint16_t pin;
	char *gpio_name;

	uint16_t poll_interval_ms;
	uint16_t debug_interval_sec;
};

#define BATTERY_CAP_TABLE_CNT		12
static const struct battery_cap battery_cap_tbl[BATTERY_CAP_TABLE_CNT] = {
#ifdef BOARD_BATTERY_CAP_MAPPINGS
	BOARD_BATTERY_CAP_MAPPINGS
#endif
};

bool bat_charge_callback(bat_charge_event_t event, bat_charge_event_para_t *para, struct acts_battery_info *bat);

/* convert battery ADC value to voltage in millivolt */
static uint32_t battery_adcval_to_voltage(uint32_t adcval)
{
	return V_LSB_MULT1000 * adcval / 1000;
}

/* convert charge ADC value to current in milliampere */
static uint32_t battery_adcval_to_current(uint32_t adcval)
{
	return C_LSB_MULT10000 * adcval / 10000;
}

static void battery_volt_selectsort(uint16_t *array, uint16_t len)
{
	uint16_t i, j, min;

	for (i = 0; i < len-1; i++)	{
		min = i;
		for (j = i+1; j < len; j++) {
			if (array[min] > array[j])
				min = j;
		}
		/* swap */
		if (min != i) {
			array[min] ^= array[i];
			array[i] ^= array[min];
			array[min] ^= array[i];
		}
	}
}

/* sample population variance */
static int battery_cal_varp(uint16_t *array, uint16_t len)
{
	uint16_t i, average;
	uint32_t sum = 0;

	if (len <= 1)
		return -EINVAL;

	for (i = 0; i < len; i++)
		sum += array[i];

	average = sum / len;

	sum = 0;
	for (i = 0; i < len; i++)
		sum += ((array[i] - average) * (array[i] - average));

	sum /= (len - 1);

	return sum;
}


/* voltage: mV */
static int __battery_get_voltage(struct acts_battery_info *bat)
{
	uint32_t adc_val, volt_mv;
	int ret;

	ret = adc_read(bat->adc_dev, &bat->bat_sequence);

	if (ret) {
		LOG_ERR("battery ADC read error %d", ret);
		return -EIO;
	}

	adc_val = sys_get_le16(bat->bat_sequence.buffer);

	volt_mv = battery_adcval_to_voltage(adc_val);
	LOG_DBG("vol_adc:%d volt_mv:%d", adc_val, volt_mv);

	return volt_mv;
}

/* current: mA */
static int __battery_get_current(struct acts_battery_info *bat)
{
	uint32_t adc_val, current_ma;
	int ret;

	/* get charging adc value */
	ret = adc_read(bat->adc_dev, &bat->charge_sequence);
	if (ret) {
		LOG_ERR("charge ADC read error %d", ret);
		return -EIO;
	}

	adc_val = sys_get_le16(bat->charge_sequence.buffer);

	current_ma = battery_adcval_to_current(adc_val);
	LOG_DBG("charge_adc:%d current_ma:%d", adc_val, current_ma);

	return current_ma;
}

static void sample_voltage_tbl_reset(struct acts_battery_info *bat)
{
	memset(bat->vol_tbl.vol_data, 0, sizeof(struct voltage_tbl));
	bat->vol_tbl.d_index = 0;
	bat->vol_tbl.data_collecting = 1;
}

static void sample_current_tbl_reset(struct acts_battery_info *bat)
{
	memset(bat->cur_tbl.cur_data, 0, sizeof(struct current_tbl));
	bat->cur_tbl.d_index = 0;
	bat->cur_tbl.data_collecting = 1;
}

/* Get a current sample point */
static uint32_t sample_one_current(struct acts_battery_info *bat)
{
	uint16_t volt_ma = __battery_get_current(bat);

	bat->cur_tbl.cur_data[bat->cur_tbl.d_index] = volt_ma;
	bat->cur_tbl.d_index++;
	bat->cur_tbl.d_index %= BAT_CURRENT_TBL_DCNT;

	if (bat->cur_tbl.d_index == 0) {
		/* data collection finish */
		if (bat->cur_tbl.data_collecting == 1)
			bat->cur_tbl.data_collecting = 0;
	}

	return volt_ma;
}

static int battery_get_current(struct acts_battery_info *bat)
{
	int varp, ma;
	uint8_t i;
	uint32_t sum = 0;

	sample_one_current(bat);

	if (bat->cur_tbl.d_index > 1) {
		varp = battery_cal_varp(bat->cur_tbl.cur_data, bat->cur_tbl.d_index);
		LOG_DBG("current varp: %d", varp);
		bat->cur_varp_err_cnt++;
		/* samples deviation is too large */
		if ((varp > BAT_VOL_VARP_LIMIT)
			&& (bat->cur_varp_err_cnt < BAT_MAX_VARP_ERROR)) {
			LOG_DBG("current sample num:%d varp:%d error",
					bat->cur_tbl.d_index, varp);
			sample_current_tbl_reset(bat);
			return -EAGAIN;
		}
	}

	/* data collection finish */
	if ((!bat->cur_tbl.data_collecting)
		|| (bat->cur_varp_err_cnt >= BAT_MAX_VARP_ERROR)) {
		if (bat->cur_varp_err_cnt >= BAT_MAX_VARP_ERROR)
			LOG_ERR("current varp error too much %d", bat->cur_varp_err_cnt);
		bat->cur_varp_err_cnt = 0;
		battery_volt_selectsort(bat->cur_tbl.cur_data, BAT_CURRENT_TBL_DCNT);
		ma = bat->cur_tbl.cur_data[BAT_CURRENT_IDEAL_INDEX];
	} else {
		for (i = 0; i < bat->cur_tbl.d_index; i++)
			sum += bat->cur_tbl.cur_data[i];
		ma = ((sum * 10 / bat->cur_tbl.d_index + 5) / 10);
	}

	return ma;
}

void battery_acts_current_poll(struct acts_battery_info *bat)
{
	int ma;

	if (soc_pmu_get_dc5v_status()) {
		ma = battery_get_current(bat);

		if (bat->cur_tbl.data_collecting)
			return ;

		bat->cur_tbl.data_collecting = 1;
		bat->cur_ma = ma;
	} else {
		bat->cur_ma = 0;
		sample_current_tbl_reset(bat);
	}
}

/* Get a voltage sample point */
static uint32_t sample_one_voltage(struct acts_battery_info *bat)
{
	uint32_t volt_mv = __battery_get_voltage(bat);

	bat->vol_tbl.vol_data[bat->vol_tbl.d_index] = volt_mv;
	bat->vol_tbl.d_index++;
	bat->vol_tbl.d_index %= BAT_VOLTAGE_TBL_DCNT;

	if (bat->vol_tbl.d_index == 0) {
		/* data collection finish */
		if (bat->vol_tbl.data_collecting == 1)
			bat->vol_tbl.data_collecting = 0;
	}

	return volt_mv;
}

static int battery_get_voltage(struct acts_battery_info *bat)
{
	int varp, vol;
	uint8_t i;
	uint32_t sum = 0;

	/* sample one voltage before getting the average voltage
	 * to avoid the 'divide 0' case when calling battery_get_voltage()
	 */
	sample_one_voltage(bat);

	if (bat->vol_tbl.d_index > 1) {
		varp = battery_cal_varp(bat->vol_tbl.vol_data, bat->vol_tbl.d_index);
		LOG_DBG("voltage varp: %d", varp);
		bat->vol_varp_err_cnt++;
		/* samples deviation is too large */
		if ((varp > BAT_VOL_VARP_LIMIT)
			&& (bat->vol_varp_err_cnt < BAT_MAX_VARP_ERROR)) {
			LOG_DBG("voltage sample num:%d varp:%d error",
					bat->vol_tbl.d_index, varp);
			sample_voltage_tbl_reset(bat);
			return -EAGAIN;
		}
	}

	/* data collection finish */
	if ((!bat->vol_tbl.data_collecting) || (bat->vol_varp_err_cnt >= BAT_MAX_VARP_ERROR)) {
		if (bat->vol_varp_err_cnt >= BAT_MAX_VARP_ERROR)
			LOG_ERR("voltage varp error too much %d", bat->vol_varp_err_cnt);
		bat->vol_varp_err_cnt = 0;
		battery_volt_selectsort(bat->vol_tbl.vol_data, BAT_VOLTAGE_TBL_DCNT);
		vol = bat->vol_tbl.vol_data[BAT_VOLTAGE_IDEAL_INDEX];
	} else {
		for (i = 0; i < bat->vol_tbl.d_index; i++)
			sum += bat->vol_tbl.vol_data[i];
		vol = ((sum * 10 / bat->vol_tbl.d_index + 5) / 10);
	}

	return vol;
}

uint32_t voltage2capacit(uint32_t volt, struct acts_battery_info *bat)
{
	const struct battery_cap *bc, *bc_prev;
	uint32_t  i, cap = 0;

	/* %0 */
	if (volt <= bat->cap_tbl[0].volt)
		return 0;

	/* %100 */
	if (volt >= bat->cap_tbl[BATTERY_CAP_TABLE_CNT - 1].volt)
		return 100;

	for (i = 1; i < BATTERY_CAP_TABLE_CNT; i++) {
		bc = &bat->cap_tbl[i];
		if (volt < bc->volt) {
			bc_prev = &bat->cap_tbl[i - 1];
			/* linear fitting */
			cap = bc_prev->cap + (((bc->cap - bc_prev->cap) *
				(volt - bc_prev->volt)*10 + 5) / (bc->volt - bc_prev->volt)) / 10;

			break;
		}
	}

	return cap;
}

static void bat_voltage_record_reset(struct acts_battery_info *bat)
{
	bat->cur_voltage = 0;
	bat->cur_cap = 0;
	bat->cur_ma = 0;

	sample_voltage_tbl_reset(bat);

	sample_current_tbl_reset(bat);
}

static int battery_get_current_voltage(struct acts_battery_info *bat)
{
	uint32_t cur_time;
	int volt;

	cur_time = k_uptime_get_32();
	volt = battery_get_voltage(bat);

	while (volt < 0) {
		volt = battery_get_voltage(bat);
		if ((k_uptime_get_32() - cur_time) > BAT_VOL_SAMPLE_ONE_TIMEOUT_MS) {
			LOG_ERR("sample one voltage timeout");
			return -ETIMEDOUT;
		}
	}

	return volt;
}

static int battery_get_current_capacity(struct acts_battery_info *bat)
{
	int volt;

	volt = battery_get_current_voltage(bat);
	if (volt < 0)
		return 0;

	return voltage2capacit(volt, bat);
}

static int battery_get_charge_status(struct acts_battery_info *bat, const void *cfg)
{
	uint32_t charge_status = 0;
	int current_ma, volt_mv, max_volt;

	LOG_DBG("dc5v status %d", soc_pmu_get_dc5v_status());

#ifdef BOARD_BATTERY_CAP_MAPPINGS
	max_volt = bat->cap_tbl[BATTERY_CAP_TABLE_CNT - 1].volt;
#else
	max_volt = BAT_FULL_VOLTAGE;
#endif

	if (soc_pmu_get_dc5v_status()) {

		current_ma = bat->cur_ma;
		volt_mv = bat->cur_voltage;

		charge_status = POWER_SUPPLY_STATUS_CHARGING;

		/* there is no valid battery current or voltage data */
		if ((!current_ma) || (!volt_mv)) {
			return charge_status;
		}

		LOG_DBG("cur_ma:%d cur_vol:%d", current_ma, volt_mv);

		/* battery precharge state */
		if ((volt_mv < BAT_PRE_CHARGE_VOL) && !bat->cur_charge_state) {

			printk("** battery precharge **\n");
			bat->cur_charge_state = BATTERY_PRE_CHARGE_STATE;

			/* set charge trickle current to protect battery */
			soc_pmu_set_max_current(BAT_TRICKLE_CHARGE_CURRENT);
			bat->state_timestamp = k_uptime_get_32();
		}

		/* battery const current charge state */
		if ((volt_mv > (BAT_PRE_CHARGE_VOL + BAT_VOL_LSB_MV))
			&& (bat->cur_charge_state <= BATTERY_PRE_CHARGE_STATE)) {

			if (bat->cur_charge_state == BATTERY_PRE_CHARGE_STATE)
				LOG_INF("precharge duration %dms", k_uptime_get_32() - bat->state_timestamp);

			printk("** battery const current charge **\n");
			bat->cur_charge_state = BATTERY_CC_CHARGE_STATE;

			/* increase current for fast charge */
			soc_pmu_set_max_current(BAT_MORMAL_CHARGE_CURRENT_MA);

			bat->state_timestamp = k_uptime_get_32();
		}

		/* battery const voltage charge state */
		if (soc_pmu_check_charge_cv_state() && (bat->cur_charge_state <= BATTERY_CC_CHARGE_STATE)
			&& (volt_mv >= max_volt)) {

			if (bat->cur_charge_state == BATTERY_CC_CHARGE_STATE)
				LOG_INF("cc charge duration %dms", k_uptime_get_32() - bat->state_timestamp);

			printk("** battery const voltage charge **\n");
			bat->cur_charge_state = BATTERY_CV_CHARGE_STATE;

			/* increase voltage for charge full */
			soc_pmu_set_const_voltage(BAT_CHARGE_FULL_CONST_VOL);

			bat->state_timestamp = k_uptime_get_32();
		}

		if (soc_pmu_check_charge_cv_state() && (bat->cur_charge_state == BATTERY_CV_CHARGE_STATE)
			&& (current_ma < BAT_TRICKLE_CHARGE_CURRENT) && (volt_mv >= max_volt)) {

			LOG_INF("cv charge duration %dms", k_uptime_get_32() - bat->state_timestamp);

			bat->cur_charge_state = BATTERY_TRICKLE_CHARGE_STATE;

			soc_pmu_set_const_voltage(BAT_CHARGE_NORMAL_CONST_VOL);
			soc_pmu_set_max_current(BAT_TRICKLE_CHARGE_CURRENT);

			bat->state_timestamp = k_uptime_get_32();
		}

		if ((bat->cur_charge_state == BATTERY_TRICKLE_CHARGE_STATE)
			&& (current_ma < BAT_TRICKLE_CHARGE_CURRENT) && (volt_mv >= max_volt)) {
			if ((k_uptime_get_32() - bat->state_timestamp) > (BAT_CHARGE_FULL_DURATION_SEC * 1000)) {
					printk("** battery full charge **\n");
				bat->cur_charge_state = BATTERY_FULL_CHARGE_STATE;
				charge_status = POWER_SUPPLY_STATUS_FULL;
			}
		}

		if (bat->cur_charge_state == BATTERY_FULL_CHARGE_STATE)
			charge_status = POWER_SUPPLY_STATUS_FULL;

		/* check the battery is esisted or not */
		if (volt_mv <= BAT_MIN_VOLTAGE) {
			charge_status = POWER_SUPPLY_STATUS_BAT_NOTEXIST;
			bat->cur_charge_state = 0;
			bat->state_timestamp = 0;
		}

	} else {
		charge_status = POWER_SUPPLY_STATUS_DISCHARGE;
		bat->cur_charge_state = 0;
		bat->state_timestamp = 0;
	}

	return charge_status;
}

static int bat_report_debounce(uint32_t last_val, uint32_t cur_val, struct report_deb_ctr *ctr)
{
	if ((cur_val  > last_val) && (cur_val - last_val >= ctr->step)) {
		ctr->rise++;
		ctr->decline = 0;
		LOG_DBG("RISE(%s) %d", cur_val>100 ? "VOL" : "CAP", ctr->rise);
	} else if ((cur_val  < last_val) && (last_val - cur_val >= ctr->step)) {
		ctr->decline++;
		ctr->rise = 0;
		LOG_DBG("DECLINE(%s) %d", cur_val>100 ? "VOL" : "CAP", ctr->decline);
	}

	if (ctr->decline >= ctr->times) {
		LOG_DBG("<DECLINE> %d>%d", last_val, cur_val);
		ctr->decline = 0;
		return 1;
	} else if (ctr->rise >= ctr->times) {
		LOG_DBG("<RISE> %d>%d", last_val, cur_val);
		ctr->rise = 0;
		return 1;
	} else {
		return 0;
	}
}

void battery_acts_voltage_poll(struct acts_battery_info *bat)
{
	struct device *dev = bat->this_dev;
	const struct acts_battery_config *cfg = dev->config;
	int volt_mv;
	bat_charge_event_para_t para;
	uint32_t cap = 0;

	volt_mv = battery_get_voltage(bat);
	if (volt_mv < 0)
		return ;

	/* wait finish data collection */
	if (bat->vol_tbl.data_collecting)
		return;

	bat->vol_tbl.data_collecting = 1;

	LOG_DBG("cur_voltage %d volt_mv %d cur_chg_status %d\n",
			bat->cur_voltage, volt_mv, bat->cur_chg_status);

	if ((bat->cur_voltage >= volt_mv + BAT_VOL_LSB_MV)
		|| (bat->cur_voltage + BAT_VOL_LSB_MV <= volt_mv)) {
		bat->cur_voltage = volt_mv;
		cap = voltage2capacit(volt_mv, bat);
	}

	if ((cap != 0) && (cap != bat->cur_cap)) {
		bat->cur_cap = cap;
	}

	if ((bat->cur_chg_status != POWER_SUPPLY_STATUS_BAT_NOTEXIST)
		&& (bat->cur_chg_status != POWER_SUPPLY_STATUS_UNKNOWN)) {
		bat->report_ctr[INDEX_CAP].times = 600;   /* unit 100ms */
		bat->report_ctr[INDEX_CAP].step = 5;      /* % */

		if (bat->cur_voltage <= 3400) {
			bat->report_ctr[INDEX_VOL].times = 50;  /* unit 100ms */
			bat->report_ctr[INDEX_VOL].step = BAT_VOL_LSB_MV*3;    /* mv */
		} else if (bat->cur_voltage <= 3700) {
			bat->report_ctr[INDEX_VOL].step = BAT_VOL_LSB_MV*5;    /* mv */
			bat->report_ctr[INDEX_VOL].times = 150;  /* unit 100ms */
		} else {
			bat->report_ctr[INDEX_VOL].step = BAT_VOL_LSB_MV*6;    /* mv */
			bat->report_ctr[INDEX_VOL].times = 350; /* unit 100ms */
		}

		if (bat_report_debounce(bat->last_voltage_report, bat->cur_voltage, &bat->report_ctr[INDEX_VOL])
			|| (bat->last_voltage_report == BAT_VOLTAGE_RESERVE)) {

			para.voltage_val = bat->cur_voltage*1000;
			if (bat_charge_callback(BAT_CHG_EVENT_VOLTAGE_CHANGE, &para, bat)) {
				bat->last_voltage_report = bat->cur_voltage;
			}
			LOG_DBG("current voltage: %d", bat->cur_voltage);
		}

		if (bat_report_debounce(bat->last_cap_report, bat->cur_cap, &bat->report_ctr[INDEX_CAP])
			|| (bat->last_cap_report == BAT_CAP_RESERVE)) {
			para.cap = bat->cur_cap;
			if (bat_charge_callback(BAT_CHG_EVENT_CAP_CHANGE, &para, bat)) {
				bat->last_cap_report = bat->cur_cap;
			}
			LOG_DBG("current cap: %d", bat->cur_cap);
		}

		if (cfg->debug_interval_sec) {
			if ((k_uptime_get_32() - bat->debug_timestamp) > (cfg->debug_interval_sec * 1000)) {
				printk("** battery voltage:%dmV capacity:%d%% **\n", bat->cur_voltage, bat->cur_cap);
				if (soc_pmu_get_dc5v_status())
					printk("** battery current:%dmA charge state:%d **\n", bat->cur_ma, bat->cur_charge_state);
				bat->debug_timestamp = k_uptime_get_32();
			}
		}
	}

}

void dc5v_status_check(struct acts_battery_info *bat)
{
	uint8_t temp_dc5v_status;

	if (!soc_pmu_get_dc5v_status()) {
		temp_dc5v_status = 0;
	} else {
		temp_dc5v_status = 1;
	}


	if (bat->cur_dc5v_status != temp_dc5v_status) {
		bat->cur_dc5v_status = temp_dc5v_status;
	}

	if (bat->last_dc5v_report != temp_dc5v_status) {
		if (bat_charge_callback((temp_dc5v_status ? BAT_CHG_EVENT_DC5V_IN : BAT_CHG_EVENT_DC5V_OUT), NULL, bat))
			bat->last_dc5v_report = temp_dc5v_status;
	}
}


uint8_t debounce(uint8_t *debounce_buf, int buf_size, uint8_t value)
{
	int  i;

	if (buf_size == 0)
		return 1;

	for (i = 0; i < buf_size - 1; i++) {
		debounce_buf[i] = debounce_buf[i+1];
	}

	debounce_buf[buf_size - 1] = value;

    for (i = 0; i < buf_size; i++) {
		if (debounce_buf[i] != value) {
		    return 0;
		}
	}

	return 1;
}

int charge_status_need_debounce(uint32_t charge_status)
{
	if ((charge_status == POWER_SUPPLY_STATUS_CHARGING)
		|| (charge_status == POWER_SUPPLY_STATUS_FULL)) {
		return 1;
	} else {
		return 0;
	}
}

void battery_acts_charge_status_check(struct acts_battery_info *bat, struct device *dev)
{
	const struct acts_battery_config *bat_cfg = dev->config;
	uint32_t charge_status = 0;

	charge_status = battery_get_charge_status(bat, bat_cfg);

	if ((charge_status_need_debounce(charge_status))
		&& (debounce(bat->chg_debounce_buf, sizeof(bat->chg_debounce_buf), charge_status) == 0)) {
		return;
	}

	if (charge_status != bat->cur_chg_status) {
		/* if battery has been plug in or pull out, need to reset the voltage record */
		if ((bat->cur_chg_status == POWER_SUPPLY_STATUS_BAT_NOTEXIST)
			|| (charge_status == POWER_SUPPLY_STATUS_BAT_NOTEXIST)) {
			bat_voltage_record_reset(bat);
		}

		LOG_DBG("cur_chg_status %d charge_status %d", bat->cur_chg_status, charge_status);
		bat->cur_chg_status = charge_status;
	}

	if (charge_status != bat->last_chg_report) {
		if (charge_status == POWER_SUPPLY_STATUS_FULL) {
			if (bat_charge_callback(BAT_CHG_EVENT_CHARGE_FULL, NULL, bat))
				bat->last_chg_report = charge_status;
		} else if (charge_status == POWER_SUPPLY_STATUS_CHARGING) {
			if (bat_charge_callback(BAT_CHG_EVENT_CHARGE_START, NULL, bat))
				bat->last_chg_report = charge_status;
		} else if (charge_status == POWER_SUPPLY_STATUS_DISCHARGE) {
			if (bat_charge_callback(BAT_CHG_EVENT_CHARGE_STOP, NULL, bat))
				bat->last_chg_report = charge_status;
		}
	}
}

static void battery_acts_poll(struct k_work *work)
{
	struct acts_battery_info *bat =
			CONTAINER_OF(work, struct acts_battery_info, timer);
	struct device *dev = bat->this_dev;
	const struct acts_battery_config *cfg = dev->config;
	uint16_t poll_ms = cfg->poll_interval_ms;

	battery_acts_charge_status_check(bat, dev);

	dc5v_status_check(bat);

	if (soc_pmu_get_dc5v_status()) {

		LOG_DBG("battery poll state %d", bat->poll_state);

		if (bat->poll_state == BATTERY_DC5V_POLL_INIT) {
			poll_ms = BAT_DC5V_POLL_TIME_MSEC;
			bat->poll_state = BATTERY_DC5V_POLL_DISABLE_CHARGE;
			soc_pmu_read_bat_lock();
		} else if (bat->poll_state == BATTERY_DC5V_POLL_DISABLE_CHARGE) {
			bat->poll_state = BATTERY_DC5V_POLL_ENABLE_CHARGE;
			poll_ms = BAT_DC5V_POLL_TIME_MSEC;
			battery_acts_voltage_poll(bat);
			soc_pmu_read_bat_unlock();
		} else if (bat->poll_state == BATTERY_DC5V_POLL_ENABLE_CHARGE) {
			bat->poll_state = BATTERY_DC5V_POLL_INIT;
			battery_acts_current_poll(bat);
		}
	} else {
		bat->poll_state = BATTERY_DC5V_POLL_INIT;
		battery_acts_voltage_poll(bat);
	}

	k_delayed_work_submit(&bat->timer, K_MSEC(poll_ms));
}

static int battery_acts_get_property(struct device *dev,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	struct acts_battery_info *bat = dev->data;
	int ret = 0;

	LOG_DBG("battery get property cmd %d", psp);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		/* val->intval = battery_get_charge_status(bat, dev->config->config); */
		val->intval = bat->cur_chg_status;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = 1;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		ret = battery_get_current_voltage(bat);
		if (ret < 0)
			return ret;
		val->intval = battery_get_current_voltage(bat) * 1000;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = battery_get_current_capacity(bat);
		break;
	case POWER_SUPPLY_PROP_DC5V:
		val->intval = soc_pmu_get_dc5v_status();
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

static void battery_acts_set_property(struct device *dev,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	/* do nothing */
}

bool bat_charge_callback(bat_charge_event_t event, bat_charge_event_para_t *para, struct acts_battery_info *bat)
{
	LOG_DBG("event %d notify %p", event, bat->notify);
	if (bat->notify) {
		bat->notify(event, para);
	} else {
		return false;
	}

	return true;
}

static void battery_acts_register_notify(struct device *dev, bat_charge_callback_t cb)
{
	struct acts_battery_info *bat = dev->data;
	int flag;

	LOG_DBG("callback %p", cb);

	flag = irq_lock();
	if ((bat->notify == NULL) && cb) {
		bat->notify = cb;
	} else {
		LOG_ERR("notify func already exist!\n");
	}
	irq_unlock(flag);
}

static void battery_acts_enable(struct device *dev)
{
	struct acts_battery_info *bat = dev->data;

	LOG_DBG("enable battery detect");

	k_delayed_work_submit(&bat->timer, K_NO_WAIT);
}

static void battery_acts_disable(struct device *dev)
{
	struct acts_battery_info *bat = dev->data;

	LOG_DBG("disable battery detect");

	k_delayed_work_cancel(&bat->timer);
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
	const struct acts_battery_config *cfg = dev->config;
	struct adc_channel_cfg channel_cfg = {0};

	bat->adc_dev = (struct device *)device_get_binding(cfg->adc_name);
	if (!bat->adc_dev) {
		LOG_ERR("cannot found ADC device \'batadc\'\n");
		return -ENODEV;
	}


	channel_cfg.channel_id = cfg->batadc_chan;
	if (adc_channel_setup(bat->adc_dev, &channel_cfg)) {
		LOG_ERR("setup channel_id %d error", channel_cfg.channel_id);
		return -EFAULT;
	}

	channel_cfg.channel_id = cfg->chargeadc_chan;
	if (adc_channel_setup(bat->adc_dev, &channel_cfg)) {
		LOG_ERR("setup channel_id %d error", channel_cfg.channel_id);
		return -EFAULT;
	}

	bat->this_dev = (struct device *)dev;

	bat->bat_sequence.channels = BIT(cfg->batadc_chan);
	bat->bat_sequence.buffer = &bat->bat_adcval;
	bat->bat_sequence.buffer_size = sizeof(bat->bat_adcval);

	bat->charge_sequence.channels = BIT(cfg->chargeadc_chan);
	bat->charge_sequence.buffer = &bat->charge_adcval;
	bat->charge_sequence.buffer_size = sizeof(bat->charge_adcval);

	bat->cap_tbl = battery_cap_tbl;

	bat_voltage_record_reset(bat);

	bat->cur_chg_status = POWER_SUPPLY_STATUS_UNKNOWN;
	bat->last_cap_report = BAT_CAP_RESERVE;
	bat->last_voltage_report = BAT_VOLTAGE_RESERVE;
	bat->debug_timestamp = 0;

	soc_pmu_set_max_current(BAT_MORMAL_CHARGE_CURRENT_MA);

	/* TODO read efuse bit[20:15] */
	soc_pmu_set_const_voltage(BAT_CHARGE_NORMAL_CONST_VOL);

	k_delayed_work_init(&bat->timer, battery_acts_poll);
	k_delayed_work_submit(&bat->timer, K_MSEC(cfg->poll_interval_ms));

	printk("battery initialized\n");

	return 0;
}

static struct acts_battery_info battery_acts_ddata;

static const struct acts_battery_config battery_acts_cdata = {
	.adc_name = CONFIG_PMUADC_NAME,
	.batadc_chan = PMUADC_ID_BATV,
	.chargeadc_chan = PMUADC_ID_CHARGI,

	.poll_interval_ms = CONFIG_BATTERY_POLL_INTERVAL_MS,
	.debug_interval_sec = CONFIG_BATTERY_DEBUG_INTERVAL_SEC,
};

DEVICE_DEFINE(battery, CONFIG_ACTS_BATTERY_DEV_NAME, battery_acts_init, device_pm_control_nop,
	    &battery_acts_ddata, &battery_acts_cdata, POST_KERNEL,
	    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &battery_acts_driver_api);

