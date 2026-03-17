/*
 * Port of the nRF5 SDK CGMS demo to the cgm_sdk ble_peripheral sample.
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/byteorder.h>
#include <sys/printk.h>
#include <zephyr.h>

#include <bluetooth/att.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/uuid.h>

#include "ble_super_service.h"
#include "ble_data_test_sample.h"

#define CGMS_DB_MAX_RECORDS                         32
#define GLUCOSE_MEAS_INTERVAL_MINUTES              1
#define GL_CONCENTRATION_INC                       10
#define GL_CONCENTRATION_DEC                       5
#define MAX_GLUCOSE_CONCENTRATION                  800
#define MIN_GLUCOSE_CONCENTRATION                  5
#define RACP_OPERAND_FILTER_TYPE_TIME_OFFSET       0x01
#define RACP_OPERAND_FILTER_TYPE_FACING_TIME       0x02

#define RACP_OPCODE_RESERVED                       0x00
#define RACP_OPCODE_REPORT_RECS                    0x01
#define RACP_OPCODE_DELETE_RECS                    0x02
#define RACP_OPCODE_ABORT_OPERATION                0x03
#define RACP_OPCODE_REPORT_NUM_RECS                0x04
#define RACP_OPCODE_NUM_RECS_RESPONSE              0x05
#define RACP_OPCODE_RESPONSE_CODE                  0x06

#define RACP_OPERATOR_NULL                         0x00
#define RACP_OPERATOR_ALL                          0x01
#define RACP_OPERATOR_LESS_OR_EQUAL                0x02
#define RACP_OPERATOR_GREATER_OR_EQUAL             0x03
#define RACP_OPERATOR_RANGE                        0x04
#define RACP_OPERATOR_FIRST                        0x05
#define RACP_OPERATOR_LAST                         0x06

/**@brief Record Access Control Point Operand Filter Type Value. */
#define RACP_OPERAND_FILTER_TYPE_TIME_OFFSET 	   0x01       /**< Record Access Control Point Operand Filter Type Value - Time Offset. */
#define RACP_OPERAND_FILTER_TYPE_FACING_TIME       0x02     // 框架未实现

#define RACP_RESPONSE_RESERVED               	   0x00 
#define RACP_RESPONSE_SUCCESS                      0x01
#define RACP_RESPONSE_OPCODE_UNSUPPORTED           0x02
#define RACP_RESPONSE_INVALID_OPERATOR             0x03
#define RACP_RESPONSE_OPERATOR_UNSUPPORTED         0x04
#define RACP_RESPONSE_INVALID_OPERAND              0x05
#define RACP_RESPONSE_NO_RECORDS_FOUND             0x06
#define RACP_RESPONSE_ABORT_FAILED                 0x07
#define RACP_RESPONSE_PROCEDURE_NOT_DONE           0x08
#define RACP_RESPONSE_OPERAND_UNSUPPORTED          0x09

