/*
 * Copyright (c) 2018 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MSG_MANAGER_H__
#define __MSG_MANAGER_H__

#define MSG_CONTENT_SIZE 4

/* msg type */
enum {
	MSG_NULL,

	MSG_KEY_INPUT,
	MSG_LOW_POWER,

	MSG_BLE_STATE,
	MSG_AUDIO_INPUT,
	MSG_APP_TIMER,
	MSG_OTA_EVENT,
	MSG_ATVV_EVENT,
	MSG_HID_OUTPUT_EVENT,
};

/**
 * @defgroup msg_manager_apis Message Manager APIs
 * @ingroup lib_system_apis
 * @{
 */
struct app_msg;
typedef void (*MSG_CALLBAK)(struct app_msg *, int, void *);

/** message structure */
struct app_msg {
	/** who send this message */
	uint8_t sender;
	/** message type ,keyword of message*/
	uint8_t type;
	/** message cmd , used to send opration type*/
	uint8_t cmd;
	/** resrved byte*/
	uint8_t reserve;
	/** user data , to transmission some user info*/
	union {
		char content[MSG_CONTENT_SIZE];
		int value;
		void *ptr;
	};
	/** message callback*/
	MSG_CALLBAK callback;
};

/**
 * @brief msg_manager_init
 *
 * This routine msg_manager_init
 *
 * @return true  init success
 * @return false init failed
 */
bool msg_manager_init(void);

/**
 * @brief Send a Asynchronous message
 *
 * This routine Send a Asynchronous message
 *
 * @param receiver name of message receiver
 * @param msg store the received message
 *
 * @return true send success
 * @return false send failed
 */
bool send_msg(struct app_msg *msg, k_timeout_t timeout);


/**
 * @brief receive message
 *
 * This routine receive msg from others
 *
 * @param msg store the received message
 * @param timeout time out of receive message
 *
 * @return NULL The listener is not in the message linsener list
 * @return tid  target thread id of the message linsener
 */

bool receive_msg(struct app_msg *msg, k_timeout_t timeout);

/**
 * @} end defgroup msg_manager_apis
 */

#endif
