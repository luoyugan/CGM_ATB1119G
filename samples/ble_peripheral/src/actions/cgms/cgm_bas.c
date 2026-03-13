/* SPDX-License-Identifier: Apache-2.0 */
/*
 * cgm_bas.c — Battery Service（BAS）Battery Level 特征（UUID 0x2A19）实现
 *
 * Battery Level：UINT8，取值 0~100（%）。
 * - 周期测量时由 cgm_meas.c 每次递减并 notify。
 * - 客户端读取时通过此回调返回当前值。
 */

#include <bluetooth/gatt.h>
#include "cgm_bas.h"
#include "cgms_state.h"

ssize_t cgm_bas_level_read_cb(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr,
			       void *buf, uint16_t len,
			       uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &cgm_battery_level_percent,
				 sizeof(cgm_battery_level_percent));
}

void cgm_bas_level_ccc_changed_cb(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);
	cgm_bas_notify_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1U : 0U;
}
