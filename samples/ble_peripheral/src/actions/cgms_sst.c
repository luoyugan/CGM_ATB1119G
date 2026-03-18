#include "cgms_sst.h"
#include "cgms_db.h"

ssize_t cgms_read_feature(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	void *buf, uint16_t len, uint16_t offset)
{
	uint8_t value[6];
	uint8_t value_len = cgms_encode_feature(value);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, value_len);
}

ssize_t cgms_read_status(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	void *buf, uint16_t len, uint16_t offset)
{
	uint8_t value[5];
	uint8_t value_len = cgms_encode_status(value);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, value_len);
}

ssize_t cgms_read_sst(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	void *buf, uint16_t len, uint16_t offset)
{
	uint8_t value[9];
	uint8_t value_len = cgms_encode_sst(value);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, value_len);
}

ssize_t cgms_read_srt(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	void *buf, uint16_t len, uint16_t offset)
{
	uint8_t value[2];
	put_le16(value, m_session_run_time);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(value));
}

static void cgms_sst_store_from_raw(const uint8_t *buf)
{
	m_sst.date_time.year = get_le16(&buf[0]);
	m_sst.date_time.month = buf[2];
	m_sst.date_time.day = buf[3];
	m_sst.date_time.hours = buf[4];
	m_sst.date_time.minutes = buf[5];
	m_sst.date_time.seconds = buf[6];
	m_sst.time_zone = buf[7];
	m_sst.dst = buf[8];
}

ssize_t cgms_write_sst(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (offset != 0U) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (len != 9U) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	cgms_sst_store_from_raw((const uint8_t *)buf);
	return len;
}
