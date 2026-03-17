/*
 * Port of the nRF5 SDK CGMS demo to the cgm_sdk ble_peripheral sample.
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/byteorder.h>
#include <sys/printk.h>
#include <zephyr.h>

#include <bluetooth/att.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/uuid.h>

#include "ble_super_service.h"
#include "ble_data_test_sample.h"

/*
 * 修改原因：与 nRF5 SDK 的 cgms_sst.h 结构定义保持一致。
 * 若工程中未引入 Nordic 的 ble_date_time_t，则提供同布局兼容定义。
 */
#ifndef BLE_DATE_TIME_H__
typedef struct
{
	uint16_t year;
	uint8_t  month;
	uint8_t  day;
	uint8_t  hours;
	uint8_t  minutes;
	uint8_t  seconds;
} ble_date_time_t;
#endif

/*
 * 修改原因：与 nRF5 SDK 的 ble_racp.h 结构定义保持一致。
 * 重点是字段名 p_operand，避免与 SDK 命名差异导致移植不一致。
 */
#ifndef BLE_RACP_H__
typedef struct
{
	uint8_t   opcode;
	uint8_t   operator;
	uint8_t   operand_len;
	uint8_t * p_operand;
} ble_racp_value_t;
#endif

/*
 * 修改原因：与 nRF5 SDK 的 nrf_ble_cgms_init_t 字段保持一致，补充兼容类型声明。
 */
#ifndef BLE_SRV_COMMON_H__
typedef void (*ble_srv_error_handler_t)(uint32_t nrf_error);
#endif

#ifndef NRF_BLE_GQ_H__
typedef struct nrf_ble_gq_s nrf_ble_gq_t;
#endif

#ifndef ENOTCONN
#define ENOTCONN 57
#endif

/*
 * 修改原因：Nordic SDK 使用 BLE_GATT_ATT_MTU_DEFAULT，Zephyr 环境下可能仅提供 BT_* 命名。
 * 这里增加兼容映射，避免 NRF_BLE_CGMS_MEAS_LEN_MAX 展开时报未定义。
 */
#ifndef BLE_GATT_ATT_MTU_DEFAULT
#if defined(BT_ATT_DEFAULT_LE_MTU)
#define BLE_GATT_ATT_MTU_DEFAULT BT_ATT_DEFAULT_LE_MTU
#elif defined(BT_GATT_ATT_MTU_DEFAULT)
#define BLE_GATT_ATT_MTU_DEFAULT BT_GATT_ATT_MTU_DEFAULT
#else
#define BLE_GATT_ATT_MTU_DEFAULT 23
#endif
#endif

#define CGMS_DB_MAX_RECORDS                         32
#define CGMS_CALIBRATION_VALUE_LEN                  10
#define GLUCOSE_MEAS_INTERVAL_MINUTES              1
#define GL_CONCENTRATION_INC                       10
#define GL_CONCENTRATION_DEC                       5
#define MAX_GLUCOSE_CONCENTRATION                  800
#define MIN_GLUCOSE_CONCENTRATION                  5
#define SOCP_COMM_INTERVAL_USE_DEFAULT             0xFF
#define OPERAND_LESS_GREATER_FILTER_TYPE_SIZE      1
#define OPERAND_LESS_GREATER_FILTER_PARAM_SIZE     2
#define OPERAND_LESS_GREATER_SIZE                  \
	(OPERAND_LESS_GREATER_FILTER_TYPE_SIZE + OPERAND_LESS_GREATER_FILTER_PARAM_SIZE)
#define RACP_OPERAND_FILTER_TYPE_TIME_OFFSET       0x01
#define RACP_OPERAND_FILTER_TYPE_FACING_TIME       0x02

#define RACP_OPCODE_RESERVED                       0x00
#define RACP_OPCODE_REPORT_RECS                    0x01
#define RACP_OPCODE_DELETE_RECS                    0x02
#define RACP_OPCODE_ABORT_OPERATION                0x03
#define RACP_OPCODE_REPORT_NUM_RECS                0x04
#define RACP_OPCODE_NUM_RECS_RESPONSE              0x05
#define RACP_OPCODE_RESPONSE_CODE                  0x06

#define RACP_OPERATOR_NULL                         0x00
#define RACP_OPERATOR_ALL                          0x01
#define RACP_OPERATOR_LESS_OR_EQUAL                0x02
#define RACP_OPERATOR_GREATER_OR_EQUAL             0x03
#define RACP_OPERATOR_RANGE                        0x04
#define RACP_OPERATOR_FIRST                        0x05
#define RACP_OPERATOR_LAST                         0x06

/**@brief Record Access Control Point Operand Filter Type Value. */
#define RACP_OPERAND_FILTER_TYPE_TIME_OFFSET 	   0x01       /**< Record Access Control Point Operand Filter Type Value - Time Offset. */
#define RACP_OPERAND_FILTER_TYPE_FACING_TIME       0x02     // 框架未实现

#define RACP_RESPONSE_RESERVED               	   0x00 
#define RACP_RESPONSE_SUCCESS                      0x01
#define RACP_RESPONSE_OPCODE_UNSUPPORTED           0x02
#define RACP_RESPONSE_INVALID_OPERATOR             0x03
#define RACP_RESPONSE_OPERATOR_UNSUPPORTED         0x04
#define RACP_RESPONSE_INVALID_OPERAND              0x05
#define RACP_RESPONSE_NO_RECORDS_FOUND             0x06
#define RACP_RESPONSE_ABORT_FAILED                 0x07
#define RACP_RESPONSE_PROCEDURE_NOT_DONE           0x08
#define RACP_RESPONSE_OPERAND_UNSUPPORTED          0x09

