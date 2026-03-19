#ifndef CGMS_MEAS_H
#define CGMS_MEAS_H

#include <stdint.h>

#include <bluetooth/gatt.h>

#include "atb_ble_cgms.h"

void cgms_meas_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);
int cgms_measurement_notify(const ble_cgms_rec_t *rec);
int cgms_measurement_notify_with_cb(const ble_cgms_rec_t *rec,
	bt_gatt_complete_func_t func,
	void *user_data);
void cgms_schedule_glucose_work(void);
void cgms_cancel_glucose_work(void);
void cgms_start_session(void);
void cgms_stop_session(void);
void cgms_meas_work_handler(struct k_work *work);

#endif