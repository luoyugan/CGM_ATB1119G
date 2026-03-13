/* SPDX-License-Identifier: Apache-2.0 */
/*
 * cgm_feature.c — CGM Feature 特征（UUID 0x2AA8）读取实现
 *
 * 返回格式（CGMS v1.0.1 §3.2）：
 *   Octet 0-1: CGM Feature flags（LE16）
 *   Octet 2:   Type-Sample Location（高 4 bit = Type，低 4 bit = Sample Location）
 */

#include <sys/byteorder.h>
#include <bluetooth/gatt.h>
#include "cgm_feature.h"
#include "cgms_state.h"

ssize_t cgm_cgms_feature_read_cb(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  void *buf, uint16_t len,
				  uint16_t offset)
{
	uint8_t value[3];

	sys_put_le16(cgm_feature_flags, value);
	value[2] = cgm_type_sample_location;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(value));
}
