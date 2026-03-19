#include <errno.h>
#include <string.h>

#include "cgms_racp.h"
#include "cgms_db.h"
#include "cgms_meas.h"
#include "atb_ble_cgms.h"

uint8_t ble_racp_encode(const ble_racp_value_t * p_racp_val, uint8_t * p_data)
{
    uint8_t len = 0;
    int     i;

    if (p_data != NULL)
    {
        p_data[len++] = p_racp_val->opcode;
        p_data[len++] = p_racp_val->operator;

        for (i = 0; i < p_racp_val->operand_len; i++)
        {
            p_data[len++] = p_racp_val->p_operand[i];
        }
    }

    return len;
}
void ble_racp_decode(uint8_t data_len, uint8_t const * p_data, ble_racp_value_t * p_racp_val)
{
    p_racp_val->opcode      = 0xFF;
    p_racp_val->operator    = 0xFF;
    p_racp_val->operand_len = 0;
    p_racp_val->p_operand   = NULL;

    if (data_len > 0)
    {
        p_racp_val->opcode = p_data[0];
    }
    if (data_len > 1)
    {
        p_racp_val->operator = p_data[1];      //lint !e415
    }
    if (data_len > 2)
    {
        p_racp_val->operand_len = data_len - 2;
        p_racp_val->p_operand   = (uint8_t*)&p_data[2];  //lint !e416
    }
}


void cgms_racp_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);
	m_racp_ind_enabled = (value == BT_GATT_CCC_INDICATE);
}

static void cgms_racp_ind_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params, uint8_t err)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(params);
	ARG_UNUSED(err);
}

static int cgms_racp_indicate(nrf_ble_cgms_t * p_cgms, ble_racp_value_t * p_racp_val)
{
	// 目前保留
	if ((m_conn == NULL) || !m_racp_ind_enabled) {
		return -ENOTCONN;
	}

	uint8_t          encoded_resp[25];
    uint16_t         len;

	len = ble_racp_encode(p_racp_val, encoded_resp);

	memset(&m_racp_ind_params, 0, sizeof(m_racp_ind_params));
	m_racp_ind_params.attr = &attr_cgms_svc[CGMS_ATTR_RACP_VAL];
	m_racp_ind_params.data = encoded_resp;
	m_racp_ind_params.len = len;
	m_racp_ind_params.func = cgms_racp_ind_cb; //nordic用的是p_cgms->gatt_err_handler
	return bt_gatt_indicate(m_conn, &m_racp_ind_params);
}

static void cgms_send_racp_response_code(nrf_ble_cgms_t * p_cgms,uint8_t req_opcode, uint8_t rsp_code)
{
	// uint8_t buf[8];
	// uint8_t operand[2];
	// uint8_t len;
	// ble_racp_value_t rsp;

	// operand[0] = req_opcode;
	// operand[1] = rsp_code;
	
	// rsp.opcode = RACP_OPCODE_RESPONSE_CODE;
	// rsp.operator = RACP_OPERATOR_NULL;
	// rsp.operand_len = sizeof(operand);
	// rsp.p_operand = operand;

	// len = ble_racp_encode(buf, sizeof(buf), &rsp);
	// if (len == 0U) {
	// 	return;
	// }
	p_cgms->racp_data.pending_racp_response.opcode      = RACP_OPCODE_RESPONSE_CODE;
    p_cgms->racp_data.pending_racp_response.operator    = RACP_OPERATOR_NULL;
    p_cgms->racp_data.pending_racp_response.operand_len = 2;
    p_cgms->racp_data.pending_racp_response.p_operand   =
        p_cgms->racp_data.pending_racp_response_operand;

    p_cgms->racp_data.pending_racp_response_operand[0] = req_opcode;
    p_cgms->racp_data.pending_racp_response_operand[1] = rsp_code;

	(void)cgms_racp_indicate(p_cgms, &p_cgms->racp_data.pending_racp_response);
}

