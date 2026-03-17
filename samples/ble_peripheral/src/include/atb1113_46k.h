/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file atb1113_46k.h
 *  @brief atb1113 46pin board config
 *  @note the board is suitable for hid or non_hid project
 */

#define CONFIG_BOARD_ATB1113_46K 1
#define CONFIG_BOARD "atb1113_46k"

/* app cfg */
//#define CONFIG_USE_AL_ENCODE_1 1
//#define CONFIG_USE_AL_ENCODE_3_4_1 1

/* led config */
#define LED_PIN_CONFIG			{18, NONE_PWM_CHAN, 0},

/* driver mfp config */
#define CONFIG_UART_0_MFP       gpio12_uart0_tx_node,gpio13_uart0_rx_node
#define CONFIG_UART_1_MFP		gpio4_uart1_tx_node,gpio5_uart1_rx_node
#define CONFIG_UART_2_MFP		gpio0_uart2_rx_node,gpio1_uart2_tx_node
#define CONFIG_PWM_MFP       	gpio9_pwm_irtx_node
#define CONFIG_CAPTURE_MFP      gpio9_ir_capture0_node
#define CONFIG_KEYPAD_MFP       gpio0_keypad_key0_node,gpio1_keypad_key1_node,gpio21_keypad_key2_node,\
								gpio20_keypad_key3_node,gpio7_keypad_key7_node,gpio8_keypad_key8_node,\
								gpio22_keypad_key9_node,gpio6_keypad_key11_node,gpio2_keypad_key13_node,\
								gpio3_keypad_key14_node,gpio26_keypad_key15_node
#define CONFIG_AUDIO_ADC_0_MFP	gpio40_amic_micinp_node,gpio41_amic_micinn_node,\
								gpio42_amic_adcvref_node,gpio19_vmic_node
#define CONFIG_SPI_FLASH_MFP 	gpio14_spinor0_miso_node,gpio15_spinor0_cs_node,gpio16_spinor0_clk_node,\
								gpio17_spinor0_mosi_node,gpio10_spinor0_io2_node,gpio11_spinor0_io3_node

/* keypad config */
#define CONFIG_KEYPAD_PIN_MAP	{{0x2, 0, 0, 0, 0, 0, 0}, KEY_NUM0},\
								{{0x4, 0, 0, 0, 0, 0, 0}, KEY_NUM1},\
								{{0x40000, 0, 0, 0, 0, 0, 0}, KEY_NUM2},\
								{{0x8, 0, 0, 0, 0, 0, 0}, KEY_NUM3},\
\
								{{0x80, 0, 0, 0, 0, 0, 0}, KEY_NUM4},\
								{{0x800000, 0, 0, 0, 0, 0, 0}, KEY_NUM5},\
								{{0, 0x80, 0, 0, 0, 0, 0}, KEY_NUM6},\
								{{0, 0x800000, 0, 0, 0, 0, 0}, KEY_NUM7},\
\
								{{0x80000, 0, 0, 0, 0, 0, 0}, KEY_NUM8},\
								{{0, 0x8, 0, 0, 0, 0, 0}, KEY_NUM9},\
								{{0x100, 0, 0, 0, 0, 0, 0}, KEY_NUM10},\
								{{0x1000000, 0, 0, 0, 0, 0, 0}, KEY_NUM11},\
\
								{{0, 0x100, 0, 0, 0, 0, 0}, KEY_NUM12},\
								{{0, 0x1000000, 0, 0, 0, 0, 0}, KEY_NUM13},\
								{{0, 0, 0, 0x1000000, 0, 0, 0}, KEY_NUM14},\
								{{0, 0, 0, 0, 0, 0x4000, 0}, KEY_NUM15},\
\
								{{0x200, 0, 0, 0, 0, 0, 0}, KEY_NUM16},\
								{{0x2000000, 0, 0, 0, 0, 0, 0}, KEY_NUM17},\
								{{0, 0x200, 0, 0, 0, 0, 0}, KEY_NUM18},\
								{{0, 0x2000000, 0, 0, 0, 0, 0}, KEY_NUM19},\
