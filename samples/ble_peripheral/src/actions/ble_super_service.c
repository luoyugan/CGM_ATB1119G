/*Copyright (c) 2018 Actions (Zhuhai) Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include "errno.h"
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/att.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include "ble_super_service.h"
#include "ble_data_test_sample.h"

/*
 * BT_GATT_SERVICE_DEFINE 会生成 attr_<service_name>[] 符号。
 * 由于本文件有回调在服务定义之前引用这些数组，这里先做前向声明。
 */
extern const struct bt_gatt_attr attr_cgm_cgms_svc[];
extern const struct bt_gatt_attr attr_cgm_bas_svc[];

/*
 * ========== Legacy Custom Service UUID ==========
 *
 * 设计目的：
 * - 保留历史私有服务，兼容旧版客户端。
 * - 新功能由 CGMS/DIS/BAS 承担，legacy 不再承担规范职责。
 */
static const struct bt_uuid_128 cgm_legacy_svc_uuid = BT_UUID_INIT_128(
	0xdd, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
0x00, 0x10, 0x00, 0x00, 0xf6, 0xfe,	0x00, 0x00);

static const struct bt_uuid_128 cgm_legacy_test_uuid = BT_UUID_INIT_128(
	0x89, 0x78, 0x61, 0x63, 0x74, 0x4c, 0x45, 0xb0,
0xd5, 0x4e, 0xf2, 0x2f, 0x02, 0x00,	0x5f, 0x00);

/* ========== CGMS 相关宏定义 ========== */
#define CGM_CGMS_SERVICE_UUID_VAL                0x181F
#define CGM_CGMS_MEAS_UUID_VAL                   0x2AA7
#define CGM_CGMS_FEATURE_UUID_VAL                0x2AA8
#define CGM_CGMS_STATUS_UUID_VAL                 0x2AA9
#define CGM_CGMS_SESSION_START_UUID_VAL          0x2AAA
#define CGM_CGMS_SESSION_RUN_TIME_UUID_VAL       0x2AAB
#define CGM_CGMS_SOCP_UUID_VAL                   0x2AAC
#define CGM_CGMS_RACP_UUID_VAL                   0x2A52

/* SOCP 最小集操作码 */
#define CGM_SOCP_OP_SET_COMM_INTERVAL            0x01
#define CGM_SOCP_OP_GET_COMM_INTERVAL            0x02
#define CGM_SOCP_OP_COMM_INTERVAL_RSP            0x03
#define CGM_SOCP_OP_START_SESSION                0x1A
#define CGM_SOCP_OP_STOP_SESSION                 0x1B
#define CGM_SOCP_OP_RESPONSE_CODE                0x1C

/* SOCP 响应码 */
#define CGM_SOCP_RSP_SUCCESS                     0x01
#define CGM_SOCP_RSP_OP_CODE_NOT_SUPPORTED       0x02
#define CGM_SOCP_RSP_INVALID_OPERAND             0x03
#define CGM_SOCP_RSP_PROCEDURE_NOT_COMPLETED     0x04
#define CGM_SOCP_RSP_PARAMETER_OUT_OF_RANGE      0x05

/* RACP 响应码（当前先实现最小成功回包） */
#define CGM_RACP_OP_RESPONSE_CODE                0x06
#define CGM_RACP_RSP_SUCCESS                     0x01

/*
 * 外部回调声明（来自 bt_le_op.c / ble_data_test_sample.c）
 *
 * 说明：
 * - 为兼容原工程，保持这些符号可用。
 */
void conn_notify(void);
void bt_data_trans_cancel(void);

/* ========== 运行时状态 ========== */
static uint8_t cgm_legacy_notify_enabled;
static uint8_t cgm_cgms_meas_notify_enabled;
static uint8_t cgm_cgms_racp_indicate_enabled;
static uint8_t cgm_cgms_socp_indicate_enabled;
static uint8_t cgm_bas_notify_enabled;

