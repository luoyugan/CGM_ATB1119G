#include <string.h>

#include "atb_ble_cgms.h"
#include "cgms_db.h"
#include "cgms_meas.h"
#include "cgms_racp.h"
#include "cgms_socp.h"
#include "cgms_sst.h"

ble_cgms_evt_handler_t m_evt_handler;
struct k_delayed_work m_glucose_work;
static nrf_ble_cgms_t *m_cgms_ctx;
ble_cgms_rec_t m_records[CGMS_DB_MAX_RECORDS];
uint16_t m_record_count;
struct cgms_feature_value m_feature = {
	.feature = NRF_BLE_CGMS_FEAT_MULTIPLE_BOND_SUPPORTED | NRF_BLE_CGMS_FEAT_MULTIPLE_SESSIONS_SUPPORTED,
	.type = NRF_BLE_CGMS_MEAS_TYPE_VEN_BLOOD,
	.sample_location = NRF_BLE_CGMS_MEAS_LOC_AST,
};
nrf_ble_cgm_status_t m_status = {
	.time_offset = 0,
	.status = {
		.warning = 0,
		.calib_temp = 0,
		.status = NRF_BLE_CGMS_STATUS_SESSION_STOPPED,
	},
};
ble_cgms_sst_t m_sst;
uint16_t m_current_offset; // 用于形成记录时间线 zephyr
uint16_t m_glucose_concentration = MIN_GLUCOSE_CONCENTRATION;
bool m_meas_notify_enabled;
bool m_racp_ind_enabled;
bool m_socp_ind_enabled;
struct bt_gatt_indicate_params m_racp_ind_params;
struct bt_gatt_indicate_params m_socp_ind_params;
uint8_t m_racp_ind_buf[8];
uint8_t m_socp_ind_buf[20];
struct cgms_racp_state m_racp;
struct cgms_alert_levels m_alert_levels;
uint8_t m_calibration_value[CGMS_CALIBRATION_VALUE_LEN] = {
	0x3E, 0x00, 0x07, 0x00, 0x06,
	0x07, 0x00, 0x00, 0x00, 0x00,
};

uint16_t uint16_decode(const uint8_t * p_encoded_data)
{
        return ( (((uint16_t)((uint8_t *)p_encoded_data)[0])) |
                 (((uint16_t)((uint8_t *)p_encoded_data)[1]) << 8 ));
}

uint8_t uint16_encode(uint16_t value, uint8_t * p_encoded_data)
{
    p_encoded_data[0] = (uint8_t) ((value & 0x00FF) >> 0);
    p_encoded_data[1] = (uint8_t) ((value & 0xFF00) >> 8);
    return sizeof(uint16_t);
}

void cgms_emit_event(ble_cgms_evt_type_t evt_type)
{
	nrf_ble_cgms_evt_t evt;

	if (m_evt_handler == NULL) {
		return;
	}

	evt.evt_type = evt_type;
	m_evt_handler(m_cgms_ctx, &evt);
}

BT_GATT_SERVICE_DEFINE(cgms_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_16(BT_UUID_CGM_VAL)),
	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(BT_UUID_CGM_MEASUREMENT_VAL),
		BT_GATT_CHRC_NOTIFY,
		BT_GATT_PERM_NONE,
		NULL, NULL, NULL),
	BT_GATT_CCC(cgms_meas_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(BT_UUID_CGM_FEATURE_VAL),
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		cgms_read_feature, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(BT_UUID_CGM_STATUS_VAL),
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		cgms_read_status, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(BT_UUID_CGM_SESSION_START_TIME_VAL),
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE | BT_GATT_PERM_PREPARE_WRITE,
		cgms_read_sst, cgms_write_sst, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(BT_UUID_CGM_SESSION_RUN_TIME_VAL),
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		cgms_read_srt, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(BT_UUID_RECORD_ACCESS_CONTROL_POINT_VAL),
		BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,
		BT_GATT_PERM_WRITE | BT_GATT_PERM_PREPARE_WRITE,    
		NULL, cgms_write_racp, NULL),
	BT_GATT_CCC(cgms_racp_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(BT_UUID_CGM_SPECIFIC_OPS_CTRL_PT_VAL),
		BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,
		BT_GATT_PERM_WRITE,
		NULL, cgms_write_socp, NULL),
	BT_GATT_CCC(cgms_socp_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
);

static uint32_t next_sequence_number_set(void)
{
    uint16_t       num_records;
    ble_cgms_rec_t rec;

    num_records = cgms_db_num_records_get();
    if (num_records > 0)
    {
        // Get last record
        uint32_t err_code = cgms_db_record_get(num_records - 1, &rec);
        if (err_code != 0)
        {
            return err_code;
        }
    }

    return 0;
}

