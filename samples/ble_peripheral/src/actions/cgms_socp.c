#include <errno.h>
#include <string.h>

#include "atb_ble_cgms.h"
#include "cgms_socp.h"
#include "cgms_db.h"
#include "cgms_meas.h"
#include "cgms_sst.h"

void cgms_socp_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);
	m_socp_ind_enabled = (value == BT_GATT_CCC_INDICATE);
}

static bool cgms_socp_response_has_result_code(uint8_t opcode)
{
	switch (opcode) {
	case SOCP_READ_CGM_COMM_INTERVAL_RSP:
	case SOCP_READ_GLUCOSE_CALIBRATION_VALUE_RESPONSE:
	case SOCP_READ_PATIENT_HIGH_ALERT_LEVEL_RESPONSE:
	case SOCP_READ_PATIENT_LOW_ALERT_LEVEL_RESPONSE:
	case SOCP_HYPO_ALERT_LEVEL_RESPONSE:
	case SOCP_HYPER_ALERT_LEVEL_RESPONSE:
	case SOCP_RATE_OF_DECREASE_ALERT_LEVEL_RESPONSE:
	case SOCP_RATE_OF_INCREASE_ALERT_LEVEL_RESPONSE:
		return false;
	default:
		return true;
	}
}

static uint8_t cgms_socp_decode_u16(const ble_cgms_socp_value_t *req, uint16_t *value)
{
	if ((req == NULL) || (value == NULL) || (req->operand_len != sizeof(uint16_t)) || (req->p_operand == NULL)) {
		return SOCP_RSP_INVALID_OPERAND;
	}

	*value = get_le16(req->p_operand);
	if ((*value == NRF_BLE_CGMS_PLUS_INFINITE) || (*value == NRF_BLE_CGMS_MINUS_INFINITE)) {
		return SOCP_RSP_OUT_OF_RANGE;
	}

	return SOCP_RSP_SUCCESS;
}

static void cgms_socp_ind_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params, uint8_t err)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(params);
	ARG_UNUSED(err);
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

static void cgms_socp_decode(const uint8_t *buf, uint16_t len, ble_cgms_socp_value_t *req)
{
	req->opcode = 0xFFU;
	req->operand_len = 0U;
	req->p_operand = NULL;

	if (len > 0U) {
		req->opcode = buf[0];
	}
	if (len > 1U) {
		req->operand_len = len - 1U;
		req->p_operand = (uint8_t *)&buf[1];
	}
}

static uint8_t ble_socp_encode(uint8_t *buf, uint16_t buf_len, const ble_socp_rsp_t *rsp)
{
	uint16_t total_len = 1U;
	uint8_t len = 0U;
	bool with_result_code;

	if ((buf == NULL) || (rsp == NULL)) {
		return 0U;
	}

	if (rsp->size_val > sizeof(rsp->resp_val)) {
		return 0U;
	}

	with_result_code = cgms_socp_response_has_result_code(rsp->opcode);
	if (with_result_code) {
		total_len += 2U;
	}
	total_len += rsp->size_val;

	if (total_len > buf_len) {
		return 0U;
	}

	buf[len++] = rsp->opcode;
	if (with_result_code) {
		buf[len++] = rsp->req_opcode;
		buf[len++] = rsp->rsp_code;
	}
	if (rsp->size_val > 0U) {
		memcpy(&buf[len], rsp->resp_val, rsp->size_val);
		len += rsp->size_val;
	}

	return len;
}

static int cgms_socp_send_response(uint8_t opcode, uint8_t req_opcode, uint8_t rsp_code,
	const uint8_t *value, uint8_t value_len)
{
	uint8_t buf[20];
	uint8_t len;
	ble_socp_rsp_t rsp;

	if (value_len > sizeof(rsp.resp_val)) {
		return -EINVAL;
	}

	memset(&rsp, 0, sizeof(rsp));
	rsp.opcode = opcode;
	rsp.req_opcode = req_opcode;
	rsp.rsp_code = rsp_code;
	rsp.size_val = value_len;
	if ((value != NULL) && (value_len > 0U)) {
		memcpy(rsp.resp_val, value, value_len);
	}

	len = ble_socp_encode(buf, sizeof(buf), &rsp);
	if (len == 0U) {
		return -EINVAL;
	}

	return cgms_socp_indicate(buf, len);
}

static int cgms_socp_send_u16_response(uint8_t opcode, uint16_t value)
{
	uint8_t resp[2];

	put_le16(resp, value);
	return cgms_socp_send_response(opcode, 0U, SOCP_RSP_SUCCESS, resp, sizeof(resp));
}

