/*
 * CGMS service public API.
 */

#ifndef ATB_BLE_CGMS_H
#define ATB_BLE_CGMS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <zephyr.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>

#define BT_UUID_CGM_VAL                          0x181F
#define BT_UUID_CGM_MEASUREMENT_VAL              0x2AA7
#define BT_UUID_CGM_FEATURE_VAL                  0x2AA8
#define BT_UUID_CGM_STATUS_VAL                   0x2AA9
#define BT_UUID_CGM_SESSION_START_TIME_VAL       0x2AAA
#define BT_UUID_CGM_SESSION_RUN_TIME_VAL         0x2AAB
#define BT_UUID_CGM_SPECIFIC_OPS_CTRL_PT_VAL     0x2AAC
#define BT_UUID_RECORD_ACCESS_CONTROL_POINT_VAL  0x2A52

#ifndef BLE_GATT_ATT_MTU_DEFAULT
#if defined(BT_ATT_DEFAULT_LE_MTU)
#define BLE_GATT_ATT_MTU_DEFAULT BT_ATT_DEFAULT_LE_MTU
#elif defined(BT_GATT_ATT_MTU_DEFAULT)
#define BLE_GATT_ATT_MTU_DEFAULT BT_GATT_ATT_MTU_DEFAULT
#else
#define BLE_GATT_ATT_MTU_DEFAULT 23
#endif
#endif
// 上面的待定

#define CGMS_CALIBRATION_VALUE_LEN                  10
#define GLUCOSE_MEAS_INTERVAL_MINUTES               1
#define GL_CONCENTRATION_INC                        10
#define GL_CONCENTRATION_DEC                        5
#define MAX_GLUCOSE_CONCENTRATION                   800
#define MIN_GLUCOSE_CONCENTRATION                   5
#define SOCP_COMM_INTERVAL_USE_DEFAULT              0xFF

#ifndef ENOTCONN
#define ENOTCONN 57
#endif

//nordic里没看到
#define NRF_BLE_CGMS_PLUS_INFINITE                      0x07FE
#define NRF_BLE_CGMS_MINUS_INFINITE                     0x0802

/**@name CGM Feature characteristic defines
 * @{ */
#define NRF_BLE_CGMS_FEAT_CALIBRATION_SUPPORTED                           (0x01 << 0)  //!< Calibration supported.
#define NRF_BLE_CGMS_FEAT_PATIENT_HIGH_LOW_ALERTS_SUPPORTED               (0x01 << 1)  //!< Patient High/Low Alerts supported.
#define NRF_BLE_CGMS_FEAT_HYPO_ALERTS_SUPPORTED                           (0x01 << 2)  //!< Hypo Alerts supported.
#define NRF_BLE_CGMS_FEAT_HYPER_ALERTS_SUPPORTED                          (0x01 << 3)  //!< Hyper Alerts supported.
#define NRF_BLE_CGMS_FEAT_RATE_OF_INCREASE_DECREASE_ALERTS_SUPPORTED      (0x01 << 4)  //!< Rate of Increase/Decrease Alerts supported.
#define NRF_BLE_CGMS_FEAT_DEVICE_SPECIFIC_ALERT_SUPPORTED                 (0x01 << 5)  //!< Device Specific Alert supported.
#define NRF_BLE_CGMS_FEAT_SENSOR_MALFUNCTION_DETECTION_SUPPORTED          (0x01 << 6)  //!< Sensor Malfunction Detection supported.
#define NRF_BLE_CGMS_FEAT_SENSOR_TEMPERATURE_HIGH_LOW_DETECTION_SUPPORTED (0x01 << 7)  //!< Sensor Temperature High-Low Detection supported.
#define NRF_BLE_CGMS_FEAT_SENSOR_RESULT_HIGH_LOW_DETECTION_SUPPORTED      (0x01 << 8)  //!< Sensor Result High-Low Detection supported.
#define NRF_BLE_CGMS_FEAT_LOW_BATTERY_DETECTION_SUPPORTED                 (0x01 << 9)  //!< Low Battery Detection supported.
#define NRF_BLE_CGMS_FEAT_SENSOR_TYPE_ERROR_DETECTION_SUPPORTED           (0x01 << 10) //!< Sensor Type Error Detection supported.
#define NRF_BLE_CGMS_FEAT_GENERAL_DEVICE_FAULT_SUPPORTED                  (0x01 << 11) //!< General Device Fault supported.
#define NRF_BLE_CGMS_FEAT_E2E_CRC_SUPPORTED                               (0x01 << 12) //!< E2E-CRC supported.
#define NRF_BLE_CGMS_FEAT_MULTIPLE_BOND_SUPPORTED                         (0x01 << 13) //!< Multiple Bond supported.
#define NRF_BLE_CGMS_FEAT_MULTIPLE_SESSIONS_SUPPORTED                     (0x01 << 14) //!< Multiple Sessions supported.
#define NRF_BLE_CGMS_FEAT_CGM_TREND_INFORMATION_SUPPORTED                 (0x01 << 15) //!< CGM Trend Information supported.
#define NRF_BLE_CGMS_FEAT_CGM_QUALITY_SUPPORTED                           (0x01 << 16) //!< CGM Quality supported.
/** @} */