\
								{{0, 0, 0, 0x2000000, 0, 0, 0}, KEY_NUM20},\
								{{0, 0, 0, 0, 0x2, 0, 0}, KEY_NUM21},\
								{{0x800, 0, 0, 0, 0, 0, 0}, KEY_NUM22},\
								{{0x8000000, 0, 0, 0, 0, 0, 0}, KEY_NUM23},\
\
								{{0, 0x800, 0, 0, 0, 0, 0}, KEY_NUM24},\
								{{0, 0x8000000, 0, 0, 0, 0, 0}, KEY_NUM25},\
								{{0, 0, 0, 0x8000000, 0, 0, 0}, KEY_NUM26},\
								{{0, 0, 0, 0, 0x8, 0, 0}, KEY_NUM27},\
\
								{{0x2000, 0, 0, 0, 0, 0, 0}, KEY_NUM28},\
								{{0x20000000, 0, 0, 0, 0, 0, 0}, KEY_NUM29},\
								{{0, 0x2000, 0, 0, 0, 0, 0}, KEY_NUM30},\
								{{0, 0x20000000, 0, 0, 0, 0, 0}, KEY_NUM31},\
\
								{{0, 0, 0, 0x20000000, 0, 0, 0}, KEY_NUM32},\
								{{0, 0, 0, 0, 0x20, 0, 0}, KEY_NUM33},\
								{{0x4000, 0, 0, 0, 0, 0, 0}, KEY_NUM34},\
								{{0x40000000, 0, 0, 0, 0, 0, 0}, KEY_NUM35},\
\
								{{0, 0x4000, 0, 0, 0, 0, 0}, KEY_NUM36},\
								{{0, 0x40000000, 0, 0, 0, 0, 0}, KEY_NUM37},\
								{{0, 0, 0, 0x40000000, 0, 0, 0}, KEY_NUM38},\
								{{0, 0, 0, 0, 0x40, 0, 0}, KEY_NUM39},\
\
								{{0x8000, 0, 0, 0, 0, 0, 0}, KEY_NUM40},\
								{{0x80000000, 0, 0, 0, 0, 0, 0}, KEY_NUM41},\
								{{0, 0x8000, 0, 0, 0, 0, 0}, KEY_NUM42},\
								{{0, 0x80000000, 0, 0, 0, 0, 0}, KEY_NUM43},\
\
								{{0, 0, 0, 0x80000000, 0, 0, 0}, KEY_NUM44},\
								{{0, 0, 0, 0, 0x80, 0, 0}, KEY_NUM45},\
\
								{{0, 0x1000080, 0, 0, 0, 0, 0}, KEY_NUM46},\
								{{0x40006, 0, 0, 0, 0, 0, 0}, KEY_NUM47},\
								{{0x8000a, 0, 0, 0, 0, 0, 0}, KEY_NUM48},

