/* SPDX-License-Identifier: Apache-2.0 */
/*
 * cgm_socp.h — Specific Operations Control Point（SOCP，UUID 0x2AAC）公共接口
 *
 * 导出：
 *   - cgm_cgms_socp_write_cb()       SOCP 写请求回调（由服务定义引用）
 *   - cgm_cgms_socp_ccc_changed_cb() SOCP CCC 变化回调
 */

#ifndef CGM_SOCP_H
#define CGM_SOCP_H

#include <zephyr/types.h>
#include <bluetooth/gatt.h>

/**
 * @brief SOCP 特征写回调。
 */
ssize_t cgm_cgms_socp_write_cb(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len,
				uint16_t offset, uint8_t flags);

/**
 * @brief SOCP CCC 变化回调（由 BT_GATT_CCC 引用）。
 */
void cgm_cgms_socp_ccc_changed_cb(const struct bt_gatt_attr *attr, uint16_t value);

#endif /* CGM_SOCP_H */
