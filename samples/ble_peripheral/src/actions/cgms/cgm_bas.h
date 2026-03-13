/* SPDX-License-Identifier: Apache-2.0 */
/*
 * cgm_bas.h — Battery Service（BAS）特征（UUID 0x2A19）接口声明
 */

#ifndef CGM_BAS_H
#define CGM_BAS_H

#include <bluetooth/gatt.h>
#include <zephyr/types.h>

/** @brief 读取 Battery Level（UUID 0x2A19），返回 0~100 UINT8。 */
ssize_t cgm_bas_level_read_cb(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr,
			       void *buf, uint16_t len,
			       uint16_t offset);

/** @brief Battery Level CCC 变化回调，更新 cgm_bas_notify_enabled 标志。 */
void cgm_bas_level_ccc_changed_cb(const struct bt_gatt_attr *attr, uint16_t value);

#endif /* CGM_BAS_H */