/* key val mapping to standard key */
#define KEY_MAPS \
	{KEY_NUM0,		REMOTE_KEY_OK,				0x10,		IR_uPD6121}, \
	{KEY_NUM1,		REMOTE_KEY_HOME,			0x42,		IR_uPD6121}, \
	{KEY_NUM2,		REMOTE_KEY_FIVE,			0x05,		IR_uPD6121}, \
	{KEY_NUM3,		REMOTE_KEY_CLOSE_CAPTION,	0x4b,		IR_uPD6121}, \
	{KEY_NUM4,		REMOTE_KEY_SIX,				0x06,		IR_uPD6121}, \
	{KEY_NUM5,		REMOTE_KEY_LEFT,			0x0f,		IR_uPD6121}, \
	{KEY_NUM6,		REMOTE_KEY_VOL_DEC,			0x1a,		IR_uPD6121}, \
	{KEY_NUM7,		REMOTE_KEY_BLUE,			0x50,		IR_uPD6121}, \
	{KEY_NUM8,		REMOTE_KEY_WEEKLY,			0x48,		IR_uPD6121}, \
	{KEY_NUM9,		REMOTE_KEY_TWO,				0x02,		IR_uPD6121}, \
	{KEY_NUM10,		REMOTE_KEY_AC_UNDO,			0x0a,		IR_uPD6121}, \
	{KEY_NUM11,		REMOTE_KEY_AC_EXIT,			0x11,		IR_uPD6121}, \
	{KEY_NUM12,		REMOTE_KEY_NETFLIX,			0x4f,		IR_uPD6121}, \
	{KEY_NUM13,		REMOTE_KEY_BACK,			0x13,		IR_uPD6121}, \
	{KEY_NUM14,		REMOTE_KEY_NINE,			0x09,		IR_uPD6121}, \
	{KEY_NUM15,		REMOTE_KEY_NEXT_TRACK,		0x44,		IR_uPD6121}, \
	{KEY_NUM16,		REMOTE_KEY_VOICE_COMMAND,	0x4d,		KEY_STATUS_RESERVED}, \
	{KEY_NUM17,		REMOTE_KEY_VOL_INC,			0x1b,		IR_uPD6121}, \
	{KEY_NUM18,		REMOTE_KEY_REWIND,			0x47,		IR_uPD6121}, \
	{KEY_NUM19,		REMOTE_KEY_ONE,				0x01,		IR_uPD6121}, \
	{KEY_NUM20,		REMOTE_KEY_HELP,			0x0b,		IR_uPD6121}, \
	{KEY_NUM21,		REMOTE_KEY_QUIT,			0x12,		IR_uPD6121}, \
	{KEY_NUM22,		REMOTE_KEY_UP,				0x0c,		IR_uPD6121}, \
	{KEY_NUM23,		REMOTE_KEY_MENU,			0x17,		IR_uPD6121}, \
	{KEY_NUM24,		REMOTE_KEY_YOUTUBE,			0x4e,		IR_uPD6121}, \
	{KEY_NUM25,		REMOTE_KEY_CHAN_INC,		0x19,		IR_uPD6121}, \
	{KEY_NUM26,		REMOTE_KEY_MUTE,			0x16,		IR_uPD6121}, \
	{KEY_NUM27,		REMOTE_KEY_RECORD,			0x40,		IR_uPD6121}, \
	{KEY_NUM28,		REMOTE_KEY_PLAY,			0x41,		IR_uPD6121}, \
	{KEY_NUM29,		REMOTE_KEY_FOUR,			0x04,		IR_uPD6121}, \
	{KEY_NUM30,		REMOTE_KEY_PAUSE,			0xb1,		IR_uPD6121}, \
	{KEY_NUM31,		REMOTE_KEY_STOP,			0x4b,		IR_uPD6121}, \
	{KEY_NUM32,		REMOTE_KEY_CHAN_DEC,		0x18,		IR_uPD6121}, \
	{KEY_NUM33,		REMOTE_KEY_RIGHT,			0x0e,		IR_uPD6121}, \
	{KEY_NUM34,		REMOTE_KEY_PREVIOUS_TRACK,	0x45,		IR_uPD6121}, \
	{KEY_NUM35,		REMOTE_KEY_DATA_ON_SCREEN,	0x14,		IR_uPD6121}, \
	{KEY_NUM36,		REMOTE_KEY_EIGHT,			0x7a,		IR_uPD6121}, \
	{KEY_NUM37,		REMOTE_KEY_POWER,			0x15,		IR_uPD6121}, \
	{KEY_NUM38,		REMOTE_KEY_ZERO,			0x00,		IR_uPD6121}, \
	{KEY_NUM39,		REMOTE_KEY_SEVEN,			0x07,		IR_uPD6121}, \
	{KEY_NUM40,		REMOTE_KEY_DOWN,			0x0d,		IR_uPD6121}, \
	{KEY_NUM41,		REMOTE_KEY_RED,				0x53,		IR_uPD6121}, \
	{KEY_NUM42,		REMOTE_KEY_GREEN,			0x52,		IR_uPD6121}, \
	{KEY_NUM43,		REMOTE_KEY_YELLOW,			0x51,		IR_uPD6121}, \
	{KEY_NUM44,		REMOTE_KEY_FAST_FORWARD,	0x46,		IR_uPD6121}, \
	{KEY_NUM45,		REMOTE_USAGE_NONE,			0x23,		IR_uPD6121}, \
	{KEY_NUM46,		REMOTE_COMB_KEY_OK_BACK,	0xffff,		KEY_STATUS_RESERVED},\
	{KEY_NUM47,		REMOTE_COMB_KEY_BQB_MODE,	0xffff,		KEY_STATUS_RESERVED},\
	{KEY_NUM48,		REMOTE_COMB_KEY_ST_MODE,	0xffff,		KEY_STATUS_RESERVED},