uint32_t ble_cgms_init(nrf_ble_cgms_t * p_cgms, const nrf_ble_cgms_init_t * p_cgms_init)
{
	uint32_t   err_code;
	// 初始化data base
	err_code = cgms_db_init();
    if (err_code != 0)
    {
		printk("Failed to initialize CGMS database (err %d)\n", err_code);
        // return err_code;
    }

	err_code = next_sequence_number_set();
    if (err_code != 0)
    {
        return err_code;
    }

	// 初始化service结构体
	p_cgms->evt_handler        = p_cgms_init->evt_handler;
	p_cgms->feature            = p_cgms_init->feature;
	p_cgms->sensor_status      = p_cgms_init->initial_sensor_status;
	p_cgms->session_run_time   = p_cgms_init->initial_run_time;
	p_cgms->is_session_started = false;
	p_cgms->nb_run_session     = 0;
	p_cgms->comm_interval      = GLUCOSE_MEAS_INTERVAL_MINUTES;
    m_cgms_ctx = p_cgms;
    // p_cgms->conn_handle        = BLE_CONN_HANDLE_INVALID;

	p_cgms->feature.feature         = 0;
    p_cgms->feature.feature        |= NRF_BLE_CGMS_FEAT_MULTIPLE_BOND_SUPPORTED;
    p_cgms->feature.feature        |= NRF_BLE_CGMS_FEAT_MULTIPLE_SESSIONS_SUPPORTED;
    p_cgms->feature.type            = NRF_BLE_CGMS_MEAS_TYPE_VEN_BLOOD;
    p_cgms->feature.sample_location = NRF_BLE_CGMS_MEAS_LOC_AST;
    p_cgms->feature.feature        |= NRF_BLE_CGMS_FEAT_MULTIPLE_BOND_SUPPORTED;


	
	memset(m_records, 0, sizeof(m_records));
	m_record_count = 0U;
	p_cgms->m_conn = NULL;
	m_meas_notify_enabled = false;
	m_racp_ind_enabled = false;
	m_socp_ind_enabled = false;
	m_current_offset = 0U;
	cgms_racp_reset_state();
	m_status.time_offset = 0U;
	m_status.status.warning = 0U;
	m_status.status.calib_temp = 0U;
	m_status.status.status = NRF_BLE_CGMS_STATUS_SESSION_STOPPED;
	m_glucose_concentration = MIN_GLUCOSE_CONCENTRATION;
	memset(&m_alert_levels, 0, sizeof(m_alert_levels));
	memset(&m_sst, 0, sizeof(m_sst));
	k_delayed_work_init(&m_glucose_work, cgms_meas_work_handler);
	return 0;
}

uint32_t nrf_ble_cgms_update_status(nrf_ble_cgms_t * p_cgms, const nrf_ble_cgm_status_t * p_status)
{
	uint8_t encoded[5];
	uint8_t len = 0U;

	if ((p_cgms == NULL) || (p_status == NULL)) {
		return EINVAL;
	}

	put_le16(&encoded[len], p_status->time_offset);
	len += sizeof(uint16_t);
	encoded[len++] = p_status->status.status;
	encoded[len++] = p_status->status.calib_temp;
	encoded[len++] = p_status->status.warning;

	if (len != 5U) {
		return EINVAL;
	}

	p_cgms->sensor_status = *p_status;
	m_status = *p_status;
	return 0;
}

void ble_cgms_register_evt_handler(ble_cgms_evt_handler_t handler)
{
	m_evt_handler = handler;
}

void ble_cgms_connected(struct bt_conn *conn)
{
	nrf_ble_cgms_t * p_cgms = ble_cgms_instance_get();

	if ((p_cgms == NULL) || (conn == NULL)) {
		return;
	}

	if (p_cgms->m_conn != NULL) {
		bt_conn_unref(p_cgms->m_conn);
		p_cgms->m_conn = NULL;
	}

	p_cgms->m_conn = bt_conn_ref(conn);
	printk("CGMS connected: %p\n", (void *)p_cgms->m_conn);
}

nrf_ble_cgms_t * ble_cgms_instance_get(void)
{
	return m_cgms_ctx;
}

void ble_cgms_disconnected(nrf_ble_cgms_t * p_cgms, struct bt_conn *conn)
{
	if ((p_cgms != NULL) && (p_cgms->m_conn == conn)) {
		bt_conn_unref(p_cgms->m_conn);
		p_cgms->m_conn = NULL;
	}
	m_meas_notify_enabled = false;
	m_racp_ind_enabled = false;
	m_socp_ind_enabled = false;
	cgms_racp_reset_state();
	cgms_cancel_glucose_work(p_cgms);
	if ((p_cgms != NULL) && p_cgms->is_session_started) {
		cgms_schedule_glucose_work(p_cgms);
	}
}

void ble_cgms_increase_glucose(void)
{
	m_glucose_concentration += GL_CONCENTRATION_INC;
	if (m_glucose_concentration > MAX_GLUCOSE_CONCENTRATION) {
		m_glucose_concentration = MIN_GLUCOSE_CONCENTRATION;
	}
}

void ble_cgms_decrease_glucose(void)
{
	if (m_glucose_concentration <= (MIN_GLUCOSE_CONCENTRATION + GL_CONCENTRATION_DEC)) {
		m_glucose_concentration = MAX_GLUCOSE_CONCENTRATION;
	} else {
		m_glucose_concentration -= GL_CONCENTRATION_DEC;
	}
}
