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
#include <settings/settings.h>
#include "sys_wakelock.h"
#include "sys_monitor.h"
#include "sys_standby.h"
#include "bt_le_op.h"
#include "msg_manager.h"
#include <device.h>

//extern void cpuload_stat_start(int interval_ms);
//extern void cpuload_stat_stop(void);

//static struct k_delayed_work ble_heart_beat_delaywork;
//static void ble_heart_beat_work_handle(struct k_work *work);
//static K_DELAYED_WORK_DEFINE(ble_heart_beat_delaywork, ble_heart_beat_work_handle);

//#define HEART_BEAT_TIMEOUT 60
//static void ble_heart_beat_work_handle(struct k_work * work)
//{
//	printk("main_thread still alive\n");
//	k_delayed_work_submit(&ble_heart_beat_delaywork, K_SECONDS(HEART_BEAT_TIMEOUT));
//	//cpuload_stat_stop();
//}

void system_app_init(void)
{
	/* init message manager */
	msg_manager_init();

	/* system monitor initialization */
	sys_monitor_init();

	/* system standby initialization */
	sys_standby_init();

#ifdef CONFIG_STANDBY_POLL_DELAY_WORK
	/* system monitor start */
	sys_monitor_start(CONFIG_MONITOR_PERIOD);
#endif
}

void main(void)
{
	int err;
	int result = 0;
	struct app_msg msg = {0};

	printk("system app init\n");
	system_app_init();

	//cpuload_stat_start(2000);

	//心跳初始化
//	k_delayed_work_submit(&ble_heart_beat_delaywork, K_SECONDS(HEART_BEAT_TIMEOUT));

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}
	
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		settings_load();
	}
	
	bt_le_op_init();
	
	printk("Bluetooth initialized\n");

	while (1) 
	{
		if (receive_msg(&msg, K_FOREVER)) 
		{
				switch (msg.type) 
				{
					case MSG_BLE_STATE:
						printk("MSG_BLE_STATE\n");
						system_ble_event_handle(msg.value);
						break;
					default:
						printk("error message type msg.type %d\n", msg.type);
						break;
				}
				if (msg.callback != NULL)
					msg.callback(&msg, result, NULL);
		}
	}
}