/**@brief Function for responding to the ALL operation.
 *
 * @param[in]   p_cgms   Service instance.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
static uint32_t racp_report_records_all(nrf_ble_cgms_t * p_cgms)
{
	return 0;
}

/**@brief Function for responding to the FIRST or the LAST operation.
 *
 * @param[in]   p_cgms   Service instance.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
static uint32_t racp_report_records_first_last(nrf_ble_cgms_t * p_cgms)
{
	return 0;
}

/**@brief Function for responding to the LESS OR EQUAL operation.
 *
 * @param[in]   p_cgms   Service instance.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
static uint32_t racp_report_records_less_equal(nrf_ble_cgms_t * p_cgms)
{
    return 0;
}

/**@brief Function for responding to the GREATER OR EQUAL operation.
 *
 * @param[in]   p_cgms   Service instance.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
static uint32_t racp_report_records_greater_equal(nrf_ble_cgms_t * p_cgms)
{
    return 0;
}

/**@brief Function for informing that the REPORT RECORDS procedure is completed.
 *
 * @param[in]   p_cgms   Service instance.
 */
static void racp_report_records_completed(nrf_ble_cgms_t * p_cgms)
{
    uint8_t resp_code_value;

    if (p_cgms->racp_data.racp_proc_records_reported > 0)
    {
        resp_code_value = RACP_RESPONSE_SUCCESS;
    }
    else
    {
        resp_code_value = RACP_RESPONSE_NO_RECORDS_FOUND;
    }

    cgms_send_racp_response_code(p_cgms, RACP_OPCODE_REPORT_RECS, resp_code_value);
}

/**@brief Function for the RACP report records procedure.
 *
 * @param[in]   p_cgms   Service instance.
 */
static void racp_report_records_procedure(nrf_ble_cgms_t * p_cgms)
{
	 uint32_t err_code;

    while (p_cgms->racp_data.racp_procesing_active)
    {
        // Execute requested procedure
        switch (p_cgms->racp_data.racp_proc_operator)
        {
            case RACP_OPERATOR_ALL:
                err_code = racp_report_records_all(p_cgms);
                break;

            case RACP_OPERATOR_FIRST:
                // Fall through.
            case RACP_OPERATOR_LAST:
                err_code = racp_report_records_first_last(p_cgms);
                break;
            case RACP_OPERATOR_GREATER_OR_EQUAL:
                err_code = racp_report_records_greater_equal(p_cgms);
                break;
            case RACP_OPERATOR_LESS_OR_EQUAL:
                err_code = racp_report_records_less_equal(p_cgms);
                break;
            default:
                // // Report error to application
                // if (p_cgms->error_handler != NULL)
                // {
                //     p_cgms->error_handler(NRF_ERROR_INTERNAL);
                // }

                p_cgms->racp_data.racp_procesing_active = false;

                return;
        }

        // Error handling
        switch (err_code)
        {
            // case NRF_SUCCESS:
			case 0:
                if (!p_cgms->racp_data.racp_procesing_active)
                {
                    racp_report_records_completed(p_cgms);
                }
                break;

            // case NRF_ERROR_RESOURCES:
            //     // Wait for TX_COMPLETE event to resume transmission.
            //     return;

            // case NRF_ERROR_INVALID_STATE:
            //     // Notification is probably not enabled. Ignore request.
            //     p_cgms->racp_data.racp_procesing_active = false;;
            //     return;

            default:
                // Report error to application.
                // if (p_cgms->error_handler != NULL)
                // {
                //     p_cgms->error_handler(err_code);
                // }

                // Make sure state machine returns to the default state.
                p_cgms->racp_data.racp_procesing_active = false;
                return;
        }
    }
}


// static void cgms_send_racp_num_records(uint16_t count)
// {
// 	uint8_t buf[8];
// 	uint8_t operand[2];
// 	uint8_t len;
// 	ble_racp_value_t rsp;

// 	put_le16(operand, count);
// 	rsp.opcode = RACP_OPCODE_NUM_RECS_RESPONSE;
// 	rsp.operator = RACP_OPERATOR_NULL;
// 	rsp.operand_len = sizeof(operand);
// 	rsp.p_operand = operand;