static struct bt_conn *cgm_active_conn;
static struct k_delayed_work cgm_meas_work;

/* CGMS 运行数据 */
static uint16_t cgm_feature_flags;
static uint8_t cgm_type_sample_location = 0x60;
static uint8_t cgm_status_annunciation[3] = {0x00, 0x00, 0x00};
static uint8_t cgm_session_start_time[9] = {0xE9, 0x07, 0x03, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint16_t cgm_session_run_time_min;
static uint16_t cgm_time_offset_min;
static uint16_t cgm_glucose_mg_dl = 110;
static uint8_t cgm_comm_interval_min = 5;
static bool cgm_session_running;

/* BAS 运行数据 */
static uint8_t cgm_battery_level_percent = 95;

/* indication 缓冲（必须保证发送期间有效） */
static struct bt_gatt_indicate_params cgm_socp_indicate_params;
static struct bt_gatt_indicate_params cgm_racp_indicate_params;
static uint8_t cgm_socp_indicate_buffer[8];
static uint8_t cgm_racp_indicate_buffer[4];

/*
 * @brief Legacy CCC 写回调。
 *
 * @details
 * 1) 该回调用于旧私有特征通知开关。
 * 2) 当通知打开时通过 conn_notify() 触发旧数据链路；关闭时取消传输。
 */
static void cgm_legacy_test_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);

	cgm_legacy_notify_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1U : 0U;
	printk("legacy notify : %d\n", cgm_legacy_notify_enabled);

	if (cgm_legacy_notify_enabled) {
		conn_notify();
	} else {
		bt_data_trans_cancel();
	}
}

/*
 * @brief Legacy 写回调。
 *
 * @details
 * 兼容原有吞吐测试行为，仅统计写入长度，不对数据内容做解析。
 */
static int cgm_legacy_test_write_cb(struct bt_conn *conn,
				    const struct bt_gatt_attr *attr,
				    const void *buf, uint16_t len,
				    uint16_t offset, uint8_t flags)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(attr);
	ARG_UNUSED(buf);
	ARG_UNUSED(offset);
	ARG_UNUSED(flags);

	update_write_stats(len);
	return len;
}

/* ========== CGMS 读写回调函数 ========== */

/**
 * @brief 读取 CGMS Feature 特征值。
 *
 * @param conn   当前连接对象。
 * @param attr   当前属性对象。
 * @param buf    输出缓冲区。
 * @param len    输出缓冲区可写长度。
 * @param offset 读取偏移。
 * @return 实际返回字节数，或 BT_GATT_ERR 错误码。
 */
static ssize_t cgm_cgms_feature_read_cb(struct bt_conn *conn,
					const struct bt_gatt_attr *attr,
					void *buf, uint16_t len,
					uint16_t offset)
{
	uint8_t value[3];

	sys_put_le16(cgm_feature_flags, value);
	value[2] = cgm_type_sample_location;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(value));
}

/**
 * @brief 读取 CGMS Status 特征值。
 *
 * @param conn   当前连接对象。
 * @param attr   当前属性对象。
 * @param buf    输出缓冲区。
 * @param len    输出缓冲区可写长度。
 * @param offset 读取偏移。
 * @return 实际返回字节数，或 BT_GATT_ERR 错误码。
 */
static ssize_t cgm_cgms_status_read_cb(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   void *buf, uint16_t len,
				   uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 cgm_status_annunciation, sizeof(cgm_status_annunciation));
}

/**
 * @brief 读取 CGMS Session Start Time 特征值。
 *
 * @param conn   当前连接对象。
 * @param attr   当前属性对象。
 * @param buf    输出缓冲区。
 * @param len    输出缓冲区可写长度。
 * @param offset 读取偏移。
 * @return 实际返回字节数，或 BT_GATT_ERR 错误码。
 */
static ssize_t cgm_cgms_session_start_read_cb(struct bt_conn *conn,
					  const struct bt_gatt_attr *attr,
					  void *buf, uint16_t len,
					  uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 cgm_session_start_time, sizeof(cgm_session_start_time));
}

