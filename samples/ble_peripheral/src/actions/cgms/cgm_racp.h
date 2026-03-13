/* SPDX-License-Identifier: Apache-2.0 */
/*
 * cgm_racp.h — Record Access Control Point（RACP，UUID 0x2A52）公共接口
 *
 * 导出：
 *   - cgm_cgms_racp_write_cb()       RACP 写请求回调（由服务定义引用）
 *   - cgm_cgms_racp_ccc_changed_cb() RACP CCC 变化回调
 *   - cgm_racp_init()                初始化 RACP 工作项
 *   - cgm_racp_cancel()              断连时取消并复位 RACP 状态
 */

#ifndef CGM_RACP_H
#define CGM_RACP_H

#include <zephyr/types.h>
#include <bluetooth/gatt.h>

/**
 * @brief RACP 特征写回调。
 */
ssize_t cgm_cgms_racp_write_cb(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len,
				uint16_t offset, uint8_t flags);

/**
 * @brief RACP CCC 变化回调（由 BT_GATT_CCC 引用）。
 */
void cgm_cgms_racp_ccc_changed_cb(const struct bt_gatt_attr *attr, uint16_t value);

/**
 * @brief 初始化 RACP 工作项，程序启动时调用一次。
 */
void cgm_racp_init(void);

/**
 * @brief 断连时取消 RACP 异步工作项并复位状态。
 */
void cgm_racp_cancel(void);

#endif /* CGM_RACP_H */
