/* SPDX-License-Identifier: Apache-2.0 */
/*
 * cgm_session.c — CGM Session Start Time（0x2AAA）& Session Run Time（0x2AAB）实现
 *
 * Session Start Time：
 *   - 可读写（加密保护），9 字节（DateTime + DST + TimeZone）。
 *   - 客户端写入后，设备使用此时间戳计算绝对时间。
 *
 * Session Run Time：
 *   - 只读，UINT16，单位分钟，表示会话已累计运行时间。
 */

#include <string.h>
#include <sys/byteorder.h>
#include <bluetooth/att.h>
#include <bluetooth/gatt.h>
#include "cgm_session.h"
#include "cgms_defs.h"
#include "cgms_state.h"

#define CGM_SESSION_START_TIME_LEN           9U
#define CGM_SESSION_START_TIME_LEN_WITH_CRC 11U
#define CGM_ATT_ERR_MISSING_CRC             0x80
#define CGM_ATT_ERR_INVALID_CRC             0x81

static uint16_t cgm_session_crc16_ccitt(const uint8_t *data, uint16_t len)
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

static bool cgm_is_leap_year(uint16_t year)
{
	return ((year % 4U) == 0U && (year % 100U) != 0U) || ((year % 400U) == 0U);
}

static uint8_t cgm_days_in_month(uint16_t year, uint8_t month)
{
	static const uint8_t days[] = { 31U, 28U, 31U, 30U, 31U, 30U,
					31U, 31U, 30U, 31U, 30U, 31U };

	if (month == 2U && cgm_is_leap_year(year)) {
		return 29U;
	}

	return days[month - 1U];
}

static bool cgm_datetime_decode(const uint8_t value[CGM_SESSION_START_TIME_LEN],
				uint16_t *year, uint8_t *month, uint8_t *day,
				uint8_t *hour, uint8_t *minute, uint8_t *second)
{
	*year   = sys_get_le16(&value[0]);
	*month  = value[2];
	*day    = value[3];
	*hour   = value[4];
	*minute = value[5];
	*second = value[6];

	if (*year < 2000U || *year > 2099U) {
		return false;
	}
	if (*month < 1U || *month > 12U) {
		return false;
	}
	if (*day < 1U || *day > cgm_days_in_month(*year, *month)) {
		return false;
	}
	if (*hour > 23U || *minute > 59U || *second > 59U) {
		return false;
	}

	return true;
}

static uint32_t cgm_days_from_2000(uint16_t year, uint8_t month, uint8_t day)
{
	uint32_t days = 0U;

	for (uint16_t y = 2000U; y < year; y++) {
		days += cgm_is_leap_year(y) ? 366U : 365U;
	}

	for (uint8_t m = 1U; m < month; m++) {
		days += cgm_days_in_month(year, m);
	}

	days += (uint32_t)(day - 1U);
	return days;
}

static void cgm_date_from_days_2000(uint32_t days,
				     uint16_t *year, uint8_t *month, uint8_t *day)
{
	uint16_t y = 2000U;

	while (1) {
		uint16_t d = cgm_is_leap_year(y) ? 366U : 365U;

		if (days < d) {
			break;
		}
		days -= d;
		y++;
	}

	*year = y;
	*month = 1U;
	while (1) {
		uint8_t dim = cgm_days_in_month(*year, *month);

		if (days < dim) {
			break;
		}
		days -= dim;
		(*month)++;
	}

	*day = (uint8_t)(days + 1U);
}

void cgm_session_clear_start_time(void)
{
	memset(cgm_session_start_time, 0, sizeof(cgm_session_start_time));
	cgm_session_start_time[7] = 0xFFU;
	cgm_session_start_time[8] = 0x80U;
}

static int cgm_session_store_from_client_now(const uint8_t client_value[CGM_SESSION_START_TIME_LEN])
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	uint32_t total_minutes;
	uint32_t days;

	if (!cgm_datetime_decode(client_value, &year, &month, &day,
				 &hour, &minute, &second)) {
		return -EINVAL;
	}

	days = cgm_days_from_2000(year, month, day);
	total_minutes = days * 1440U + (uint32_t)hour * 60U + (uint32_t)minute;

	if (total_minutes > cgm_time_offset_min) {
		total_minutes -= cgm_time_offset_min;
	} else {
		total_minutes = 0U;
	}

	days = total_minutes / 1440U;
	hour = (uint8_t)((total_minutes % 1440U) / 60U);
	minute = (uint8_t)(total_minutes % 60U);

	cgm_date_from_days_2000(days, &year, &month, &day);

	sys_put_le16(year, &cgm_session_start_time[0]);
	cgm_session_start_time[2] = month;
	cgm_session_start_time[3] = day;
	cgm_session_start_time[4] = hour;
	cgm_session_start_time[5] = minute;
	cgm_session_start_time[6] = second;
	cgm_session_start_time[7] = client_value[7];
	cgm_session_start_time[8] = client_value[8];

	return 0;
}

/* ========== Session Start Time ========== */

ssize_t cgm_cgms_session_start_read_cb(struct bt_conn *conn,
					const struct bt_gatt_attr *attr,
					void *buf, uint16_t len,
					uint16_t offset)
{
	bool crc_en = !!(cgm_feature_flags & CGM_FEATURE_FLAG_E2E_CRC_SUPPORTED);

	if (crc_en) {
		uint8_t value[CGM_SESSION_START_TIME_LEN_WITH_CRC];
		uint16_t crc;

		memcpy(value, cgm_session_start_time, CGM_SESSION_START_TIME_LEN);
		crc = cgm_session_crc16_ccitt(value, CGM_SESSION_START_TIME_LEN);
		sys_put_le16(crc, &value[CGM_SESSION_START_TIME_LEN]);

		return bt_gatt_attr_read(conn, attr, buf, len, offset,
					 value, sizeof(value));
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 cgm_session_start_time,
				 CGM_SESSION_START_TIME_LEN);
}

ssize_t cgm_cgms_session_start_write_cb(struct bt_conn *conn,
					 const struct bt_gatt_attr *attr,
					 const void *buf, uint16_t len,
					 uint16_t offset, uint8_t flags)
{
	const uint8_t *data = buf;
	bool crc_en = !!(cgm_feature_flags & CGM_FEATURE_FLAG_E2E_CRC_SUPPORTED);
	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (offset != 0U) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (crc_en) {
		uint16_t expected_crc;
		uint16_t actual_crc;

		if (len == CGM_SESSION_START_TIME_LEN) {
			return BT_GATT_ERR(CGM_ATT_ERR_MISSING_CRC);
		}
		if (len != CGM_SESSION_START_TIME_LEN_WITH_CRC) {
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		expected_crc = sys_get_le16(&data[CGM_SESSION_START_TIME_LEN]);
		actual_crc = cgm_session_crc16_ccitt(data, CGM_SESSION_START_TIME_LEN);
		if (expected_crc != actual_crc) {
			return BT_GATT_ERR(CGM_ATT_ERR_INVALID_CRC);
		}
	} else {
		if (len != CGM_SESSION_START_TIME_LEN) {
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}
	}

	if (cgm_session_store_from_client_now(data) != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	return len;
}

/* ========== Session Run Time ========== */

ssize_t cgm_cgms_session_run_time_read_cb(struct bt_conn *conn,
					   const struct bt_gatt_attr *attr,
					   void *buf, uint16_t len,
					   uint16_t offset)
{
	uint8_t value[2];

	sys_put_le16(cgm_session_run_time_min, value);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(value));
}