/**
 * @brief 写入 CGMS Session Start Time 特征值。
 *
 * @param conn   当前连接对象。
 * @param attr   当前属性对象。
 * @param buf    输入数据缓冲区。
 * @param len    输入数据长度。
 * @param offset 写入偏移。
 * @param flags  ATT 写标志。
 * @return 成功返回写入字节数，失败返回 BT_GATT_ERR 错误码。
 */
static ssize_t cgm_cgms_session_start_write_cb(struct bt_conn *conn,
					   const struct bt_gatt_attr *attr,
					   const void *buf, uint16_t len,
					   uint16_t offset, uint8_t flags)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (offset > sizeof(cgm_session_start_time) ||
	    (offset + len) > sizeof(cgm_session_start_time)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(&cgm_session_start_time[offset], buf, len);
	return len;
}

/**
 * @brief 读取 CGMS Session Run Time 特征值。
 *
 * @param conn   当前连接对象。
 * @param attr   当前属性对象。
 * @param buf    输出缓冲区。
 * @param len    输出缓冲区可写长度。
 * @param offset 读取偏移。
 * @return 实际返回字节数，或 BT_GATT_ERR 错误码。
 */
static ssize_t cgm_cgms_session_run_time_read_cb(struct bt_conn *conn,
					     const struct bt_gatt_attr *attr,
					     void *buf, uint16_t len,
					     uint16_t offset)
{
	uint8_t value[2];

	sys_put_le16(cgm_session_run_time_min, value);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(value));
}

/**
 * @brief 发送 SOCP indication。
 *
 * @param conn     当前连接对象。
 * @param data     需要发送的载荷。
 * @param data_len 载荷长度。
 * @return 无返回值。
 *
 * @details
 * 该函数会先把数据复制到静态缓冲，再调用 bt_gatt_indicate，
 * 保证异步发送阶段数据地址有效。
 */
static void cgm_cgms_socp_send_indication(struct bt_conn *conn,
					   const uint8_t *data,
					   uint16_t data_len)
{
	memcpy(cgm_socp_indicate_buffer, data, data_len);
	cgm_socp_indicate_params.attr = &attr_cgm_cgms_svc[CGM_CGMS_SOCP_HDL];
	cgm_socp_indicate_params.data = cgm_socp_indicate_buffer;
	cgm_socp_indicate_params.len = data_len;
	cgm_socp_indicate_params.func = NULL;
	cgm_socp_indicate_params.destroy = NULL;

	bt_gatt_indicate(conn, &cgm_socp_indicate_params);
}

/**
 * @brief 发送 SOCP Response Code 响应。
 *
 * @param conn       当前连接对象。
 * @param req_opcode 请求操作码。
 * @param rsp_code   响应码。
 * @return 无返回值。
 */
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

/**
 * @brief 处理 SOCP 写请求。
 *
 * @param conn   当前连接对象。
 * @param attr   当前属性对象。
 * @param buf    输入命令缓冲区。
 * @param len    输入命令长度。
 * @param offset 写入偏移。
 * @param flags  ATT 写标志。
 * @return 成功返回写入字节数，失败返回 BT_GATT_ERR 错误码。
 */
static ssize_t cgm_cgms_socp_write_cb(struct bt_conn *conn,
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

		if (data[1] == 0 || data[1] > 60) {
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
		cgm_cgms_socp_send_indication(conn, cgm_socp_indicate_buffer, 2);
		break;

	case CGM_SOCP_OP_START_SESSION:
		cgm_session_running = true;
		k_delayed_work_submit(&cgm_meas_work, K_SECONDS(1));
		cgm_cgms_socp_send_response_code(conn, opcode, CGM_SOCP_RSP_SUCCESS);
		break;

	case CGM_SOCP_OP_STOP_SESSION:
		if (!cgm_session_running) {
			cgm_cgms_socp_send_response_code(conn, opcode,
							 CGM_SOCP_RSP_OP_CODE_NOT_SUPPORTED);
			break;
		}

		cgm_session_running = false;
		k_delayed_work_cancel(&cgm_meas_work);
		cgm_cgms_socp_send_response_code(conn, opcode, CGM_SOCP_RSP_SUCCESS);
		break;

	default:
		cgm_cgms_socp_send_response_code(conn, opcode,
						     CGM_SOCP_RSP_OP_CODE_NOT_SUPPORTED);
		break;
	}

	return len;
}

