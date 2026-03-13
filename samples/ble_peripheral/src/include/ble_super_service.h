/** @file
 *  @brief Wireless Data Exchange service implementation sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BLE_SUPER_SERVICE_H
#define BLE_SUPER_SERVICE_H
#ifdef __cplusplus
extern "C" {
#endif
#include <zephyr.h>

/*
 * Legacy custom service handle range.
 *
 * 说明：
 * 1) 该服务是工程历史遗留的私有服务，当前保留用于兼容旧版上位机/调试工具。
 * 2) 新增的 CGMS/DIS/BAS 不依赖本段句柄，但本段句柄仍需稳定，避免旧链路断裂。
 */
#define CGM_LEGACY_START_HDL            0x0
#define CGM_LEGACY_END_HDL              (CGM_LEGACY_MAX_HDL - 1)

/* Legacy custom service handles */
enum {
	CGM_LEGACY_SVC_HDL = CGM_LEGACY_START_HDL,
	CGM_LEGACY_TEST_CH_HDL,
	CGM_LEGACY_TEST_HDL,
	CGM_LEGACY_TEST_CH_CCC_HDL,
	CGM_LEGACY_MAX_HDL
};

/*
 * Standard CGMS service handles.
 *
 * 说明：
 * - 句柄索引用于访问 BT_GATT_SERVICE_DEFINE 自动生成的 attr_xxx[]。
 * - 顺序需与 ble_super_service.c 中 cgm_cgms_svc 的特征定义顺序完全一致。
 */
enum {
	CGM_CGMS_START_HDL = 0,
	CGM_CGMS_SVC_HDL = CGM_CGMS_START_HDL,
	CGM_CGMS_MEAS_CH_HDL,
	CGM_CGMS_MEAS_HDL,
	CGM_CGMS_MEAS_CCC_HDL,
	CGM_CGMS_FEATURE_CH_HDL,
	CGM_CGMS_FEATURE_HDL,
	CGM_CGMS_STATUS_CH_HDL,
	CGM_CGMS_STATUS_HDL,
	CGM_CGMS_SESSION_START_CH_HDL,
	CGM_CGMS_SESSION_START_HDL,
	CGM_CGMS_SESSION_RUN_TIME_CH_HDL,
	CGM_CGMS_SESSION_RUN_TIME_HDL,
	CGM_CGMS_RACP_CH_HDL,
	CGM_CGMS_RACP_HDL,
	CGM_CGMS_RACP_CCC_HDL,
	CGM_CGMS_SOCP_CH_HDL,
	CGM_CGMS_SOCP_HDL,
	CGM_CGMS_SOCP_CCC_HDL,
	CGM_CGMS_MAX_HDL
};

/* Battery Service handles */
enum {
	CGM_BAS_START_HDL = 0,
	CGM_BAS_SVC_HDL = CGM_BAS_START_HDL,
	CGM_BAS_LEVEL_CH_HDL,
	CGM_BAS_LEVEL_HDL,
	CGM_BAS_LEVEL_CCC_HDL,
	CGM_BAS_MAX_HDL
};

/*
 * 兼容旧代码的别名宏。
 *
 * 说明：
 * - 旧模块（例如 bt_le_op.c）仍通过 SUPER_TEST_HDL 引用 legacy notify。
 * - 这些别名仅用于平滑迁移，后续建议统一替换为 CGM_LEGACY_xxx。
 */
#define SUPER_START_HDL                CGM_LEGACY_START_HDL
#define SUPER_END_HDL                  CGM_LEGACY_END_HDL
#define SUPER_SVC_HDL                  CGM_LEGACY_SVC_HDL
#define SUPER_TEST_CH_HDL              CGM_LEGACY_TEST_CH_HDL
#define SUPER_TEST_HDL                 CGM_LEGACY_TEST_HDL
#define SUPER_TEST_CH_CCC_HDL          CGM_LEGACY_TEST_CH_CCC_HDL
#define SUPER_MAX_HDL                  CGM_LEGACY_MAX_HDL

uint8_t ble_super_ccc_enabled(struct bt_conn *conn, uint8_t handle);

/*
 * CGM 命名风格接口：查询指定句柄 CCC/通知开关状态。
 */
uint8_t cgm_ble_super_ccc_enabled(struct bt_conn *conn, uint8_t handle);

/** @brief Notify attribute value to peer device.
 *
 *
 *  @param conn  		: current connection.
 *  @param handle		: attr index.
 *  @param len 			: Attribute value length.
 *  @param p_value 	:	Pointer to Attribute data.
 */
void ble_super_send_notify(struct bt_conn *conn, uint8_t index, uint16_t len, uint8_t *p_value);

/*
 * CGM 命名风格接口：对 legacy 服务指定属性发送 notify。
 */
void cgm_ble_super_send_notify(struct bt_conn *conn, uint8_t index, uint16_t len, uint8_t *p_value);

/*
 * 服务生命周期接口：
 * 1) init 在 bt_enable 成功后调用一次。
 * 2) on_connected / on_disconnected 在连接状态变化时调用。
 */
void ble_super_service_init(void);
void ble_super_on_connected(struct bt_conn *conn);
void ble_super_on_disconnected(struct bt_conn *conn);

void cgm_ble_super_service_init(void);
void cgm_ble_super_on_connected(struct bt_conn *conn);
void cgm_ble_super_on_disconnected(struct bt_conn *conn);

#ifdef __cplusplus
}
#endif

#endif /* BLE_SUPER_SERVICE_H */