#define SOCP_OPCODE_RESERVED                       0x00
#define SOCP_WRITE_CGM_COMMUNICATION_INTERVAL      0x01
#define SOCP_READ_CGM_COMMUNICATION_INTERVAL       0x02
#define SOCP_READ_CGM_COMM_INTERVAL_RSP            0x03
#define SOCP_WRITE_GLUCOSE_CALIBRATION_VALUE          0x04
#define SOCP_READ_GLUCOSE_CALIBRATION_VALUE           0x05
#define SOCP_READ_GLUCOSE_CALIBRATION_VALUE_RESPONSE  0x06
#define SOCP_WRITE_PATIENT_HIGH_ALERT_LEVEL           0x07
#define SOCP_READ_PATIENT_HIGH_ALERT_LEVEL            0x08
#define SOCP_READ_PATIENT_HIGH_ALERT_LEVEL_RESPONSE   0x09
#define SOCP_WRITE_PATIENT_LOW_ALERT_LEVEL            0x0A
#define SOCP_READ_PATIENT_LOW_ALERT_LEVEL             0x0B
#define SOCP_READ_PATIENT_LOW_ALERT_LEVEL_RESPONSE    0x0C
#define SOCP_SET_HYPO_ALERT_LEVEL                     0x0D /**Set Hypo Alert Level    Hypo Alert Level value in mg/dL    The response to this control point is Response Code.                                            */
#define SOCP_GET_HYPO_ALERT_LEVEL                     0x0E /**Get Hypo Alert Level    N/A    The normal response to this control point is Op Code 0x0F. For error conditions, the response is Response Code              */
#define SOCP_HYPO_ALERT_LEVEL_RESPONSE                0x0F /**Hypo Alert Level Response    Hypo Alert Level value in mg/dL    This is the normal response to Op Code 0x0E                                             */
#define SOCP_SET_HYPER_ALERT_LEVEL                    0x10 /**Set Hyper Alert Level    Hyper Alert Level value in mg/dL    The response to this control point is Response Code.                                    */
#define SOCP_GET_HYPER_ALERT_LEVEL                    0x11 /**Get Hyper Alert Level    N/A    The normal response to this control point is Op Code 0x12. For error conditions, the response is Response Code          */
#define SOCP_HYPER_ALERT_LEVEL_RESPONSE               0x12 /**Hyper Alert Level Response    Hyper Alert Level value in mg/dL    This is the normal response to Op Code 0x11                                         */
#define SOCP_SET_RATE_OF_DECREASE_ALERT_LEVEL         0x13 /**Set Rate of Decrease Alert Level    Rate of Decrease Alert Level value in mg/dL/min    The response to this control point is Response Code.                */
#define SOCP_GET_RATE_OF_DECREASE_ALERT_LEVEL         0x14 /**Get Rate of Decrease Alert Level    N/A    The normal response to this control point is Op Code 0x15. For error conditions, the response is Response Code  */
#define SOCP_RATE_OF_DECREASE_ALERT_LEVEL_RESPONSE    0x15 /**Rate of Decrease Alert Level Response    Rate of Decrease Alert Level value in mg/dL/min    This is the normal response to Op Code 0x14                 */
#define SOCP_SET_RATE_OF_INCREASE_ALERT_LEVEL         0x16 /**Set Rate of Increase Alert Level    Rate of Increase Alert Level value in mg/dL/min    The response to this control point is Response Code.                */
#define SOCP_GET_RATE_OF_INCREASE_ALERT_LEVEL         0x17 /**Get Rate of Increase Alert Level    N/A    The normal response to this control point is Op Code 0x18. For error conditions, the response is Response Code  */
#define SOCP_RATE_OF_INCREASE_ALERT_LEVEL_RESPONSE    0x18 /**Rate of Increase Alert Level Response    Rate of Increase Alert Level value in mg/dL/min    This is the normal response to Op Code 0x17                 */
#define SOCP_RESET_DEVICE_SPECIFIC_ALERT              0x19 /**Reset Device Specific Alert    N/A    The response to this control point is Response Code. */

#define SOCP_START_THE_SESSION                     0x1A
#define SOCP_STOP_THE_SESSION                      0x1B
#define SOCP_RESPONSE_CODE                         0x1C

#define SOCP_RSP_RESERVED_FOR_FUTURE_USE           0x00
#define SOCP_RSP_SUCCESS                           0x01
#define SOCP_RSP_OP_CODE_NOT_SUPPORTED             0x02
#define SOCP_RSP_INVALID_OPERAND                   0x03
#define SOCP_RSP_PROCEDURE_NOT_COMPLETED           0x04
#define SOCP_RSP_OUT_OF_RANGE                      0x05

#define NRF_BLE_CGMS_PLUS_INFINITE                     0x07FE
#define NRF_BLE_CGMS_MINUS_INFINITE                    0x0802
#define NRF_BLE_CGMS_FEAT_MULTIPLE_BOND_SUPPORTED      (0x01 << 13)
#define NRF_BLE_CGMS_FEAT_MULTIPLE_SESSIONS_SUPPORTED  (0x01 << 14)
#define NRF_BLE_CGMS_STATUS_SESSION_STOPPED            (0x01 << 0)
#define NRF_BLE_CGMS_STATUS_DEVICE_SPECIFIC_ALERT      (0x01 << 4)
#define NRF_BLE_CGMS_MEAS_TYPE_VEN_BLOOD               0x03
#define NRF_BLE_CGMS_MEAS_LOC_AST                      0x02
#define NRF_BLE_CGMS_STATUS_FLAGS_WARNING_OCT_PRESENT  0x20
#define NRF_BLE_CGMS_STATUS_FLAGS_CALTEMP_OCT_PRESENT  0x40
#define NRF_BLE_CGMS_STATUS_FLAGS_STATUS_OCT_PRESENT   0x80

/**@name Byte length of various commands (used for validating, encoding, and decoding data).
 * @{ */
#define NRF_BLE_CGMS_MEAS_OP_LEN            1                               //!< Length of the opcode inside the Glucose Measurement packet.
#define NRF_BLE_CGMS_MEAS_HANDLE_LEN        2                               //!< Length of the handle inside the Glucose Measurement packet.
#define NRF_BLE_CGMS_MEAS_LEN_MAX           (BLE_GATT_ATT_MTU_DEFAULT - \
                                             NRF_BLE_CGMS_MEAS_OP_LEN - \
                                             NRF_BLE_CGMS_MEAS_HANDLE_LEN)  //!< Maximum size of a transmitted Glucose Measurement.

#define NRF_BLE_CGMS_MEAS_REC_LEN_MAX       15                              //!< Maximum length of one measurement record. Size 1 byte, flags 1 byte, glucose concentration 2 bytes, offset 2 bytes, status 3 bytes, trend 2 bytes, quality 2 bytes, CRC 2 bytes.
#define NRF_BLE_CGMS_MEAS_REC_LEN_MIN       6                               //!< Minimum length of one measurement record. Size 1 byte, flags 1 byte, glucose concentration 2 bytes, offset 2 bytes.
#define NRF_BLE_CGMS_MEAS_REC_PER_NOTIF_MAX (NRF_BLE_CGMS_MEAS_LEN_MAX / \
                                             NRF_BLE_CGMS_MEAS_REC_LEN_MIN) //!< Maximum number of records per notification. We can send more than one measurement record per notification, but we do not want a a single record split over two notifications.

#define NRF_BLE_CGMS_SOCP_RESP_CODE_LEN     2                               //!< Length of a response. Response code 1 byte, response value 1 byte.
#define NRF_BLE_CGMS_FEATURE_LEN            6                               //!< Length of a feature. Feature 3 bytes, type 4 bits, sample location 4 bits, CRC 2 bytes.
#define NRF_BLE_CGMS_STATUS_LEN             7                               //!< Length of a status. Offset 2 bytes, status 3 bytes, CRC 2 bytes.
#define NRF_BLE_CGMS_MAX_CALIB_LEN          10                              //!< Length of a calibration record. Concentration 2 bytes, time 2 bytes, calibration 4 bits, calibration sample location 4 bits, next calibration time 2 bytes, record number 2 bytes, calibration status 1 byte.
#define NRF_BLE_CGMS_CALIBS_NB_MAX          5                               //!< Maximum number of calibration values that can be stored.
#define NRF_BLE_CGMS_SST_LEN                9                               //!< Length of the start time. Date time 7 bytes, time zone 1 byte, DST 1 byte.
#define NRF_BLE_CGMS_CRC_LEN                2                               //!< Length of the CRC bytes (if used).
#define NRF_BLE_CGMS_SRT_LEN                2                               //!< Length of the Session Run Time attribute.

