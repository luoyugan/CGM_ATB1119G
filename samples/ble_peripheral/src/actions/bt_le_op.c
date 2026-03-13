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

#define CGM_DEVICE_NAME                      CONFIG_BT_DEVICE_NAME
// #define CGM_DEVICE_NAME                         "111Able_111Able_111Ab"
#define CGM_DEVICE_NAME_LEN                     (sizeof(CGM_DEVICE_NAME) - 1)

/* 广播参数：对应 CGMP 建议窗口 */
#define CGM_ADV_FAST_INTERVAL_MIN               48      /* 30 ms */
#define CGM_ADV_FAST_INTERVAL_MAX               480     /* 300 ms */
#define CGM_ADV_SLOW_INTERVAL_MIN               1600    /* 1 s */
#define CGM_ADV_SLOW_INTERVAL_MAX               16384   /* 10.24 s */

#define CGM_ADV_FAST_PHASE_SECONDS              30
#define CGM_ADV_WHITELIST_WAITING_SECONDS       120     /* 建议范围 30~600 */

static const uint8_t cgm_adv_manufacturer_data[] = {
	0xe0, 0x03,
};
	
static struct bt_conn *slave_conn;
static bt_addr_le_t cgm_bonded_peer;
static bool cgm_bonded_peer_valid;
static bool cgm_adv_whitelist_mode;

static struct k_delayed_work cgm_adv_phase_work;
static struct k_delayed_work cgm_whitelist_wait_work;

/**
 * @brief 向系统消息队列投递 BLE 事件。
 *
 * @param type  事件类型（如 MSG_BLE_STATE）。
 * @param event 事件值（如 START_ADV / CONNECTED）。
 * @return 无返回值。
 *
 * @details
 * 该函数是本模块内部统一事件上报入口，使用 send_msg() 异步通知主流程。
 */
void cgm_app_to_msg(uint8_t type, uint8_t event)
{
	struct app_msg msg = {0};
	msg.type = type;
	msg.value = event;
	send_msg(&msg, K_MSEC(100));
}

/**
 * @brief 旧接口兼容包装。
 *
 * @param type  事件类型。
 * @param event 事件值。
 * @return 无返回值。
 */
void app_to_msg(uint8_t type, uint8_t event)
{
	cgm_app_to_msg(type, event);
}

/**
 * @brief 连接参数更新回调。
 *
 * @param conn     当前连接对象。
 * @param interval 当前连接间隔。
 * @param latency  当前连接从机延迟。
 * @param timeout  当前监督超时。
 * @return 无返回值。
 */
static void cgm_le_param_updated_cb(struct bt_conn *conn, uint16_t interval,
				    uint16_t latency, uint16_t timeout)
{
	ARG_UNUSED(conn);
	printk("LE conn param updated: int 0x%04x lat %d to %d\n", interval, latency, timeout);
}

/**
 * @brief MTU 交换完成回调。
 *
 * @param conn   当前连接对象。
 * @param err    MTU 交换错误码，0 表示成功。
 * @param params 交换参数对象（当前仅用于接口匹配）。
 * @return 无返回值。
 */
static void cgm_exchange_mtu_cb(struct bt_conn *conn, uint8_t err,
				struct bt_gatt_exchange_params *params)
{
	uint16_t mtu;
	ARG_UNUSED(params);

	mtu = bt_gatt_get_mtu(conn);
	printk("Exchange %s mtu:%d\n", err == 0 ? "successful" : "failed", mtu);
}

static struct bt_gatt_exchange_params exchange_params = {
	.func = cgm_exchange_mtu_cb,
};

/**
 * @brief 广播启动包装函数。
 *
 * @param param  广播参数（模式、选项、间隔等）。
 * @param ad     Advertising Data 数组。
 * @param ad_len Advertising Data 条目数。
 * @param sd     Scan Response Data 数组。
 * @param sd_len Scan Response Data 条目数。
 * @return 0 表示广播启动成功；非 0 表示失败。
 *
 * @details
 * 启动广播前会执行 RC32K 校准，降低时间基准漂移带来的调度误差。
 */