#define SOCP_OPCODE_RESERVED                       0x00
#define SOCP_WRITE_CGM_COMMUNICATION_INTERVAL      0x01
#define SOCP_READ_CGM_COMMUNICATION_INTERVAL       0x02
#define SOCP_READ_CGM_COMM_INTERVAL_RSP            0x03
#define SOCP_WRITE_GLUCOSE_CALIBRATION_VALUE          0x04
#define SOCP_READ_GLUCOSE_CALIBRATION_VALUE           0x05
#define SOCP_READ_GLUCOSE_CALIBRATION_VALUE_RESPONSE  0x06
#define SOCP_WRITE_PATIENT_HIGH_ALERT_LEVEL           0x07
#define SOCP_READ_PATIENT_HIGH_ALERT_LEVEL            0x08
#define SOCP_READ_PATIENT_HIGH_ALERT_LEVEL_RESPONSE   0x09
#define SOCP_WRITE_PATIENT_LOW_ALERT_LEVEL            0x0A
#define SOCP_READ_PATIENT_LOW_ALERT_LEVEL             0x0B
#define SOCP_READ_PATIENT_LOW_ALERT_LEVEL_RESPONSE    0x0C
#define SOCP_SET_HYPO_ALERT_LEVEL                     0x0D /**Set Hypo Alert Level    Hypo Alert Level value in mg/dL    The response to this control point is Response Code.                                            */
#define SOCP_GET_HYPO_ALERT_LEVEL                     0x0E /**Get Hypo Alert Level    N/A    The normal response to this control point is Op Code 0x0F. For error conditions, the response is Response Code              */
#define SOCP_HYPO_ALERT_LEVEL_RESPONSE                0x0F /**Hypo Alert Level Response    Hypo Alert Level value in mg/dL    This is the normal response to Op Code 0x0E                                             */
#define SOCP_SET_HYPER_ALERT_LEVEL                    0x10 /**Set Hyper Alert Level    Hyper Alert Level value in mg/dL    The response to this control point is Response Code.                                    */
#define SOCP_GET_HYPER_ALERT_LEVEL                    0x11 /**Get Hyper Alert Level    N/A    The normal response to this control point is Op Code 0x12. For error conditions, the response is Response Code          */
#define SOCP_HYPER_ALERT_LEVEL_RESPONSE               0x12 /**Hyper Alert Level Response    Hyper Alert Level value in mg/dL    This is the normal response to Op Code 0x11                                         */
#define SOCP_SET_RATE_OF_DECREASE_ALERT_LEVEL         0x13 /**Set Rate of Decrease Alert Level    Rate of Decrease Alert Level value in mg/dL/min    The response to this control point is Response Code.                */
#define SOCP_GET_RATE_OF_DECREASE_ALERT_LEVEL         0x14 /**Get Rate of Decrease Alert Level    N/A    The normal response to this control point is Op Code 0x15. For error conditions, the response is Response Code  */
#define SOCP_RATE_OF_DECREASE_ALERT_LEVEL_RESPONSE    0x15 /**Rate of Decrease Alert Level Response    Rate of Decrease Alert Level value in mg/dL/min    This is the normal response to Op Code 0x14                 */
#define SOCP_SET_RATE_OF_INCREASE_ALERT_LEVEL         0x16 /**Set Rate of Increase Alert Level    Rate of Increase Alert Level value in mg/dL/min    The response to this control point is Response Code.                */
#define SOCP_GET_RATE_OF_INCREASE_ALERT_LEVEL         0x17 /**Get Rate of Increase Alert Level    N/A    The normal response to this control point is Op Code 0x18. For error conditions, the response is Response Code  */
#define SOCP_RATE_OF_INCREASE_ALERT_LEVEL_RESPONSE    0x18 /**Rate of Increase Alert Level Response    Rate of Increase Alert Level value in mg/dL/min    This is the normal response to Op Code 0x17                 */
#define SOCP_RESET_DEVICE_SPECIFIC_ALERT              0x19 /**Reset Device Specific Alert    N/A    The response to this control point is Response Code. */

#define SOCP_START_THE_SESSION                     0x1A
#define SOCP_STOP_THE_SESSION                      0x1B
#define SOCP_RESPONSE_CODE                         0x1C

#define SOCP_RSP_RESERVED_FOR_FUTURE_USE           0x00
#define SOCP_RSP_SUCCESS                           0x01
#define SOCP_RSP_OP_CODE_NOT_SUPPORTED             0x02
#define SOCP_RSP_INVALID_OPERAND                   0x03
#define SOCP_RSP_PROCEDURE_NOT_COMPLETED           0x04
#define SOCP_RSP_OUT_OF_RANGE                      0x05

#define NRF_BLE_CGMS_FEAT_MULTIPLE_BOND_SUPPORTED      (0x01 << 13)
#define NRF_BLE_CGMS_FEAT_MULTIPLE_SESSIONS_SUPPORTED  (0x01 << 14)
#define NRF_BLE_CGMS_STATUS_SESSION_STOPPED            (0x01 << 0)
#define NRF_BLE_CGMS_MEAS_TYPE_VEN_BLOOD               0x03
#define NRF_BLE_CGMS_MEAS_LOC_AST                      0x02
#define NRF_BLE_CGMS_STATUS_FLAGS_WARNING_OCT_PRESENT  0x20
#define NRF_BLE_CGMS_STATUS_FLAGS_CALTEMP_OCT_PRESENT  0x40
#define NRF_BLE_CGMS_STATUS_FLAGS_STATUS_OCT_PRESENT   0x80