#define NRF_BLE_CGMS_SOCP_RESP_LEN          (NRF_BLE_CGMS_MEAS_LEN_MAX - \
                                            NRF_BLE_CGMS_SOCP_RESP_CODE_LEN) //!< Max lenth of a SOCP response.

#define NRF_BLE_CGMS_RACP_PENDING_OPERANDS_MAX 2                             // !< Maximum number of pending Record Access Control Point operations.
/** @} */

/**@brief CGM Measurement Sensor Status Annunciation. */
struct cgms_sensor_annunciation {
	uint8_t warning;
	uint8_t calib_temp;
	uint8_t status;
};

/**@brief CGM measurement. */
struct cgms_measurement {
	uint8_t flags;
	uint16_t glucose_concentration;
	uint16_t time_offset;
	struct cgms_sensor_annunciation sensor_status_annunciation;
	uint16_t trend;
	uint16_t quality;
};

/**@brief CGM Measurement record. */
struct cgms_record {
	struct cgms_measurement meas;
};

/**@brief Status of the CGM measurement. */
struct cgms_status {
	uint16_t time_offset;
	struct cgms_sensor_annunciation annunciation;
};

/**@brief Features supported by the CGM Service. */
struct cgms_feature_value {
	uint32_t feature;
	uint8_t type;
	uint8_t sample_location;
};

/**@brief CGM Service initialization structure that contains all options and data needed for
 *        initializing the service. */
struct cgms_ble_cgms_init {
    ble_cgms_evt_handler_t    evt_handler;           /**< Event handler to be called for handling events in the CGM Service. */
	/* 修改原因：按 nRF5 SDK 的 nrf_ble_cgms_init_t 对齐字段。 */
	ble_srv_error_handler_t   error_handler;         /**< Function to be called when an error occurs. */
	nrf_ble_gq_t            * p_gatt_queue;          /**< Pointer to BLE GATT Queue instance. */
    struct cgms_feature_value feature;               /**< Features supported by the service. */
    struct cgms_status        initial_sensor_status; /**< Sensor status. */
    uint16_t                  initial_run_time;      /**< Run time. */
} nrf_ble_cgms_init_t;

/**@brief Specific Operation Control Point response structure. */
typedef struct
{
    uint8_t opcode;                               /**< Opcode describing the response. */
    uint8_t req_opcode;                           /**< The original opcode for the request to which this response belongs. */
    uint8_t rsp_code;                             /**< Response code. */
    uint8_t resp_val[NRF_BLE_CGMS_SOCP_RESP_LEN]; /**< Array containing the response value. */
    uint8_t size_val;                             /**< Length of the response value. */
} ble_socp_rsp_t;

/**@brief Calibration value. */
typedef struct
{
    uint8_t value[NRF_BLE_CGMS_MAX_CALIB_LEN]; /**< Array containing the calibration value. */
} nrf_ble_cgms_calib_t;

/**@brief Record Access Control Point transaction data. */
typedef struct
{
    uint8_t          racp_proc_operator;                                                    /**< Operator of the current request. */
    uint16_t         racp_proc_record_ndx;                                                  /**< Current record index. */
    uint16_t         racp_proc_records_ndx_last_to_send;                                    /**< The last record to send, can be used together with racp_proc_record_ndx to determine a range of records to send. (used by greater/less filters). */
    uint16_t         racp_proc_records_reported;                                            /**< Number of reported records. */
    ble_racp_value_t racp_request;                                                          /**< RACP procedure that has been requested from the peer. */
    ble_racp_value_t pending_racp_response;                                                 /**< RACP response to be sent. */
    bool             racp_procesing_active;                                                 /**< RACP processing active. */
    uint8_t          pending_racp_response_operand[NRF_BLE_CGMS_RACP_PENDING_OPERANDS_MAX]; /**< Operand of the RACP response to be sent. */
} nrf_ble_cgms_racp_t;

/*
 * 修改原因：与 nRF5 SDK 的 ble_cgms_sst_t 保持一致，使用 date_time 子结构。
 */
typedef struct
{
	ble_date_time_t date_time;
	uint8_t         time_zone;
	uint8_t         dst;
} ble_cgms_sst_t;


// 注销，用nrf_ble_cgms_racp_t替换
struct racp_response {
	uint8_t opcode;
	uint8_t operator;
	uint8_t operand[4];
	uint8_t operand_len;
};

/*
 * 修改原因：与 nRF5 SDK 的 ble_cgms_socp_value_t 保持一致，字段名改为 p_operand。
 */
typedef struct
{
	uint8_t   opcode;
	uint8_t   operand_len;
	uint8_t * p_operand;
} ble_cgms_socp_value_t;

struct cgms_racp_state {
	bool processing_active;
	uint8_t proc_operator;
	uint16_t proc_record_ndx;
	uint16_t proc_records_ndx_last_to_send;
	uint16_t proc_records_reported;
};

struct cgms_alert_levels {
	uint16_t patient_high;
	uint16_t patient_low;
	uint16_t hypo;
	uint16_t hyper;
	uint16_t rate_decrease;
	uint16_t rate_increase;
};
extern const struct bt_gatt_attr attr_cgms_svc[];

static struct bt_conn *m_conn;
static ble_cgms_evt_handler_t m_evt_handler;
static struct k_delayed_work m_glucose_work;
static struct cgms_record m_records[CGMS_DB_MAX_RECORDS];
static uint16_t m_record_count;
static struct cgms_feature_value m_feature = {
	.feature = NRF_BLE_CGMS_FEAT_MULTIPLE_BOND_SUPPORTED |
		   NRF_BLE_CGMS_FEAT_MULTIPLE_SESSIONS_SUPPORTED,
	.type = NRF_BLE_CGMS_MEAS_TYPE_VEN_BLOOD,
	.sample_location = NRF_BLE_CGMS_MEAS_LOC_AST,
};
static struct cgms_status m_status = {
	.time_offset = 0,
	.annunciation = {
		.warning = 0,
		.calib_temp = 0,
		.status = NRF_BLE_CGMS_STATUS_SESSION_STOPPED,
	},
};
static ble_cgms_sst_t m_sst;
static uint16_t m_session_run_time = 20;
static uint8_t m_comm_interval = GLUCOSE_MEAS_INTERVAL_MINUTES;
static bool m_session_started;
static uint8_t m_nb_run_session;
static uint16_t m_current_offset;
static uint16_t m_glucose_concentration = MIN_GLUCOSE_CONCENTRATION;
static bool m_meas_notify_enabled;
static bool m_racp_ind_enabled;
static bool m_socp_ind_enabled;
static struct bt_gatt_indicate_params m_racp_ind_params;
static struct bt_gatt_indicate_params m_socp_ind_params;
static uint8_t m_racp_ind_buf[8];
static uint8_t m_socp_ind_buf[20];
static struct cgms_racp_state m_racp;
static struct cgms_alert_levels m_alert_levels;
static uint8_t m_calibration_value[CGMS_CALIBRATION_VALUE_LEN] = {
	0x3E, 0x00, 0x07, 0x00, 0x06,
	0x07, 0x00, 0x00, 0x00, 0x00,
};

