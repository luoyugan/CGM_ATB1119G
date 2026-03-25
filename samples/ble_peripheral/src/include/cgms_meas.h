#ifndef CGMS_MEAS_H
#define CGMS_MEAS_H

#include <stdint.h>

#include <bluetooth/gatt.h>

#include "atb_ble_cgms.h"

void cgms_meas_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);
int cgms_measurement_notify(nrf_ble_cgms_t * p_cgms, const ble_cgms_rec_t *p_rec, uint8_t * p_count);
int cgms_measurement_notify_with_cb(nrf_ble_cgms_t * p_cgms, const ble_cgms_rec_t *p_rec, uint8_t * p_count, 
	bt_gatt_complete_func_t func,
	void *user_data);
void cgms_schedule_glucose_work(nrf_ble_cgms_t * p_cgms);
void cgms_cancel_glucose_work(nrf_ble_cgms_t * p_cgms);
void cgms_start_session(nrf_ble_cgms_t * p_cgms);
void cgms_stop_session(nrf_ble_cgms_t * p_cgms);
void cgms_meas_work_handler(struct k_work *work);

#endif