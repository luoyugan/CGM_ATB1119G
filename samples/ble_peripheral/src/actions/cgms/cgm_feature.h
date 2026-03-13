/* SPDX-License-Identifier: Apache-2.0 */
/*
 * cgm_feature.h — CGM Feature 特征（UUID 0x2AA8）接口声明
 *
 * Feature 特征为只读，返回设备支持的功能位图和 Type-Sample Location。
 * 客户端读取后才能正确解析 Measurement 特征中的可选字段。
 */

#ifndef CGM_FEATURE_H
#define CGM_FEATURE_H

#include <bluetooth/gatt.h>
#include <zephyr/types.h>

/**
 * @brief 读取 CGM Feature 特征值（UUID 0x2AA8）。
 *
 * 报文格式（CGMS v1.0.1 §3.2）：
 *   [Feature Flags LE16][Type-Sample Location UINT8]
 */
ssize_t cgm_cgms_feature_read_cb(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  void *buf, uint16_t len,
				  uint16_t offset);

#endif /* CGM_FEATURE_H */
