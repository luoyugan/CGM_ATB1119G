/* SPDX-License-Identifier: Apache-2.0 */
/*
 * cgms_defs.h — 全局常量宏定义（UUID / OpCode / 响应码 / Flags）
 *
 * 本文件只放 #define，不依赖任何运行时头文件，可被所有特征实现文件引用。
 */

#ifndef CGMS_DEFS_H
#define CGMS_DEFS_H

#include <sys/util.h>   /* BIT() */

/* ========== CGMS 特征 UUID 值 ========== */
#define CGM_CGMS_SERVICE_UUID_VAL              0x181F
#define CGM_CGMS_MEAS_UUID_VAL                 0x2AA7
#define CGM_CGMS_FEATURE_UUID_VAL              0x2AA8
#define CGM_CGMS_STATUS_UUID_VAL               0x2AA9
#define CGM_CGMS_SESSION_START_UUID_VAL        0x2AAA
#define CGM_CGMS_SESSION_RUN_TIME_UUID_VAL     0x2AAB
#define CGM_CGMS_SOCP_UUID_VAL                 0x2AAC
#define CGM_CGMS_RACP_UUID_VAL                 0x2A52

/* ========== SOCP 操作码（CGMS v1.0.1 §3.7） ========== */
#define CGM_SOCP_OP_SET_COMM_INTERVAL          0x01
#define CGM_SOCP_OP_GET_COMM_INTERVAL          0x02
#define CGM_SOCP_OP_COMM_INTERVAL_RSP          0x03
#define CGM_SOCP_OP_START_SESSION              0x1A
#define CGM_SOCP_OP_STOP_SESSION               0x1B
#define CGM_SOCP_OP_RESPONSE_CODE              0x1C

/* ========== SOCP 响应码 ========== */
#define CGM_SOCP_RSP_SUCCESS                   0x01
#define CGM_SOCP_RSP_OP_CODE_NOT_SUPPORTED     0x02
#define CGM_SOCP_RSP_INVALID_OPERAND           0x03
#define CGM_SOCP_RSP_PROCEDURE_NOT_COMPLETED   0x04
#define CGM_SOCP_RSP_PARAMETER_OUT_OF_RANGE    0x05

/* ========== RACP 操作码（Generic RACP + CGMS v1.0.1 §3.6） ========== */
#define CGM_RACP_OP_REPORT_STORED_RECORDS      0x01
#define CGM_RACP_OP_DELETE_STORED_RECORDS      0x02
#define CGM_RACP_OP_ABORT_OPERATION            0x03
#define CGM_RACP_OP_REPORT_NUM_STORED_RECORDS  0x04
#define CGM_RACP_OP_NUM_STORED_RECORDS_RSP     0x05
#define CGM_RACP_OP_RESPONSE_CODE              0x06

/* ========== RACP 算子 ========== */
#define CGM_RACP_OPERATOR_NULL                 0x00
#define CGM_RACP_OPERATOR_ALL_RECORDS          0x01
#define CGM_RACP_OPERATOR_LT_EQ               0x02
#define CGM_RACP_OPERATOR_GT_EQ               0x03
#define CGM_RACP_OPERATOR_WITHIN_RANGE         0x04
#define CGM_RACP_OPERATOR_FIRST                0x05
#define CGM_RACP_OPERATOR_LAST                 0x06

/* ========== RACP 响应码 ========== */
#define CGM_RACP_RSP_SUCCESS                   0x01
#define CGM_RACP_RSP_OP_NOT_SUPPORTED          0x02
#define CGM_RACP_RSP_INVALID_OPERATOR          0x03
#define CGM_RACP_RSP_OPERATOR_NOT_SUPPORTED    0x04
#define CGM_RACP_RSP_INVALID_OPERAND           0x05
#define CGM_RACP_RSP_NO_RECORDS_FOUND          0x06
#define CGM_RACP_RSP_ABORT_UNSUCCESSFUL        0x07
#define CGM_RACP_RSP_PROCEDURE_NOT_COMPLETED   0x08
#define CGM_RACP_RSP_OPERAND_NOT_SUPPORTED     0x09

/* RACP 滤波类型（CGM 服务仅允许 Time Offset 0x01） */
#define CGM_RACP_FILTER_TYPE_TIME_OFFSET       0x01

/* ATT 应用层错误码（用于 RACP/SOCP 流程保障） */
#define CGM_ATT_ERR_PROCEDURE_ALREADY_IN_PROGRESS  0x80

/* ========== CGM Measurement Flags（CGMS v1.0.1 §3.1.1.2） ========== */
#define CGM_MEAS_FLAG_TREND_PRESENT            BIT(0)
#define CGM_MEAS_FLAG_QUALITY_PRESENT          BIT(1)
#define CGM_MEAS_FLAG_STATUS_OCTET_PRESENT     BIT(5)
#define CGM_MEAS_FLAG_CAL_TEMP_OCTET_PRESENT   BIT(6)
#define CGM_MEAS_FLAG_WARNING_OCTET_PRESENT    BIT(7)

/* ========== 测量记录数据库容量 ========== */
#define CGM_DB_MAX_RECORDS                     96U

#endif /* CGMS_DEFS_H */
