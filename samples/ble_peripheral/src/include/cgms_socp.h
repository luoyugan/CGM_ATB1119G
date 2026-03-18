#ifndef CGMS_SOCP_H
#define CGMS_SOCP_H

#include <stdint.h>
#include <sys/types.h>

#include <bluetooth/gatt.h>
#include "atb_ble_cgms.h"

#define SOCP_OPCODE_RESERVED                           0x00
#define SOCP_WRITE_CGM_COMMUNICATION_INTERVAL          0x01
#define SOCP_READ_CGM_COMMUNICATION_INTERVAL           0x02
#define SOCP_READ_CGM_COMM_INTERVAL_RSP                0x03
#define SOCP_WRITE_GLUCOSE_CALIBRATION_VALUE           0x04
#define SOCP_READ_GLUCOSE_CALIBRATION_VALUE            0x05
#define SOCP_READ_GLUCOSE_CALIBRATION_VALUE_RESPONSE   0x06
#define SOCP_WRITE_PATIENT_HIGH_ALERT_LEVEL            0x07
#define SOCP_READ_PATIENT_HIGH_ALERT_LEVEL             0x08
#define SOCP_READ_PATIENT_HIGH_ALERT_LEVEL_RESPONSE    0x09
#define SOCP_WRITE_PATIENT_LOW_ALERT_LEVEL             0x0A
#define SOCP_READ_PATIENT_LOW_ALERT_LEVEL              0x0B
#define SOCP_READ_PATIENT_LOW_ALERT_LEVEL_RESPONSE     0x0C
#define SOCP_SET_HYPO_ALERT_LEVEL                      0x0D
#define SOCP_GET_HYPO_ALERT_LEVEL                      0x0E
#define SOCP_HYPO_ALERT_LEVEL_RESPONSE                 0x0F
#define SOCP_SET_HYPER_ALERT_LEVEL                     0x10
#define SOCP_GET_HYPER_ALERT_LEVEL                     0x11
#define SOCP_HYPER_ALERT_LEVEL_RESPONSE                0x12
#define SOCP_SET_RATE_OF_DECREASE_ALERT_LEVEL          0x13
#define SOCP_GET_RATE_OF_DECREASE_ALERT_LEVEL          0x14
#define SOCP_RATE_OF_DECREASE_ALERT_LEVEL_RESPONSE     0x15
#define SOCP_SET_RATE_OF_INCREASE_ALERT_LEVEL          0x16
#define SOCP_GET_RATE_OF_INCREASE_ALERT_LEVEL          0x17
#define SOCP_RATE_OF_INCREASE_ALERT_LEVEL_RESPONSE     0x18
#define SOCP_RESET_DEVICE_SPECIFIC_ALERT               0x19

#define SOCP_START_THE_SESSION                         0x1A
#define SOCP_STOP_THE_SESSION                          0x1B
#define SOCP_RESPONSE_CODE                             0x1C

#define SOCP_RSP_RESERVED_FOR_FUTURE_USE               0x00
#define SOCP_RSP_SUCCESS                               0x01
#define SOCP_RSP_OP_CODE_NOT_SUPPORTED                 0x02
#define SOCP_RSP_INVALID_OPERAND                       0x03
#define SOCP_RSP_PROCEDURE_NOT_COMPLETED               0x04
#define SOCP_RSP_OUT_OF_RANGE                          0x05

typedef struct {
	uint8_t opcode;
	uint8_t operand_len;
	uint8_t *p_operand;
} ble_cgms_socp_value_t;

void cgms_socp_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);
ssize_t cgms_write_socp(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	const void *buf, uint16_t len, uint16_t offset, uint8_t flags);

#endif