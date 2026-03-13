/* SPDX-License-Identifier: Apache-2.0 */
/*
 * cgm_session.h — CGM Session Start Time / Session Run Time 特征接口声明
 *
 * Session Start Time（UUID 0x2AAA）：可读写，9 字节，描述会话起始时间戳。
 * Session Run Time （UUID 0x2AAB）：只读，2 字节（UINT16），单位分钟。
 */

#ifndef CGM_SESSION_H
#define CGM_SESSION_H

#include <bluetooth/gatt.h>
#include <zephyr/types.h>

/** @brief 读取 Session Start Time（UUID 0x2AAA）。 */
ssize_t cgm_cgms_session_start_read_cb(struct bt_conn *conn,
					const struct bt_gatt_attr *attr,
					void *buf, uint16_t len,
					uint16_t offset);

/** @brief 写入 Session Start Time（UUID 0x2AAA）。 */
ssize_t cgm_cgms_session_start_write_cb(struct bt_conn *conn,
					 const struct bt_gatt_attr *attr,
					 const void *buf, uint16_t len,
					 uint16_t offset, uint8_t flags);

/** @brief 读取 Session Run Time（UUID 0x2AAB）。 */
ssize_t cgm_cgms_session_run_time_read_cb(struct bt_conn *conn,
					   const struct bt_gatt_attr *attr,
					   void *buf, uint16_t len,
					   uint16_t offset);

/** @brief 清空 Session Start Time（Time 字段置 0，DST/TZ 置 unknown）。 */
void cgm_session_clear_start_time(void);

#endif /* CGM_SESSION_H */