/* key val mapping to standard learn key */
#define LEARN_KEY_MAPS \
	{KEY_NUM41,		REMOTE_KEY_RED,				0xffff,		0xff,		KEY_STATUS_RESERVED}, \
	{KEY_NUM42,		REMOTE_KEY_GREEN,			0xffff,		0xff,		KEY_STATUS_RESERVED}, \
	{KEY_NUM43,		REMOTE_KEY_YELLOW,			0xffff,		0xff,		KEY_STATUS_RESERVED},

/* key status */
#define KEY_STATUS \
	{KEY_NUM0,		KEY_STATUS_UP}, \
	{KEY_NUM1,		KEY_STATUS_UP}, \
	{KEY_NUM2,		KEY_STATUS_UP}, \
	{KEY_NUM3,		KEY_STATUS_UP}, \
	{KEY_NUM4,		KEY_STATUS_UP}, \
	{KEY_NUM5,		KEY_STATUS_UP}, \
	{KEY_NUM6,		KEY_STATUS_UP}, \
	{KEY_NUM7,		KEY_STATUS_UP}, \
	{KEY_NUM8,		KEY_STATUS_UP}, \
	{KEY_NUM9,		KEY_STATUS_UP}, \
	{KEY_NUM10,		KEY_STATUS_UP}, \
	{KEY_NUM11,		KEY_STATUS_UP}, \
	{KEY_NUM12,		KEY_STATUS_UP}, \
	{KEY_NUM13,		KEY_STATUS_UP}, \
	{KEY_NUM14,		KEY_STATUS_UP}, \
	{KEY_NUM15,		KEY_STATUS_UP}, \
	{KEY_NUM16,		KEY_STATUS_UP}, \
	{KEY_NUM17,		KEY_STATUS_UP}, \
	{KEY_NUM18,		KEY_STATUS_UP}, \
	{KEY_NUM19,		KEY_STATUS_UP}, \
	{KEY_NUM20,		KEY_STATUS_UP}, \
	{KEY_NUM21,		KEY_STATUS_UP}, \
	{KEY_NUM22,		KEY_STATUS_UP}, \
	{KEY_NUM23,		KEY_STATUS_UP}, \
	{KEY_NUM24,		KEY_STATUS_UP}, \
	{KEY_NUM25,		KEY_STATUS_UP}, \
	{KEY_NUM26,		KEY_STATUS_UP}, \
	{KEY_NUM27,		KEY_STATUS_UP}, \
	{KEY_NUM28,		KEY_STATUS_UP}, \
	{KEY_NUM29,		KEY_STATUS_UP}, \
	{KEY_NUM30,		KEY_STATUS_UP}, \
	{KEY_NUM31,		KEY_STATUS_UP}, \
	{KEY_NUM32,		KEY_STATUS_UP}, \
	{KEY_NUM33,		KEY_STATUS_UP}, \
	{KEY_NUM34,		KEY_STATUS_UP}, \
	{KEY_NUM35,		KEY_STATUS_UP}, \
	{KEY_NUM36,		KEY_STATUS_UP}, \
	{KEY_NUM37,		KEY_STATUS_UP}, \
	{KEY_NUM38,		KEY_STATUS_UP}, \
	{KEY_NUM39,		KEY_STATUS_UP}, \
	{KEY_NUM40,		KEY_STATUS_UP}, \
	{KEY_NUM41,		KEY_STATUS_UP}, \
	{KEY_NUM42,		KEY_STATUS_UP}, \
	{KEY_NUM43,		KEY_STATUS_UP}, \
	{KEY_NUM44,		KEY_STATUS_UP}, \
	{KEY_NUM45,		KEY_STATUS_UP}, \
	{KEY_NUM46,		KEY_STATUS_UP}, \
	{KEY_NUM47,		KEY_STATUS_UP}, \
	{KEY_NUM48,		KEY_STATUS_UP},
