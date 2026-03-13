/* SPDX-License-Identifier: Apache-2.0 */
/*
 * cgm_dis.c — Device Information Service（DIS）只读特征实现
 *
 * 实现三个只读字符串/二进制特征：
 *   Manufacturer Name  (UUID 0x2A29) — "Actions"
 *   Model Number       (UUID 0x2A24) — "ATB1119G-CGM"
 *   System ID          (UUID 0x2A23) — 8 字节 OUI+MFR ID
 */

#include <string.h>
#include <bluetooth/gatt.h>
#include "cgm_dis.h"

ssize_t cgm_dis_manufacturer_name_read_cb(struct bt_conn *conn,
					   const struct bt_gatt_attr *attr,
					   void *buf, uint16_t len,
					   uint16_t offset)
{
	static const char value[] = "Actions";

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 value, strlen(value));
}

ssize_t cgm_dis_model_number_read_cb(struct bt_conn *conn,
				      const struct bt_gatt_attr *attr,
				      void *buf, uint16_t len,
				      uint16_t offset)
{
	static const char value[] = "ATB1119G-CGM";

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 value, strlen(value));
}

ssize_t cgm_dis_system_id_read_cb(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   void *buf, uint16_t len,
				   uint16_t offset)
{
	/* Format: OUI (5 bytes) + Manufacturer-defined (3 bytes), little-endian */
	static const uint8_t value[8] = {
		0x11, 0x19, 0x00, 0xFE, 0x00,   /* OUI part */
		0x03, 0x26, 0x01                 /* Manufacturer ID */
	};

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 value, sizeof(value));
}
