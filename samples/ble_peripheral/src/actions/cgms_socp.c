#include <errno.h>
#include <string.h>

#include "atb_ble_cgms.h"
#include "cgms_socp.h"
#include "cgms_db.h"
#include "cgms_meas.h"
#include "cgms_sst.h"

static bool m_socp_ind_inflight;

static void ble_socp_decode(uint8_t data_len, uint8_t const * p_data, ble_cgms_socp_value_t * p_socp_val)
{
    p_socp_val->opcode      = 0xFF;
    p_socp_val->operand_len = 0;
    p_socp_val->p_operand   = NULL;

    if (data_len > 0)
    {
        p_socp_val->opcode = p_data[0];
    }
    if (data_len > 1)
    {
        p_socp_val->operand_len = data_len - 1;
        p_socp_val->p_operand   = (uint8_t*)&p_data[1]; // lint !e416
    }
}

void cgms_socp_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);
	m_socp_ind_enabled = (value == BT_GATT_CCC_INDICATE);
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
	if ((req == NULL) || (value == NULL) || (req->operand_len != sizeof(uint16_t)) || (req->p_operand == NULL)) {
		return SOCP_RSP_INVALID_OPERAND;
	}

	*value = get_le16(req->p_operand);
	if ((*value == NRF_BLE_CGMS_PLUS_INFINITE) || (*value == NRF_BLE_CGMS_MINUS_INFINITE)) {
		return SOCP_RSP_OUT_OF_RANGE;
	}

	return SOCP_RSP_SUCCESS;
}

static void cgms_socp_ind_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params, uint8_t err)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(params);
	m_socp_ind_inflight = false;
	if (err != 0U) {
		printk("SOCP indication complete with err: %u\n", err);
	}
}

static int cgms_socp_indicate(nrf_ble_cgms_t * p_cgms, const uint8_t *data, uint16_t len)
{
	if ((p_cgms == NULL) || (p_cgms->m_conn == NULL) || !m_socp_ind_enabled) {
		return -ENOTCONN;
	}

	if (m_socp_ind_inflight) {
		return -EBUSY;
	}

	if ((data == NULL) || (len == 0U) || (len > sizeof(m_socp_ind_buf))) {
		return -EINVAL;
	}
	
	memset(&m_socp_ind_params, 0, sizeof(m_socp_ind_params));
	m_socp_ind_params.attr = &attr_cgms_svc[CGMS_ATTR_SOCP_VAL];
	m_socp_ind_params.data = data;
	m_socp_ind_params.len = len;
	m_socp_ind_params.func = cgms_socp_ind_cb;
	m_socp_ind_params.destroy = NULL;
	m_socp_ind_params.uuid = NULL;
	printk("Sending SOCP indication, len: %u\n", len);
	printk("Connection: %p\n", (void *)p_cgms->m_conn);
	int err = bt_gatt_indicate(p_cgms->m_conn, &m_socp_ind_params);
	if (err == 0) {
		m_socp_ind_inflight = true;
	}
	return err;
}

static int cgms_socp_send_response_code(nrf_ble_cgms_t * p_cgms, uint8_t req_opcode, uint8_t rsp_code)
{
	m_socp_ind_buf[0] = SOCP_RESPONSE_CODE;
	m_socp_ind_buf[1] = req_opcode;
	m_socp_ind_buf[2] = rsp_code;
	return cgms_socp_indicate(p_cgms, m_socp_ind_buf, 3U);
}

int cgms_transport_indicate_socp(uint8_t *data, uint16_t len, bt_gatt_indicate_func_t cb)
{
    const struct bt_gatt_attr *attr = &attr_cgms_svc[17];
	nrf_ble_cgms_t * p_cgms = ble_cgms_instance_get();
	if (p_cgms == NULL) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}
    struct bt_conn *conn = p_cgms->m_conn;
    printk("Indicating SOCP response, conn %p, attr %p, data %p, len %u", conn, attr, data, len);

    if ((conn == NULL) || (attr == NULL) || (data == NULL) || (len == 0)) {
        return -EINVAL;
    }

    socp_ind_params.attr = attr;
    socp_ind_params.data = data;
    socp_ind_params.len = len;
    socp_ind_params.func = cb;
    socp_ind_params.destroy = NULL;
    socp_ind_params.uuid = NULL;

    return bt_gatt_indicate(conn, &socp_ind_params);
}

