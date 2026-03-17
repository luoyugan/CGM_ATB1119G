/*
 * CGMS demo service port for cgm_sdk ble_peripheral sample.
 */

#ifndef BLE_SUPER_SERVICE_H
#define BLE_SUPER_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr.h>
#include <bluetooth/conn.h>

#define BT_UUID_CGM_VAL                          0x181F
#define BT_UUID_CGM_MEASUREMENT_VAL              0x2AA7
#define BT_UUID_CGM_FEATURE_VAL                  0x2AA8
#define BT_UUID_CGM_STATUS_VAL                   0x2AA9
#define BT_UUID_CGM_SESSION_START_TIME_VAL       0x2AAA
#define BT_UUID_CGM_SESSION_RUN_TIME_VAL         0x2AAB
#define BT_UUID_CGM_SPECIFIC_OPS_CTRL_PT_VAL     0x2AAC
#define BT_UUID_RECORD_ACCESS_CONTROL_POINT_VAL  0x2A52

typedef enum {
	BLE_CGMS_EVT_NOTIFICATION_ENABLED = 0,
	BLE_CGMS_EVT_NOTIFICATION_DISABLED,
	BLE_CGMS_EVT_START_SESSION,
	BLE_CGMS_EVT_STOP_SESSION,
	BLE_CGMS_EVT_WRITE_COMM_INTERVAL,
} ble_cgms_evt_type_t;

typedef struct {
	ble_cgms_evt_type_t evt_type;
	uint8_t comm_interval;
} ble_cgms_evt_t;

typedef void (*ble_cgms_evt_handler_t)(const ble_cgms_evt_t *evt);

void ble_cgms_init(void);
void ble_cgms_register_evt_handler(ble_cgms_evt_handler_t handler);
void ble_cgms_connected(struct bt_conn *conn);
void ble_cgms_disconnected(struct bt_conn *conn);
void ble_cgms_increase_glucose(void);
void ble_cgms_decrease_glucose(void);

#ifdef __cplusplus
}
#endif

#endif /* BLE_SUPER_SERVICE_H */
