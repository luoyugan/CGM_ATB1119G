/* SPDX-License-Identifier: Apache-2.0 */
/*
 * cgm_socp.c — Specific Operations Control Point（SOCP，UUID 0x2AAC）实现
 *
 * 支持的操作码（CGMS v1.0.1 §3.5 Minimum Set）：
 *   0x01  Set Communication Interval
 *   0x02  Get Communication Interval
 *   0x1A  Start Session
 *   0x1B  Stop Session
 */

#include <string.h>
#include <bluetooth/att.h>
#include <bluetooth/gatt.h>

#include "cgm_socp.h"
#include "cgms_defs.h"
#include "cgms_state.h"
#include "cgm_meas.h"
#include "cgm_status.h"
#include "cgm_session.h"

/* ========== 私有 indication 缓冲 ========== */
static struct bt_gatt_indicate_params cgm_socp_indicate_params;
static uint8_t cgm_socp_indicate_buffer[8];

/* ========== 内部发送帮助函数 ========== */

static void cgm_cgms_socp_send_indication(struct bt_conn *conn,
					   const uint8_t *data, uint16_t data_len)
{
	if (data_len > sizeof(cgm_socp_indicate_buffer)) {
		return;
	}

	memcpy(cgm_socp_indicate_buffer, data, data_len);
	cgm_socp_indicate_params.attr    = &attr_cgm_cgms_svc[CGM_CGMS_SOCP_HDL];
	cgm_socp_indicate_params.data    = cgm_socp_indicate_buffer;
	cgm_socp_indicate_params.len     = data_len;
	cgm_socp_indicate_params.func    = NULL;
	cgm_socp_indicate_params.destroy = NULL;
	bt_gatt_indicate(conn, &cgm_socp_indicate_params);
}

static void cgm_cgms_socp_send_response_code(struct bt_conn *conn,
					      uint8_t req_opcode,
					      uint8_t rsp_code)
{
	uint8_t rsp[3] = {
		CGM_SOCP_OP_RESPONSE_CODE,
		req_opcode,
		rsp_code,
	};

	cgm_cgms_socp_send_indication(conn, rsp, sizeof(rsp));
}

/* ========== 写回调 ========== */

ssize_t cgm_cgms_socp_write_cb(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len,
				uint16_t offset, uint8_t flags)
{
	const uint8_t *data = buf;
	uint8_t opcode;

	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (offset != 0 || len < 1) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (!cgm_cgms_socp_indicate_enabled) {
		return BT_GATT_ERR(BT_ATT_ERR_CCC_IMPROPER_CONF);
	}

	opcode = data[0];

	switch (opcode) {
	case CGM_SOCP_OP_SET_COMM_INTERVAL:
		if (len < 2) {
			cgm_cgms_socp_send_response_code(conn, opcode,
							 CGM_SOCP_RSP_INVALID_OPERAND);
			break;
		}
		if (data[1] == 0U || data[1] > 60U) {
			cgm_cgms_socp_send_response_code(conn, opcode,
							 CGM_SOCP_RSP_PARAMETER_OUT_OF_RANGE);
			break;
		}
		cgm_comm_interval_min = data[1];
		cgm_cgms_socp_send_response_code(conn, opcode, CGM_SOCP_RSP_SUCCESS);
		break;

	case CGM_SOCP_OP_GET_COMM_INTERVAL:
		cgm_socp_indicate_buffer[0] = CGM_SOCP_OP_COMM_INTERVAL_RSP;
		cgm_socp_indicate_buffer[1] = cgm_comm_interval_min;
		cgm_cgms_socp_send_indication(conn, cgm_socp_indicate_buffer, 2U);
		break;

	case CGM_SOCP_OP_START_SESSION:
		cgm_db_count = 0U;
		cgm_db_head = 0U;
		memset(cgm_db, 0, sizeof(cgm_db));
		cgm_time_offset_min = 0U;
		cgm_session_run_time_min = 0U;
		cgm_session_clear_start_time();
		cgm_meas_start();
		cgm_status_set_session_stopped(false);
		cgm_cgms_socp_send_response_code(conn, opcode, CGM_SOCP_RSP_SUCCESS);
		break;

	case CGM_SOCP_OP_STOP_SESSION:
		if (!cgm_session_running) {
			cgm_cgms_socp_send_response_code(conn, opcode,
							 CGM_SOCP_RSP_OP_CODE_NOT_SUPPORTED);
			break;
		}
		cgm_meas_stop();
		cgm_status_set_session_stopped(true);
		cgm_cgms_socp_send_response_code(conn, opcode, CGM_SOCP_RSP_SUCCESS);
		break;

	default:
		cgm_cgms_socp_send_response_code(conn, opcode,
						 CGM_SOCP_RSP_OP_CODE_NOT_SUPPORTED);
		break;
	}

	return len;
}

/* ========== CCC 回调 ========== */

void cgm_cgms_socp_ccc_changed_cb(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);
	cgm_cgms_socp_indicate_enabled = (value == BT_GATT_CCC_INDICATE) ? 1U : 0U;
}
