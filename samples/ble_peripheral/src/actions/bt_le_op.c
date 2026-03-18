/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include "bt_le_op.h"
#include "ble_data_test_sample.h"
#include "ble_super_service.h"
#include "soc_clock.h"
#include "soc_pm.h"
#include "atb_ble_cgms.h"

#define DEVICE_NAME "CGMS_Demo"
//#define DEVICE_NAME			CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN		(sizeof(DEVICE_NAME) - 1)
	
static struct bt_conn *slave_conn;

static void ble_cgms_evt_handle(const ble_cgms_evt_t *evt)
{
	if (evt == NULL) {
		return;
	}

	switch (evt->evt_type) {
	case BLE_CGMS_EVT_NOTIFICATION_ENABLED:
		printk("CGMS measurement notify enabled\n");
		break;
	case BLE_CGMS_EVT_NOTIFICATION_DISABLED:
		printk("CGMS measurement notify disabled\n");
		break;
	case BLE_CGMS_EVT_START_SESSION:
		printk("CGMS start session\n");
		break;
	case BLE_CGMS_EVT_STOP_SESSION:
		printk("CGMS stop session\n");
		break;
	case BLE_CGMS_EVT_WRITE_COMM_INTERVAL:
		printk("CGMS comm interval -> %u min\n", evt->comm_interval);
		break;
	default:
		break;
	}
}

void app_to_msg(uint8_t type, uint8_t event)
{
	struct app_msg msg = {0};
	msg.type = type;
	msg.value = event;
	send_msg(&msg, K_MSEC(100));
}

void le_param_updated(struct bt_conn *conn, uint16_t interval,
			     uint16_t latency, uint16_t timeout)
{
	printk("LE conn param updated: int 0x%04x lat %d to %d\n", interval, latency, timeout);
}

void exchange_func(struct bt_conn *conn, uint8_t err,
			   struct bt_gatt_exchange_params *params)
{
	uint16_t mtu;
	mtu = bt_gatt_get_mtu(conn);
	printk("Exchange %s mtu:%d\n", err == 0 ? "successful" : "failed",mtu);
}

static struct bt_gatt_exchange_params exchange_params = {
	.func = exchange_func,
};


static struct bt_le_adv_param hid_app_adv_param = {
	.id = BT_ID_DEFAULT,
	.sid = 0,
	.secondary_max_skip = 0,
	.options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME | BT_LE_ADV_OPT_USE_IDENTITY,
	.interval_min = 1600,
	.interval_max = 1600,
	.peer = NULL,
};

int act_bt_le_adv_start(const struct bt_le_adv_param *param,
		    const struct bt_data *ad, size_t ad_len,
		    const struct bt_data *sd, size_t sd_len)
{
  acts_clock_rc32k_measure_then_calibrate();	
	return bt_le_adv_start(param, ad, ad_len, sd, sd_len);
}

static struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA, 0xe0, 0x03),
};

#define ADV_TIMEOUT_SECONDS 50
static struct k_delayed_work adv_timeout_work;
static void adv_timeout_handler(struct k_work *work)
{
    printk("BLE advertising timeout, stop advertising.\n");
    bt_le_adv_stop();
}

void adv_timeout_init(void)
{
    k_delayed_work_init(&adv_timeout_work, adv_timeout_handler);
}

void adv_timeout_start(void)
{
    k_delayed_work_submit(&adv_timeout_work, K_SECONDS(ADV_TIMEOUT_SECONDS));
}

void adv_timeout_stop(void)
{
    k_delayed_work_cancel(&adv_timeout_work);
}


static int start_adv(void)
{
	int err;
	adv_timeout_init();
	struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
		BT_DATA_BYTES(BT_DATA_NAME_COMPLETE, DEVICE_NAME),
		BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_CGM_VAL)),
	};

//	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), NULL, 0);
	err = act_bt_le_adv_start(&hid_app_adv_param, ad, ARRAY_SIZE(ad), (const struct bt_data *)sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return err;
	}
	adv_timeout_start();
	printk("Advertising successfully started\n");
	return 0;
}

static void stop_adv(void)
{
	bt_le_adv_stop();
	adv_timeout_stop();
	printk("stop_adv\n");
}

void conn_update(void)
{
	uint8_t err;
	struct bt_le_conn_param app_update_cfg = {
		.interval_min = (800),
		.interval_max = (800),
		.latency = (0),
		.timeout = (400),
	};

	stop_adv();
	if (exchange_params.func) 
	{
		bt_gatt_exchange_mtu(slave_conn, &exchange_params);
	}

	err = bt_conn_le_param_update(slave_conn, &app_update_cfg);
	if (err) 
	{
		printk("Conn param update failed(err %d)\n", err);
		return;
	}	
}

void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	
	printk("Connected: %s\n", addr);
	if (err)
	{
		printk("Connection failed (err 0x%02x)\n", err);
		return;
	} 
	else
	{
		printk("Connected\n");
		slave_conn = bt_conn_ref(conn);
		ble_cgms_connected(conn);

		app_to_msg(MSG_BLE_STATE, CONNECTED);

	}
	
}

void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != slave_conn)
	{
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);
	ble_cgms_disconnected(conn);
	bt_conn_unref(conn);
	slave_conn = NULL;
	
	bt_data_trans_cancel();
	start_adv();
}

void system_ble_event_handle(uint32_t event)
{
	switch(event)
	{
		case START_ADV:
			start_adv();
			break;
		case STOP_ADV:
			stop_adv();
			break;
		case CONNECTED:
			conn_update();
			break;
		default:
		printk("unkown ble_event code\n");
		break;
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_updated = le_param_updated,
};

void bt_le_op_init(void)
{
	/* register conn callbacks into bt stack */
	bt_conn_cb_register((struct bt_conn_cb *)&conn_callbacks);
	
	bt_data_trans_init();
	ble_cgms_register_evt_handler(ble_cgms_evt_handle);
	
	start_adv();
}