// 	len = ble_racp_encode(buf, sizeof(buf), &rsp);
// 	if (len == 0U) {
// 		return;
// 	}

// 	(void)cgms_racp_indicate(p_cgms, &rsp);
// }

// static void cgms_racp_decode(const uint8_t *buf, uint16_t len, ble_racp_value_t *req)
// {
// 	req->opcode = 0xFFU;
// 	req->operator = 0xFFU;
// 	req->operand_len = 0U;
// 	req->p_operand = NULL;

// 	if (len > 0U) {
// 		req->opcode = buf[0];
// 	}
// 	if (len > 1U) {
// 		req->operator = buf[1];
// 	}
// 	if (len > 2U) {
// 		req->operand_len = len - 2U;
// 		req->p_operand = (uint8_t *)&buf[2];
// 	}
// }

/**@brief Function for testing if the received request is to be executed.
 *
 * @param[in]    p_racp_request    Request to be checked.
 * @param[out]   p_response_code   Response code to be sent in case the request is rejected.
 *                                 RACP_RESPONSE_RESERVED is returned if the received message is
 *                                 to be rejected without sending a respone.
 *
 * @return       TRUE if the request is to be executed, FALSE if it is to be rejected.
 *               If it is to be rejected, p_response_code will contain the response code to be
 *               returned to the central.
 */
static bool is_request_to_be_executed(nrf_ble_cgms_t         * p_cgms,
                                      const ble_racp_value_t * p_racp_request,
                                      uint8_t                * p_response_code)
{
    *p_response_code = RACP_RESPONSE_RESERVED;

    if (p_racp_request->opcode == RACP_OPCODE_ABORT_OPERATION)
    {
        if (p_cgms->racp_data.racp_procesing_active)
        {
            if (p_racp_request->operator != RACP_OPERATOR_NULL)
            {
                *p_response_code = RACP_RESPONSE_INVALID_OPERATOR;
            }
            else if (p_racp_request->operand_len != 0)
            {
                *p_response_code = RACP_RESPONSE_INVALID_OPERAND;
            }
            else
            {
                *p_response_code = RACP_RESPONSE_SUCCESS;
            }
        }
        else
        {
            *p_response_code = RACP_RESPONSE_ABORT_FAILED;
        }
    }
    else if (p_cgms->racp_data.racp_procesing_active)
    {
        return false;
    }
    // Supported opcodes
    else if ((p_racp_request->opcode == RACP_OPCODE_REPORT_RECS) ||
             (p_racp_request->opcode == RACP_OPCODE_REPORT_NUM_RECS))
    {
        switch (p_racp_request->operator)
        {
            // Operators without a filter.
            case RACP_OPERATOR_ALL:
                // Fall through.
            case RACP_OPERATOR_FIRST:
                // Fall through.
            case RACP_OPERATOR_LAST:
                if (p_racp_request->operand_len != 0)
                {
                    *p_response_code = RACP_RESPONSE_INVALID_OPERAND;
                }
                break;

            // Operators with a filter as part of the operand.
            case RACP_OPERATOR_LESS_OR_EQUAL:
                // Fall Through.
            case RACP_OPERATOR_GREATER_OR_EQUAL:
                if (*(p_racp_request->p_operand) == RACP_OPERAND_FILTER_TYPE_FACING_TIME)
                {
                    *p_response_code = RACP_RESPONSE_PROCEDURE_NOT_DONE;
                }
                if (p_racp_request->operand_len != OPERAND_LESS_GREATER_SIZE)
                {
                    *p_response_code = RACP_RESPONSE_INVALID_OPERAND;
                }
                break;

            case RACP_OPERATOR_RANGE:
                *p_response_code = RACP_RESPONSE_OPERATOR_UNSUPPORTED;
                break;

            // Invalid operators.
            case RACP_OPERATOR_NULL:
                // Fall through.
            default:
                *p_response_code = RACP_RESPONSE_INVALID_OPERATOR;
                break;
        }
    }
    // Unsupported opcodes.
    else if (p_racp_request->opcode == RACP_OPCODE_DELETE_RECS)
    {
        *p_response_code = RACP_RESPONSE_OPCODE_UNSUPPORTED;
    }
    // Unknown opcodes.
    else
    {
        *p_response_code = RACP_RESPONSE_OPCODE_UNSUPPORTED;
    }

    return (*p_response_code == RACP_RESPONSE_RESERVED);
}

