/* SPDX-License-Identifier: Apache-2.0 */
/*
 * cgm_session.c — CGM Session Start Time（0x2AAA）& Session Run Time（0x2AAB）实现
 *
 * Session Start Time：
 *   - 可读写（加密保护），9 字节（DateTime + DST + TimeZone）。
 *   - 客户端写入后，设备使用此时间戳计算绝对时间。
 *
 * Session Run Time：
 *   - 只读，UINT16，单位分钟，表示会话已累计运行时间。
 */

#include <string.h>
#include <sys/byteorder.h>
#include <bluetooth/att.h>
#include <bluetooth/gatt.h>
#include "cgm_session.h"
#include "cgms_state.h"

/* ========== Session Start Time ========== */

ssize_t cgm_cgms_session_start_read_cb(struct bt_conn *conn,
					const struct bt_gatt_attr *attr,
					void *buf, uint16_t len,
					uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 cgm_session_start_time,
				 sizeof(cgm_session_start_time));
}

ssize_t cgm_cgms_session_start_write_cb(struct bt_conn *conn,
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

/* ========== Session Run Time ========== */

ssize_t cgm_cgms_session_run_time_read_cb(struct bt_conn *conn,
					   const struct bt_gatt_attr *attr,
					   void *buf, uint16_t len,
					   uint16_t offset)
{
	uint8_t value[2];

	sys_put_le16(cgm_session_run_time_min, value);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(value));
}