struct cgms_sensor_annunciation {
	uint8_t warning;
	uint8_t calib_temp;
	uint8_t status;
};

struct cgms_measurement {
	uint8_t flags;
	uint16_t glucose_concentration;
	uint16_t time_offset;
	struct cgms_sensor_annunciation sensor_status_annunciation;
	uint16_t trend;
	uint16_t quality;
};

struct cgms_record {
	struct cgms_measurement meas;
};

struct cgms_status {
	uint16_t time_offset;
	struct cgms_sensor_annunciation annunciation;
};

struct cgms_feature_value {
	uint32_t feature;
	uint8_t type;
	uint8_t sample_location;
};

struct cgms_sst_value {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
	uint8_t time_zone;
	uint8_t dst;
};

struct racp_request {
	uint8_t opcode;
	uint8_t operator;
	uint8_t operand_len;
	const uint8_t *operand;
};

struct racp_response {
	uint8_t opcode;
	uint8_t operator;
	uint8_t operand[4];
	uint8_t operand_len;
};

struct socp_request {
	uint8_t opcode;
	uint8_t operand_len;
	const uint8_t *operand;
};

static struct bt_conn *m_conn;
static ble_cgms_evt_handler_t m_evt_handler;
static struct k_delayed_work m_glucose_work;
static struct cgms_record m_records[CGMS_DB_MAX_RECORDS];
static uint16_t m_record_count;
static struct cgms_feature_value m_feature = {
	.feature = NRF_BLE_CGMS_FEAT_MULTIPLE_BOND_SUPPORTED |
		   NRF_BLE_CGMS_FEAT_MULTIPLE_SESSIONS_SUPPORTED,
	.type = NRF_BLE_CGMS_MEAS_TYPE_VEN_BLOOD,
	.sample_location = NRF_BLE_CGMS_MEAS_LOC_AST,
};
static struct cgms_status m_status = {
	.time_offset = 0,
	.annunciation = {
		.warning = 0,
		.calib_temp = 0,
		.status = NRF_BLE_CGMS_STATUS_SESSION_STOPPED,
	},
};
static struct cgms_sst_value m_sst;
static uint16_t m_session_run_time = 20;
static uint8_t m_comm_interval = GLUCOSE_MEAS_INTERVAL_MINUTES;
static bool m_session_started;
static uint8_t m_nb_run_session;
static uint16_t m_current_offset;
static uint16_t m_glucose_concentration = MIN_GLUCOSE_CONCENTRATION;
static bool m_meas_notify_enabled;
static bool m_racp_ind_enabled;
static bool m_socp_ind_enabled;
static struct bt_gatt_indicate_params m_racp_ind_params;
static struct bt_gatt_indicate_params m_socp_ind_params;
static uint8_t m_racp_ind_buf[8];
static uint8_t m_socp_ind_buf[20];

enum {
	CGMS_ATTR_SVC = 0,
	CGMS_ATTR_MEAS_CHRC,
	CGMS_ATTR_MEAS_VAL,
	CGMS_ATTR_MEAS_CCC,
	CGMS_ATTR_FEATURE_CHRC,
	CGMS_ATTR_FEATURE_VAL,
	CGMS_ATTR_STATUS_CHRC,
	CGMS_ATTR_STATUS_VAL,
	CGMS_ATTR_SST_CHRC,
	CGMS_ATTR_SST_VAL,
	CGMS_ATTR_SRT_CHRC,
	CGMS_ATTR_SRT_VAL,
	CGMS_ATTR_RACP_CHRC,
	CGMS_ATTR_RACP_VAL,
	CGMS_ATTR_RACP_CCC,
	CGMS_ATTR_SOCP_CHRC,
	CGMS_ATTR_SOCP_VAL,
	CGMS_ATTR_SOCP_CCC,
};