static int cgms_socp_send_u16_response(uint8_t req_opcode, uint8_t rsp_code)
{
	m_socp_ind_buf[0] = SOCP_RESPONSE_CODE;
	m_socp_ind_buf[1] = req_opcode;
	m_socp_ind_buf[2] = rsp_code;
	return cgms_socp_indicate(m_socp_ind_buf, 3, cgms_socp_ind_cb);
}
static uint8_t ble_socp_encode(const ble_socp_rsp_t * p_socp_rsp, uint8_t * p_data)
{
    uint8_t len = 0;
    int     i;


    if (p_data != NULL)
    {
        p_data[len++] = p_socp_rsp->opcode;

        if (
			(p_socp_rsp->opcode != SOCP_READ_CGM_COMM_INTERVAL_RSP)
            && (p_socp_rsp->opcode != SOCP_READ_PATIENT_HIGH_ALERT_LEVEL_RESPONSE)
            && (p_socp_rsp->opcode != SOCP_READ_PATIENT_LOW_ALERT_LEVEL_RESPONSE)
            && (p_socp_rsp->opcode != SOCP_HYPO_ALERT_LEVEL_RESPONSE)
            && (p_socp_rsp->opcode != SOCP_HYPER_ALERT_LEVEL_RESPONSE)
            && (p_socp_rsp->opcode != SOCP_RATE_OF_DECREASE_ALERT_LEVEL_RESPONSE)
            && (p_socp_rsp->opcode != SOCP_RATE_OF_INCREASE_ALERT_LEVEL_RESPONSE)
            && (p_socp_rsp->opcode != SOCP_READ_GLUCOSE_CALIBRATION_VALUE_RESPONSE)
           )
        {
            p_data[len++] = p_socp_rsp->req_opcode;
            p_data[len++] = p_socp_rsp->rsp_code;
        }

        for (i = 0; i < p_socp_rsp->size_val; i++)
        {
            p_data[len++] = p_socp_rsp->resp_val[i];
        }
    }

    return len;
}

// static int cgms_socp_send_response(uint8_t opcode, uint8_t req_opcode, uint8_t rsp_code,
// 	const uint8_t *value, uint8_t value_len)
static int cgms_socp_send_response(nrf_ble_cgms_t * p_cgms)
{
	// uint8_t buf[20];
	// uint8_t len;
	uint8_t          encoded_resp[25];
    uint16_t         len;
    
	// Send indication
    len = ble_socp_encode(&(p_cgms->socp_response), encoded_resp);

	if (len == 0U) {
		return -EINVAL;
	}

	return cgms_socp_indicate(p_cgms, encoded_resp, len);
}

// static int cgms_socp_send_u16_response(uint8_t opcode, uint16_t value)
// {
// 	uint8_t resp[2];

