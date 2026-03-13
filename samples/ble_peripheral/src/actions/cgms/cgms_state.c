/* SPDX-License-Identifier: Apache-2.0 */
/*
 * cgms_state.c — 跨特征模块共享运行时状态变量定义
 *
 * 职责：
 *   唯一负责定义所有跨文件共享的可变状态；各特征模块通过
 *   cgms_state.h 中的 extern 声明访问这些变量。
 *   本文件本身不包含任何业务逻辑。
 */

#include "cgms_state.h"

/* CCC 使能标志（默认全 0，即通知/指示关闭） */
uint8_t cgm_legacy_notify_enabled;
uint8_t cgm_cgms_meas_notify_enabled;
uint8_t cgm_cgms_racp_indicate_enabled;
uint8_t cgm_cgms_socp_indicate_enabled;
uint8_t cgm_bas_notify_enabled;

/* 活跃连接（NULL 表示无连接） */
struct bt_conn *cgm_active_conn;

/* CGMS 特征运行数据 */
uint16_t cgm_feature_flags;
uint8_t  cgm_type_sample_location   = 0x60;   /* Interstitial Fluid, Finger */
uint8_t  cgm_status_annunciation[3] = {0x00, 0x00, 0x00};
uint8_t  cgm_session_start_time[9]  = {
	0xE9, 0x07,             /* Year  = 2025 */
	0x03,                   /* Month = March */
	0x0A,                   /* Day   = 10 */
	0x00, 0x00, 0x00,       /* Hour, Minute, Second */
	0x00,                   /* DST Offset */
	0x00                    /* Time Zone */
};
uint16_t cgm_session_run_time_min;
uint16_t cgm_time_offset_min;
uint16_t cgm_glucose_mg_dl          = 110;
uint8_t  cgm_comm_interval_min      = 5;
bool     cgm_session_running;

/* BAS 数据 */
uint8_t cgm_battery_level_percent   = 95;
