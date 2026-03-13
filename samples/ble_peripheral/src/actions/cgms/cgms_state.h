/* SPDX-License-Identifier: Apache-2.0 */
/*
 * cgms_state.h — 跨特征模块共享的运行时状态声明（extern）
 *
 * 定义在 cgms_state.c 中；所有特征实现文件通过此头文件访问共享数据。
 */

#ifndef CGMS_STATE_H
#define CGMS_STATE_H

#include <zephyr/types.h>
#include <stdbool.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include "ble_super_service.h"   /* CGM_CGMS_*_HDL / CGM_BAS_*_HDL 等句柄枚举 */

/* ========== CCC 通知 / 指示使能标志 ========== */
extern uint8_t cgm_legacy_notify_enabled;
extern uint8_t cgm_cgms_meas_notify_enabled;
extern uint8_t cgm_cgms_racp_indicate_enabled;
extern uint8_t cgm_cgms_socp_indicate_enabled;
extern uint8_t cgm_bas_notify_enabled;

/* ========== 活跃连接 ========== */
extern struct bt_conn *cgm_active_conn;

/* ========== CGMS 特征运行数据 ========== */
extern uint32_t cgm_feature_flags;        /**< 24-bit CGM Feature（§3.2），用 uint32_t 容纳 bit 16 */
extern uint8_t  cgm_type_sample_location;
extern uint8_t  cgm_status_annunciation[3];
extern uint8_t  cgm_session_start_time[9];
extern uint16_t cgm_session_run_time_min;
extern uint16_t cgm_time_offset_min;
extern uint16_t cgm_glucose_mg_dl;
extern uint8_t  cgm_comm_interval_min;
extern bool     cgm_session_running;
extern uint16_t cgm_trend_sfloat;         /**< CGM Trend Information（SFLOAT，mg/dL/min × 10^-1） */
extern uint16_t cgm_quality_sfloat;       /**< CGM Quality（SFLOAT，% × 10^-1） */

/* ========== BAS 数据 ========== */
extern uint8_t cgm_battery_level_percent;

/*
 * GATT 属性数组（由 BT_GATT_SERVICE_DEFINE 在 ble_super_service.c 中生成）。
 * 特征回调通过这些数组的索引调用 bt_gatt_notify / bt_gatt_indicate。
 */
extern const struct bt_gatt_attr attr_cgm_cgms_svc[];
extern const struct bt_gatt_attr attr_cgm_bas_svc[];
extern const struct bt_gatt_attr attr_super_svc[];

#endif /* CGMS_STATE_H */