/**@name Continuous Glucose Monitoring type
 * @{ */
#define NRF_BLE_CGMS_MEAS_TYPE_CAP_BLOOD     0x01  //!< Capillary Whole blood.
#define NRF_BLE_CGMS_MEAS_TYPE_CAP_PLASMA    0x02  //!< Capillary Plasma.
#define NRF_BLE_CGMS_MEAS_TYPE_VEN_BLOOD     0x03  //!< Venous Whole blood.
#define NRF_BLE_CGMS_MEAS_TYPE_VEN_PLASMA    0x04  //!< Venous Plasma.
#define NRF_BLE_CGMS_MEAS_TYPE_ART_BLOOD     0x05  //!< Arterial Whole blood.
#define NRF_BLE_CGMS_MEAS_TYPE_ART_PLASMA    0x06  //!< Arterial Plasma.
#define NRF_BLE_CGMS_MEAS_TYPE_UNDET_BLOOD   0x07  //!< Undetermined Whole blood.
#define NRF_BLE_CGMS_MEAS_TYPE_UNDET_PLASMA  0x08  //!< Undetermined Plasma.
#define NRF_BLE_CGMS_MEAS_TYPE_FLUID         0x09  //!< Interstitial Fluid (ISF).
#define NRF_BLE_CGMS_MEAS_TYPE_CONTROL       0x0A  //!< Control Solution.
/** @} */

/**@name CGM sample location
 * @{ */
#define NRF_BLE_CGMS_MEAS_LOC_FINGER         0x01  //!< Finger.
#define NRF_BLE_CGMS_MEAS_LOC_AST            0x02  //!< Alternate Site Test (AST).
#define NRF_BLE_CGMS_MEAS_LOC_EAR            0x03  //!< Earlobe.
#define NRF_BLE_CGMS_MEAS_LOC_CONTROL        0x04  //!< Control solution.
#define NRF_BLE_CGMS_MEAS_LOC_SUB_TISSUE     0x05  //!< Subcutaneous tissue.
#define NRF_BLE_CGMS_MEAS_LOC_NOT_AVAIL      0x0F  //!< Sample Location value not available.
/** @} */

/**@name CGM Measurement Sensor Status Annunciation
 * @{ */
#define NRF_BLE_CGMS_STATUS_SESSION_STOPPED                  (0x01 << 0) //!< Status: Session Stopped.
#define NRF_BLE_CGMS_STATUS_DEVICE_BATTERY_LOW               (0x01 << 1) //!< Status: Device Battery Low.
#define NRF_BLE_CGMS_STATUS_SENSOR_TYPE_INCORRECT_FOR_DEVICE (0x01 << 2) //!< Status: Sensor type incorrect for device.
#define NRF_BLE_CGMS_STATUS_SENSOR_MALFUNCTION               (0x01 << 3) //!< Status: Sensor malfunction.
#define NRF_BLE_CGMS_STATUS_DEVICE_SPECIFIC_ALERT            (0x01 << 4) //!< Status: Device Specific Alert.
#define NRF_BLE_CGMS_STATUS_GENERAL_DEVICE_FAULT             (0x01 << 5) //!< Status: General device fault has occurred in the sensor.
/** @} */

/**@name CGM Measurement flags
 * @{ */
