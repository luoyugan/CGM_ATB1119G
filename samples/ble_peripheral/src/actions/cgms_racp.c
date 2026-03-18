#include <errno.h>
#include <string.h>

#include "cgms_racp.h"
#include "cgms_db.h"
#include "cgms_meas.h"

void cgms_racp_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);
	m_racp_ind_enabled = (value == BT_GATT_CCC_INDICATE);
}

static void cgms_racp_ind_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params, uint8_t err)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(params);
	ARG_UNUSED(err);
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

static uint8_t ble_racp_encode(uint8_t *buf, uint16_t buf_len, const ble_racp_value_t *racp)
{
	uint16_t total_len;

	if ((buf == NULL) || (racp == NULL)) {
		return 0U;
	}
	if ((racp->operand_len > 0U) && (racp->p_operand == NULL)) {
		return 0U;
	}

	total_len = (uint16_t)(2U + racp->operand_len);
	if (total_len > buf_len) {
		return 0U;
	}

	buf[0] = racp->opcode;
	buf[1] = racp->operator;
	if (racp->operand_len > 0U) {
		memcpy(&buf[2], racp->p_operand, racp->operand_len);
	}

	return (uint8_t)total_len;
}

static void cgms_send_racp_response_code(uint8_t req_opcode, uint8_t rsp_code)
{
	uint8_t buf[8];
	uint8_t operand[2];
	uint8_t len;
	ble_racp_value_t rsp;

	operand[0] = req_opcode;
	operand[1] = rsp_code;
	rsp.opcode = RACP_OPCODE_RESPONSE_CODE;
	rsp.operator = RACP_OPERATOR_NULL;
	rsp.operand_len = sizeof(operand);
	rsp.p_operand = operand;

	len = ble_racp_encode(buf, sizeof(buf), &rsp);
	if (len == 0U) {
		return;
	}

	(void)cgms_racp_indicate(buf, len);
}

static void cgms_send_racp_num_records(uint16_t count)
{
	uint8_t buf[8];
	uint8_t operand[2];
	uint8_t len;
	ble_racp_value_t rsp;

	put_le16(operand, count);
	rsp.opcode = RACP_OPCODE_NUM_RECS_RESPONSE;
	rsp.operator = RACP_OPERATOR_NULL;
	rsp.operand_len = sizeof(operand);
	rsp.p_operand = operand;

	len = ble_racp_encode(buf, sizeof(buf), &rsp);
	if (len == 0U) {
		return;
	}

	(void)cgms_racp_indicate(buf, len);
}

static void cgms_racp_decode(const uint8_t *buf, uint16_t len, ble_racp_value_t *req)
{
	req->opcode = 0xFFU;
	req->operator = 0xFFU;
	req->operand_len = 0U;
	req->p_operand = NULL;

	if (len > 0U) {
		req->opcode = buf[0];
	}
	if (len > 1U) {
		req->operator = buf[1];
	}
	if (len > 2U) {
		req->operand_len = len - 2U;
		req->p_operand = (uint8_t *)&buf[2];
	}
}