/**
 * @brief 处理 RACP 写请求。
 *
 * @param conn   当前连接对象。
 * @param attr   当前属性对象。
 * @param buf    输入命令缓冲区。
 * @param len    输入命令长度。
 * @param offset 写入偏移。
 * @param flags  ATT 写标志。
 * @return 成功返回写入字节数，失败返回 BT_GATT_ERR 错误码。
 *
 * @details
 * 当前实现为最小流程：收到任意 RACP OpCode 后回 Success。
 */
static ssize_t cgm_cgms_racp_write_cb(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  const void *buf, uint16_t len,
				  uint16_t offset, uint8_t flags)
{
	const uint8_t *data = buf;

	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (offset != 0 || len < 1) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (!cgm_cgms_racp_indicate_enabled) {
		return BT_GATT_ERR(BT_ATT_ERR_CCC_IMPROPER_CONF);
	}

	cgm_racp_indicate_buffer[0] = CGM_RACP_OP_RESPONSE_CODE;
	cgm_racp_indicate_buffer[1] = data[0];
	cgm_racp_indicate_buffer[2] = CGM_RACP_RSP_SUCCESS;

	cgm_racp_indicate_params.attr = &attr_cgm_cgms_svc[CGM_CGMS_RACP_HDL];
	cgm_racp_indicate_params.data = cgm_racp_indicate_buffer;
	cgm_racp_indicate_params.len = 3;
	cgm_racp_indicate_params.func = NULL;
	cgm_racp_indicate_params.destroy = NULL;

	bt_gatt_indicate(conn, &cgm_racp_indicate_params);
	return len;
}

/**
 * @brief Measurement CCC 变化回调。
 *
 * @param attr  当前属性对象。
 * @param value 新 CCC 值。
 * @return 无返回值。
 */
static void cgm_cgms_meas_ccc_changed_cb(const struct bt_gatt_attr *attr,
					 uint16_t value)
{
	ARG_UNUSED(attr);
	cgm_cgms_meas_notify_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1U : 0U;

	if (!cgm_cgms_meas_notify_enabled) {
		k_delayed_work_cancel(&cgm_meas_work);
	}
}

/**
 * @brief RACP CCC 变化回调。
 *
 * @param attr  当前属性对象。
 * @param value 新 CCC 值。
 * @return 无返回值。
 */
static void cgm_cgms_racp_ccc_changed_cb(const struct bt_gatt_attr *attr,
					 uint16_t value)
{
	ARG_UNUSED(attr);
	cgm_cgms_racp_indicate_enabled = (value == BT_GATT_CCC_INDICATE) ? 1U : 0U;
}

/**
 * @brief SOCP CCC 变化回调。
 *
 * @param attr  当前属性对象。
 * @param value 新 CCC 值。
 * @return 无返回值。
 */
static void cgm_cgms_socp_ccc_changed_cb(const struct bt_gatt_attr *attr,
					 uint16_t value)
{
	ARG_UNUSED(attr);
	cgm_cgms_socp_indicate_enabled = (value == BT_GATT_CCC_INDICATE) ? 1U : 0U;
}

/* ========== DIS 回调 ========== */
/**
 * @brief 读取 DIS Manufacturer Name。
 *
 * @param conn   当前连接对象。
 * @param attr   当前属性对象。
 * @param buf    输出缓冲区。
 * @param len    输出缓冲区长度。
 * @param offset 读取偏移。
 * @return 实际返回字节数，或 BT_GATT_ERR 错误码。
 */