enum {
	CGMS_ATTR_SVC = 0,
	CGMS_ATTR_MEAS_CHRC,
	CGMS_ATTR_MEAS_VAL,
	CGMS_ATTR_MEAS_CCC,
	CGMS_ATTR_FEATURE_CHRC,
	CGMS_ATTR_FEATURE_VAL,
	CGMS_ATTR_STATUS_CHRC,
	CGMS_ATTR_STATUS_VAL,
	CGMS_ATTR_SST_CHRC,
	CGMS_ATTR_SST_VAL,
	CGMS_ATTR_SRT_CHRC,
	CGMS_ATTR_SRT_VAL,
	CGMS_ATTR_RACP_CHRC,
	CGMS_ATTR_RACP_VAL,
	CGMS_ATTR_RACP_CCC,
	CGMS_ATTR_SOCP_CHRC,
	CGMS_ATTR_SOCP_VAL,
	CGMS_ATTR_SOCP_CCC,
};

static void cgms_emit_event(ble_cgms_evt_type_t evt_type)
{
	ble_cgms_evt_t evt;

	if (m_evt_handler == NULL) {
		return;
	}

	evt.evt_type = evt_type;
	evt.comm_interval = m_comm_interval;
	m_evt_handler(&evt);
}

static void put_le16(uint8_t *dst, uint16_t value)
{
	dst[0] = (uint8_t)(value & 0xFF);
	dst[1] = (uint8_t)(value >> 8);
}

static void put_le24(uint8_t *dst, uint32_t value)
{
	dst[0] = (uint8_t)(value & 0xFF);
	dst[1] = (uint8_t)((value >> 8) & 0xFF);
	dst[2] = (uint8_t)((value >> 16) & 0xFF);
}

static uint16_t get_le16(const uint8_t *src)
{
	return (uint16_t)src[0] | ((uint16_t)src[1] << 8);
}

static bool cgms_feature_present(uint32_t feature)
{
	return ((m_feature.feature & feature) != 0U);
}

static void cgms_racp_reset_state(void)
{
	memset(&m_racp, 0, sizeof(m_racp));
}

static uint8_t cgms_normalize_comm_interval(uint8_t interval)
{
	if (interval == SOCP_COMM_INTERVAL_USE_DEFAULT) {
		return GLUCOSE_MEAS_INTERVAL_MINUTES;
	}

	return interval;
}

static int cgms_record_index_offset_less_or_equal_get(uint16_t offset, uint16_t *record_num)
{
	uint16_t i;

	if ((record_num == NULL) || (m_record_count == 0U)) {
		return -ENOENT;
	}

	for (i = m_record_count; i > 0U; i--) {
		if (m_records[i - 1U].meas.time_offset <= offset) {
			*record_num = (uint16_t)(i - 1U);
			return 0;
		}
	}

	return -ENOENT;
}

static int cgms_record_index_offset_greater_or_equal_get(uint16_t offset, uint16_t *record_num)
{
	uint16_t i;

	if ((record_num == NULL) || (m_record_count == 0U)) {
		return -ENOENT;
	}

	for (i = 0U; i < m_record_count; i++) {
		if (m_records[i].meas.time_offset >= offset) {
			*record_num = i;
			return 0;
		}
	}

	return -ENOENT;
}

static bool cgms_socp_response_has_result_code(uint8_t opcode)
{
	switch (opcode) {
	case SOCP_READ_CGM_COMM_INTERVAL_RSP:
	case SOCP_READ_GLUCOSE_CALIBRATION_VALUE_RESPONSE:
	case SOCP_READ_PATIENT_HIGH_ALERT_LEVEL_RESPONSE:
	case SOCP_READ_PATIENT_LOW_ALERT_LEVEL_RESPONSE:
	case SOCP_HYPO_ALERT_LEVEL_RESPONSE:
	case SOCP_HYPER_ALERT_LEVEL_RESPONSE:
	case SOCP_RATE_OF_DECREASE_ALERT_LEVEL_RESPONSE:
	case SOCP_RATE_OF_INCREASE_ALERT_LEVEL_RESPONSE:
		return false;
	default:
		return true;
	}
}

static uint8_t cgms_socp_decode_u16(const ble_cgms_socp_value_t *req, uint16_t *value)
{
	if ((req == NULL) || (value == NULL) || (req->operand_len != sizeof(uint16_t)) ||
	    (req->p_operand == NULL)) {
		return SOCP_RSP_INVALID_OPERAND;
	}

	*value = get_le16(req->p_operand);
	if ((*value == NRF_BLE_CGMS_PLUS_INFINITE) || (*value == NRF_BLE_CGMS_MINUS_INFINITE)) {
		return SOCP_RSP_OUT_OF_RANGE;
	}

	return SOCP_RSP_SUCCESS;
}

static void cgms_schedule_glucose_work(void)
{
	if (!m_session_started || (m_comm_interval == 0U)) {
		return;
	}

	k_delayed_work_submit(&m_glucose_work, K_MINUTES(m_comm_interval));
}

static void cgms_cancel_glucose_work(void)
{
	k_delayed_work_cancel(&m_glucose_work);
}

static void cgms_record_add(const struct cgms_record *rec)
{
	if (m_record_count >= CGMS_DB_MAX_RECORDS) {
		memmove(&m_records[0], &m_records[1], sizeof(m_records[0]) * (CGMS_DB_MAX_RECORDS - 1));
		m_record_count = CGMS_DB_MAX_RECORDS - 1;
	}

	m_records[m_record_count++] = *rec;
}

static uint8_t cgms_encode_feature(uint8_t *buf)
{
	put_le24(buf, m_feature.feature);
	buf[3] = (uint8_t)((m_feature.sample_location << 4) | (m_feature.type & 0x0F));
	buf[4] = 0xFF;
	buf[5] = 0xFF;
	return 6;
}

static uint8_t cgms_encode_status(uint8_t *buf)
{
	put_le16(buf, m_status.time_offset);
	buf[2] = m_status.annunciation.status;
	buf[3] = m_status.annunciation.calib_temp;
	buf[4] = m_status.annunciation.warning;
	return 5;
}

/* 修改原因：SST 字段访问改为 date_time.*，与 nRF5 SDK 的 ble_cgms_sst_t 对齐。 */
static uint8_t cgms_encode_sst(uint8_t *buf)
{
	put_le16(&buf[0], m_sst.date_time.year);
	buf[2] = m_sst.date_time.month;
	buf[3] = m_sst.date_time.day;
	buf[4] = m_sst.date_time.hours;
	buf[5] = m_sst.date_time.minutes;
	buf[6] = m_sst.date_time.seconds;
	buf[7] = m_sst.time_zone;
	buf[8] = m_sst.dst;
	return 9;
}