#define NRF_BLE_CGMS_FLAG_TREND_INFO_PRESENT                 0x01        //!< CGM Trend Information Present.
#define NRF_BLE_CGMS_FLAGS_QUALITY_PRESENT                   0x02        //!< CGM Quality Present.
#define NRF_BLE_CGMS_STATUS_FLAGS_WARNING_OCT_PRESENT        0x20        //!< Sensor Status Annunciation Field, Warning-Octet present.
#define NRF_BLE_CGMS_STATUS_FLAGS_CALTEMP_OCT_PRESENT        0x40        //!< Sensor Status Annunciation Field, Cal/Temp-Octet present.
#define NRF_BLE_CGMS_STATUS_FLAGS_STATUS_OCT_PRESENT         0x80        //!< Sensor Status Annunciation Field, Status-Octet present.
/** @} */

/**@name Byte length of various commands (used for validating, encoding, and decoding data).
 * @{ */
#define NRF_BLE_CGMS_MEAS_OP_LEN            1
#define NRF_BLE_CGMS_MEAS_HANDLE_LEN        2
#define NRF_BLE_CGMS_MEAS_LEN_MAX           (BLE_GATT_ATT_MTU_DEFAULT - \
											 NRF_BLE_CGMS_MEAS_OP_LEN - \
											 NRF_BLE_CGMS_MEAS_HANDLE_LEN)

#define NRF_BLE_CGMS_MEAS_REC_LEN_MAX       15
#define NRF_BLE_CGMS_MEAS_REC_LEN_MIN       6
#define NRF_BLE_CGMS_MEAS_REC_PER_NOTIF_MAX (NRF_BLE_CGMS_MEAS_LEN_MAX / \
											 NRF_BLE_CGMS_MEAS_REC_LEN_MIN)

#define NRF_BLE_CGMS_SOCP_RESP_CODE_LEN     2
#define NRF_BLE_CGMS_FEATURE_LEN            6
#define NRF_BLE_CGMS_STATUS_LEN             7
#define NRF_BLE_CGMS_MAX_CALIB_LEN          10
#define NRF_BLE_CGMS_CALIBS_NB_MAX          5
#define NRF_BLE_CGMS_SST_LEN                9
#define NRF_BLE_CGMS_CRC_LEN                2
#define NRF_BLE_CGMS_SRT_LEN                2

#define NRF_BLE_CGMS_SOCP_RESP_LEN          (NRF_BLE_CGMS_MEAS_LEN_MAX - \
											NRF_BLE_CGMS_SOCP_RESP_CODE_LEN)

#define NRF_BLE_CGMS_RACP_PENDING_OPERANDS_MAX 2
/** @} */

typedef enum {
	BLE_CGMS_EVT_NOTIFICATION_ENABLED = 0,
	BLE_CGMS_EVT_NOTIFICATION_DISABLED,
	BLE_CGMS_EVT_START_SESSION,
	BLE_CGMS_EVT_STOP_SESSION,
	BLE_CGMS_EVT_WRITE_COMM_INTERVAL,
} nrf_ble_cgms_evt_type_t;

typedef nrf_ble_cgms_evt_type_t ble_cgms_evt_type_t;

/**
 * @defgroup nrf_ble_cgms_structs Structures
 * @{
 */

/**@brief CGM Service event. */
typedef struct
{
    nrf_ble_cgms_evt_type_t evt_type; /**< Type of event. */
} nrf_ble_cgms_evt_t;

typedef struct {
    ble_cgms_evt_type_t evt_type;
    uint8_t comm_interval;
} ble_cgms_evt_t;

/** @} */ // End tag for Structure group.

/**
 * @defgroup nrf_ble_cgms_types Types
 * @{
 */

/**@brief Forward declaration of the nrf_ble_cgms_t type. */
typedef struct ble_cgms_s nrf_ble_cgms_t;

/**@brief CGM Service event handler type. */
typedef void (*ble_cgms_evt_handler_t)(nrf_ble_cgms_t * p_cgms, nrf_ble_cgms_evt_t * p_evt);

/**
 * @addtogroup nrf_ble_cgms_structs
 * @{
 */

/**@brief CGM Measurement Sensor Status Annunciation. */
typedef struct
{
    uint8_t warning;               /**< Warning annunciation. */
    uint8_t calib_temp;            /**< Calibration and Temperature annunciation. */
    uint8_t status;                /**< Status annunciation. */
} nrf_ble_cgms_sensor_annunc_t;

