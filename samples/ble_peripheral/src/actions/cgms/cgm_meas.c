/* SPDX-License-Identifier: Apache-2.0 */
/*
 * cgm_meas.c — CGM Measurement 特征（UUID 0x2AA7）核心实现
 *
 * 包含：
 *   - SFLOAT 编码（cgm_encode_sfloat）
 *   - 96 条环形记录数据库（cgm_db / cgm_db_push）
 *   - 从数据库记录重建 Measurement 报文（cgm_build_meas_packet_from_db）
 *   - 当前测量报文组包（cgm_build_measurement_record）
 *   - 周期测量工作项（cgm_cgms_measurement_work_cb）
 *   - Measurement CCC 变化回调（cgm_cgms_meas_ccc_changed_cb）
 *   - 生命周期接口：cgm_meas_init / start / stop / cancel
 */

#include <string.h>
#include <sys/byteorder.h>
#include <bluetooth/gatt.h>

#include "cgm_meas.h"
#include "cgms_defs.h"
#include "cgms_state.h"

/* Flags 已在 cgms_defs.h 中定义（CGM_MEAS_FLAG_*） */

/* ========== 私有工作项 ========== */
static struct k_delayed_work cgm_meas_work;
#define CGM_MEAS_MAX_PACKET_SIZE 36

/* ========== 数据库 ========== */
struct cgm_db_record cgm_db[CGM_DB_MAX_RECORDS];
uint16_t cgm_db_count;
uint16_t cgm_db_head;

/* ========== SFLOAT 编码 ========== */

uint16_t cgm_encode_sfloat(int16_t mantissa, int8_t exponent)
{
	uint16_t exp = ((uint8_t)exponent) & 0x0FU;
	uint16_t man = ((uint16_t)mantissa) & 0x0FFFU;

	return (uint16_t)((exp << 12) | man);
}

/* ========== 数据库操作 ========== */

/* ========== E2E-CRC CCITT（Poly=0x1021, Init=0xFFFF） ========== */

static uint16_t cgm_crc16_ccitt(const uint8_t *data, uint16_t len)
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

/* ========== 数据库操作 ========== */

void cgm_db_push(uint16_t time_offset, uint16_t glucose_sfloat,
		 const uint8_t status[3],
		 uint16_t trend_sfloat, uint16_t quality_sfloat)
{
	uint16_t idx;

	if (cgm_db_count < CGM_DB_MAX_RECORDS) {
		idx = (cgm_db_head + cgm_db_count) % CGM_DB_MAX_RECORDS;
		cgm_db_count++;
	} else {
		/* 满时覆盖最旧记录（环形缓冲） */
		idx = cgm_db_head;
		cgm_db_head = (cgm_db_head + 1U) % CGM_DB_MAX_RECORDS;
	}

	cgm_db[idx].time_offset    = time_offset;
	cgm_db[idx].glucose_sfloat = glucose_sfloat;
	memcpy(cgm_db[idx].status, status, 3U);
	cgm_db[idx].trend_sfloat   = trend_sfloat;
	cgm_db[idx].quality_sfloat = quality_sfloat;
}

/* ========== 从数据库记录重建报文 ========== */

uint16_t cgm_build_meas_packet_from_db(uint8_t *packet, uint16_t capacity,
					const struct cgm_db_record *rec)
{
	uint8_t  flags = 0U;
	uint16_t index = 0U;
	bool     trend_en   = !!(cgm_feature_flags & CGM_FEATURE_FLAG_TREND_INFORMATION_SUPPORTED);
	bool     quality_en = !!(cgm_feature_flags & CGM_FEATURE_FLAG_QUALITY_SUPPORTED);
	bool     crc_en     = !!(cgm_feature_flags & CGM_FEATURE_FLAG_E2E_CRC_SUPPORTED);

	if (capacity < 6U) {
		return 0U;
	}

	if (rec->status[0] != 0U) { flags |= CGM_MEAS_FLAG_STATUS_OCTET_PRESENT; }
	if (rec->status[1] != 0U) { flags |= CGM_MEAS_FLAG_CAL_TEMP_OCTET_PRESENT; }
	if (rec->status[2] != 0U) { flags |= CGM_MEAS_FLAG_WARNING_OCTET_PRESENT; }
	if (trend_en)             { flags |= CGM_MEAS_FLAG_TREND_PRESENT; }
	if (quality_en)           { flags |= CGM_MEAS_FLAG_QUALITY_PRESENT; }

	packet[index++] = 0U;   /* Size placeholder */
	packet[index++] = flags;
	sys_put_le16(rec->glucose_sfloat, &packet[index]); index += 2U;
	sys_put_le16(rec->time_offset,    &packet[index]); index += 2U;

	if (flags & CGM_MEAS_FLAG_STATUS_OCTET_PRESENT)   { packet[index++] = rec->status[0]; }
	if (flags & CGM_MEAS_FLAG_CAL_TEMP_OCTET_PRESENT) { packet[index++] = rec->status[1]; }
	if (flags & CGM_MEAS_FLAG_WARNING_OCTET_PRESENT)  { packet[index++] = rec->status[2]; }

	if (trend_en) {
		sys_put_le16(rec->trend_sfloat, &packet[index]); index += 2U;
	}
	if (quality_en) {
		sys_put_le16(rec->quality_sfloat, &packet[index]); index += 2U;
	}

	if (index > capacity || index > UINT8_MAX) {
		return 0U;
	}

	packet[0] = (uint8_t)index;

	if (crc_en) {
		if ((uint16_t)(index + 2U) > capacity) {
			return 0U;
		}
		uint16_t crc = cgm_crc16_ccitt(packet, index);

		sys_put_le16(crc, &packet[index]); index += 2U;
	}

	return index;
}