static uint8_t cgms_encode_measurement(const struct cgms_record *rec, uint8_t *buf)
{
	uint8_t len = 2;
	uint8_t flags = rec->meas.flags;

	put_le16(&buf[len], rec->meas.glucose_concentration);
	len += 2;
	put_le16(&buf[len], rec->meas.time_offset);
	len += 2;

	if (rec->meas.sensor_status_annunciation.warning != 0U) {
		buf[len++] = rec->meas.sensor_status_annunciation.warning;
		flags |= NRF_BLE_CGMS_STATUS_FLAGS_WARNING_OCT_PRESENT;
	}
	if (rec->meas.sensor_status_annunciation.calib_temp != 0U) {
		buf[len++] = rec->meas.sensor_status_annunciation.calib_temp;
		flags |= NRF_BLE_CGMS_STATUS_FLAGS_CALTEMP_OCT_PRESENT;
	}
	if (rec->meas.sensor_status_annunciation.status != 0U) {
		buf[len++] = rec->meas.sensor_status_annunciation.status;
		flags |= NRF_BLE_CGMS_STATUS_FLAGS_STATUS_OCT_PRESENT;
	}

	buf[0] = len;
	buf[1] = flags;
	return len;
}

static void cgms_meas_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);
	m_meas_notify_enabled = (value == BT_GATT_CCC_NOTIFY);
	printk("CGMS measurement ccc %u\n", value);
	cgms_emit_event(m_meas_notify_enabled ? BLE_CGMS_EVT_NOTIFICATION_ENABLED :
			BLE_CGMS_EVT_NOTIFICATION_DISABLED);
}

static void cgms_racp_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);
	m_racp_ind_enabled = (value == BT_GATT_CCC_INDICATE);
}

static void cgms_socp_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);
	m_socp_ind_enabled = (value == BT_GATT_CCC_INDICATE);
}

static ssize_t cgms_read_feature(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	void *buf, uint16_t len, uint16_t offset)
{
	uint8_t value[6];
	uint8_t value_len = cgms_encode_feature(value);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, value_len);
}

static ssize_t cgms_read_status(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	void *buf, uint16_t len, uint16_t offset)
{
	uint8_t value[5];
	uint8_t value_len = cgms_encode_status(value);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, value_len);
}

static ssize_t cgms_read_sst(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	void *buf, uint16_t len, uint16_t offset)
{
	uint8_t value[9];
	uint8_t value_len = cgms_encode_sst(value);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, value_len);
}

static ssize_t cgms_read_srt(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	void *buf, uint16_t len, uint16_t offset)
{
	uint8_t value[2];
	put_le16(value, m_session_run_time);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(value));
}

/* 修改原因：SST 写入解析保持与 ble_cgms_sst_t 的布局一致。 */
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

static ssize_t cgms_write_sst(struct bt_conn *conn, const struct bt_gatt_attr *attr,
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

static void cgms_racp_ind_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params, uint8_t err)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(params);
	printk("RACP indicate done err=%u\n", err);
}

static void cgms_socp_ind_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params, uint8_t err)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(params);
	printk("SOCP indicate done err=%u\n", err);
}

static int cgms_racp_indicate(const uint8_t *data, uint16_t len)
{
	if ((m_conn == NULL) || !m_racp_ind_enabled) {
		return -ENOTCONN;
	}

	memcpy(m_racp_ind_buf, data, len);
	memset(&m_racp_ind_params, 0, sizeof(m_racp_ind_params));
	m_racp_ind_params.attr = &attr_cgms_svc[CGMS_ATTR_RACP_VAL];
	m_racp_ind_params.data = m_racp_ind_buf;
	m_racp_ind_params.len = len;
	m_racp_ind_params.func = cgms_racp_ind_cb;
	return bt_gatt_indicate(m_conn, &m_racp_ind_params);
}

static int cgms_socp_indicate(const uint8_t *data, uint16_t len)
{
	if ((m_conn == NULL) || !m_socp_ind_enabled) {
		return -ENOTCONN;
	}

	memcpy(m_socp_ind_buf, data, len);
	memset(&m_socp_ind_params, 0, sizeof(m_socp_ind_params));
	m_socp_ind_params.attr = &attr_cgms_svc[CGMS_ATTR_SOCP_VAL];
	m_socp_ind_params.data = m_socp_ind_buf;
	m_socp_ind_params.len = len;
	m_socp_ind_params.func = cgms_socp_ind_cb;
	return bt_gatt_indicate(m_conn, &m_socp_ind_params);
}

static int cgms_measurement_notify(const struct cgms_record *rec)
{
	uint8_t encoded[16];
	uint8_t len;

	if ((m_conn == NULL) || !m_meas_notify_enabled) {
		return -ENOTCONN;
	}

	len = cgms_encode_measurement(rec, encoded);
	return bt_gatt_notify(m_conn, &attr_cgms_svc[CGMS_ATTR_MEAS_VAL], encoded, len);
}

static void cgms_send_racp_response_code(uint8_t req_opcode, uint8_t rsp_code)
{
	uint8_t buf[4];

	buf[0] = RACP_OPCODE_RESPONSE_CODE;
	buf[1] = RACP_OPERATOR_NULL;
	buf[2] = req_opcode;
	buf[3] = rsp_code;
	(void)cgms_racp_indicate(buf, sizeof(buf));
}

static void cgms_send_racp_num_records(uint16_t count)
{
	uint8_t buf[4];

	buf[0] = RACP_OPCODE_NUM_RECS_RESPONSE;
	buf[1] = RACP_OPERATOR_NULL;
	put_le16(&buf[2], count);
	(void)cgms_racp_indicate(buf, sizeof(buf));
}

/* 修改原因：RACP 请求结构改为 ble_racp_value_t，并使用 SDK 同名字段 p_operand。 */
static void cgms_racp_decode(const uint8_t *buf, uint16_t len, ble_racp_value_t *req)
{
	req->opcode = 0xFFU;
	req->operator = 0xFFU;
	req->operand_len = 0U;
	req->p_operand = NULL;

	if (len > 0U) {
		req->opcode = buf[0];
	}
	if (len > 1U) {
		req->operator = buf[1];
	}
	if (len > 2U) {
		req->operand_len = len - 2U;
		req->p_operand = (uint8_t *)&buf[2];
	}
}

/* 修改原因：SOCP 请求结构改为 ble_cgms_socp_value_t，并使用 SDK 同名字段 p_operand。 */
static void cgms_socp_decode(const uint8_t *buf, uint16_t len, ble_cgms_socp_value_t *req)
{
	req->opcode = 0xFFU;
	req->operand_len = 0U;
	req->p_operand = NULL;

	if (len > 0U) {
		req->opcode = buf[0];
	}
	if (len > 1U) {
		req->operand_len = len - 1U;
		req->p_operand = (uint8_t *)&buf[1];
	}
}

