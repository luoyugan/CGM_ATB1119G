#include <errno.h>
#include <string.h>

#include <zephyr.h>
#include <bluetooth/gatt.h>
#include <bluetooth/uuid.h>

#include "atb_ble_std_services.h"

#ifndef ENOTCONN
#define ENOTCONN 57
#endif

#define ATB_DIS_MANUFACTURER "Actions"
#define ATB_DIS_MODEL        "ATB1119G-CGMS"
#define ATB_DIS_SERIAL       "ATB1119G-0001"
#define ATB_DIS_FW_REV       "CGM-1.0.0"

static uint8_t m_battery_level = 100U;

static void atb_bas_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);
	printk("BAS notification %s\n", (value == BT_GATT_CCC_NOTIFY) ? "enabled" : "disabled");
}

static ssize_t atb_bas_read_level(struct bt_conn *conn,
	const struct bt_gatt_attr *attr, void *buf,
	uint16_t len, uint16_t offset)
{
	uint8_t level = m_battery_level;

	ARG_UNUSED(attr);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &level, sizeof(level));
}

static ssize_t atb_dis_read_str(struct bt_conn *conn,
	const struct bt_gatt_attr *attr, void *buf,
	uint16_t len, uint16_t offset)
{
	const char *value = (const char *)attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value, strlen(value));
}

BT_GATT_SERVICE_DEFINE(atb_bas_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
	BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
		BT_GATT_PERM_READ,
		atb_bas_read_level, NULL, &m_battery_level),
	BT_GATT_CCC(atb_bas_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
);

BT_GATT_SERVICE_DEFINE(atb_dis_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_DIS),
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MANUFACTURER_NAME,
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		atb_dis_read_str, NULL, ATB_DIS_MANUFACTURER),
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MODEL_NUMBER,
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		atb_dis_read_str, NULL, ATB_DIS_MODEL),
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_SERIAL_NUMBER,
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		atb_dis_read_str, NULL, ATB_DIS_SERIAL),
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_FIRMWARE_REVISION,
		BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ,
		atb_dis_read_str, NULL, ATB_DIS_FW_REV)
);

void atb_std_services_init(void)
{
	m_battery_level = 100U;
}

uint8_t atb_bas_get_level(void)
{
	return m_battery_level;
}

int atb_bas_set_level(uint8_t level)
{
	int err;

	if (level > 100U) {
		return -EINVAL;
	}

	m_battery_level = level;
	err = bt_gatt_notify(NULL, &atb_bas_svc.attrs[1], &m_battery_level, sizeof(m_battery_level));
	return (err == -ENOTCONN) ? 0 : err;
}
