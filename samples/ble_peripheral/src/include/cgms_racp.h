#ifndef CGMS_RACP_H
#define CGMS_RACP_H

#include <stdint.h>
#include <sys/types.h>

#include <bluetooth/gatt.h>
#include "atb_ble_cgms.h"

#define OPERAND_LESS_GREATER_FILTER_TYPE_SIZE       1
#define OPERAND_LESS_GREATER_FILTER_PARAM_SIZE      2
#define OPERAND_LESS_GREATER_SIZE (OPERAND_LESS_GREATER_FILTER_TYPE_SIZE + OPERAND_LESS_GREATER_FILTER_PARAM_SIZE)

#define RACP_OPCODE_RESERVED                        0x00
#define RACP_OPCODE_REPORT_RECS                     0x01
#define RACP_OPCODE_DELETE_RECS                     0x02
#define RACP_OPCODE_ABORT_OPERATION                 0x03
#define RACP_OPCODE_REPORT_NUM_RECS                 0x04
#define RACP_OPCODE_NUM_RECS_RESPONSE               0x05
#define RACP_OPCODE_RESPONSE_CODE                   0x06

#define RACP_OPERATOR_NULL                          0x00
#define RACP_OPERATOR_ALL                           0x01
#define RACP_OPERATOR_LESS_OR_EQUAL                 0x02
#define RACP_OPERATOR_GREATER_OR_EQUAL              0x03
#define RACP_OPERATOR_RANGE                         0x04
#define RACP_OPERATOR_FIRST                         0x05
#define RACP_OPERATOR_LAST                          0x06
#define RACP_OPERATOR_RFU_START                     0x07

#define RACP_OPERAND_FILTER_TYPE_TIME_OFFSET        0x01
#define RACP_OPERAND_FILTER_TYPE_FACING_TIME        0x02

#define RACP_RESPONSE_RESERVED                      0x00
#define RACP_RESPONSE_SUCCESS                       0x01
#define RACP_RESPONSE_OPCODE_UNSUPPORTED            0x02
#define RACP_RESPONSE_INVALID_OPERATOR              0x03
#define RACP_RESPONSE_OPERATOR_UNSUPPORTED          0x04
#define RACP_RESPONSE_INVALID_OPERAND               0x05
#define RACP_RESPONSE_NO_RECORDS_FOUND              0x06
#define RACP_RESPONSE_ABORT_FAILED                  0x07
#define RACP_RESPONSE_PROCEDURE_NOT_DONE            0x08
#define RACP_RESPONSE_OPERAND_UNSUPPORTED           0x09

void cgms_racp_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);
void cgms_racp_on_meas_tx_complete(struct bt_conn *conn, void *user_data);
ssize_t cgms_write_racp(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	const void *buf, uint16_t len, uint16_t offset, uint8_t flags);

#endif