static void cgms_emit_event(ble_cgms_evt_type_t evt_type)
{
	ble_cgms_evt_t evt;

	if (m_evt_handler == NULL) {
		return;
	}

	evt.evt_type = evt_type;
	evt.comm_interval = m_comm_interval;
	m_evt_handler(&evt);
}

static void put_le16(uint8_t *dst, uint16_t value)
{
	dst[0] = (uint8_t)(value & 0xFF);
	dst[1] = (uint8_t)(value >> 8);
}

static void put_le24(uint8_t *dst, uint32_t value)
{
	dst[0] = (uint8_t)(value & 0xFF);
	dst[1] = (uint8_t)((value >> 8) & 0xFF);
	dst[2] = (uint8_t)((value >> 16) & 0xFF);
}

static uint16_t get_le16(const uint8_t *src)
{
	return (uint16_t)src[0] | ((uint16_t)src[1] << 8);
}

static void cgms_schedule_glucose_work(void)
{
	if (!m_session_started || (m_comm_interval == 0U)) {
		return;
	}

	k_delayed_work_submit(&m_glucose_work, K_MINUTES(m_comm_interval));
}

static void cgms_cancel_glucose_work(void)
{
	k_delayed_work_cancel(&m_glucose_work);
}

static void cgms_record_add(const struct cgms_record *rec)
{
	if (m_record_count >= CGMS_DB_MAX_RECORDS) {
		memmove(&m_records[0], &m_records[1], sizeof(m_records[0]) * (CGMS_DB_MAX_RECORDS - 1));
		m_record_count = CGMS_DB_MAX_RECORDS - 1;
	}

	m_records[m_record_count++] = *rec;
}

static uint8_t cgms_encode_feature(uint8_t *buf)
{
	put_le24(buf, m_feature.feature);
	buf[3] = (uint8_t)((m_feature.sample_location << 4) | (m_feature.type & 0x0F));
	buf[4] = 0xFF;
	buf[5] = 0xFF;
	return 6;
}

static uint8_t cgms_encode_status(uint8_t *buf)
{
	put_le16(buf, m_status.time_offset);
	buf[2] = m_status.annunciation.status;
	buf[3] = m_status.annunciation.calib_temp;
	buf[4] = m_status.annunciation.warning;
	return 5;
}

static uint8_t cgms_encode_sst(uint8_t *buf)
{
	put_le16(&buf[0], m_sst.year);
	buf[2] = m_sst.month;
	buf[3] = m_sst.day;
	buf[4] = m_sst.hours;
	buf[5] = m_sst.minutes;
	buf[6] = m_sst.seconds;
	buf[7] = m_sst.time_zone;
	buf[8] = m_sst.dst;
	return 9;
}

static uint8_t cgms_encode_measurement(const struct cgms_record *rec, uint8_t *buf)
{
	uint8_t len = 2;
	uint8_t flags = rec->meas.flags;

	put_le16(&buf[len], rec->meas.glucose_concentration);
	len += 2;
	put_le16(&buf[len], rec->meas.time_offset);
	len += 2;

	if (rec->meas.sensor_status_annunciation.warning != 0U) {
		buf[len++] = rec->meas.sensor_status_annunciation.warning;
		flags |= NRF_BLE_CGMS_STATUS_FLAGS_WARNING_OCT_PRESENT;
	}
	if (rec->meas.sensor_status_annunciation.calib_temp != 0U) {
		buf[len++] = rec->meas.sensor_status_annunciation.calib_temp;
		flags |= NRF_BLE_CGMS_STATUS_FLAGS_CALTEMP_OCT_PRESENT;
	}
	if (rec->meas.sensor_status_annunciation.status != 0U) {
		buf[len++] = rec->meas.sensor_status_annunciation.status;
		flags |= NRF_BLE_CGMS_STATUS_FLAGS_STATUS_OCT_PRESENT;
	}

	buf[0] = len;
	buf[1] = flags;
	return len;
}

