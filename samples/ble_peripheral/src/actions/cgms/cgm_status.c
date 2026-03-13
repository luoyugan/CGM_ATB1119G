/* SPDX-License-Identifier: Apache-2.0 */
/*
 * cgm_status.c — CGM Status 特征（UUID 0x2AA9）读取实现
 *
 * 返回格式：
 *   Octet 0-1: Time Offset（LE16）
 *   Octet 2:   Status bits      (Bit 0-7)
 *   Octet 3:   Cal/Temp bits    (Bit 8-15)
 *   Octet 4:   Warning bits     (Bit 16-23)
 */

#include <sys/byteorder.h>
#include <bluetooth/gatt.h>
#include "cgm_status.h"
#include "cgms_defs.h"
#include "cgms_state.h"

static uint16_t cgm_status_crc16_ccitt(const uint8_t *data, uint16_t len)
{
	uint16_t crc = 0xFFFFU;

	for (uint16_t i = 0U; i < len; i++) {
		crc ^= (uint16_t)((uint16_t)data[i] << 8);
		for (uint8_t bit = 0U; bit < 8U; bit++) {
			if (crc & 0x8000U) {
				crc = (uint16_t)((crc << 1) ^ 0x1021U);
			} else {
				crc = (uint16_t)(crc << 1);
			}
		}
	}

	return crc;
}

void cgm_status_set_session_stopped(bool stopped)
{
	if (stopped) {
		cgm_status_annunciation[0] |= (uint8_t)CGM_STATUS_BIT_SESSION_STOPPED;
	} else {
		cgm_status_annunciation[0] &= (uint8_t)~CGM_STATUS_BIT_SESSION_STOPPED;
	}
}

bool cgm_status_get_session_stopped(void)
{
	return !!(cgm_status_annunciation[0] & CGM_STATUS_BIT_SESSION_STOPPED);
}

ssize_t cgm_cgms_status_read_cb(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 void *buf, uint16_t len,
				 uint16_t offset)
{
	bool crc_en = !!(cgm_feature_flags & CGM_FEATURE_FLAG_E2E_CRC_SUPPORTED);
	uint8_t value[7];
	uint16_t payload_len = 5U;

	sys_put_le16(cgm_time_offset_min, &value[0]);
	value[2] = cgm_status_annunciation[0];
	value[3] = cgm_status_annunciation[1];
	value[4] = cgm_status_annunciation[2];

	if (crc_en) {
		uint16_t crc;

		crc = cgm_status_crc16_ccitt(value, payload_len);
		sys_put_le16(crc, &value[payload_len]);
		payload_len = 7U;
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 value,
				 payload_len);
}
