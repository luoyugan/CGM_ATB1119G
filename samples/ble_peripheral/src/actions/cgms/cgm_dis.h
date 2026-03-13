/* SPDX-License-Identifier: Apache-2.0 */
/*
 * cgm_dis.h — Device Information Service（DIS）特征接口声明
 *
 * 实现以下三个只读特征：
 *   Manufacturer Name  (UUID 0x2A29)
 *   Model Number       (UUID 0x2A24)
 *   System ID          (UUID 0x2A23)
 */

#ifndef CGM_DIS_H
#define CGM_DIS_H

#include <bluetooth/gatt.h>
#include <zephyr/types.h>

/** @brief 读取 Manufacturer Name String（UUID 0x2A29）。 */
ssize_t cgm_dis_manufacturer_name_read_cb(struct bt_conn *conn,
					   const struct bt_gatt_attr *attr,
					   void *buf, uint16_t len,
					   uint16_t offset);

/** @brief 读取 Model Number String（UUID 0x2A24）。 */
ssize_t cgm_dis_model_number_read_cb(struct bt_conn *conn,
				      const struct bt_gatt_attr *attr,
				      void *buf, uint16_t len,
				      uint16_t offset);

/** @brief 读取 System ID（UUID 0x2A23），8 字节。 */
ssize_t cgm_dis_system_id_read_cb(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr,
				   void *buf, uint16_t len,
				   uint16_t offset);

#endif /* CGM_DIS_H */
