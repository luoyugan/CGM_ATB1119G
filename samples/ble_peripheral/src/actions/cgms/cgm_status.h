/* SPDX-License-Identifier: Apache-2.0 */
/*
 * cgm_status.h — CGM Status 特征（UUID 0x2AA9）接口声明
 *
 * Status 特征为只读，返回 Sensor Status Annunciation（1~3 字节）。
 */

#ifndef CGM_STATUS_H
#define CGM_STATUS_H

#include <bluetooth/gatt.h>
#include <zephyr/types.h>

/**
 * @brief 读取 CGM Status 特征值（UUID 0x2AA9）。
 *
 * 报文格式：[Status UINT8][Cal/Temp UINT8][Warning UINT8]（共 3 字节）
 */
ssize_t cgm_cgms_status_read_cb(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 void *buf, uint16_t len,
				 uint16_t offset);

#endif /* CGM_STATUS_H */