/**@brief Function for getting a record with time offset less or equal to the input param.
 *
 * @param[in]  offset     The record that this function returns must have an time offset less or greater to this.
 * @param[out] record_num Pointer to the record index of the record that has the desired time offset.
 *
 * @retval NRF_SUCCESS If the record was successfully retrieved.
 * @retval NRF_ERROR_NOT_FOUND A record with the desired offset does not exist in the database.
 * @return                     If functions from other modules return errors to this function,
 *                             the @ref nrf_error are propagated.
 */
static int32_t record_index_offset_less_or_equal_get(uint16_t offset, uint16_t * record_num)
{
    int32_t err_code;
    ble_cgms_rec_t rec;
    uint16_t upper_bound = cgms_db_num_records_get();

    for((*record_num) = upper_bound; (*record_num)-- >0;)
    {
        err_code = cgms_db_record_get(*record_num, &rec);
        if (err_code != 0)
        {
            return err_code;
        }
        if (rec.meas.time_offset <= offset)
        {
            return 0;
        }
    }
    return -1;
}

/**@brief Function for getting a record with time offset greater or equal to the input param.
 *
 * @param[in]  offset     The record that this function returns must have an time offset equal or
 *                        greater to this.
 * @param[out] record_num Pointer to the record index of the record that has the desired time offset.
 *
 * @retval NRF_SUCCESS         If the record was successfully retrieved.
 * @retval NRF_ERROR_NOT_FOUND A record with the desired offset does not exist in the database.
 * @return                     If functions from other modules return errors to this function,
 *                             the @ref nrf_error are propagated.
 */
static int32_t record_index_offset_greater_or_equal_get(uint16_t offset, uint16_t * record_num)
{
    int32_t     err_code;
    ble_cgms_rec_t rec;
    uint16_t       upper_bound = cgms_db_num_records_get();

    for(*record_num = 0; *record_num < upper_bound; (*record_num)++)
    {
        err_code = cgms_db_record_get(*record_num, &rec);
        if (err_code != 0)
        {
            return err_code;
        }
        if (rec.meas.time_offset >= offset)
        {
            return 0;
        }
    }
    return -1;
}

/**@brief Function for processing a REPORT RECORDS request.
 *
 * @details Set initial values before entering the state machine of racp_report_records_procedure().
 *
 * @param[in]   p_cgms           Service instance.
 * @param[in]   p_racp_request   Request to be executed.
 */
static void report_records_request_execute(nrf_ble_cgms_t   * p_cgms,
                                           ble_racp_value_t * p_racp_request)
{
    p_cgms->racp_data.racp_procesing_active = true;

    p_cgms->racp_data.racp_proc_record_ndx               = 0;
    p_cgms->racp_data.racp_proc_operator                 = p_racp_request->operator;
    p_cgms->racp_data.racp_proc_records_reported         = 0;
    p_cgms->racp_data.racp_proc_records_ndx_last_to_send = 0;

    if (p_cgms->racp_data.racp_proc_operator == RACP_OPERATOR_GREATER_OR_EQUAL)
    {
        uint16_t  offset_requested = uint16_decode(&p_cgms->racp_data.racp_request.p_operand[OPERAND_LESS_GREATER_FILTER_TYPE_SIZE]);
        int32_t err_code = record_index_offset_greater_or_equal_get(offset_requested, &p_cgms->racp_data.racp_proc_record_ndx);
        if (err_code != 0)
        {
            racp_report_records_completed(p_cgms);
        }

    }
    if (p_cgms->racp_data.racp_proc_operator == RACP_OPERATOR_LESS_OR_EQUAL)
    {
        uint16_t   offset_requested = uint16_decode(&p_cgms->racp_data.racp_request.p_operand[OPERAND_LESS_GREATER_FILTER_TYPE_SIZE]);
        int32_t err_code         = record_index_offset_less_or_equal_get(offset_requested,
                                                                            &p_cgms->racp_data.racp_proc_records_ndx_last_to_send);
        if (err_code != 0)
        {
            racp_report_records_completed(p_cgms);
        }
    }
    racp_report_records_procedure(p_cgms);
}