static ssize_t cgm_dis_manufacturer_name_read_cb(struct bt_conn *conn,
						 const struct bt_gatt_attr *attr,
						 void *buf, uint16_t len,
						 uint16_t offset)
{
	const char *value = "Actions";
	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, strlen(value));
}

/**
 * @brief 读取 DIS Model Number。
 *
 * @param conn   当前连接对象。
 * @param attr   当前属性对象。
 * @param buf    输出缓冲区。
 * @param len    输出缓冲区长度。
 * @param offset 读取偏移。
 * @return 实际返回字节数，或 BT_GATT_ERR 错误码。
 */
static ssize_t cgm_dis_model_number_read_cb(struct bt_conn *conn,
					    const struct bt_gatt_attr *attr,
					    void *buf, uint16_t len,
					    uint16_t offset)
{
	const char *value = "ATB1119G-CGM";
	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, strlen(value));
}

/**
 * @brief 读取 DIS System ID。
 *
 * @param conn   当前连接对象。
 * @param attr   当前属性对象。
 * @param buf    输出缓冲区。
 * @param len    输出缓冲区长度。
 * @param offset 读取偏移。
 * @return 实际返回字节数，或 BT_GATT_ERR 错误码。
 */
static ssize_t cgm_dis_system_id_read_cb(struct bt_conn *conn,
					 const struct bt_gatt_attr *attr,
					 void *buf, uint16_t len,
					 uint16_t offset)
{
	static const uint8_t value[8] = {0x11, 0x19, 0x00, 0xFE, 0x00, 0x03, 0x26, 0x01};
	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(value));
}

/* ========== BAS 回调 ========== */
/**
 * @brief 读取 BAS Battery Level。
 *
 * @param conn   当前连接对象。
 * @param attr   当前属性对象。
 * @param buf    输出缓冲区。
 * @param len    输出缓冲区长度。
 * @param offset 读取偏移。
 * @return 实际返回字节数，或 BT_GATT_ERR 错误码。
 */
static ssize_t cgm_bas_level_read_cb(struct bt_conn *conn,
				      const struct bt_gatt_attr *attr,
				      void *buf, uint16_t len,
				      uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &cgm_battery_level_percent, sizeof(cgm_battery_level_percent));
}

/**
 * @brief BAS Battery Level CCC 变化回调。
 *
 * @param attr  当前属性对象。
 * @param value 新 CCC 值。
 * @return 无返回值。
 */
static void cgm_bas_level_ccc_changed_cb(const struct bt_gatt_attr *attr,
					 uint16_t value)
{
	ARG_UNUSED(attr);
	cgm_bas_notify_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1U : 0U;
}

/*
 * @brief CGMS 周期测量任务。
 *
 * @details
 * 1) 在 session running + measurement notify enabled 时发送。
 * 2) 发送后更新 time offset / run time / battery level。
 * 3) 当前为演示数据，后续可接真实传感算法。
 */
static void cgm_cgms_measurement_work_cb(struct k_work *work)
{
	uint8_t packet[9];

	ARG_UNUSED(work);

	if (!cgm_active_conn || !cgm_cgms_meas_notify_enabled || !cgm_session_running) {
		return;
	}

	packet[0] = sizeof(packet) - 1;  /* record length */
	packet[1] = 0x00;                /* flags (minimal set) */
	sys_put_le16(cgm_glucose_mg_dl, &packet[2]);
	sys_put_le16(cgm_time_offset_min, &packet[4]);
	memcpy(&packet[6], cgm_status_annunciation, 3);

	bt_gatt_notify(cgm_active_conn, &attr_cgm_cgms_svc[CGM_CGMS_MEAS_HDL],
		       packet, sizeof(packet));

	cgm_time_offset_min += cgm_comm_interval_min;
	cgm_session_run_time_min += cgm_comm_interval_min;
	cgm_glucose_mg_dl = (cgm_glucose_mg_dl < 160) ? (cgm_glucose_mg_dl + 1U) : 105U;

	if (cgm_bas_notify_enabled && cgm_battery_level_percent > 0) {
		cgm_battery_level_percent--;
		bt_gatt_notify(cgm_active_conn, &attr_cgm_bas_svc[CGM_BAS_LEVEL_HDL],
			       &cgm_battery_level_percent,
			       sizeof(cgm_battery_level_percent));
	}

	k_delayed_work_submit(&cgm_meas_work, K_SECONDS(cgm_comm_interval_min));
}