static int cgm_bt_le_adv_start(const struct bt_le_adv_param *param,
			       const struct bt_data *ad, size_t ad_len,
			       const struct bt_data *sd, size_t sd_len)
{
	acts_clock_rc32k_measure_then_calibrate();
	return bt_le_adv_start(param, ad, ad_len, sd, sd_len);
}

/**
 * @brief 白名单等待窗口超时回调。
 *
 * @param work 内核工作项对象。
 * @return 无返回值。
 *
 * @details
 * 仅在“白名单广播模式 + 当前未连接”时生效，超时后切换为 allow-all 模式。
 */
static void cgm_whitelist_wait_timeout_cb(struct k_work *work)
{
	ARG_UNUSED(work);

	if (!cgm_adv_whitelist_mode || slave_conn) {
		return;
	}

	printk("Whitelist waiting window timeout, switch to allow-all mode\n");
	cgm_adv_whitelist_mode = false;
}

/**
 * @brief 广播阶段切换回调（快连 -> 慢速）。
 *
 * @param work 内核工作项对象。
 * @return 无返回值。
 *
 * @details
 * - 快速窗口结束后执行；
 * - 若当前已连接则不执行切换；
 * - 保持广播数据不变，仅降低广播间隔以降低功耗。
 */
static void cgm_adv_phase_switch_cb(struct k_work *work)
{
	int err;
	uint16_t cgm_uuid16 = 0x181F;
	uint16_t bas_uuid16 = BT_UUID_BAS_VAL;
	struct bt_le_adv_param adv_param;
	struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
		BT_DATA(BT_DATA_MANUFACTURER_DATA,
			cgm_adv_manufacturer_data,
			sizeof(cgm_adv_manufacturer_data)),
		BT_DATA(BT_DATA_UUID16_ALL, &cgm_uuid16, sizeof(cgm_uuid16)),
		BT_DATA(BT_DATA_UUID16_SOME, &bas_uuid16, sizeof(bas_uuid16)),
	};
	struct bt_data sd[] = {
		BT_DATA_BYTES(BT_DATA_NAME_COMPLETE, CGM_DEVICE_NAME),
	};

	ARG_UNUSED(work);

	if (slave_conn) {
		return;
	}

	err = bt_le_adv_stop();
	if (err) {
		printk("Failed to stop advertising before phase switch (%d)\n", err);
	}

	adv_param.id = BT_ID_DEFAULT;
	adv_param.sid = 0;
	adv_param.secondary_max_skip = 0;
	adv_param.options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_IDENTITY;
	if (cgm_adv_whitelist_mode) {
		adv_param.options |= BT_LE_ADV_OPT_FILTER_CONN | BT_LE_ADV_OPT_FILTER_SCAN_REQ;
	}
	adv_param.interval_min = CGM_ADV_SLOW_INTERVAL_MIN;
	adv_param.interval_max = CGM_ADV_SLOW_INTERVAL_MAX;
	adv_param.peer = NULL;

	err = cgm_bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Slow-phase advertising start failed (%d)\n", err);
		return;
	}

	printk("Advertising switched to slow phase\n");
}

/**
 * @brief 按当前策略启动广播。
 *
 * @param whitelist_only true 表示仅允许白名单设备扫描/连接；false 表示允许所有设备。
 * @return 0 表示成功；非 0 表示失败。
 *
 * @details
 * 启动流程包含：
 * 1) 停止旧广播；
 * 2) 配置白名单（若启用）；
 * 3) 启动快连窗口广播；
 * 4) 投递慢速切换定时器与白名单等待窗口定时器。
 */
