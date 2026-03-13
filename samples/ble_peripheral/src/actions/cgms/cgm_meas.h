/* SPDX-License-Identifier: Apache-2.0 */
/*
 * cgm_meas.h — CGM Measurement 特征（UUID 0x2AA7）公共接口
 *
 * 导出：
 *   - cgm_db_record          数据库记录结构体
 *   - cgm_encode_sfloat()    SFLOAT 编码辅助
 *   - cgm_db_push()          向环形数据库压入一条记录
 *   - cgm_build_meas_packet_from_db()  从数据库记录重建 Measurement 报文
 *   - cgm_cgms_meas_ccc_changed_cb()  Measurement CCC 变化回调
 *   - cgm_meas_init()        初始化测量工作项（调用一次）
 *   - cgm_meas_start()       启动周期测量（由 SOCP START SESSION 调用）
 *   - cgm_meas_stop()        停止周期测量（由 SOCP STOP SESSION 调用）
 *   - cgm_meas_cancel()      取消并复位（由断连回调调用）
 */

#ifndef CGM_MEAS_H
#define CGM_MEAS_H

#include <zephyr/types.h>
#include <bluetooth/gatt.h>
#include "cgms_defs.h"

/* ---- 数据库记录结构体（供 cgm_racp.c 访问） ---- */
struct cgm_db_record {
	uint16_t time_offset;    /**< session 内相对分钟偏移 */
	uint16_t glucose_sfloat; /**< SFLOAT 编码的葡萄糖浓度 (mg/dL) */
	uint8_t  status[3];      /**< Sensor Status Annunciation 快照 */
	uint16_t trend_sfloat;   /**< CGM Trend Information（SFLOAT，mg/dL/min）*/
	uint16_t quality_sfloat; /**< CGM Quality（SFLOAT，%）*/
};

/* ---- 数据库状态（供 cgm_racp.c 只读访问） ---- */
extern struct cgm_db_record cgm_db[CGM_DB_MAX_RECORDS];
extern uint16_t cgm_db_count;  /**< 当前有效记录数 */
extern uint16_t cgm_db_head;   /**< 最旧记录的环形索引 */

/**
 * @brief SFLOAT 编码。
 *
 * @param mantissa 有效数字（12 bit 有效）。
 * @param exponent 指数（4 bit 有效，2 的补码）。
 * @return 16-bit SFLOAT。
 */
uint16_t cgm_encode_sfloat(int16_t mantissa, int8_t exponent);

/**
 * @brief 向测量数据库压入一条记录。
 *
 * @param time_offset    session 内分钟偏移。
 * @param glucose_sfloat SFLOAT 编码浓度。
 * @param status         3 字节 Sensor Status Annunciation 快照。
 * @param trend_sfloat   SFLOAT 编码趋势值（Feature 未声明时可传 0）。
 * @param quality_sfloat SFLOAT 编码质量值（Feature 未声明时可传 0）。
 */
void cgm_db_push(uint16_t time_offset, uint16_t glucose_sfloat,
		 const uint8_t status[3],
		 uint16_t trend_sfloat, uint16_t quality_sfloat);

/**
 * @brief 从数据库记录重建 CGM Measurement 报文。
 *
 * @param packet   输出缓冲区（至少 15 字节）。
 * @param capacity 缓冲区字节容量。
 * @param rec      数据库记录指针。
 * @return 报文实际字节数；0 表示失败。
 */
uint16_t cgm_build_meas_packet_from_db(uint8_t *packet, uint16_t capacity,
					const struct cgm_db_record *rec);

/**
 * @brief Measurement CCC 变化回调（由 BT_GATT_CCC 引用）。
 */
void cgm_cgms_meas_ccc_changed_cb(const struct bt_gatt_attr *attr, uint16_t value);

/**
 * @brief 初始化测量工作项，程序启动时调用一次。
 */
void cgm_meas_init(void);

/**
 * @brief 启动周期性 CGM 测量（Start Session）。
 */
void cgm_meas_start(void);

/**
 * @brief 停止周期性 CGM 测量（Stop Session）。
 */
void cgm_meas_stop(void);

/**
 * @brief 断连时强制取消测量工作项并复位会话状态。
 */
void cgm_meas_cancel(void);

#endif /* CGM_MEAS_H */