static bool cgms_racp_request_valid(const ble_racp_value_t *req, uint8_t *rsp_code)
{
	*rsp_code = RACP_RESPONSE_RESERVED;

	if (req->opcode == RACP_OPCODE_ABORT_OPERATION) {
		if (!m_racp.processing_active) {
			*rsp_code = RACP_RESPONSE_ABORT_FAILED;
		} else if (req->operator != RACP_OPERATOR_NULL) {
			*rsp_code = RACP_RESPONSE_INVALID_OPERATOR;
		} else if (req->operand_len != 0U) {
			*rsp_code = RACP_RESPONSE_INVALID_OPERAND;
		} else {
			*rsp_code = RACP_RESPONSE_SUCCESS;
		}
		return false;
	}

	if (m_racp.processing_active) {
		return false;
	}

	if ((req->opcode != RACP_OPCODE_REPORT_RECS) &&
	    (req->opcode != RACP_OPCODE_REPORT_NUM_RECS)) {
		*rsp_code = RACP_RESPONSE_OPCODE_UNSUPPORTED;
		return false;
	}

	switch (req->operator) {
	case RACP_OPERATOR_ALL:
	case RACP_OPERATOR_FIRST:
	case RACP_OPERATOR_LAST:
		if (req->operand_len != 0U) {
			*rsp_code = RACP_RESPONSE_INVALID_OPERAND;
			return false;
		}
		return true;
	case RACP_OPERATOR_LESS_OR_EQUAL:
	case RACP_OPERATOR_GREATER_OR_EQUAL:
		if (req->operand_len != OPERAND_LESS_GREATER_SIZE) {
			*rsp_code = RACP_RESPONSE_INVALID_OPERAND;
			return false;
		}
		if (req->p_operand[0] == RACP_OPERAND_FILTER_TYPE_FACING_TIME) {
			*rsp_code = RACP_RESPONSE_PROCEDURE_NOT_DONE;
			return false;
		}
		if (req->p_operand[0] != RACP_OPERAND_FILTER_TYPE_TIME_OFFSET) {
			*rsp_code = RACP_RESPONSE_INVALID_OPERAND;
			return false;
		}
		return true;
	case RACP_OPERATOR_RANGE:
		*rsp_code = RACP_RESPONSE_OPERATOR_UNSUPPORTED;
		return false;
	default:
		*rsp_code = RACP_RESPONSE_INVALID_OPERATOR;
		return false;
	}
}

static uint16_t cgms_count_matching_records(const ble_racp_value_t *req)
{
	uint16_t total_records = m_record_count;
	uint16_t record_index;

	switch (req->operator) {
	case RACP_OPERATOR_ALL:
		return total_records;
	case RACP_OPERATOR_FIRST:
	case RACP_OPERATOR_LAST:
		return (total_records > 0U) ? 1U : 0U;
	case RACP_OPERATOR_LESS_OR_EQUAL:
		if (cgms_record_index_offset_less_or_equal_get(get_le16(&req->p_operand[1]),
						      &record_index) == 0) {
			return (uint16_t)(record_index + 1U);
		}
		return 0U;
	case RACP_OPERATOR_GREATER_OR_EQUAL:
		if (cgms_record_index_offset_greater_or_equal_get(get_le16(&req->p_operand[1]),
							 &record_index) == 0) {
			return (uint16_t)(total_records - record_index);
		}
		return 0U;
	default:
		return 0U;
	}
}

static int cgms_report_records(const ble_racp_value_t *req, uint16_t *sent)
{
	uint16_t i;
	uint16_t start = 0U;
	uint16_t end = 0U;
	int err;

	*sent = 0U;
	m_racp.proc_operator = req->operator;
	m_racp.proc_record_ndx = 0U;
	m_racp.proc_records_ndx_last_to_send = 0U;
	m_racp.proc_records_reported = 0U;
	m_racp.processing_active = true;

	if (m_record_count == 0U) {
		m_racp.processing_active = false;
		return 0;
	}

	switch (req->operator) {
	case RACP_OPERATOR_ALL:
		start = 0U;
		end = (uint16_t)(m_record_count - 1U);
		break;
	case RACP_OPERATOR_FIRST:
		start = 0U;
		end = 0U;
		break;
	case RACP_OPERATOR_LAST:
		start = (uint16_t)(m_record_count - 1U);
		end = start;
		break;
	case RACP_OPERATOR_LESS_OR_EQUAL:
		if (cgms_record_index_offset_less_or_equal_get(get_le16(&req->p_operand[1]), &end) != 0) {
			m_racp.processing_active = false;
			return 0;
		}
		start = 0U;
		break;
	case RACP_OPERATOR_GREATER_OR_EQUAL:
		if (cgms_record_index_offset_greater_or_equal_get(get_le16(&req->p_operand[1]), &start) != 0) {
			m_racp.processing_active = false;
			return 0;
		}
		end = (uint16_t)(m_record_count - 1U);
		break;
	default:
		m_racp.processing_active = false;
		return -EINVAL;
	}

	m_racp.proc_record_ndx = start;
	m_racp.proc_records_ndx_last_to_send = end;
	for (i = start; i <= end; i++) {
		err = cgms_measurement_notify(&m_records[i]);
		if (err != 0) {
			m_racp.processing_active = false;
			return err;
		}
		(*sent)++;
		m_racp.proc_records_reported = *sent;
		m_racp.proc_record_ndx = (uint16_t)(i + 1U);
		if (i == UINT16_MAX) {
			break;
		}
	}

	m_racp.processing_active = false;
	return 0;
}

static ssize_t cgms_write_racp(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	ble_racp_value_t req;
	uint8_t rsp_code;
	uint16_t count;

	ARG_UNUSED(conn);
	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (offset != 0U) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (!m_racp_ind_enabled) {
		return BT_GATT_ERR(BT_ATT_ERR_CCC_IMPROPER_CONF);
	}

	cgms_racp_decode((const uint8_t *)buf, len, &req);
	if (!cgms_racp_request_valid(&req, &rsp_code)) {
		if (rsp_code == RACP_RESPONSE_SUCCESS) {
			cgms_racp_reset_state();
		}
		if (rsp_code != RACP_RESPONSE_RESERVED) {
			m_racp.processing_active = false;
			cgms_send_racp_response_code(req.opcode, rsp_code);
		}
		return len;
	}

	if (req.opcode == RACP_OPCODE_REPORT_NUM_RECS) {
		cgms_send_racp_num_records(cgms_count_matching_records(&req));
		return len;
	}
	if (req.opcode == RACP_OPCODE_REPORT_RECS) {
		if (cgms_report_records(&req, &count) != 0) {
			cgms_send_racp_response_code(RACP_OPCODE_REPORT_RECS,
							RACP_RESPONSE_PROCEDURE_NOT_DONE);
			return len;
		}

		if (count > 0U) {
			cgms_send_racp_response_code(RACP_OPCODE_REPORT_RECS, RACP_RESPONSE_SUCCESS);
		} else {
			cgms_send_racp_response_code(RACP_OPCODE_REPORT_RECS, RACP_RESPONSE_NO_RECORDS_FOUND);
		}
	}

	// if (cgms_report_records(&req, &count) != 0) {
	// 	cgms_send_racp_response_code(RACP_OPCODE_REPORT_RECS,
	// 				    RACP_RESPONSE_PROCEDURE_NOT_DONE);
	// 	return len;
	// }

	// if (count > 0U) {
	// 	cgms_send_racp_response_code(RACP_OPCODE_REPORT_RECS, RACP_RESPONSE_SUCCESS);
	// } else {
	// 	cgms_send_racp_response_code(RACP_OPCODE_REPORT_RECS, RACP_RESPONSE_NO_RECORDS_FOUND);
	// }

	return len;
}

