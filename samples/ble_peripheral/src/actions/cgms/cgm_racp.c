/* SPDX-License-Identifier: Apache-2.0 */
/*
 * cgm_racp.c — Record Access Control Point（RACP，UUID 0x2A52）完整实现
 *
 * 支持的 OpCode（CGMS v1.0.1 §3.6 Mandatory）：
 *   0x01  Report Stored Records       — All / ≥ / ≤ / within range（Time Offset 过滤）
 *   0x03  Abort Operation             — 停止进行中的 RACP 操作
 *   0x04  Report Number Stored Records
 */

#include <string.h>
#include <sys/byteorder.h>
#include <bluetooth/att.h>
#include <bluetooth/gatt.h>

#include "cgm_racp.h"
#include "cgms_defs.h"
#include "cgms_state.h"
#include "cgm_meas.h"

/* ========== 私有状态 ========== */
static struct k_delayed_work cgm_racp_work;
static volatile bool cgm_racp_in_progress;
static volatile bool cgm_racp_abort_req;
static uint8_t  cgm_racp_req_opcode;
static uint8_t  cgm_racp_req_operator;
static uint16_t cgm_racp_filter_min;
static uint16_t cgm_racp_filter_max;

/* indication buffer（必须在发送期间有效） */
static struct bt_gatt_indicate_params cgm_racp_indicate_params;
static uint8_t cgm_racp_indicate_buffer[4];

/* ========== 内部发送帮助函数 ========== */

static void cgm_cgms_racp_send_indication(struct bt_conn *conn,
					   const uint8_t *data, uint16_t data_len)
{
	if (data_len > sizeof(cgm_racp_indicate_buffer)) {
		return;
	}

	memcpy(cgm_racp_indicate_buffer, data, data_len);
	cgm_racp_indicate_params.attr    = &attr_cgm_cgms_svc[CGM_CGMS_RACP_HDL];
	cgm_racp_indicate_params.data    = cgm_racp_indicate_buffer;
	cgm_racp_indicate_params.len     = data_len;
	cgm_racp_indicate_params.func    = NULL;
	cgm_racp_indicate_params.destroy = NULL;
	bt_gatt_indicate(conn, &cgm_racp_indicate_params);
}

static void cgm_cgms_racp_send_response(struct bt_conn *conn,
					 uint8_t req_opcode, uint8_t rsp_code)
{
	uint8_t rsp[4] = {
		CGM_RACP_OP_RESPONSE_CODE,
		CGM_RACP_OPERATOR_NULL,
		req_opcode,
		rsp_code,
	};

	cgm_cgms_racp_send_indication(conn, rsp, sizeof(rsp));
}

/* ========== 异步报告工作项 ========== */

