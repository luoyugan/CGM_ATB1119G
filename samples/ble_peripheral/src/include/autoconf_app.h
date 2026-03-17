/* battery cap */
#define BOARD_BATTERY_CAP_MAPPINGS      \
	{0, 1800},  \
	{5, 1950},  \
	{10, 2000}, \
	{20, 2150}, \
	{30, 2300}, \
	{40, 2450}, \
	{50, 2600}, \
	{60, 2750}, \
	{70, 2900}, \
	{80, 3050}, \
	{90, 3150}, \
	{100, 3250},

/* app config */
#define CONFIG_HID_VOICE_REPORT_ID 5
#define CONFIG_MAX_RECONN_TIMES 3
#define CONFIG_BT_DEVICE_NAME_1 "BLE_RMC_TEST_1"
#define CONFIG_BT_DEVICE_NAME_2 "BLE_RMC_TEST_2"
#define CONFIG_BT_DEVICE_NAME_3 "BLE_RMC_TEST_3"
#define CONFIG_BT_DEVICE_NAME_4 "BLE_RMC_TEST_4"

//#define CONFIG_OTA_WITH_APP 1

/* Add ack mechanism in OTA profile.
   The master sends a data (200bytes), and then waits for the slave to ack. 
	 After receiving the ACK, the master will continue to send the next data. 
*/	
//#define CONFIG_OTA_SYNC_MODE 1

#ifndef CONFIG_OTA_SYNC_MODE
/* By default, OTA will write to norflash immediately after receiving a data (20bytes) from stack.
	When CONFIG_OTA_USE_DATA_BUF is enabled, the data received from the stack will be copied to a OTA_DATA_BUF first. 
	When the buf is full (1024bytes), a message will be sent to the main thread, and the data will be written to norflash in the main thread.
*/	
//#define CONFIG_OTA_USE_DATA_BUF 1
//#define CONFIG_OTA_DATA_BUF_COUNT 4
//#define CONFIG_OTA_DATA_BUF_LEN 1024
#endif

#define CONFIG_EXT_VANT 1

/* bluetooth config */
//#define CONFIG_BT_MIN_SIZE_BUF 1
//#define CONFIG_BT_SMP_RX_BUF_COUNT 2
//#define CONFIG_BT_MIN_RX_BUF_LEN 31
#define CONFIG_BT_USER_PHY_UPDATE 1
//#define CONFIG_BT_USER_DATA_LEN_UPDATE 1

#undef CONFIG_BT_GAP_AUTO_UPDATE_CONN_PARAMS
#undef CONFIG_BT_PERIPHERAL_PREF_MIN_INT
#undef CONFIG_BT_PERIPHERAL_PREF_MAX_INT
#undef CONFIG_BT_PERIPHERAL_PREF_SLAVE_LATENCY
#undef CONFIG_BT_PERIPHERAL_PREF_TIMEOUT
#undef CONFIG_BT_CONN_PARAM_UPDATE_TIMEOUT
#define CONFIG_BT_PERIPHERAL_PREF_MIN_INT 10
#define CONFIG_BT_PERIPHERAL_PREF_MAX_INT 10
#define CONFIG_BT_PERIPHERAL_PREF_SLAVE_LATENCY 0
#define CONFIG_BT_PERIPHERAL_PREF_TIMEOUT 400
#define CONFIG_BT_CONN_PARAM_UPDATE_TIMEOUT 1000

#define CONFIG_BT_SETTINGS_CCC_STORE_ON_WRITE 1
//#define CONFIG_BT_AUTO_PHY_UPDATE 1

/* bqb config */

//#define CONFIG_BT_CTRL_ST 1
//#define CONFIG_BT_CTRL_BLE_BQB 1
#ifdef CONFIG_BT_CTRL_BLE_BQB
#define CONFIG_BQB_TX_RBUF_SIZE 128
#define CONFIG_BQB_RX_RBUF_SIZE 128
#define CONFIG_BLE_BQB_UART CONFIG_UART_0_NAME
#define CONFIG_BLE_BQB_UART_SPEED	115200
#endif

/* timer config */
#define CONFIG_RMC_ADV_TIMEOUT 30
#define CONFIG_RMC_DIRECT_ADV_TIMEOUT 2
#define CONFIG_RMC_NO_ACT_TIMEOUT (5*60)
#define CONFIG_RMC_ADC_BATTERY_TIMEOUT (3*60)
#define CONFIG_RMC_PAIR_COMB_KEY_TIMEOUT 3
#define CONFIG_RMC_HCI_MODE_COMB_KEY_TIMEOUT 8
#define CONFIG_RMC_BQB_MODE_COMB_KEY_TIMEOUT 3
#define CONFIG_RMC_SINGLE_TONE_COMB_KEY_TIMEOUT 3
#define CONFIG_RMC_SEC_TIMEOUT 30
#define CONFIG_RMC_LEARN_KEY_TIMEOUT 5
#define CONFIG_RMC_NO_ACTION_TIMEOUT 15
#ifdef CONFIG_ENABLE_MIC_HID_OUPUT_CMD
#define CONFIG_RMC_OPEN_MIC_TIMEOUT 30
#endif

/* dis config */
#define CONFIG_DIS_MANUFACTURER_NAME "PT corp."
#define CONFIG_DIS_MODEL "ATB111x"
#define CONFIG_DIS_PNP_COMPANY_ID_TYPE 0x02
#define CONFIG_DIS_PNP_VENDOR_ID 0x2B54
#define CONFIG_DIS_PNP_PRODUCT_ID 0x1600
#define CONFIG_DIS_PNP_PRODUCT_VERSION 0x0000

/* led config */
//#define CONFIG_USE_PWM_LED 1
#define CONFIG_USE_GPIO_LED 1

/* IR config */
#define IRP_NEC_USER_CODE 0x00fe
#define IRP_9012_USER_CODE 0x55aa
#define IRP_RC6_USER_CODE 0x55aa
#define IRP_50462_USER_CODE 0x55aa
#define IRP_50560_USER_CODE 0x55aa
#define IRP_RC5X_USER_CODE 0x55aa
#define IRP_7461_USER_CODE 0x55aa
#define IRP_3004_USER_CODE 0x55aa
#define IRP_RCA_USER_CODE 0x55aa
#define IRP_RC5_USER_CODE 0x55aa
#define IR_RX_ANA_DEFAULT_PARAMA 0x1e954a01
#define IR_RX_ANA_NEAR_PARAMA 0x1ec64a01
#define IR_RX_ANA_FAR_PARAMA 0x1ec44a01

/* keypad stuck key config */
//#define CONFIG_KEYPAD_SUPPORT_PRO_STUCK_KEY 1
#define CONFIG_STUCK_KEY_LONG_TIMEOUT 60
#define CONFIG_STUCK_KEY_SHORT_TIME_OUT 1
#define CONFIG_GPIO_LED_PIN 18
#define CONFIG_GPIO_LED_OFF 1

/* flash write protect enable */
//#define CONFIG_NOR_ACTS_DATA_PROTECTION_ENABLE 1

/* IR transmitting voltage threshold
 * Attintion: when the battery voltage is less than this value, no IR will be sent.
 */
#define CONFIG_IR_SEND_VOLTAGE_THRESHOLD 17 // 2.1v

/* temporary modify:because of board bug, TX gpio12 will affect gpio9 */
#define CONFIG_UART_TEMP_COVER_MFP 1

/* board config */
#include "atb1113_46k.h"
//#include "atb1113_46k_atv.h"
//#include "atb1113_16k_jmgo.h"
