/*Copyright (c) 2018 Actions (Zhuhai) Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include "errno.h"
#include <sys/printk.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <sys/byteorder.h>
#include <settings/settings.h>

#include "ble_super_service.h"
#include "ble_data_test_sample.h"

#include "soc.h"

#define ARRY_BUF_SIZE 23
static struct k_delayed_work slave_notify_delaywork;

static struct bt_conn *notify_conn;
static uint16_t notify_index;
static bool writing;

void update_write_stats(uint16_t len)
{
	static uint32_t curr_time;
	static uint32_t pre_time;
	static uint32_t write_total;
	
	write_total += len;

	/* if last data rx-ed was greater than 1 second in the past,
	 * reset the metrics.
	 */
	curr_time = k_uptime_get_32();
	if ((curr_time - pre_time) >= 1000) {
		printk("rev write Rx: %d byte\n", write_total);
		write_total = 0;
		pre_time = curr_time;
	}
}	

static void slave_notify_work_handle(struct k_work *work)
{
//	static uint32_t notify_curr_time;
//	static uint32_t notify_pre_time;
//	static uint32_t notify_TxCount;
//	uint16_t mtu;
//	uint8_t i;
//	uint8_t notify_send_buf[ARRY_BUF_SIZE];
//	static uint8_t value = 0;

//	if(notify_conn == NULL)
//		return;

//	/* Test data */
//	{
//		for(uint8_t ops=0; ops<ARRY_BUF_SIZE; ops++)
//			notify_send_buf[ops] = value;
//		value++;
//		printk("mtu %d\n", bt_gatt_get_mtu(notify_conn));
//	}

//	if (ble_super_ccc_enabled(notify_conn, notify_index))
//	{
//		mtu = (notify_conn ? bt_gatt_get_mtu(notify_conn) : 0) - 3;
//		mtu = (mtu > ARRY_BUF_SIZE) ? ARRY_BUF_SIZE : mtu;
//		writing = true; 
//		
//		for(i=0; i<CONFIG_BT_CONN_TX_MAX; i++)	
//		{
//			ble_super_send_notify(notify_conn, notify_index, mtu, notify_send_buf);
//			notify_TxCount += mtu;
//			notify_curr_time = k_uptime_get_32();
//			
//			k_yield();
//		}
//		if ((notify_curr_time - notify_pre_time) >= 1000) 
//		{
//			printk("notify Tx: %d byte\n", notify_TxCount);
//			notify_TxCount = 0;
//			notify_pre_time = notify_curr_time;
//		}

//		k_delayed_work_submit(&slave_notify_delaywork, K_SECONDS(1));
//	}
//	else
//	{
//		k_delayed_work_submit(&slave_notify_delaywork, K_SECONDS(10));
//		printk("trying\n");
//	}

}

void bt_data_trans_init()
{
	k_delayed_work_init(&slave_notify_delaywork, slave_notify_work_handle);
}

void bt_data_submit_handle(struct bt_conn *conn, uint16_t index)
{
	if(conn == NULL)
		return;
	
	if(!writing)
	{
		printk(
"submit\n");
		notify_conn = conn;
		notify_index = index;
		k_delayed_work_submit(&slave_notify_delaywork, K_SECONDS(1));
	}
}

void bt_data_trans_cancel(void)
{
	printk(
"cancel\n");
	notify_conn = NULL;
	writing = false;
	k_delayed_work_cancel(&slave_notify_delaywork);
}
