#include <string.h>

#include "atb_ble_cgms.h"
#include "cgms_meas.h"
#include "cgms_db.h"

static struct bt_gatt_notify_params m_meas_notify_params;
static uint8_t m_meas_notify_buf[16];

static uint8_t cgms_meas_encode(nrf_ble_cgms_t            * p_cgms,
                                const nrf_ble_cgms_meas_t * p_meas,
                                uint8_t                   * p_encoded_buffer)
{
    uint8_t len = 2;

    uint8_t flags = p_meas->flags;

	
    len += uint16_encode(p_meas->glucose_concentration,
                         &p_encoded_buffer[len]);
    len += uint16_encode(p_meas->time_offset,
                         &p_encoded_buffer[len]);

    if (p_meas->sensor_status_annunciation.warning != 0)
    {
        p_encoded_buffer[len++] = p_meas->sensor_status_annunciation.warning;
        flags                  |= NRF_BLE_CGMS_STATUS_FLAGS_WARNING_OCT_PRESENT;
    }

    if (p_meas->sensor_status_annunciation.calib_temp != 0)
    {
        p_encoded_buffer[len++] = p_meas->sensor_status_annunciation.calib_temp;
        flags                  |= NRF_BLE_CGMS_STATUS_FLAGS_CALTEMP_OCT_PRESENT;
    }

    if (p_meas->sensor_status_annunciation.status != 0)
    {
        p_encoded_buffer[len++] = p_meas->sensor_status_annunciation.status;
        flags                  |= NRF_BLE_CGMS_STATUS_FLAGS_STATUS_OCT_PRESENT;
    }

    // Trend field
    if (p_cgms->feature.feature & NRF_BLE_CGMS_FEAT_CGM_TREND_INFORMATION_SUPPORTED)
    {
        if (flags & NRF_BLE_CGMS_FLAG_TREND_INFO_PRESENT)
        {
            len += uint16_encode(p_meas->trend, &p_encoded_buffer[len]);
        }
    }

    // Quality field
    if (p_cgms->feature.feature & NRF_BLE_CGMS_FEAT_CGM_QUALITY_SUPPORTED)
    {
        if (flags & NRF_BLE_CGMS_FLAGS_QUALITY_PRESENT)
        {
            len += uint16_encode(p_meas->quality, &p_encoded_buffer[len]);
        }
    }

    p_encoded_buffer[1] = flags;
    p_encoded_buffer[0] = len;
    return len;
}

void cgms_meas_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	printk("CGMS Measurement CCCD changed: %u\n", value);
	ARG_UNUSED(attr);
	m_meas_notify_enabled = (value == BT_GATT_CCC_NOTIFY);
	cgms_emit_event(m_meas_notify_enabled ? BLE_CGMS_EVT_NOTIFICATION_ENABLED :
			BLE_CGMS_EVT_NOTIFICATION_DISABLED);
}

int cgms_measurement_notify(const ble_cgms_rec_t *rec)
{
	printk("CGMS Measurement notify called\n");
	return cgms_measurement_notify_with_cb(rec, NULL, NULL);
}

int cgms_measurement_notify_with_cb(const ble_cgms_rec_t *rec,
	bt_gatt_complete_func_t func,
	void *user_data)
{
	uint8_t len;

	if ((m_conn == NULL) || !m_meas_notify_enabled) {
		return -ENOTCONN;
	}

	// len = cgms_meas_encode(m_cgms, &rec->meas, m_meas_notify_buf);
	memset(&m_meas_notify_params, 0, sizeof(m_meas_notify_params));
	m_meas_notify_params.attr = &attr_cgms_svc[CGMS_ATTR_MEAS_VAL];
	m_meas_notify_params.data = m_meas_notify_buf;
	m_meas_notify_params.len = len;
	m_meas_notify_params.func = func;
	m_meas_notify_params.user_data = user_data;

	return bt_gatt_notify_cb(m_conn, &m_meas_notify_params);
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
	ble_cgms_rec_t rec;
	int32_t err_code = 0;

	ARG_UNUSED(work);
	if (!m_session_started) {
		return;
	}

	memset(&rec, 0, sizeof(ble_cgms_rec_t));

	m_current_offset += (m_comm_interval != 0U) ? m_comm_interval : GLUCOSE_MEAS_INTERVAL_MINUTES;

	// 填写 CGMS Measurement 的核心字段。
    rec.meas.glucose_concentration                 = m_glucose_concentration;
    // 以下三项状态位均置 0，表示示例中无额外告警/标定/错误状态。
    rec.meas.sensor_status_annunciation.warning    = 0;
    rec.meas.sensor_status_annunciation.calib_temp = 0;
    rec.meas.sensor_status_annunciation.status     = 0;
    // flags=0 表示仅携带最小必选字段（无趋势/无质量/无扩展信息）。
    rec.meas.flags                                 = 0;
    // 时间偏移（分钟）用于形成记录时间线。
    rec.meas.time_offset                           = m_current_offset;

	err_code = cgms_db_record_add(&rec);
	if (err_code < 0) {
		// 记录添加失败，可能是数据库已满。此处仅打印错误日志，实际应用中可根据需求进行处理。
		printk("Failed to add CGMS record to database (err: %d)\n", err_code);
	}
	(void)cgms_measurement_notify(&rec);
	cgms_schedule_glucose_work();
}