/**@brief Function for processing a REPORT NUM RECORDS request.
 *
 * @param[in]   p_cgms           Service instance.
 * @param[in]   p_racp_request   Request to be executed.
 */
static void report_num_records_request_execute(nrf_ble_cgms_t   * p_cgms,
                                               ble_racp_value_t * p_racp_request)
{
    // 没有移植
    uint16_t total_records;
    uint16_t num_records;

    total_records = cgms_db_num_records_get();
    num_records   = 0;

    if (p_racp_request->operator == RACP_OPERATOR_ALL)
    {
        num_records = total_records;
    }
    else if ((p_racp_request->operator == RACP_OPERATOR_FIRST) ||
             (p_racp_request->operator == RACP_OPERATOR_LAST))
    {
        if (total_records > 0)
        {
            num_records = 1;
        }
    }
    else if (p_racp_request->operator == RACP_OPERATOR_GREATER_OR_EQUAL)
    {
        uint16_t   index_of_offset;
        uint16_t   offset_requested = uint16_decode(&p_cgms->racp_data.racp_request.p_operand[OPERAND_LESS_GREATER_FILTER_TYPE_SIZE]);
        int32_t err_code         = record_index_offset_greater_or_equal_get(offset_requested, &index_of_offset);

        if (err_code != 0)
        {
            num_records = 0;
        }
        else
        {
            num_records = total_records - index_of_offset;
        }
    }

    p_cgms->racp_data.pending_racp_response.opcode      = RACP_OPCODE_NUM_RECS_RESPONSE;
    p_cgms->racp_data.pending_racp_response.operator    = RACP_OPERATOR_NULL;
    p_cgms->racp_data.pending_racp_response.operand_len = sizeof(uint16_t);
    p_cgms->racp_data.pending_racp_response.p_operand   =
        p_cgms->racp_data.pending_racp_response_operand;

    p_cgms->racp_data.pending_racp_response_operand[0] = num_records & 0xFF;
    p_cgms->racp_data.pending_racp_response_operand[1] = num_records >> 8;

    cgms_racp_indicate(p_cgms, &p_cgms->racp_data.pending_racp_response);
}


// 目前保留，回调事件处理逻辑 增加cb
void cgms_racp_on_meas_tx_complete(struct bt_conn *conn, void *user_data)
{
	// ARG_UNUSED(conn);
	// ARG_UNUSED(user_data);

	// if (!m_racp.processing_active) {
	// 	return;
	// }

	// m_racp.proc_records_reported++;

	// if (m_racp.proc_record_ndx >= m_racp.proc_records_ndx_last_to_send) {
	// 	m_racp.processing_active = false;
	// 	cgms_send_racp_response_code(RACP_OPCODE_REPORT_RECS,
	// 		(m_racp.proc_records_reported > 0U) ? RACP_RESPONSE_SUCCESS : RACP_RESPONSE_NO_RECORDS_FOUND);
	// 	return;
	// }

	// m_racp.proc_record_ndx++;
	// if (cgms_measurement_notify_with_cb(&m_records[m_racp.proc_record_ndx],
	// 	cgms_racp_on_meas_tx_complete,
	// 	NULL) != 0) {
	// 	m_racp.processing_active = false;
	// 	cgms_send_racp_response_code(RACP_OPCODE_REPORT_RECS, RACP_RESPONSE_PROCEDURE_NOT_DONE);
	// }
}

