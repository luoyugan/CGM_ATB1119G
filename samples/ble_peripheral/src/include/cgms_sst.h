#ifndef CGMS_SST_H
#define CGMS_SST_H

#include <stdint.h>
#include <sys/types.h>

#include <bluetooth/gatt.h>
#include "atb_ble_cgms.h"

ssize_t cgms_read_feature(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	void *buf, uint16_t len, uint16_t offset);
ssize_t cgms_read_status(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	void *buf, uint16_t len, uint16_t offset);
ssize_t cgms_read_sst(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	void *buf, uint16_t len, uint16_t offset);
ssize_t cgms_read_srt(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	void *buf, uint16_t len, uint16_t offset);
ssize_t cgms_write_sst(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	const void *buf, uint16_t len, uint16_t offset, uint8_t flags);

#endif