static void cgms_meas_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);
	m_meas_notify_enabled = (value == BT_GATT_CCC_NOTIFY);
	printk("CGMS measurement ccc %u\n", value);
	cgms_emit_event(m_meas_notify_enabled ? BLE_CGMS_EVT_NOTIFICATION_ENABLED :
			BLE_CGMS_EVT_NOTIFICATION_DISABLED);
}

static void cgms_racp_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);
	m_racp_ind_enabled = (value == BT_GATT_CCC_INDICATE);
}

static void cgms_socp_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);
	m_socp_ind_enabled = (value == BT_GATT_CCC_INDICATE);
}

static ssize_t cgms_read_feature(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	void *buf, uint16_t len, uint16_t offset)
{
	uint8_t value[6];
	uint8_t value_len = cgms_encode_feature(value);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, value_len);
}

static ssize_t cgms_read_status(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	void *buf, uint16_t len, uint16_t offset)
{
	uint8_t value[5];
	uint8_t value_len = cgms_encode_status(value);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, value_len);
}

static ssize_t cgms_read_sst(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	void *buf, uint16_t len, uint16_t offset)
{
	uint8_t value[9];
	uint8_t value_len = cgms_encode_sst(value);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, value_len);
}

static ssize_t cgms_read_srt(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	void *buf, uint16_t len, uint16_t offset)
{
	uint8_t value[2];
	put_le16(value, m_session_run_time);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(value));
}

static void cgms_sst_store_from_raw(const uint8_t *buf)
{
	m_sst.year = get_le16(&buf[0]);
	m_sst.month = buf[2];
	m_sst.day = buf[3];
	m_sst.hours = buf[4];
	m_sst.minutes = buf[5];
	m_sst.seconds = buf[6];
	m_sst.time_zone = buf[7];
	m_sst.dst = buf[8];
}

static ssize_t cgms_write_sst(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (offset != 0U) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (len != 9U) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	cgms_sst_store_from_raw((const uint8_t *)buf);
	return len;
}

static void cgms_racp_ind_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params, uint8_t err)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(params);
	printk("RACP indicate done err=%u\n", err);
}

static void cgms_socp_ind_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params, uint8_t err)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(params);
	printk("SOCP indicate done err=%u\n", err);
}

static int cgms_racp_indicate(const uint8_t *data, uint16_t len)
{
	if ((m_conn == NULL) || !m_racp_ind_enabled) {
		return -ENOTCONN;
	}

	memcpy(m_racp_ind_buf, data, len);
	memset(&m_racp_ind_params, 0, sizeof(m_racp_ind_params));
	m_racp_ind_params.attr = &attr_cgms_svc[CGMS_ATTR_RACP_VAL];
	m_racp_ind_params.data = m_racp_ind_buf;
	m_racp_ind_params.len = len;
	m_racp_ind_params.func = cgms_racp_ind_cb;
	return bt_gatt_indicate(m_conn, &m_racp_ind_params);
}

static int cgms_socp_indicate(const uint8_t *data, uint16_t len)
{
	if ((m_conn == NULL) || !m_socp_ind_enabled) {
		return -ENOTCONN;
	}

	memcpy(m_socp_ind_buf, data, len);
	memset(&m_socp_ind_params, 0, sizeof(m_socp_ind_params));
	m_socp_ind_params.attr = &attr_cgms_svc[CGMS_ATTR_SOCP_VAL];
	m_socp_ind_params.data = m_socp_ind_buf;
	m_socp_ind_params.len = len;
	m_socp_ind_params.func = cgms_socp_ind_cb;
	return bt_gatt_indicate(m_conn, &m_socp_ind_params);
}

static int cgms_measurement_notify(const struct cgms_record *rec)
{
	uint8_t encoded[16];
	uint8_t len;

	if ((m_conn == NULL) || !m_meas_notify_enabled) {
		return -ENOTCONN;
	}

	len = cgms_encode_measurement(rec, encoded);
	return bt_gatt_notify(m_conn, &attr_cgms_svc[CGMS_ATTR_MEAS_VAL], encoded, len);
}