static bool cgms_racp_request_valid(const ble_racp_value_t *req, uint8_t *rsp_code)
{
	*rsp_code = RACP_RESPONSE_RESERVED;

	if (req->opcode == RACP_OPCODE_ABORT_OPERATION) {
		if (!m_racp.processing_active) {
			*rsp_code = RACP_RESPONSE_ABORT_FAILED;
		} else if (req->operator != RACP_OPERATOR_NULL) {
			*rsp_code = RACP_RESPONSE_INVALID_OPERATOR;
		} else if (req->operand_len != 0U) {
			*rsp_code = RACP_RESPONSE_INVALID_OPERAND;
		} else {
			*rsp_code = RACP_RESPONSE_SUCCESS;
		}
		return false;
	}

	if (m_racp.processing_active) {
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
		if (req->operand_len != OPERAND_LESS_GREATER_SIZE) {
			*rsp_code = RACP_RESPONSE_INVALID_OPERAND;
			return false;
		}
		if (req->p_operand[0] == RACP_OPERAND_FILTER_TYPE_FACING_TIME) {
			*rsp_code = RACP_RESPONSE_PROCEDURE_NOT_DONE;
			return false;
		}
		if (req->p_operand[0] != RACP_OPERAND_FILTER_TYPE_TIME_OFFSET) {
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

static uint16_t cgms_count_matching_records(const ble_racp_value_t *req)
{
	uint16_t total_records = m_record_count;
	uint16_t record_index;

	switch (req->operator) {
	case RACP_OPERATOR_ALL:
		return total_records;
	case RACP_OPERATOR_FIRST:
	case RACP_OPERATOR_LAST:
		return (total_records > 0U) ? 1U : 0U;
	case RACP_OPERATOR_LESS_OR_EQUAL:
		if (cgms_record_index_offset_less_or_equal_get(get_le16(&req->p_operand[1]), &record_index) == 0) {
			return (uint16_t)(record_index + 1U);
		}
		return 0U;
	case RACP_OPERATOR_GREATER_OR_EQUAL:
		if (cgms_record_index_offset_greater_or_equal_get(get_le16(&req->p_operand[1]), &record_index) == 0) {
			return (uint16_t)(total_records - record_index);
		}
		return 0U;
	default:
		return 0U;
	}
}

static int cgms_report_records(const ble_racp_value_t *req, uint16_t *sent)
{
	uint16_t i;
	uint16_t start = 0U;
	uint16_t end = 0U;
	int err;

	*sent = 0U;
	m_racp.processing_active = true;

	if (m_record_count == 0U) {
		m_racp.processing_active = false;
		return 0;
	}

	switch (req->operator) {
	case RACP_OPERATOR_ALL:
		start = 0U;
		end = (uint16_t)(m_record_count - 1U);
		break;
	case RACP_OPERATOR_FIRST:
		start = 0U;
		end = 0U;
		break;
	case RACP_OPERATOR_LAST:
		start = (uint16_t)(m_record_count - 1U);
		end = start;
		break;
	case RACP_OPERATOR_LESS_OR_EQUAL:
		if (cgms_record_index_offset_less_or_equal_get(get_le16(&req->p_operand[1]), &end) != 0) {
			m_racp.processing_active = false;
			return 0;
		}
		start = 0U;
		break;
	case RACP_OPERATOR_GREATER_OR_EQUAL:
		if (cgms_record_index_offset_greater_or_equal_get(get_le16(&req->p_operand[1]), &start) != 0) {
			m_racp.processing_active = false;
			return 0;
		}
		end = (uint16_t)(m_record_count - 1U);
		break;
	default:
		m_racp.processing_active = false;
		return -EINVAL;
	}

	for (i = start; i <= end; i++) {
		err = cgms_measurement_notify(&m_records[i]);
		if (err != 0) {
			m_racp.processing_active = false;
			return err;
		}
		(*sent)++;
		if (i == UINT16_MAX) {
			break;
		}
	}

	m_racp.processing_active = false;
	return 0;
}

ssize_t cgms_write_racp(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	ble_racp_value_t req;
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
		if (rsp_code == RACP_RESPONSE_SUCCESS) {
			cgms_racp_reset_state();
		}
		if (rsp_code != RACP_RESPONSE_RESERVED) {
			m_racp.processing_active = false;
			cgms_send_racp_response_code(req.opcode, rsp_code);
		}
		return len;
	}

	if (req.opcode == RACP_OPCODE_REPORT_NUM_RECS) {
		cgms_send_racp_num_records(cgms_count_matching_records(&req));
		return len;
	}
	if (req.opcode == RACP_OPCODE_REPORT_RECS) {
		if (cgms_report_records(&req, &count) != 0) {
			cgms_send_racp_response_code(RACP_OPCODE_REPORT_RECS, RACP_RESPONSE_PROCEDURE_NOT_DONE);
			return len;
		}

		if (count > 0U) {
			cgms_send_racp_response_code(RACP_OPCODE_REPORT_RECS, RACP_RESPONSE_SUCCESS);
		} else {
			cgms_send_racp_response_code(RACP_OPCODE_REPORT_RECS, RACP_RESPONSE_NO_RECORDS_FOUND);
		}
	}

	return len;
}
