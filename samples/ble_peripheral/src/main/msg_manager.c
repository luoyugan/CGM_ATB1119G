/*Copyright (c) 2018 Actions (Zhuhai) Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <kernel.h>
#include <stdio.h>
#include <string.h>
#include <sys/printk.h>
#include <net/buf.h>
#include "msg_manager.h"


#include "sys_log.h"
LOG_MODULE_REGISTER(msg, CONFIG_LOG_DEFAULT_LEVEL);

static K_FIFO_DEFINE(msg_queue);

NET_BUF_POOL_INIT_DEFINE(msg_buffer_pool_async, 8, sizeof(struct app_msg), NULL);
NET_BUF_POOL_INIT_DEFINE(msg_buffer_pool_sync, 8, sizeof(struct app_msg), NULL);

/*init manager*/
bool msg_manager_init(void)
{
	return true;
}

static struct net_buf *msg_buf_get(k_timeout_t timeout)
{
	struct net_buf *buf = NULL;

	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		buf = net_buf_alloc(&msg_buffer_pool_async, timeout);
	} else {
		buf = net_buf_alloc(&msg_buffer_pool_sync, timeout);
	}

	return buf;
}

bool send_msg(struct app_msg *msg, k_timeout_t timeout)
{
	struct net_buf *buf;

	/* alloc msg buf */
	buf = msg_buf_get(timeout);
	if (buf == NULL) {
		SYS_LOG_ERR("drop msg type: %d", msg->type);
		return false;
	}

	/* copy msg to send buffer */
	memcpy(buf->data, msg, sizeof(struct app_msg));

	/* send message containing most current data and loop around */
	net_buf_put(&msg_queue, buf);

	return true;
}

bool receive_msg(struct app_msg *msg, k_timeout_t timeout)
{
	struct net_buf *buf;

	/* get a data item, waiting as long as needed */
	buf = net_buf_get(&msg_queue, timeout);
	if (buf == NULL)
		return false;

	/* copy msg from recvied buffer */
	memcpy(msg, buf->data, sizeof(struct app_msg));

	/* free msg buf */
	net_buf_unref(buf);
	return true;
}