static int cgms_socp_send_response(uint8_t opcode, uint8_t req_opcode, uint8_t rsp_code,
	const uint8_t *value, uint8_t value_len)
{
	uint8_t buf[20];
	uint8_t len = 0U;

	buf[len++] = opcode;
	if (cgms_socp_response_has_result_code(opcode)) {
		buf[len++] = req_opcode;
		buf[len++] = rsp_code;
	}
	if ((value != NULL) && (value_len > 0U)) {
		memcpy(&buf[len], value, value_len);
		len += value_len;
	}

	return cgms_socp_indicate(buf, len);
}

static int cgms_socp_send_u16_response(uint8_t opcode, uint16_t value)
{
	uint8_t resp[2];

	put_le16(resp, value);
	return cgms_socp_send_response(opcode, 0U, SOCP_RSP_SUCCESS, resp, sizeof(resp));
}

static void cgms_start_session(void)
{
	m_session_started = true;
	m_nb_run_session++;
	m_status.time_offset = 0U;
	m_current_offset = 0U;
	m_status.annunciation.status &= (uint8_t)(~NRF_BLE_CGMS_STATUS_SESSION_STOPPED);
	memset(&m_sst, 0, sizeof(m_sst));
	cgms_emit_event(BLE_CGMS_EVT_START_SESSION);
	cgms_cancel_glucose_work();
	cgms_schedule_glucose_work();
}

static void cgms_stop_session(void)
{
	m_session_started = false;
	m_status.annunciation.status |= NRF_BLE_CGMS_STATUS_SESSION_STOPPED;
	cgms_cancel_glucose_work();
	cgms_emit_event(BLE_CGMS_EVT_STOP_SESSION);
}

static ssize_t cgms_write_socp(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	ble_cgms_socp_value_t req;
	uint8_t value[2];

	ARG_UNUSED(conn);
	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (offset != 0U) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	if (!m_socp_ind_enabled) {
		return BT_GATT_ERR(BT_ATT_ERR_CCC_IMPROPER_CONF);
	}

	cgms_socp_decode((const uint8_t *)buf, len, &req);

	switch (req.opcode) {
	case SOCP_WRITE_CGM_COMMUNICATION_INTERVAL:
		if (req.operand_len < 1U) {
			cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, SOCP_RSP_INVALID_OPERAND, NULL, 0U);
			break;
		}
		m_comm_interval = cgms_normalize_comm_interval(req.p_operand[0]);
		cgms_emit_event(BLE_CGMS_EVT_WRITE_COMM_INTERVAL);
		cgms_cancel_glucose_work();
		if (m_session_started && (m_comm_interval != 0U)) {
			cgms_schedule_glucose_work();
		}
		(void)cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, SOCP_RSP_SUCCESS,
						 NULL, 0U);
		break;
	case SOCP_READ_CGM_COMMUNICATION_INTERVAL:
		value[0] = m_comm_interval;
		(void)cgms_socp_send_response(SOCP_READ_CGM_COMM_INTERVAL_RSP, req.opcode,
						 SOCP_RSP_SUCCESS, value, 1U);
		break;
	case SOCP_WRITE_GLUCOSE_CALIBRATION_VALUE:
		if (req.operand_len != CGMS_CALIBRATION_VALUE_LEN) {
			(void)cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode,
						 SOCP_RSP_INVALID_OPERAND, NULL, 0U);
			break;
		}
		memcpy(m_calibration_value, req.p_operand, CGMS_CALIBRATION_VALUE_LEN);
		(void)cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, SOCP_RSP_SUCCESS,
						 NULL, 0U);
		break;
	case SOCP_READ_GLUCOSE_CALIBRATION_VALUE:
		(void)cgms_socp_send_response(SOCP_READ_GLUCOSE_CALIBRATION_VALUE_RESPONSE,
					 req.opcode, SOCP_RSP_SUCCESS,
					 m_calibration_value, CGMS_CALIBRATION_VALUE_LEN);
		break;
	case SOCP_WRITE_PATIENT_HIGH_ALERT_LEVEL:
	{
		uint16_t level;
		uint8_t status = cgms_socp_decode_u16(&req, &level);

		if (status == SOCP_RSP_SUCCESS) {
			m_alert_levels.patient_high = level;
		}
		(void)cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, status, NULL, 0U);
		break;
	}
	case SOCP_READ_PATIENT_HIGH_ALERT_LEVEL:
		(void)cgms_socp_send_u16_response(SOCP_READ_PATIENT_HIGH_ALERT_LEVEL_RESPONSE,
						 m_alert_levels.patient_high);
		break;
	case SOCP_WRITE_PATIENT_LOW_ALERT_LEVEL:
	{
		uint16_t level;
		uint8_t status = cgms_socp_decode_u16(&req, &level);

		if (status == SOCP_RSP_SUCCESS) {
			m_alert_levels.patient_low = level;
		}
		(void)cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, status, NULL, 0U);
		break;
	}
	case SOCP_READ_PATIENT_LOW_ALERT_LEVEL:
		(void)cgms_socp_send_u16_response(SOCP_READ_PATIENT_LOW_ALERT_LEVEL_RESPONSE,
						 m_alert_levels.patient_low);
		break;
	case SOCP_SET_HYPO_ALERT_LEVEL:
	{
		uint16_t level;
		uint8_t status = cgms_socp_decode_u16(&req, &level);

		if (status == SOCP_RSP_SUCCESS) {
			m_alert_levels.hypo = level;
		}
		(void)cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, status, NULL, 0U);
		break;
	}
	case SOCP_GET_HYPO_ALERT_LEVEL:
		(void)cgms_socp_send_u16_response(SOCP_HYPO_ALERT_LEVEL_RESPONSE,
						 m_alert_levels.hypo);
		break;
	case SOCP_SET_HYPER_ALERT_LEVEL:
	{
		uint16_t level;
		uint8_t status = cgms_socp_decode_u16(&req, &level);

		if (status == SOCP_RSP_SUCCESS) {
			m_alert_levels.hyper = level;
		}
		(void)cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, status, NULL, 0U);
		break;
	}
	case SOCP_GET_HYPER_ALERT_LEVEL:
		(void)cgms_socp_send_u16_response(SOCP_HYPER_ALERT_LEVEL_RESPONSE,
						 m_alert_levels.hyper);
		break;
	case SOCP_SET_RATE_OF_DECREASE_ALERT_LEVEL:
	{
		uint16_t level;
		uint8_t status = cgms_socp_decode_u16(&req, &level);

		if (status == SOCP_RSP_SUCCESS) {
			m_alert_levels.rate_decrease = level;
		}
		(void)cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, status, NULL, 0U);
		break;
	}
	case SOCP_GET_RATE_OF_DECREASE_ALERT_LEVEL:
		(void)cgms_socp_send_u16_response(SOCP_RATE_OF_DECREASE_ALERT_LEVEL_RESPONSE,
						 m_alert_levels.rate_decrease);
		break;
	case SOCP_SET_RATE_OF_INCREASE_ALERT_LEVEL:
	{
		uint16_t level;
		uint8_t status = cgms_socp_decode_u16(&req, &level);

		if (status == SOCP_RSP_SUCCESS) {
			m_alert_levels.rate_increase = level;
		}
		(void)cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, status, NULL, 0U);
		break;
	}
	case SOCP_GET_RATE_OF_INCREASE_ALERT_LEVEL:
		(void)cgms_socp_send_u16_response(SOCP_RATE_OF_INCREASE_ALERT_LEVEL_RESPONSE,
						 m_alert_levels.rate_increase);
		break;
	case SOCP_RESET_DEVICE_SPECIFIC_ALERT:
		m_status.annunciation.status &= (uint8_t)(~NRF_BLE_CGMS_STATUS_DEVICE_SPECIFIC_ALERT);
		(void)cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, SOCP_RSP_SUCCESS,
						 NULL, 0U);
		break;
	case SOCP_START_THE_SESSION:
		if (m_session_started) {
			(void)cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode,
						 SOCP_RSP_PROCEDURE_NOT_COMPLETED, NULL, 0U);
			break;
		}
		if ((m_nb_run_session != 0U) &&
		    !cgms_feature_present(NRF_BLE_CGMS_FEAT_MULTIPLE_SESSIONS_SUPPORTED)) {
			(void)cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode,
						 SOCP_RSP_PROCEDURE_NOT_COMPLETED, NULL, 0U);
			break;
		}
		cgms_start_session();
		(void)cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, SOCP_RSP_SUCCESS,
						 NULL, 0U);
		break;
	case SOCP_STOP_THE_SESSION:
		cgms_stop_session();
		(void)cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode, SOCP_RSP_SUCCESS,
						 NULL, 0U);
		break;
	default:
		(void)cgms_socp_send_response(SOCP_RESPONSE_CODE, req.opcode,
						 SOCP_RSP_OP_CODE_NOT_SUPPORTED, NULL, 0U);
		break;
	}

	return len;
}