static void cgms_send_racp_response_code(uint8_t req_opcode, uint8_t rsp_code)
{
	uint8_t buf[4];

	buf[0] = RACP_OPCODE_RESPONSE_CODE;
	buf[1] = RACP_OPERATOR_NULL;
	buf[2] = req_opcode;
	buf[3] = rsp_code;
	(void)cgms_racp_indicate(buf, sizeof(buf));
}

static void cgms_send_racp_num_records(uint16_t count)
{
	uint8_t buf[4];

	buf[0] = RACP_OPCODE_NUM_RECS_RESPONSE;
	buf[1] = RACP_OPERATOR_NULL;
	put_le16(&buf[2], count);
	(void)cgms_racp_indicate(buf, sizeof(buf));
}

static void cgms_racp_decode(const uint8_t *buf, uint16_t len, struct racp_request *req)
{
	req->opcode = 0xFFU;
	req->operator = 0xFFU;
	req->operand_len = 0U;
	req->operand = NULL;

	if (len > 0U) {
		req->opcode = buf[0];
	}
	if (len > 1U) {
		req->operator = buf[1];
	}
	if (len > 2U) {
		req->operand_len = len - 2U;
		req->operand = &buf[2];
	}
}

static void cgms_socp_decode(const uint8_t *buf, uint16_t len, struct socp_request *req)
{
	req->opcode = 0xFFU;
	req->operand_len = 0U;
	req->operand = NULL;

	if (len > 0U) {
		req->opcode = buf[0];
	}
	if (len > 1U) {
		req->operand_len = len - 1U;
		req->operand = &buf[1];
	}
}

static bool cgms_racp_request_valid(const struct racp_request *req, uint8_t *rsp_code)
{
	*rsp_code = 0U;

	if (req->opcode == RACP_OPCODE_ABORT_OPERATION) {
		*rsp_code = RACP_RESPONSE_ABORT_FAILED;
		return false;
	}

	if ((req->opcode != RACP_OPCODE_REPORT_RECS) &&
	    (req->opcode != RACP_OPCODE_REPORT_NUM_RECS)) {
		*rsp_code = RACP_RESPONSE_OPCODE_UNSUPPORTED;
		return false;
	}

	switch (req->operator) {
	case RACP_OPERATOR_ALL:
	case RACP_OPERATOR_FIRST:
	case RACP_OPERATOR_LAST:
		if (req->operand_len != 0U) {
			*rsp_code = RACP_RESPONSE_INVALID_OPERAND;
			return false;
		}
		return true;
	case RACP_OPERATOR_LESS_OR_EQUAL:
	case RACP_OPERATOR_GREATER_OR_EQUAL:
		if (req->operand_len != 3U) {
			*rsp_code = RACP_RESPONSE_INVALID_OPERAND;
			return false;
		}
		if (req->operand[0] == RACP_OPERAND_FILTER_TYPE_FACING_TIME) {
			*rsp_code = RACP_RESPONSE_PROCEDURE_NOT_DONE;
			return false;
		}
		if (req->operand[0] != RACP_OPERAND_FILTER_TYPE_TIME_OFFSET) {
			*rsp_code = RACP_RESPONSE_INVALID_OPERAND;
			return false;
		}
		return true;
	case RACP_OPERATOR_RANGE:
		*rsp_code = RACP_RESPONSE_OPERATOR_UNSUPPORTED;
		return false;
	default:
		*rsp_code = RACP_RESPONSE_INVALID_OPERATOR;
		return false;
	}
}

static bool cgms_record_matches(const struct cgms_record *rec, const struct racp_request *req)
{
	uint16_t offset;

	switch (req->operator) {
	case RACP_OPERATOR_ALL:
		return true;
	case RACP_OPERATOR_FIRST:
	case RACP_OPERATOR_LAST:
		return true;
	case RACP_OPERATOR_LESS_OR_EQUAL:
		offset = get_le16(&req->operand[1]);
		return rec->meas.time_offset <= offset;
	case RACP_OPERATOR_GREATER_OR_EQUAL:
		offset = get_le16(&req->operand[1]);
		return rec->meas.time_offset >= offset;
	default:
		return false;
	}
}