/**@brief CGM measurement. */
typedef struct
{
    uint8_t                      flags;                      /**< Indicates the presence of optional fields and the Sensor Status Annunciation field. */
    uint16_t                     glucose_concentration;      /**< Glucose concentration. 16-bit word comprising 4-bit exponent and signed 12-bit mantissa. */
    uint16_t                     time_offset;                /**< Time offset. Represents the time difference between measurements. */
    nrf_ble_cgms_sensor_annunc_t sensor_status_annunciation; /**< Sensor Status Annunciation. Variable length, can include Status, Cal/Temp, and Warning. */
    uint16_t                     trend;                      /**< Optional field that can include Trend Information. */
    uint16_t                     quality;                    /**< Optional field that includes the Quality of the measurement. */
} nrf_ble_cgms_meas_t;

/**@brief CGM Measurement record. */
typedef struct
{
    nrf_ble_cgms_meas_t meas; /**< CGM measurement. */
} ble_cgms_rec_t;

struct cgms_measurement {
    uint8_t flags;
    uint16_t glucose_concentration;
    uint16_t time_offset;
    nrf_ble_cgms_sensor_annunc_t sensor_status_annunciation;
    uint16_t trend;
    uint16_t quality;
};

/**@brief Features supported by the CGM Service. */
typedef struct
{
    uint32_t feature;         /**< Information on supported features in the CGM Service. */
    uint8_t  type;            /**< Type. */
    uint8_t  sample_location; /**< Sample location. */
}nrf_ble_cgms_feature_t;

struct cgms_feature_value {
	uint32_t feature;
	uint8_t type;
	uint8_t sample_location;
};

/**@brief Status of the CGM measurement. */
typedef struct
{
    uint16_t                     time_offset; /**< Time offset. */
    nrf_ble_cgms_sensor_annunc_t status;      /**< Status. */
} nrf_ble_cgm_status_t;


typedef struct {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
} ble_date_time_t;

// 存储在CGMS实例中会更好，目前没有接口，先用全局变量
typedef struct {
    ble_date_time_t date_time;
    uint8_t time_zone;
    uint8_t dst;
} ble_cgms_sst_t;

typedef struct
{
    ble_cgms_evt_handler_t    evt_handler;           /**< Event handler to be called for handling events in the CGM Service. */
    // ble_srv_error_handler_t   error_handler;         /**< Function to be called when an error occurs. */
    // nrf_ble_gq_t            * p_gatt_queue;          /**< Pointer to BLE GATT Queue instance. */
    nrf_ble_cgms_feature_t    feature;               /**< Features supported by the service. */
    nrf_ble_cgm_status_t      initial_sensor_status; /**< Sensor status. */
    uint16_t                  initial_run_time;      /**< Run time. */
} nrf_ble_cgms_init_t;

typedef struct
{
	uint8_t opcode;
	uint8_t req_opcode;
	uint8_t rsp_code;
	uint8_t resp_val[NRF_BLE_CGMS_SOCP_RESP_LEN];
	uint8_t size_val;
} ble_socp_rsp_t;

/**@brief Calibration value. */
typedef struct
{
    uint8_t value[NRF_BLE_CGMS_MAX_CALIB_LEN]; /**< Array containing the calibration value. */
} nrf_ble_cgms_calib_t;

typedef struct {
	uint8_t opcode;
	uint8_t operator;
	uint8_t operand_len;
	uint8_t *p_operand;
} ble_racp_value_t;

/**@brief Record Access Control Point transaction data. */
typedef struct
{
    uint8_t          racp_proc_operator;                                                    /**< Operator of the current request. */
    uint16_t         racp_proc_record_ndx;                                                  /**< Current record index. */
    uint16_t         racp_proc_records_ndx_last_to_send;                                    /**< The last record to send, can be used together with racp_proc_record_ndx to determine a range of records to send. (used by greater/less filters). */
    uint16_t         racp_proc_records_reported;                                            /**< Number of reported records. */
    ble_racp_value_t racp_request;                                                      /**< Compatibility placeholder. */
    ble_racp_value_t pending_racp_response;                                                /**< Compatibility placeholder. */
    bool             racp_procesing_active;                                                 /**< RACP processing active. */
    uint8_t          pending_racp_response_operand[NRF_BLE_CGMS_RACP_PENDING_OPERANDS_MAX]; /**< Operand of the RACP response to be sent. */
} nrf_ble_cgms_racp_t;


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