static void cgms_glucose_work_handler(struct k_work *work)
{
	struct cgms_record rec;

	ARG_UNUSED(work);
	if (!m_session_started) {
		return;
	}

	m_current_offset += (m_comm_interval != 0U) ? m_comm_interval : GLUCOSE_MEAS_INTERVAL_MINUTES;
	memset(&rec, 0, sizeof(rec));
	rec.meas.glucose_concentration = m_glucose_concentration;
	rec.meas.time_offset = m_current_offset;
	rec.meas.flags = 0U;
	rec.meas.sensor_status_annunciation.warning = 0U;
	rec.meas.sensor_status_annunciation.calib_temp = 0U;
	rec.meas.sensor_status_annunciation.status = 0U;

	cgms_record_add(&rec);
	m_status.time_offset = m_current_offset;
	(void)cgms_measurement_notify(&rec);
	cgms_schedule_glucose_work();
}

BT_GATT_SERVICE_DEFINE(cgms_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_16(BT_UUID_CGM_VAL)),
	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(BT_UUID_CGM_MEASUREMENT_VAL),
		BT_GATT_CHRC_NOTIFY,
		BT_GATT_PERM_NONE,
		NULL, NULL, NULL),
	BT_GATT_CCC(cgms_meas_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(BT_UUID_CGM_FEATURE_VAL),
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		cgms_read_feature, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(BT_UUID_CGM_STATUS_VAL),
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		cgms_read_status, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(BT_UUID_CGM_SESSION_START_TIME_VAL),
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		cgms_read_sst, cgms_write_sst, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(BT_UUID_CGM_SESSION_RUN_TIME_VAL),
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		cgms_read_srt, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(BT_UUID_RECORD_ACCESS_CONTROL_POINT_VAL),
		BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,
		BT_GATT_PERM_WRITE,
		NULL, cgms_write_racp, NULL),
	BT_GATT_CCC(cgms_racp_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(BT_UUID_CGM_SPECIFIC_OPS_CTRL_PT_VAL),
		BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,
		BT_GATT_PERM_WRITE,
		NULL, cgms_write_socp, NULL),
	BT_GATT_CCC(cgms_socp_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
);

void ble_cgms_init(void)
{
	memset(m_records, 0, sizeof(m_records));
	m_record_count = 0U;
	m_conn = NULL;
	m_meas_notify_enabled = false;
	m_racp_ind_enabled = false;
	m_socp_ind_enabled = false;
	m_session_started = false;
	m_nb_run_session = 0U;
	m_current_offset = 0U;
	cgms_racp_reset_state();
	m_status.time_offset = 0U;
	m_status.annunciation.warning = 0U;
	m_status.annunciation.calib_temp = 0U;
	m_status.annunciation.status = NRF_BLE_CGMS_STATUS_SESSION_STOPPED;
	m_comm_interval = GLUCOSE_MEAS_INTERVAL_MINUTES;
	m_glucose_concentration = MIN_GLUCOSE_CONCENTRATION;
	memset(&m_alert_levels, 0, sizeof(m_alert_levels));
	m_calibration_value[0] = 0x3E;
	m_calibration_value[1] = 0x00;
	m_calibration_value[2] = 0x07;
	m_calibration_value[3] = 0x00;
	m_calibration_value[4] = 0x06;
	m_calibration_value[5] = 0x07;
	m_calibration_value[6] = 0x00;
	m_calibration_value[7] = 0x00;
	m_calibration_value[8] = 0x00;
	m_calibration_value[9] = 0x00;
	memset(&m_sst, 0, sizeof(m_sst));
	k_delayed_work_init(&m_glucose_work, cgms_glucose_work_handler);
	printk("CGMS service initialized\n");
}

void ble_cgms_register_evt_handler(ble_cgms_evt_handler_t handler)
{
	m_evt_handler = handler;
}

void ble_cgms_connected(struct bt_conn *conn)
{
	m_conn = conn;
}

void ble_cgms_disconnected(struct bt_conn *conn)
{
	if (m_conn == conn) {
		m_conn = NULL;
	}
	m_meas_notify_enabled = false;
	m_racp_ind_enabled = false;
	m_socp_ind_enabled = false;
	cgms_racp_reset_state();
	cgms_cancel_glucose_work();
	if (m_session_started) {
		cgms_schedule_glucose_work();
	}
}

void ble_cgms_increase_glucose(void)
{
	m_glucose_concentration += GL_CONCENTRATION_INC;
	if (m_glucose_concentration > MAX_GLUCOSE_CONCENTRATION) {
		m_glucose_concentration = MIN_GLUCOSE_CONCENTRATION;
	}
}

void ble_cgms_decrease_glucose(void)
{
	if (m_glucose_concentration <= (MIN_GLUCOSE_CONCENTRATION + GL_CONCENTRATION_DEC)) {
		m_glucose_concentration = MAX_GLUCOSE_CONCENTRATION;
	} else {
		m_glucose_concentration -= GL_CONCENTRATION_DEC;
	}
}