ssize_t cgms_write_socp(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	ble_cgms_socp_value_t req;
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
		m_comm_interval = cgms_normalize_comm_interval(req.p_operand[0]);
		cgms_emit_event(BLE_CGMS_EVT_WRITE_COMM_INTERVAL);
		cgms_cancel_glucose_work();
		if (m_session_started && (m_comm_interval != 0U)) {
			cgms_schedule_glucose_work();
		}
		(void)cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, SOCP_RSP_SUCCESS, NULL, 0U);
		break;
	case SOCP_READ_CGM_COMMUNICATION_INTERVAL:
		value[0] = m_comm_interval;
		(void)cgms_socp_send_response(SOCP_READ_CGM_COMM_INTERVAL_RSP, req.opcode, SOCP_RSP_SUCCESS, value, 1U);
		break;
	case SOCP_WRITE_GLUCOSE_CALIBRATION_VALUE:
		if (req.operand_len != CGMS_CALIBRATION_VALUE_LEN) {
			(void)cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, SOCP_RSP_INVALID_OPERAND, NULL, 0U);
			break;
		}
		memcpy(m_calibration_value, req.p_operand, CGMS_CALIBRATION_VALUE_LEN);
		(void)cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, SOCP_RSP_SUCCESS, NULL, 0U);
		break;
	case SOCP_READ_GLUCOSE_CALIBRATION_VALUE:
		(void)cgms_socp_send_response(SOCP_READ_GLUCOSE_CALIBRATION_VALUE_RESPONSE, req.opcode, SOCP_RSP_SUCCESS,
			m_calibration_value, CGMS_CALIBRATION_VALUE_LEN);
		break;
	case SOCP_WRITE_PATIENT_HIGH_ALERT_LEVEL:
	case SOCP_WRITE_PATIENT_LOW_ALERT_LEVEL:
	case SOCP_SET_HYPO_ALERT_LEVEL:
	case SOCP_SET_HYPER_ALERT_LEVEL:
	case SOCP_SET_RATE_OF_DECREASE_ALERT_LEVEL:
	case SOCP_SET_RATE_OF_INCREASE_ALERT_LEVEL:
	{
		uint16_t level;
		uint8_t status = cgms_socp_decode_u16(&req, &level);
		if (status == SOCP_RSP_SUCCESS) {
			switch (req.opcode) {
			case SOCP_WRITE_PATIENT_HIGH_ALERT_LEVEL: m_alert_levels.patient_high = level; break;
			case SOCP_WRITE_PATIENT_LOW_ALERT_LEVEL: m_alert_levels.patient_low = level; break;
			case SOCP_SET_HYPO_ALERT_LEVEL: m_alert_levels.hypo = level; break;
			case SOCP_SET_HYPER_ALERT_LEVEL: m_alert_levels.hyper = level; break;
			case SOCP_SET_RATE_OF_DECREASE_ALERT_LEVEL: m_alert_levels.rate_decrease = level; break;
			default: m_alert_levels.rate_increase = level; break;
			}
		}
		(void)cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, status, NULL, 0U);
		break;
	}
	case SOCP_READ_PATIENT_HIGH_ALERT_LEVEL:
		(void)cgms_socp_send_u16_response(SOCP_READ_PATIENT_HIGH_ALERT_LEVEL_RESPONSE, m_alert_levels.patient_high);
		break;
	case SOCP_READ_PATIENT_LOW_ALERT_LEVEL:
		(void)cgms_socp_send_u16_response(SOCP_READ_PATIENT_LOW_ALERT_LEVEL_RESPONSE, m_alert_levels.patient_low);
		break;
	case SOCP_GET_HYPO_ALERT_LEVEL:
		(void)cgms_socp_send_u16_response(SOCP_HYPO_ALERT_LEVEL_RESPONSE, m_alert_levels.hypo);
		break;
	case SOCP_GET_HYPER_ALERT_LEVEL:
		(void)cgms_socp_send_u16_response(SOCP_HYPER_ALERT_LEVEL_RESPONSE, m_alert_levels.hyper);
		break;
	case SOCP_GET_RATE_OF_DECREASE_ALERT_LEVEL:
		(void)cgms_socp_send_u16_response(SOCP_RATE_OF_DECREASE_ALERT_LEVEL_RESPONSE, m_alert_levels.rate_decrease);
		break;
	case SOCP_GET_RATE_OF_INCREASE_ALERT_LEVEL:
		(void)cgms_socp_send_u16_response(SOCP_RATE_OF_INCREASE_ALERT_LEVEL_RESPONSE, m_alert_levels.rate_increase);
		break;
	case SOCP_RESET_DEVICE_SPECIFIC_ALERT:
		m_status.annunciation.status &= (uint8_t)(~NRF_BLE_CGMS_STATUS_DEVICE_SPECIFIC_ALERT);
		(void)cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, SOCP_RSP_SUCCESS, NULL, 0U);
		break;
	case SOCP_START_THE_SESSION:
		if (m_session_started) {
			(void)cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, SOCP_RSP_PROCEDURE_NOT_COMPLETED, NULL, 0U);
			break;
		}
		if ((m_nb_run_session != 0U) && !cgms_feature_present(NRF_BLE_CGMS_FEAT_MULTIPLE_SESSIONS_SUPPORTED)) {
			(void)cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, SOCP_RSP_PROCEDURE_NOT_COMPLETED, NULL, 0U);
			break;
		}
		if (cgms_sst_set(NULL, &m_sst) != 0) {
			(void)cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, SOCP_RSP_PROCEDURE_NOT_COMPLETED, NULL, 0U);
			break;
		}
		cgms_start_session();
		(void)cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, SOCP_RSP_SUCCESS, NULL, 0U);
		break;
	case SOCP_STOP_THE_SESSION:
		cgms_stop_session();
		(void)cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, SOCP_RSP_SUCCESS, NULL, 0U);
		break;
	default:
		(void)cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, SOCP_RSP_OP_CODE_NOT_SUPPORTED, NULL, 0U);
		break;
	}

	return len;
}