static void cgm_racp_report_work_cb(struct k_work *work)
{
	uint16_t i;
	uint16_t matched = 0U;
	uint8_t  pkt[9];
	uint16_t pkt_len;

	ARG_UNUSED(work);

	if (!cgm_active_conn || !cgm_racp_in_progress) {
		return;
	}

	/* ---- Abort 优先处理 ---- */
	if (cgm_racp_abort_req) {
		cgm_racp_abort_req   = false;
		cgm_racp_in_progress = false;
		cgm_cgms_racp_send_response(cgm_active_conn,
					    CGM_RACP_OP_ABORT_OPERATION,
					    CGM_RACP_RSP_SUCCESS);
		return;
	}

	/* ---- Report Number of Stored Records ---- */
	if (cgm_racp_req_opcode == CGM_RACP_OP_REPORT_NUM_STORED_RECORDS) {
		uint16_t count = 0U;
		uint8_t  rsp[4];

		for (i = 0U; i < cgm_db_count; i++) {
			const struct cgm_db_record *rec =
				&cgm_db[(cgm_db_head + i) % CGM_DB_MAX_RECORDS];
			uint16_t t = rec->time_offset;
			bool m = false;

			switch (cgm_racp_req_operator) {
			case CGM_RACP_OPERATOR_ALL_RECORDS:
				m = true; break;
			case CGM_RACP_OPERATOR_GT_EQ:
				m = (t >= cgm_racp_filter_min); break;
			case CGM_RACP_OPERATOR_LT_EQ:
				m = (t <= cgm_racp_filter_max); break;
			case CGM_RACP_OPERATOR_WITHIN_RANGE:
				m = (t >= cgm_racp_filter_min && t <= cgm_racp_filter_max); break;
			default:
				break;
			}
			if (m) { count++; }
		}

		rsp[0] = CGM_RACP_OP_NUM_STORED_RECORDS_RSP;
		rsp[1] = CGM_RACP_OPERATOR_NULL;
		sys_put_le16(count, &rsp[2]);
		cgm_cgms_racp_send_indication(cgm_active_conn, rsp, sizeof(rsp));
		cgm_racp_in_progress = false;
		return;
	}

	/* ---- Report Stored Records ---- */
	for (i = 0U; i < cgm_db_count; i++) {
		const struct cgm_db_record *rec =
			&cgm_db[(cgm_db_head + i) % CGM_DB_MAX_RECORDS];
		uint16_t t   = rec->time_offset;
		bool matches = false;

		if (cgm_racp_abort_req) {
			break;
		}

		switch (cgm_racp_req_operator) {
		case CGM_RACP_OPERATOR_ALL_RECORDS:
			matches = true; break;
		case CGM_RACP_OPERATOR_GT_EQ:
			matches = (t >= cgm_racp_filter_min); break;
		case CGM_RACP_OPERATOR_LT_EQ:
			matches = (t <= cgm_racp_filter_max); break;
		case CGM_RACP_OPERATOR_WITHIN_RANGE:
			matches = (t >= cgm_racp_filter_min && t <= cgm_racp_filter_max); break;
		default:
			break;
		}

		if (matches && cgm_cgms_meas_notify_enabled) {
			pkt_len = cgm_build_meas_packet_from_db(pkt, sizeof(pkt), rec);
			if (pkt_len > 0U) {
				bt_gatt_notify(cgm_active_conn,
					       &attr_cgm_cgms_svc[CGM_CGMS_MEAS_HDL],
					       pkt, pkt_len);
				matched++;
			}
		}
	}

	cgm_racp_in_progress = false;

	if (cgm_racp_abort_req) {
		cgm_racp_abort_req = false;
		cgm_cgms_racp_send_response(cgm_active_conn,
					    CGM_RACP_OP_ABORT_OPERATION,
					    CGM_RACP_RSP_SUCCESS);
		return;
	}

	if (matched == 0U) {
		cgm_cgms_racp_send_response(cgm_active_conn, cgm_racp_req_opcode,
					    CGM_RACP_RSP_NO_RECORDS_FOUND);
	} else {
		cgm_cgms_racp_send_response(cgm_active_conn, cgm_racp_req_opcode,
					    CGM_RACP_RSP_SUCCESS);
	}
}

/* ========== 写回调 ========== */

