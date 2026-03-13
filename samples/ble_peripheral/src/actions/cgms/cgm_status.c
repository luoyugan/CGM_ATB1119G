/* SPDX-License-Identifier: Apache-2.0 */
/*
 * cgm_status.c — CGM Status 特征（UUID 0x2AA9）读取实现
 *
 * 返回 Sensor Status Annunciation（固定 3 字节快照）：
 *   Octet 0: Status bits      (Bit 0-7)
 *   Octet 1: Cal/Temp bits    (Bit 8-15)
 *   Octet 2: Warning bits     (Bit 16-23)
 */

#include <bluetooth/gatt.h>
#include "cgm_status.h"
#include "cgms_state.h"

ssize_t cgm_cgms_status_read_cb(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 void *buf, uint16_t len,
				 uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 cgm_status_annunciation,
				 sizeof(cgm_status_annunciation));
}
