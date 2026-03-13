/* SPDX-License-Identifier: Apache-2.0 */
/*
 * cgm_status.h — CGM Status 特征（UUID 0x2AA9）接口声明
 *
 * Status 特征为只读，返回 Time Offset + CGM Status（可选 E2E-CRC）。
 */

#ifndef CGM_STATUS_H
#define CGM_STATUS_H

#include <stdbool.h>
#include <bluetooth/gatt.h>
#include <zephyr/types.h>

/**
 * @brief 读取 CGM Status 特征值（UUID 0x2AA9）。
 *
 * 报文格式：
 *   - 无 E2E-CRC: [TimeOffset_LE16][Status][Cal/Temp][Warning]（5 字节）
 *   - 有 E2E-CRC: [TimeOffset_LE16][Status][Cal/Temp][Warning][CRC16_LE]（7 字节）
 */
ssize_t cgm_cgms_status_read_cb(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 void *buf, uint16_t len,
				 uint16_t offset);

void cgm_status_set_session_stopped(bool stopped);

bool cgm_status_get_session_stopped(void);

#endif /* CGM_STATUS_H */