/*
 * Legacy custom service definition.
 */
BT_GATT_SERVICE_DEFINE(super_svc,
	BT_GATT_PRIMARY_SERVICE((void *)&cgm_legacy_svc_uuid),

	BT_GATT_CHARACTERISTIC(&cgm_legacy_test_uuid.uuid,
				BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE, 
				BT_GATT_PERM_WRITE, NULL, cgm_legacy_test_write_cb, NULL),
	BT_GATT_CCC(cgm_legacy_test_ccc_cfg_changed,
			BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

/*
 * Standard CGMS service definition.
 */
BT_GATT_SERVICE_DEFINE(cgm_cgms_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_16(CGM_CGMS_SERVICE_UUID_VAL)),

	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(CGM_CGMS_MEAS_UUID_VAL),
		BT_GATT_CHRC_NOTIFY,
		BT_GATT_PERM_NONE,
		NULL, NULL, NULL),
	BT_GATT_CCC(cgm_cgms_meas_ccc_changed_cb, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(CGM_CGMS_FEATURE_UUID_VAL),
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		cgm_cgms_feature_read_cb, NULL, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(CGM_CGMS_STATUS_UUID_VAL),
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		cgm_cgms_status_read_cb, NULL, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(CGM_CGMS_SESSION_START_UUID_VAL),
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT,
		cgm_cgms_session_start_read_cb, cgm_cgms_session_start_write_cb, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(CGM_CGMS_SESSION_RUN_TIME_UUID_VAL),
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		cgm_cgms_session_run_time_read_cb, NULL, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(CGM_CGMS_RACP_UUID_VAL),
		BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,
		BT_GATT_PERM_WRITE_ENCRYPT,
		NULL, cgm_cgms_racp_write_cb, NULL),
	BT_GATT_CCC(cgm_cgms_racp_ccc_changed_cb, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(CGM_CGMS_SOCP_UUID_VAL),
		BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,
		BT_GATT_PERM_WRITE_ENCRYPT,
		NULL, cgm_cgms_socp_write_cb, NULL),
	BT_GATT_CCC(cgm_cgms_socp_ccc_changed_cb, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

/* Device Information Service definition */
BT_GATT_SERVICE_DEFINE(cgm_dis_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_DIS),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MANUFACTURER_NAME,
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		cgm_dis_manufacturer_name_read_cb, NULL, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MODEL_NUMBER,
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		cgm_dis_model_number_read_cb, NULL, NULL),

	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_SYSTEM_ID,
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		cgm_dis_system_id_read_cb, NULL, NULL),
);