static int cgm_start_advertising(bool whitelist_only)
{
	int err;
	uint16_t cgm_uuid16 = 0x181F;
	uint16_t bas_uuid16 = BT_UUID_BAS_VAL;
	struct bt_le_adv_param adv_param;
	struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
		BT_DATA(BT_DATA_MANUFACTURER_DATA,
			cgm_adv_manufacturer_data,
			sizeof(cgm_adv_manufacturer_data)),
		BT_DATA(BT_DATA_UUID16_ALL, &cgm_uuid16, sizeof(cgm_uuid16)),
		BT_DATA(BT_DATA_UUID16_SOME, &bas_uuid16, sizeof(bas_uuid16)),
	};
	struct bt_data sd[] = {
		BT_DATA_BYTES(BT_DATA_NAME_COMPLETE, CGM_DEVICE_NAME),
	};

	cgm_adv_whitelist_mode = whitelist_only;

	err = bt_le_adv_stop();
	if (err) {
		printk("Stop previous advertising failed (%d)\n", err);
	}

	if (cgm_adv_whitelist_mode && cgm_bonded_peer_valid) {
		bt_le_whitelist_clear();
		err = bt_le_whitelist_add(&cgm_bonded_peer);
		if (err) {
			printk("Whitelist add failed (%d), fallback allow-all\n", err);
			cgm_adv_whitelist_mode = false;
		}
	} else {
		bt_le_whitelist_clear();
		cgm_adv_whitelist_mode = false;
	}

	adv_param.id = BT_ID_DEFAULT;
	adv_param.sid = 0;
	adv_param.secondary_max_skip = 0;
	adv_param.options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_IDENTITY;
	if (cgm_adv_whitelist_mode) {
		adv_param.options |= BT_LE_ADV_OPT_FILTER_CONN | BT_LE_ADV_OPT_FILTER_SCAN_REQ;
	}
	adv_param.interval_min = CGM_ADV_FAST_INTERVAL_MIN;
	adv_param.interval_max = CGM_ADV_FAST_INTERVAL_MAX;
	adv_param.peer = NULL;

	err = cgm_bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Fast-phase advertising start failed (%d)\n", err);
		return err;
	}

	k_delayed_work_submit(&cgm_adv_phase_work, K_SECONDS(CGM_ADV_FAST_PHASE_SECONDS));
	if (cgm_adv_whitelist_mode) {
		k_delayed_work_submit(&cgm_whitelist_wait_work,
				     K_SECONDS(CGM_ADV_WHITELIST_WAITING_SECONDS));
	}

	printk("Advertising started (fast phase, mode=%s)\n",
		cgm_adv_whitelist_mode ? "whitelist" : "allow-all");
	return 0;
}

/**
 * @brief 停止当前广播并取消广播相关定时器。
 *
 * @param 无。
 * @return 无返回值。
 */
static void cgm_stop_advertising(void)
{
	bt_le_adv_stop();
	k_delayed_work_cancel(&cgm_adv_phase_work);
	k_delayed_work_cancel(&cgm_whitelist_wait_work);
	printk("Advertising stopped\n");
}

/**
 * @brief 在连接建立后执行链路参数协商。
 *
 * @param 无。
 * @return 无返回值。
 *
 * @details
 * - 发起 MTU Exchange；
 * - 发起连接参数更新到模块偏好值；
 * - 当前实现保持与历史流程兼容，时序可在上层状态机中进一步细化。
 */
static void cgm_connection_update(void)
{
	uint8_t err;
	struct bt_le_conn_param cgm_preferred_param = {
		.interval_min = (800),
		.interval_max = (800),
		.latency = (0),
		.timeout = (400),
	};

	cgm_stop_advertising();
	if (exchange_params.func) 
	{
		bt_gatt_exchange_mtu(slave_conn, &exchange_params);
	}

	err = bt_conn_le_param_update(slave_conn, &cgm_preferred_param);
	if (err) 
	{
		printk("Conn param update failed(err %d)\n", err);
		return;
	}	
}

/**
 * @brief Legacy 数据通知入口（内部实现）。
 *
 * @param 无。
 * @return 无返回值。
 *
 * @details
 * 通过 bt_data_submit_handle() 触发旧私有服务的通知链路。
 */
static void cgm_conn_notify(void)
{
	bt_data_submit_handle(slave_conn, SUPER_TEST_HDL);
}

/**
 * @brief Legacy 兼容通知入口。
 *
 * @param 无。
 * @return 无返回值。
 */
void conn_notify(void)
{
	cgm_conn_notify();
}

/**
 * @brief 蓝牙连接建立回调。
 *
 * @param conn 当前连接对象。
 * @param err  连接结果，0 表示成功。
 * @return 无返回值。
 *
 * @details
 * 成功连接后会：
 * - 停止广播；
 * - 保存连接引用；
 * - 通知服务层 on_connected；
 * - 请求链路安全升级到 L2；
 * - 向系统上报 CONNECTED 事件。
 */
