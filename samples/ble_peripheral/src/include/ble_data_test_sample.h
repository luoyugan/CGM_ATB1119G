/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef BLE_DATA_TEST_SAMPLE_H
#define BLE_DATA_TEST_SAMPLE_H

#include <zephyr.h>

void update_write_stats(uint16_t len);
uint8_t slave_notify_work(struct bt_conn *conn);

void bt_data_trans_init();
void bt_data_submit_handle(struct bt_conn *conn, uint16_t index);
void bt_data_trans_cancel(void);


#endif /* BLE_DATA_TEST_SAMPLE_H */