/**@brief Status information for the CGM Service. */
struct ble_cgms_s
{
    ble_cgms_evt_handler_t      evt_handler;                                 /**< Event handler to be called for handling events in the CGM Service. */
    // ble_srv_error_handler_t     error_handler;                               /**< Function to be called if an error occurs. */
    // nrf_ble_gq_t              * p_gatt_queue;                                /**< Pointer to BLE GATT Queue instance. */
    // nrf_ble_gq_req_error_cb_t   gatt_err_handler;                            /**< Error handler to be called in case of an error from SoftDevice. */
    uint16_t                    service_handle;                              /**< Handle of the CGM Service (as provided by the BLE stack). */
    // nrf_ble_cgms_char_handler_t char_handles;                                /**< GATTS characteristic handles for the different characteristics in the service. */
    uint16_t                    conn_handle;                                 /**< Handle of the current connection (as provided by the BLE stack; @ref BLE_CONN_HANDLE_INVALID if not in a connection). */
    struct bt_conn            * m_conn;                                      /**< Zephyr connection reference used by CGMS data path. */
    nrf_ble_cgms_feature_t      feature;                                     /**< Structure to store the value of the feature characteristic. */
    uint8_t                     comm_interval;                               /**< Variable to keep track of the communication interval. */
    ble_socp_rsp_t              socp_response;                               /**< Structure containing reponse data to be indicated to the peer device. */
    nrf_ble_cgms_calib_t        calibration_val[NRF_BLE_CGMS_CALIBS_NB_MAX]; /**< Calibration value. Can be read from and written to SOCP. */
    bool                        is_session_started;                          /**< Indicator if we are currently in a session. */
    uint8_t                     nb_run_session;                              /**< Variable to keep track of the number of sessions that were run. */
    uint16_t                    session_run_time;                            /**< Variable to store the expected run time of a session. */
    nrf_ble_cgm_status_t        sensor_status;                              /**< Structure to keep track of the sensor status. */
    nrf_ble_cgms_racp_t         racp_data;                                   /**< Structure to manage Record Access requests. */
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

#define NRF_BLE_CGMS_DEF(_name)                                                                     \
static nrf_ble_cgms_t _name; 

extern const struct bt_gatt_attr attr_cgms_svc[];

extern ble_cgms_evt_handler_t m_evt_handler;
extern struct k_delayed_work m_glucose_work;
// extern struct cgms_record m_records[];
extern uint16_t m_record_count;
extern struct cgms_feature_value m_feature;
extern nrf_ble_cgm_status_t m_status;
extern ble_cgms_sst_t m_sst;
extern uint16_t m_current_offset;
extern uint16_t m_glucose_concentration;
extern bool m_meas_notify_enabled;
extern bool m_racp_ind_enabled;
extern bool m_socp_ind_enabled;
extern struct bt_gatt_indicate_params m_racp_ind_params;
extern struct bt_gatt_indicate_params m_socp_ind_params;
extern uint8_t m_racp_ind_buf[8];
extern uint8_t m_socp_ind_buf[20];
extern struct cgms_racp_state m_racp;
extern struct cgms_alert_levels m_alert_levels;
extern uint8_t m_calibration_value[CGMS_CALIBRATION_VALUE_LEN];

void cgms_emit_event(ble_cgms_evt_type_t evt_type);

uint32_t ble_cgms_init(nrf_ble_cgms_t * p_cgms, const nrf_ble_cgms_init_t * p_cgms_init);
uint32_t nrf_ble_cgms_update_status(nrf_ble_cgms_t * p_cgms, const nrf_ble_cgm_status_t * p_status);
void ble_cgms_register_evt_handler(ble_cgms_evt_handler_t handler);
void ble_cgms_connected(struct bt_conn *conn);
void ble_cgms_disconnected(nrf_ble_cgms_t * p_cgms, struct bt_conn *conn);
nrf_ble_cgms_t * ble_cgms_instance_get(void);
void ble_cgms_increase_glucose(void);
void ble_cgms_decrease_glucose(void);
uint16_t uint16_decode(const uint8_t * p_encoded_data);
uint8_t uint16_encode(uint16_t value, uint8_t * p_encoded_data);

#ifdef __cplusplus
}
#endif

#endif