// ssize_t cgms_write_racp(struct bt_conn *conn, const struct bt_gatt_attr *attr,
// 	const void *buf, uint16_t len, uint16_t offset, uint8_t flags, nrf_ble_cgms_t * p_cgms)
// {
// 	// ble_racp_value_t req;
// 	uint8_t response_code;
// 	uint16_t count;

// 	ARG_UNUSED(conn);
// 	ARG_UNUSED(attr);
// 	ARG_UNUSED(flags);

// 	if (offset != 0U) {
// 		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
// 	}

// 	// 如果没有使能indication，直接返回错误
// 	if (!m_racp_ind_enabled) {
// 		return BT_GATT_ERR(BT_ATT_ERR_CCC_IMPROPER_CONF);
// 	}

// 	// cgms_racp_decode((const uint8_t *)buf, len, &req);
// 	ble_racp_decode(len, (const uint8_t *)buf, &p_cgms->racp_data.racp_request);
// 	if (!is_request_to_be_executed(p_cgms, &p_cgms->racp_data.racp_request, &response_code)) {
// 		if (response_code == RACP_RESPONSE_SUCCESS) {
// 			cgms_racp_reset_state();
// 		}
// 		if (response_code != RACP_RESPONSE_RESERVED) {
// 			m_racp.processing_active = false;
// 			cgms_send_racp_response_code(p_cgms, p_cgms->racp_data.racp_request.opcode, response_code);
// 		}
// 		return len;
// 	}

// 	if (p_cgms->racp_data.racp_request.opcode == RACP_OPCODE_REPORT_NUM_RECS) {
// 		cgms_send_racp_num_records(cgms_count_matching_records(&p_cgms->racp_data.racp_request));
// 		return len;
// 	}
// 	if (p_cgms->racp_data.racp_request.opcode == RACP_OPCODE_REPORT_RECS) {
// 		if (cgms_report_records(&p_cgms->racp_data.racp_request, &count) != 0) {
// 			cgms_send_racp_response_code(p_cgms, RACP_OPCODE_REPORT_RECS, RACP_RESPONSE_PROCEDURE_NOT_DONE);
// 			return len;
// 		}
// 		if (count == 0U) {
// 			cgms_send_racp_response_code(p_cgms, RACP_OPCODE_REPORT_RECS, RACP_RESPONSE_NO_RECORDS_FOUND);
// 		}
// 	}

// 	return len;
// }

ssize_t cgms_write_racp(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	// ble_racp_value_t req;
	nrf_ble_cgms_t * p_cgms;
	uint8_t response_code;
	// uint16_t count;

	ARG_UNUSED(conn);
	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (offset != 0U) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	// auth reply 适配

	// 如果没有使能indication，直接返回错误
	if (!m_racp_ind_enabled) {
		return BT_GATT_ERR(BT_ATT_ERR_CCC_IMPROPER_CONF);
	}

	// cgms_racp_decode((const uint8_t *)buf, len, &req);
	ble_racp_decode(len, (const uint8_t *)buf, &p_cgms->racp_data.racp_request);
	if (is_request_to_be_executed(p_cgms, &p_cgms->racp_data.racp_request, &response_code)) {
		// auth reply 适配、
		// Execute request
        if (p_cgms->racp_data.racp_request.opcode == RACP_OPCODE_REPORT_RECS)
        {
            report_records_request_execute(p_cgms, &p_cgms->racp_data.racp_request);
        }
        else if (p_cgms->racp_data.racp_request.opcode == RACP_OPCODE_REPORT_NUM_RECS)
        {
            report_num_records_request_execute(p_cgms, &p_cgms->racp_data.racp_request);
        }
	}
	else if (response_code != RACP_RESPONSE_RESERVED)
    {
        // auth reply 适配

        // Abort any running procedure
        p_cgms->racp_data.racp_procesing_active = false;

        // Respond with error code
        cgms_send_racp_response_code(p_cgms, p_cgms->racp_data.racp_request.opcode, response_code);
    }
    else
    {
        // auth reply 适配
		// ignore request
    }
	return len;
}
