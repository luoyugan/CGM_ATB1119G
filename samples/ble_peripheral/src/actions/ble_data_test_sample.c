/* CGMS port keeps the original sample entry points as thin wrappers. */

#include <sys/printk.h>
#include "ble_data_test_sample.h"
#include "ble_super_service.h"

void update_write_stats(uint16_t len)
{
	printk("cgms write rx: %u byte\n", len);
}

void bt_data_submit_handle(struct bt_conn *conn, uint16_t index)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(index);
}

void bt_data_trans_cancel(void)
{
}
