#include <string.h>

#include "cgms_meas.h"
#include "cgms_db.h"

void cgms_meas_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);
	m_meas_notify_enabled = (value == BT_GATT_CCC_NOTIFY);
	cgms_emit_event(m_meas_notify_enabled ? BLE_CGMS_EVT_NOTIFICATION_ENABLED :
			BLE_CGMS_EVT_NOTIFICATION_DISABLED);
}

int cgms_measurement_notify(const struct cgms_record *rec)
{
	uint8_t encoded[16];
	uint8_t len;

	if ((m_conn == NULL) || !m_meas_notify_enabled) {
		return -ENOTCONN;
	}

	len = cgms_encode_measurement(rec, encoded);
	return bt_gatt_notify(m_conn, &attr_cgms_svc[CGMS_ATTR_MEAS_VAL], encoded, len);
}

void cgms_schedule_glucose_work(void)
{
	if (!m_session_started || (m_comm_interval == 0U)) {
		return;
	}

	k_delayed_work_submit(&m_glucose_work, K_MINUTES(m_comm_interval));
}

void cgms_cancel_glucose_work(void)
{
	k_delayed_work_cancel(&m_glucose_work);
}

void cgms_start_session(void)
{
	m_session_started = true;
	m_nb_run_session++;
	m_status.time_offset = 0U;
	m_current_offset = 0U;
	m_status.annunciation.status &= (uint8_t)(~NRF_BLE_CGMS_STATUS_SESSION_STOPPED);
	memset(&m_sst, 0, sizeof(m_sst));
	cgms_emit_event(BLE_CGMS_EVT_START_SESSION);
	cgms_cancel_glucose_work();
	cgms_schedule_glucose_work();
}

void cgms_stop_session(void)
{
	m_session_started = false;
	m_status.annunciation.status |= NRF_BLE_CGMS_STATUS_SESSION_STOPPED;
	cgms_cancel_glucose_work();
	cgms_emit_event(BLE_CGMS_EVT_STOP_SESSION);
}

void cgms_meas_work_handler(struct k_work *work)
{
	struct cgms_record rec;

	ARG_UNUSED(work);
	if (!m_session_started) {
		return;
	}

	m_current_offset += (m_comm_interval != 0U) ? m_comm_interval : GLUCOSE_MEAS_INTERVAL_MINUTES;
	memset(&rec, 0, sizeof(rec));
	rec.meas.glucose_concentration = m_glucose_concentration;
	rec.meas.time_offset = m_current_offset;

	cgms_record_add(&rec);
	m_status.time_offset = m_current_offset;
	(void)cgms_measurement_notify(&rec);
	cgms_schedule_glucose_work();
}
