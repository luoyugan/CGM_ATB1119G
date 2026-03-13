#ifndef BT_LE_OP_H
#define BT_LE_OP_H

#include <zephyr.h>
#include "msg_manager.h"
#include <device.h>
#include <drivers/input/input_dev.h>

void system_ble_event_handle(uint32_t event);
void system_input_event_handle(uint32_t key_event);
void system_input_handle_init(void);

void bt_le_op_init(void);
void app_to_msg(uint8_t type, uint8_t event);

enum
{
	START_ADV,
	STOP_ADV,
	CONNECTED,
};
#endif /* BLE_SUPER_SERVICE_H */