static void cgm_connected_cb(struct bt_conn *conn, uint8_t err)
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
		cgm_stop_advertising();
		slave_conn = bt_conn_ref(conn);
		ble_super_on_connected(conn);
		bt_conn_set_security(conn, BT_SECURITY_L2);

		cgm_app_to_msg(MSG_BLE_STATE, CONNECTED);

	}
	
}

/**
 * @brief 蓝牙连接断开回调。
 *
 * @param conn   断开的连接对象。
 * @param reason 断开原因码。
 * @return 无返回值。
 *
 * @details
 * 仅处理当前 active 连接；断开后会释放引用、取消传输并重新进入广播流程。
 */
static void cgm_disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != slave_conn)
	{
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);
	ble_super_on_disconnected(conn);
	bt_conn_unref(conn);
	slave_conn = NULL;
	
	bt_data_trans_cancel();
	cgm_start_advertising(cgm_bonded_peer_valid);
}

/**
 * @brief 链路安全级别变化回调。
 *
 * @param conn  当前连接对象。
 * @param level 新的安全级别。
 * @param err   安全建立错误码，0 表示成功。
 * @return 无返回值。
 *
 * @details
 * 当链路达到 L2 及以上时，记录当前对端地址并写入控制器白名单，
 * 用于后续 bonded 重连优化。
 */
static void cgm_security_changed_cb(struct bt_conn *conn,
				    bt_security_t level,
				    enum bt_security_err err)
{
	const bt_addr_le_t *peer;

	if (err) {
		printk("Security failed(level=%u err=%d)\n", level, err);
		return;
	}

	if (level < BT_SECURITY_L2) {
		return;
	}

	peer = bt_conn_get_dst(conn);
	if (!peer) {
		return;
	}

	memcpy(&cgm_bonded_peer, peer, sizeof(cgm_bonded_peer));
	cgm_bonded_peer_valid = true;

	bt_le_whitelist_clear();
	bt_le_whitelist_add(&cgm_bonded_peer);

	printk("Security ready, peer stored to whitelist\n");
}

/**
 * @brief CGM 风格 BLE 事件分发入口。
 *
 * @param event 事件码（START_ADV / STOP_ADV / CONNECTED）。
 * @return 无返回值。
 *
 * @details
 * 按事件类型路由到对应动作函数，是 main 线程与 BLE 子模块之间的统一入口。
 */
void cgm_system_ble_event_handle(uint32_t event)
{
	switch(event)
	{
		case START_ADV:
			cgm_start_advertising(cgm_bonded_peer_valid);
			break;
		case STOP_ADV:
			cgm_stop_advertising();
			break;
		case CONNECTED:
			cgm_connection_update();
			bt_data_submit_handle(slave_conn, SUPER_TEST_HDL);
			break;
		default:
		printk("unkown ble_event code\n");
		break;
	}
}

/**
 * @brief 旧接口兼容包装。
 *
 * @param event 事件码。
 * @return 无返回值。
 */
void system_ble_event_handle(uint32_t event)
{
	cgm_system_ble_event_handle(event);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = cgm_connected_cb,
	.disconnected = cgm_disconnected_cb,
	.le_param_updated = cgm_le_param_updated_cb,
};

/**
 * @brief CGM 风格 BLE 模块初始化入口。
 *
 * @param 无。
 * @return 无返回值。
 *
 * @details
 * 初始化步骤：
 * 1) 初始化广播阶段/白名单等待定时器；
 * 2) 注册连接回调；
 * 3) 打开 bondable 模式；
 * 4) 初始化服务层与数据传输层；
 * 5) 启动首次广播。
 */
void cgm_bt_le_op_init(void)
{
	k_delayed_work_init(&cgm_adv_phase_work, cgm_adv_phase_switch_cb);
	k_delayed_work_init(&cgm_whitelist_wait_work, cgm_whitelist_wait_timeout_cb);

	bt_conn_cb_register((struct bt_conn_cb *)&conn_callbacks);
	bt_set_bondable(true);

	ble_super_service_init();
	bt_data_trans_init();

	cgm_start_advertising(false);
}

/**
 * @brief 旧接口兼容初始化入口。
 *
 * @param 无。
 * @return 无返回值。
 */
void bt_le_op_init(void)
{
	cgm_bt_le_op_init();
}