ssize_t cgm_cgms_racp_write_cb(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len,
				uint16_t offset, uint8_t flags)
{
	const uint8_t *data = buf;
	uint8_t opcode;
	uint8_t oper;

	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (offset != 0 || len < 1) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (!cgm_cgms_racp_indicate_enabled) {
		return BT_GATT_ERR(BT_ATT_ERR_CCC_IMPROPER_CONF);
	}

	opcode = data[0];

	/* Abort 始终受理 */
	if (opcode == CGM_RACP_OP_ABORT_OPERATION) {
		if (cgm_racp_in_progress) {
			cgm_racp_abort_req = true;
		} else {
			cgm_cgms_racp_send_response(conn, opcode, CGM_RACP_RSP_SUCCESS);
		}
		return len;
	}

	/* 拒绝在操作进行中写入新 OpCode */
	if (cgm_racp_in_progress) {
		return BT_GATT_ERR(CGM_ATT_ERR_PROCEDURE_ALREADY_IN_PROGRESS);
	}

	if (len < 2) {
		cgm_cgms_racp_send_response(conn, opcode, CGM_RACP_RSP_INVALID_OPERATOR);
		return len;
	}

	oper = data[1];

	switch (opcode) {
	case CGM_RACP_OP_REPORT_STORED_RECORDS:
		if (!cgm_cgms_meas_notify_enabled) {
			return BT_GATT_ERR(BT_ATT_ERR_CCC_IMPROPER_CONF);
		}
		/* fall-through */
	case CGM_RACP_OP_REPORT_NUM_STORED_RECORDS:
		switch (oper) {
		case CGM_RACP_OPERATOR_ALL_RECORDS:
			cgm_racp_filter_min = 0U;
			cgm_racp_filter_max = UINT16_MAX;
			break;
		case CGM_RACP_OPERATOR_GT_EQ:
			if (len < 5 || data[2] != CGM_RACP_FILTER_TYPE_TIME_OFFSET) {
				cgm_cgms_racp_send_response(conn, opcode,
					len < 5 ? CGM_RACP_RSP_INVALID_OPERAND
						: CGM_RACP_RSP_OPERAND_NOT_SUPPORTED);
				return len;
			}
			cgm_racp_filter_min = sys_get_le16(&data[3]);
			cgm_racp_filter_max = UINT16_MAX;
			break;
		case CGM_RACP_OPERATOR_LT_EQ:
			if (len < 5 || data[2] != CGM_RACP_FILTER_TYPE_TIME_OFFSET) {
				cgm_cgms_racp_send_response(conn, opcode,
					len < 5 ? CGM_RACP_RSP_INVALID_OPERAND
						: CGM_RACP_RSP_OPERAND_NOT_SUPPORTED);
				return len;
			}
			cgm_racp_filter_min = 0U;
			cgm_racp_filter_max = sys_get_le16(&data[3]);
			break;
		case CGM_RACP_OPERATOR_WITHIN_RANGE:
			if (len < 7 || data[2] != CGM_RACP_FILTER_TYPE_TIME_OFFSET) {
				cgm_cgms_racp_send_response(conn, opcode,
					len < 7 ? CGM_RACP_RSP_INVALID_OPERAND
						: CGM_RACP_RSP_OPERAND_NOT_SUPPORTED);
				return len;
			}
			cgm_racp_filter_min = sys_get_le16(&data[3]);
			cgm_racp_filter_max = sys_get_le16(&data[5]);
			if (cgm_racp_filter_min > cgm_racp_filter_max) {
				cgm_cgms_racp_send_response(conn, opcode,
							    CGM_RACP_RSP_INVALID_OPERAND);
				return len;
			}
			break;
		default:
			cgm_cgms_racp_send_response(conn, opcode,
						    CGM_RACP_RSP_OPERATOR_NOT_SUPPORTED);
			return len;
		}

		cgm_racp_req_opcode   = opcode;
		cgm_racp_req_operator = oper;
		cgm_racp_in_progress  = true;
		cgm_racp_abort_req    = false;
		k_delayed_work_submit(&cgm_racp_work, K_NO_WAIT);
		break;

	default:
		cgm_cgms_racp_send_response(conn, opcode, CGM_RACP_RSP_OP_NOT_SUPPORTED);
		break;
	}

	return len;
}

/* ========== CCC 回调 ========== */

void cgm_cgms_racp_ccc_changed_cb(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);
	cgm_cgms_racp_indicate_enabled = (value == BT_GATT_CCC_INDICATE) ? 1U : 0U;
}

/* ========== 生命周期接口 ========== */

void cgm_racp_init(void)
{
	k_delayed_work_init(&cgm_racp_work, cgm_racp_report_work_cb);
}

void cgm_racp_cancel(void)
{
	k_delayed_work_cancel(&cgm_racp_work);
	cgm_racp_in_progress = false;
	cgm_racp_abort_req   = false;
}