static uint16_t cgms_count_matching_records(const struct racp_request *req)
{
	uint16_t i;
	uint16_t count = 0U;

	if (req->operator == RACP_OPERATOR_FIRST || req->operator == RACP_OPERATOR_LAST) {
		return (m_record_count > 0U) ? 1U : 0U;
	}

	for (i = 0U; i < m_record_count; i++) {
		if (cgms_record_matches(&m_records[i], req)) {
			count++;
		}
	}

	return count;
}

static uint16_t cgms_report_records(const struct racp_request *req)
{
	uint16_t sent = 0U;
	uint16_t i;

	if (req->operator == RACP_OPERATOR_FIRST) {
		if (m_record_count > 0U && cgms_measurement_notify(&m_records[0]) == 0) {
			return 1U;
		}
		return 0U;
	}

	if (req->operator == RACP_OPERATOR_LAST) {
		if (m_record_count > 0U && cgms_measurement_notify(&m_records[m_record_count - 1U]) == 0) {
			return 1U;
		}
		return 0U;
	}

	for (i = 0U; i < m_record_count; i++) {
		if (!cgms_record_matches(&m_records[i], req)) {
			continue;
		}
		if (cgms_measurement_notify(&m_records[i]) != 0) {
			break;
		}
		sent++;
	}

	return sent;
}

static ssize_t cgms_write_racp(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	struct racp_request req;
	uint8_t rsp_code;
	uint16_t count;

	ARG_UNUSED(conn);
	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (offset != 0U) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (!m_racp_ind_enabled) {
		return BT_GATT_ERR(BT_ATT_ERR_CCC_IMPROPER_CONF);
	}

	cgms_racp_decode((const uint8_t *)buf, len, &req);
	if (!cgms_racp_request_valid(&req, &rsp_code)) {
		cgms_send_racp_response_code(req.opcode, rsp_code);
		return len;
	}

	if (req.opcode == RACP_OPCODE_REPORT_NUM_RECS) {
		cgms_send_racp_num_records(cgms_count_matching_records(&req));
		return len;
	}
	
	count = cgms_report_records(&req);
	if (count > 0U) {
		cgms_send_racp_response_code(RACP_OPCODE_REPORT_RECS, RACP_RESPONSE_SUCCESS);
	} else {
		cgms_send_racp_response_code(RACP_OPCODE_REPORT_RECS, RACP_RESPONSE_NO_RECORDS_FOUND);
	}

	return len;
}

static void cgms_socp_send_response(uint8_t opcode, uint8_t req_opcode, uint8_t rsp_code,
	const uint8_t *value, uint8_t value_len)
{
	uint8_t buf[20];
	uint8_t len = 0U;

	buf[len++] = opcode;
	if ((opcode != SOCP_READ_CGM_COMM_INTERVAL_RSP)) {
		buf[len++] = req_opcode;
		buf[len++] = rsp_code;
	}
	if ((value != NULL) && (value_len > 0U)) {
		memcpy(&buf[len], value, value_len);
		len += value_len;
	}

	(void)cgms_socp_indicate(buf, len);
}

static void cgms_start_session(void)
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

static void cgms_stop_session(void)
{
	m_session_started = false;
	m_status.annunciation.status |= NRF_BLE_CGMS_STATUS_SESSION_STOPPED;
	cgms_cancel_glucose_work();
	cgms_emit_event(BLE_CGMS_EVT_STOP_SESSION);
}