// 	put_le16(resp, value);
// 	return cgms_socp_send_response(opcode, 0U, SOCP_RSP_SUCCESS, resp, sizeof(resp));
// }
ssize_t cgms_write_socp(struct bt_conn *conn, const struct bt_gatt_attr *attr,

	const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	printk("Received write to SOCP characteristic, len: %u, offset: %u\n", len, offset);
	// ble_cgms_socp_value_t req;
	ble_cgms_socp_value_t                 socp_request;
	nrf_ble_cgms_evt_t                    evt;
	// uint8_t value[2];
	// ble_gatts_rw_authorize_reply_params_t auth_reply;
    uint32_t                              err_code;
	int                                   socp_rsp_err;
	nrf_ble_cgms_t * p_cgms;

	ARG_UNUSED(conn);
	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (offset != 0U) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	// auth reply 适配

	if (!m_socp_ind_enabled) {
		return BT_GATT_ERR(BT_ATT_ERR_CCC_IMPROPER_CONF);
	}

	// 解码opcode和参数
	// ble_socp_decode(len, (const uint8_t *)buf, &socp_request);

	// 获取全局cgms结构体
	p_cgms = ble_cgms_instance_get();
	if (p_cgms == NULL) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	if ((conn != NULL) && (p_cgms->m_conn == NULL)) {
		p_cgms->m_conn = bt_conn_ref(conn);
	}

	p_cgms->socp_response.opcode     = SOCP_RESPONSE_CODE;
    p_cgms->socp_response.req_opcode = socp_request.opcode;
    p_cgms->socp_response.rsp_code   = SOCP_RSP_OP_CODE_NOT_SUPPORTED;
    p_cgms->socp_response.size_val   = 0;

	switch (socp_request.opcode) {
	case SOCP_WRITE_CGM_COMMUNICATION_INTERVAL:
		printk("SOCP Write CGM Communication Interval received\n");
		if ((len != 2U) || (socp_request.operand_len != 1U) || (socp_request.p_operand == NULL)) {
			socp_rsp_err = cgms_socp_send_response_code(p_cgms,
				socp_request.opcode,
				SOCP_RSP_INVALID_OPERAND);
			if (socp_rsp_err != 0) {
				printk("SOCP response indicate failed (opcode: 0x%02X, rsp: 0x%02X, err: %d)\n",
					socp_request.opcode, SOCP_RSP_INVALID_OPERAND, socp_rsp_err);
			}
			return len;
		}
		p_cgms->comm_interval = cgms_normalize_comm_interval(socp_request.p_operand[0]);
		cgms_emit_event(BLE_CGMS_EVT_WRITE_COMM_INTERVAL);
		cgms_cancel_glucose_work(p_cgms);
		if (p_cgms->is_session_started && (p_cgms->comm_interval != 0U)) {
			cgms_schedule_glucose_work(p_cgms);
		}
		socp_rsp_err = cgms_socp_send_response_code(p_cgms,
			socp_request.opcode,
			SOCP_RSP_SUCCESS);
		if (socp_rsp_err != 0) {
			printk("SOCP response indicate failed (opcode: 0x%02X, rsp: 0x%02X, err: %d)\n",
				socp_request.opcode, SOCP_RSP_SUCCESS, socp_rsp_err);
		}
		return len;
	case SOCP_READ_CGM_COMMUNICATION_INTERVAL:
		p_cgms->socp_response.opcode      = SOCP_READ_CGM_COMM_INTERVAL_RSP;
		p_cgms->socp_response.resp_val[0] = p_cgms->comm_interval;
		p_cgms->socp_response.size_val++;
		break;
	
	case SOCP_START_THE_SESSION:
		if (p_cgms->is_session_started) {
			p_cgms->socp_response.rsp_code = SOCP_RSP_PROCEDURE_NOT_COMPLETED;
		}
		else if ((p_cgms->nb_run_session != 0U) && 
				 !cgms_feature_present(NRF_BLE_CGMS_FEAT_MULTIPLE_SESSIONS_SUPPORTED))
		{
			p_cgms->socp_response.rsp_code = SOCP_RSP_PROCEDURE_NOT_COMPLETED;
		}
		else
		{
			p_cgms->socp_response.rsp_code = SOCP_RSP_SUCCESS;
			p_cgms->is_session_started     = true;
			p_cgms->nb_run_session++;
			
			if (p_cgms->evt_handler != NULL)
			{
				evt.evt_type = BLE_CGMS_EVT_START_SESSION;
				p_cgms->evt_handler(p_cgms, &evt);
				// cgms_start_session(p_cgms);
			}

			ble_cgms_sst_t sst;
			memset(&sst, 0, sizeof(ble_cgms_sst_t));

			err_code = cgms_sst_set(p_cgms, &sst);
			if (err_code != 0) {
				p_cgms->socp_response.rsp_code = SOCP_RSP_PROCEDURE_NOT_COMPLETED;
				printk("Failed to set SST (err %d)\n", err_code);
				return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
			}
			p_cgms->sensor_status.time_offset    = 0;
			p_cgms->sensor_status.status.status &= (~NRF_BLE_CGMS_STATUS_SESSION_STOPPED);
			
			err_code = nrf_ble_cgms_update_status(p_cgms, &p_cgms->sensor_status);
			if (err_code != 0)
			{
				p_cgms->socp_response.rsp_code = SOCP_RSP_PROCEDURE_NOT_COMPLETED;
				printk("Failed to update status (err %d)\n", err_code);
				return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
			}
		}
		break;

	case SOCP_STOP_THE_SESSION:
        {
            nrf_ble_cgm_status_t status;
            memset(&status, 0, sizeof(nrf_ble_cgm_status_t));
			
            p_cgms->socp_response.rsp_code = SOCP_RSP_SUCCESS;
            p_cgms->is_session_started     = false;

            status.time_offset   = p_cgms->sensor_status.time_offset;
            status.status.status = p_cgms->sensor_status.status.status |
                                   NRF_BLE_CGMS_STATUS_SESSION_STOPPED;

            if (p_cgms->evt_handler != NULL)
            {
                evt.evt_type = BLE_CGMS_EVT_STOP_SESSION;
                p_cgms->evt_handler(p_cgms, &evt);
            }
            err_code = nrf_ble_cgms_update_status(p_cgms, &status);
            if (err_code != 0)
            {
               	p_cgms->socp_response.rsp_code = SOCP_RSP_PROCEDURE_NOT_COMPLETED;
				printk("Failed to update status (err %d)\n", err_code);
				return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
            }
            break;
        }

		default:
            p_cgms->socp_response.rsp_code = SOCP_RSP_OP_CODE_NOT_SUPPORTED;
            break;
	}
	socp_rsp_err = cgms_socp_send_response(p_cgms);
	if (socp_rsp_err != 0) {
		printk("SOCP response indicate failed (opcode: 0x%02X, rsp: 0x%02X, err: %d)\n",
			socp_request.opcode, p_cgms->socp_response.rsp_code, socp_rsp_err);
	} else {
		printk("Received SOCP opcode: 0x%02X, sent response with code: 0x%02X\n",
			socp_request.opcode, p_cgms->socp_response.rsp_code);
	}
	return len;
}