/* ========== 当前测量报文组包（用于周期 notify） ========== */

static uint16_t cgm_build_measurement_record(uint8_t *packet, uint16_t capacity)
{
	uint8_t  flags = 0U;
	uint16_t index = 0U;
	uint16_t glucose_sfloat;
	bool     trend_en   = !!(cgm_feature_flags & CGM_FEATURE_FLAG_TREND_INFORMATION_SUPPORTED);
	bool     quality_en = !!(cgm_feature_flags & CGM_FEATURE_FLAG_QUALITY_SUPPORTED);
	bool     crc_en     = !!(cgm_feature_flags & CGM_FEATURE_FLAG_E2E_CRC_SUPPORTED);

	if (capacity < 6U) {
		return 0U;
	}

	if (cgm_status_annunciation[0] != 0U) { flags |= CGM_MEAS_FLAG_STATUS_OCTET_PRESENT; }
	if (cgm_status_annunciation[1] != 0U) { flags |= CGM_MEAS_FLAG_CAL_TEMP_OCTET_PRESENT; }
	if (cgm_status_annunciation[2] != 0U) { flags |= CGM_MEAS_FLAG_WARNING_OCTET_PRESENT; }
	if (trend_en)                         { flags |= CGM_MEAS_FLAG_TREND_PRESENT; }
	if (quality_en)                       { flags |= CGM_MEAS_FLAG_QUALITY_PRESENT; }

	packet[index++] = 0U;   /* Size placeholder */
	packet[index++] = flags;

	glucose_sfloat = cgm_encode_sfloat((int16_t)cgm_glucose_mg_dl, 0);
	sys_put_le16(glucose_sfloat, &packet[index]); index += 2U;
	sys_put_le16(cgm_time_offset_min, &packet[index]); index += 2U;

	if (flags & CGM_MEAS_FLAG_STATUS_OCTET_PRESENT)   { packet[index++] = cgm_status_annunciation[0]; }
	if (flags & CGM_MEAS_FLAG_CAL_TEMP_OCTET_PRESENT) { packet[index++] = cgm_status_annunciation[1]; }
	if (flags & CGM_MEAS_FLAG_WARNING_OCTET_PRESENT)  { packet[index++] = cgm_status_annunciation[2]; }

	if (trend_en) {
		sys_put_le16(cgm_trend_sfloat, &packet[index]); index += 2U;
	}
	if (quality_en) {
		sys_put_le16(cgm_quality_sfloat, &packet[index]); index += 2U;
	}

	if (index > capacity || index > UINT8_MAX) {
		return 0U;
	}

	packet[0] = (uint8_t)index;

	if (crc_en) {
		if ((uint16_t)(index + 2U) > capacity) {
			return 0U;
		}
		uint16_t crc = cgm_crc16_ccitt(packet, index);

		sys_put_le16(crc, &packet[index]); index += 2U;
	}

	return index;
}

/* ========== 周期测量工作项 ========== */

static void cgm_cgms_measurement_work_cb(struct k_work *work)
{
	uint8_t  packet[CGM_MEAS_MAX_PACKET_SIZE];
	uint16_t packet_len;

	ARG_UNUSED(work);

	if (!cgm_active_conn || !cgm_cgms_meas_notify_enabled || !cgm_session_running) {
		return;
	}

	/* 先入库，保证历史数据可通过 RACP 重放 */
	cgm_db_push(cgm_time_offset_min,
		    cgm_encode_sfloat((int16_t)cgm_glucose_mg_dl, 0),
		    cgm_status_annunciation,
		    cgm_trend_sfloat,
		    cgm_quality_sfloat);

	packet_len = cgm_build_measurement_record(packet, sizeof(packet));
	if (packet_len == 0U) {
		return;
	}

	bt_gatt_notify(cgm_active_conn,
		       &attr_cgm_cgms_svc[CGM_CGMS_MEAS_HDL],
		       packet, packet_len);

	cgm_time_offset_min      += cgm_comm_interval_min;
	cgm_session_run_time_min += cgm_comm_interval_min;
	cgm_glucose_mg_dl = (cgm_glucose_mg_dl < 160U)
			    ? (cgm_glucose_mg_dl + 1U) : 105U;

	if (cgm_bas_notify_enabled && cgm_battery_level_percent > 0U) {
		cgm_battery_level_percent--;
		bt_gatt_notify(cgm_active_conn,
			       &attr_cgm_bas_svc[CGM_BAS_LEVEL_HDL],
			       &cgm_battery_level_percent,
			       sizeof(cgm_battery_level_percent));
	}

	k_delayed_work_submit(&cgm_meas_work,
			      K_SECONDS(cgm_comm_interval_min));
}

/* ========== CCC 回调 ========== */

void cgm_cgms_meas_ccc_changed_cb(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);
	cgm_cgms_meas_notify_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1U : 0U;

	if (!cgm_cgms_meas_notify_enabled) {
		k_delayed_work_cancel(&cgm_meas_work);
	}
}

/* ========== 生命周期接口 ========== */

void cgm_meas_init(void)
{
	k_delayed_work_init(&cgm_meas_work, cgm_cgms_measurement_work_cb);
}

void cgm_meas_start(void)
{
	cgm_session_running = true;
	k_delayed_work_submit(&cgm_meas_work, K_SECONDS(1));
}

void cgm_meas_stop(void)
{
	cgm_session_running = false;
	k_delayed_work_cancel(&cgm_meas_work);
}

void cgm_meas_cancel(void)
{
	k_delayed_work_cancel(&cgm_meas_work);
	cgm_session_running = false;
}