/* Battery Service definition */
BT_GATT_SERVICE_DEFINE(cgm_bas_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),

	BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
		BT_GATT_PERM_READ,
		cgm_bas_level_read_cb, NULL, NULL),
	BT_GATT_CCC(cgm_bas_level_ccc_changed_cb, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

/**
 * @brief 查询 Legacy 服务指定句柄的通知使能状态。
 *
 * @param conn   当前连接对象（当前实现未使用）。
 * @param handle 属性句柄索引。
 * @return 1 表示已使能；0 表示未使能或不支持。
 */
uint8_t cgm_ble_super_ccc_enabled(struct bt_conn *conn, uint8_t handle)
{
	ARG_UNUSED(conn);

	/*
	 * 兼容 API 当前只在 legacy 数据链路中使用。
	 * 如需查询 CGMS/BAS 的 CCC 状态，建议新增专用查询接口，
	 * 避免跨服务句柄索引（都从 0 开始）产生歧义。
	 */
	if (handle == CGM_LEGACY_TEST_HDL) {
		return cgm_legacy_notify_enabled;
	}

	return 0;
}

/**
 * @brief 旧接口兼容：查询 CCC 状态。
 *
 * @param conn   当前连接对象。
 * @param handle 属性句柄索引。
 * @return 1 表示已使能；0 表示未使能或不支持。
 */
uint8_t ble_super_ccc_enabled(struct bt_conn *conn, uint8_t handle)
{
	return cgm_ble_super_ccc_enabled(conn, handle);
}

/**
 * @brief 发送 Legacy 服务 notify。
 *
 * @param conn    当前连接对象。
 * @param index   Legacy 服务属性索引。
 * @param len     发送数据长度。
 * @param p_value 发送数据指针。
 * @return 无返回值。
 */
void cgm_ble_super_send_notify(struct bt_conn *conn, uint8_t index,
			       uint16_t len, uint8_t *p_value)
{
	bt_gatt_notify(conn, &attr_super_svc[index], p_value, len);
}

/**
 * @brief 旧接口兼容：发送 Legacy notify。
 *
 * @param conn    当前连接对象。
 * @param index   属性索引。
 * @param len     数据长度。
 * @param p_value 数据指针。
 * @return 无返回值。
 */
void ble_super_send_notify(struct bt_conn *conn, uint8_t index, uint16_t len, uint8_t *p_value)
{
	cgm_ble_super_send_notify(conn, index, len, p_value);
}

/**
 * @brief 服务层初始化。
 *
 * @param 无。
 * @return 无返回值。
 *
 * @details
 * 负责初始化测量周期任务队列，需在 BLE 协议栈使能后调用一次。
 */
void cgm_ble_super_service_init(void)
{
	k_delayed_work_init(&cgm_meas_work, cgm_cgms_measurement_work_cb);
}

/**
 * @brief 旧接口兼容：服务初始化。
 *
 * @param 无。
 * @return 无返回值。
 */
void ble_super_service_init(void)
{
	cgm_ble_super_service_init();
}

/**
 * @brief 服务层连接建立入口。
 *
 * @param conn 新建立的连接对象。
 * @return 无返回值。
 *
 * @details
 * 保存连接引用供 CGMS/BAS 通知使用；若已有旧连接引用会先释放。
 */
void cgm_ble_super_on_connected(struct bt_conn *conn)
{
	if (cgm_active_conn) {
		bt_conn_unref(cgm_active_conn);
	}

	cgm_active_conn = bt_conn_ref(conn);
}

/**
 * @brief 旧接口兼容：连接建立入口。
 *
 * @param conn 新建立的连接对象。
 * @return 无返回值。
 */
void ble_super_on_connected(struct bt_conn *conn)
{
	cgm_ble_super_on_connected(conn);
}

/**
 * @brief 服务层连接断开入口。
 *
 * @param conn 断开的连接对象。
 * @return 无返回值。
 *
 * @details
 * 断开后会取消测量任务、释放连接引用并复位会话运行状态。
 */
void cgm_ble_super_on_disconnected(struct bt_conn *conn)
{
	if (!cgm_active_conn || cgm_active_conn != conn) {
		return;
	}

	k_delayed_work_cancel(&cgm_meas_work);
	bt_conn_unref(cgm_active_conn);
	cgm_active_conn = NULL;
	cgm_session_running = false;
}

/**
 * @brief 旧接口兼容：连接断开入口。
 *
 * @param conn 断开的连接对象。
 * @return 无返回值。
 */
void ble_super_on_disconnected(struct bt_conn *conn)
{
	cgm_ble_super_on_disconnected(conn);
}