static ssize_t cgms_write_socp(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	struct socp_request req;
	uint8_t value[2];

	ARG_UNUSED(conn);
	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (offset != 0U) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (!m_socp_ind_enabled) {
		return BT_GATT_ERR(BT_ATT_ERR_CCC_IMPROPER_CONF);
	}

	cgms_socp_decode((const uint8_t *)buf, len, &req);

	switch (req.opcode) {
	case SOCP_WRITE_CGM_COMMUNICATION_INTERVAL:
		if (req.operand_len < 1U) {
			cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, SOCP_RSP_INVALID_OPERAND, NULL, 0U);
			break;
		}
		m_comm_interval = req.operand[0];
		cgms_emit_event(BLE_CGMS_EVT_WRITE_COMM_INTERVAL);
		cgms_cancel_glucose_work();
		if (m_session_started && (m_comm_interval != 0U)) {
			cgms_schedule_glucose_work();
		}
		cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, SOCP_RSP_SUCCESS, NULL, 0U);
		break;
	case SOCP_READ_CGM_COMMUNICATION_INTERVAL:
		value[0] = m_comm_interval;
		cgms_socp_send_response(SOCP_READ_CGM_COMM_INTERVAL_RSP, req.opcode, SOCP_RSP_SUCCESS, value, 1U);
		break;
	case SOCP_START_THE_SESSION:
		if (m_session_started) {
			cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, SOCP_RSP_PROCEDURE_NOT_COMPLETED, NULL, 0U);
			break;
		}
		cgms_start_session();
		cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, SOCP_RSP_SUCCESS, NULL, 0U);
		break;
	case SOCP_STOP_THE_SESSION:
		cgms_stop_session();
		cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, SOCP_RSP_SUCCESS, NULL, 0U);
		break;
	default:
		cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, SOCP_RSP_OP_CODE_NOT_SUPPORTED, NULL, 0U);
		break;
	}

	return len;
}

static void cgms_glucose_work_handler(struct k_work *work)
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
	rec.meas.flags = 0U;
	rec.meas.sensor_status_annunciation.warning = 0U;
	rec.meas.sensor_status_annunciation.calib_temp = 0U;
	rec.meas.sensor_status_annunciation.status = 0U;

	cgms_record_add(&rec);
	m_status.time_offset = m_current_offset;
	(void)cgms_measurement_notify(&rec);
	cgms_schedule_glucose_work();
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
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		cgms_read_sst, cgms_write_sst, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(BT_UUID_CGM_SESSION_RUN_TIME_VAL),
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		cgms_read_srt, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(BT_UUID_RECORD_ACCESS_CONTROL_POINT_VAL),
		BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,
		BT_GATT_PERM_WRITE,
		NULL, cgms_write_racp, NULL),
	BT_GATT_CCC(cgms_racp_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(BT_UUID_CGM_SPECIFIC_OPS_CTRL_PT_VAL),
		BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,
		BT_GATT_PERM_WRITE,
		NULL, cgms_write_socp, NULL),
	BT_GATT_CCC(cgms_socp_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
);

void ble_cgms_init(void)
{
	memset(m_records, 0, sizeof(m_records));
	m_record_count = 0U;
	m_conn = NULL;
	m_meas_notify_enabled = false;
	m_racp_ind_enabled = false;
	m_socp_ind_enabled = false;
	m_session_started = false;
	m_nb_run_session = 0U;
	m_current_offset = 0U;
	m_status.time_offset = 0U;
	m_status.annunciation.warning = 0U;
	m_status.annunciation.calib_temp = 0U;
	m_status.annunciation.status = NRF_BLE_CGMS_STATUS_SESSION_STOPPED;
	m_comm_interval = GLUCOSE_MEAS_INTERVAL_MINUTES;
	m_glucose_concentration = MIN_GLUCOSE_CONCENTRATION;
	memset(&m_sst, 0, sizeof(m_sst));
	k_delayed_work_init(&m_glucose_work, cgms_glucose_work_handler);
	printk("CGMS service initialized\n");
}

void ble_cgms_register_evt_handler(ble_cgms_evt_handler_t handler)
{
	m_evt_handler = handler;
}

void ble_cgms_connected(struct bt_conn *conn)
{
	m_conn = conn;
}

void ble_cgms_disconnected(struct bt_conn *conn)
{
	if (m_conn == conn) {
		m_conn = NULL;
	}
	m_meas_notify_enabled = false;
	m_racp_ind_enabled = false;
	m_socp_ind_enabled = false;
	cgms_cancel_glucose_work();
	if (m_session_started) {
		cgms_schedule_glucose_work();
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
