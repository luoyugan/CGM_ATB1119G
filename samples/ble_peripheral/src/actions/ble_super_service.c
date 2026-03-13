/*Copyright (c) 2018 Actions (Zhuhai) Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ble_super_service.c  BLE 服务编排层（抽象层）
 *
 * 本文件职责：
 *   1) 声明所有 BT_GATT_SERVICE_DEFINE 服务块（Legacy / CGMS / DIS / BAS）。
 *   2) 实现 Legacy 特征的本地回调（通知开关 + 写回调）。
 *   3) 实现服务生命周期公共 API：init / on_connected / on_disconnected / ccc_enabled / send_notify。
 *
 * 各特征的具体回调实现位于 cgms/ 子目录：
 *   cgm_feature.c  cgm_status.c  cgm_session.c
 *   cgm_meas.c     cgm_racp.c    cgm_socp.c
 *   cgm_bas.c      cgm_dis.c
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include "errno.h"
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/att.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include "ble_super_service.h"
#include "ble_data_test_sample.h"

/* ---- cgms/ 子模块头文件 ---- */
#include "cgms/cgms_defs.h"
#include "cgms/cgms_state.h"
#include "cgms/cgm_feature.h"
#include "cgms/cgm_status.h"
#include "cgms/cgm_session.h"
#include "cgms/cgm_meas.h"
#include "cgms/cgm_racp.h"
#include "cgms/cgm_socp.h"
#include "cgms/cgm_bas.h"
#include "cgms/cgm_dis.h"

/*
 * BT_GATT_SERVICE_DEFINE 会生成 attr_<service_name>[] 符号。
 * cgms_state.h 中已声明 extern，此处无需重复声明。
 */

/* ========== Legacy Custom Service ========== */

static const struct bt_uuid_128 cgm_legacy_svc_uuid = BT_UUID_INIT_128(
0xdd, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
0x00, 0x10, 0x00, 0x00, 0xf6, 0xfe, 0x00, 0x00);

static const struct bt_uuid_128 cgm_legacy_test_uuid = BT_UUID_INIT_128(
0x89, 0x78, 0x61, 0x63, 0x74, 0x4c, 0x45, 0xb0,
0xd5, 0x4e, 0xf2, 0x2f, 0x02, 0x00, 0x5f, 0x00);

/* Legacy 外部依赖 */
void conn_notify(void);
void bt_data_trans_cancel(void);

static void cgm_legacy_test_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
ARG_UNUSED(attr);

cgm_legacy_notify_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1U : 0U;
printk("legacy notify : %d\n", cgm_legacy_notify_enabled);

if (cgm_legacy_notify_enabled) {
conn_notify();
} else {
bt_data_trans_cancel();
}
}

static int cgm_legacy_test_write_cb(struct bt_conn *conn,
    const struct bt_gatt_attr *attr,
    const void *buf, uint16_t len,
    uint16_t offset, uint8_t flags)
{
ARG_UNUSED(conn);
ARG_UNUSED(attr);
ARG_UNUSED(buf);
ARG_UNUSED(offset);
ARG_UNUSED(flags);

update_write_stats(len);
return len;
}

/* ========== GATT 服务定义 ========== */

BT_GATT_SERVICE_DEFINE(super_svc,
BT_GATT_PRIMARY_SERVICE((void *)&cgm_legacy_svc_uuid),

BT_GATT_CHARACTERISTIC(&cgm_legacy_test_uuid.uuid,
BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_WRITE,
BT_GATT_PERM_WRITE, NULL, cgm_legacy_test_write_cb, NULL),
BT_GATT_CCC(cgm_legacy_test_ccc_cfg_changed,
    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

BT_GATT_SERVICE_DEFINE(cgm_cgms_svc,
BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_16(CGM_CGMS_SERVICE_UUID_VAL)),

BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(CGM_CGMS_MEAS_UUID_VAL),
BT_GATT_CHRC_NOTIFY,
BT_GATT_PERM_NONE,
NULL, NULL, NULL),
BT_GATT_CCC(cgm_cgms_meas_ccc_changed_cb,
    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),

BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(CGM_CGMS_FEATURE_UUID_VAL),
BT_GATT_CHRC_READ,
BT_GATT_PERM_READ,
cgm_cgms_feature_read_cb, NULL, NULL),

BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(CGM_CGMS_STATUS_UUID_VAL),
BT_GATT_CHRC_READ,
BT_GATT_PERM_READ,
cgm_cgms_status_read_cb, NULL, NULL),

BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(CGM_CGMS_SESSION_START_UUID_VAL),
BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT,
cgm_cgms_session_start_read_cb, cgm_cgms_session_start_write_cb, NULL),

BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(CGM_CGMS_SESSION_RUN_TIME_UUID_VAL),
BT_GATT_CHRC_READ,
BT_GATT_PERM_READ,
cgm_cgms_session_run_time_read_cb, NULL, NULL),

BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(CGM_CGMS_RACP_UUID_VAL),
BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,
BT_GATT_PERM_WRITE_ENCRYPT,
NULL, cgm_cgms_racp_write_cb, NULL),
BT_GATT_CCC(cgm_cgms_racp_ccc_changed_cb,
    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(CGM_CGMS_SOCP_UUID_VAL),
BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,
BT_GATT_PERM_WRITE_ENCRYPT,
NULL, cgm_cgms_socp_write_cb, NULL),
BT_GATT_CCC(cgm_cgms_socp_ccc_changed_cb,
    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

BT_GATT_SERVICE_DEFINE(cgm_dis_svc,
BT_GATT_PRIMARY_SERVICE(BT_UUID_DIS),

BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MANUFACTURER_NAME,
BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
cgm_dis_manufacturer_name_read_cb, NULL, NULL),

BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MODEL_NUMBER,
BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
cgm_dis_model_number_read_cb, NULL, NULL),

BT_GATT_CHARACTERISTIC(BT_UUID_DIS_SYSTEM_ID,
BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
cgm_dis_system_id_read_cb, NULL, NULL),
);

BT_GATT_SERVICE_DEFINE(cgm_bas_svc,
BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),

BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL,
BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
BT_GATT_PERM_READ,
cgm_bas_level_read_cb, NULL, NULL),
BT_GATT_CCC(cgm_bas_level_ccc_changed_cb,
    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

/* ========== 公共 API ========== */

uint8_t cgm_ble_super_ccc_enabled(struct bt_conn *conn, uint8_t handle)
{
ARG_UNUSED(conn);

if (handle == CGM_LEGACY_TEST_HDL) {
return cgm_legacy_notify_enabled;
}

return 0;
}

uint8_t ble_super_ccc_enabled(struct bt_conn *conn, uint8_t handle)
{
return cgm_ble_super_ccc_enabled(conn, handle);
}

void cgm_ble_super_send_notify(struct bt_conn *conn, uint8_t index,
       uint16_t len, uint8_t *p_value)
{
bt_gatt_notify(conn, &attr_super_svc[index], p_value, len);
}

void ble_super_send_notify(struct bt_conn *conn, uint8_t index, uint16_t len, uint8_t *p_value)
{
cgm_ble_super_send_notify(conn, index, len, p_value);
}

void cgm_ble_super_service_init(void)
{
cgm_meas_init();
cgm_racp_init();
}

void ble_super_service_init(void)
{
cgm_ble_super_service_init();
}

void cgm_ble_super_on_connected(struct bt_conn *conn)
{
if (cgm_active_conn) {
bt_conn_unref(cgm_active_conn);
}

cgm_active_conn = bt_conn_ref(conn);
}

void ble_super_on_connected(struct bt_conn *conn)
{
cgm_ble_super_on_connected(conn);
}

void cgm_ble_super_on_disconnected(struct bt_conn *conn)
{
if (!cgm_active_conn || cgm_active_conn != conn) {
return;
}

cgm_meas_cancel();
cgm_racp_cancel();

bt_conn_unref(cgm_active_conn);
cgm_active_conn = NULL;
}

void ble_super_on_disconnected(struct bt_conn *conn)
{
cgm_ble_super_on_disconnected(conn);
}
