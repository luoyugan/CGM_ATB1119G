/*!
 * \file      driver_item.h
 * \brief     驱动配置项定义
 * \details   
 * \author    
 * \date      
 * \copyright Actions
 */


#ifndef ZEPHYR_INCLUDE_DRIVERS_DRIVER_ITEM_H_
#define ZEPHYR_INCLUDE_DRIVERS_DRIVER_ITEM_H_

#define Console_UART_DRV_ID 0x03
#define Console_UART_TX_Pin_INDEX 0x01
#define Console_UART_RX_Pin_INDEX 0x02
#define Console_UART_Baudrate_INDEX 0x03
#define Console_UART_Print_Time_Stamp_INDEX 0x04
#define LED_Drives_DRV_ID 0x11
#define LED_Drives_LED_INDEX 0x01
#define ONOFF_Key_DRV_ID 0x0A
#define ONOFF_Key_Use_Inner_ONOFF_Key_INDEX 0x01
#define ONOFF_Key_Continue_Key_Function_After_Wake_Up_INDEX 0x02
#define ONOFF_Key_Key_Value_INDEX 0x03
#define ONOFF_Key_Time_Press_Power_On_INDEX 0x04
#define ONOFF_Key_Time_Long_Press_Reset_INDEX 0x05
#define ONOFF_Key_Boot_Hold_Key_Func_INDEX 0x06
#define ONOFF_Key_Boot_Hold_Key_Time_Ms_INDEX 0x07
#define ONOFF_Key_Debounce_Time_Ms_INDEX 0x08
#define ONOFF_Key_Reboot_After_Boot_Hold_Key_Clear_Paired_List_INDEX 0x09
#define LRADC_Keys_DRV_ID 0x0B
#define LRADC_Keys_Key_INDEX 0x01
#define LRADC_Keys_LRADC_Ctrl_INDEX 0x02
#define LRADC_Keys_LRADC_Pull_Up_INDEX 0x03
#define LRADC_Keys_Use_LRADC_Key_Wake_Up_INDEX 0x04
#define LRADC_Keys_LRADC_Value_Test_INDEX 0x05
#define LRADC_Keys_Debounce_Time_Ms_INDEX 0x06
#define GPIO_Keys_DRV_ID 0x0C
#define GPIO_Keys_Key 0x01
#define TAP_KEY_DRV_ID 0x0D
#define TAP_KEY_Tap_Tap_Key_Control_INDEX 0x01
#define Audio_DRV_ID 0x18
#define Audio_Settings_Audio_Out_Mode_INDEX 0x01
#define Audio_Settings_I2STX_Select_GPIO_INDEX 0x02
#define Audio_Settings_Channel_Select_Mode_INDEX 0x03
#define Audio_Settings_Channel_Select_GPIO_INDEX 0x04
#define Audio_Settings_Channel_Select_LRADC_INDEX 0x05
#define Audio_Settings_TWS_Alone_Audio_Channel_INDEX 0x06
#define Audio_Settings_L_Speaker_Out_INDEX 0x07
#define Audio_Settings_R_Speaker_Out_INDEX 0x08
#define Audio_Settings_ADC_Bias_Setting_INDEX 0x09
#define Audio_Settings_DAC_Bias_Setting_INDEX 0x0A
#define Audio_Settings_Keep_DA_Enabled_When_Play_Pause_INDEX 0x0B
#define Audio_Settings_Disable_PA_When_Reconnect_INDEX 0x0C
#define Audio_Settings_Extern_PA_Control_INDEX 0x0D
#define Audio_Settings_Large_Noise_Optimize_Enable_INDEX 0x0E
#define Audio_Settings_AntiPOP_Process_Disable_INDEX 0x0F
#define Audio_Settings_DMIC_Channel_Aligning_INDEX 0x10
#define Audio_Settings_DMIC_Select_GPIO_INDEX 0x11
#define Audio_Settings_Enable_ANC_INDEX 0x12
#define Audio_Settings_ANCDMIC_Select_GPIO_INDEX 0x13
#define Audio_Settings_Record_Adc_Select_INDEX 0x14
#define Audio_Settings_Enable_VMIC_INDEX 0x15
#define Audio_Settings_Hw_Aec_Select_INDEX 0x16
#define Audio_Settings_Tm_Adc_Select_INDEX 0x17
#define Audio_Settings_Mic_Config_INDEX 0x18
#define Battery_Charge_DRV_ID 0x1E
#define Battery_Charge_Select_Charge_Mode_INDEX 0x01
#define Battery_Charge_Charge_Current_INDEX 0x02
#define Battery_Charge_Charge_Voltage_INDEX 0x03
#define Battery_Charge_Charge_Stop_Mode_INDEX 0x04
#define Battery_Charge_Charge_Stop_Voltage_INDEX 0x05
#define Battery_Charge_Charge_Stop_Current_INDEX 0x06
#define Battery_Charge_Precharge_Stop_Voltage_INDEX 0x07
#define Battery_Charge_Battery_Check_Period_Sec_INDEX 0x08
#define Battery_Charge_Charge_Check_Period_Sec_INDEX 0x09
#define Battery_Charge_Charge_Full_Continue_Sec_INDEX 0x0A
#define Battery_Charge_Front_Charge_Full_Power_Off_Wait_Sec_INDEX 0x0B
#define Battery_Charge_DC5V_Detect_Debounce_Time_Ms_INDEX 0x0C
#define Charger_Box_DRV_ID 0x1F
#define Charger_Box_Enable_Charger_Box_INDEX 0x01
#define Charger_Box_DC5V_Pull_Down_Current_INDEX 0x02
#define Charger_Box_DC5V_Pull_Down_Hold_Ms_INDEX 0x03
#define Charger_Box_Charger_Standby_Delay_Ms_INDEX 0x04
#define Charger_Box_Charger_Standby_Voltage_INDEX 0x05
#define Charger_Box_Charger_Wake_Delay_Ms_INDEX 0x06
#define Charger_Box_Enable_Battery_Recharge_INDEX 0x07
#define Charger_Box_Battery_Recharge_Threshold_INDEX 0x08
#define Charger_Box_Charger_Box_Standby_Current_INDEX 0x09
#define Charger_Box_DC5V_UART_Comm_Settings_INDEX 0x0A
#define Charger_Box_DC5V_IO_Comm_Settings_INDEX 0x0B

#define Battery_Level_DRV_ID 0x20
#define Battery_Level_Level_INDEX 0x01
#define Battery_Low_DRV_ID 0x21
#define Battery_Low_Battery_Too_Low_Voltage_INDEX 0x01
#define Battery_Low_Battery_Low_Voltage_INDEX 0x02
#define Battery_Low_Battery_Low_Voltage_Ex_INDEX 0x03
#define Battery_Low_Battery_Low_Prompt_Interval_Sec_INDEX 0x04


// 各个驱动具体的配置项ITEM
/* UART */
#define ITEM_Console_UART_TX_Pin			((Console_UART_DRV_ID<<16) | (Console_UART_TX_Pin_INDEX))
#define ITEM_Console_UART_RX_Pin			((Console_UART_DRV_ID<<16) | (Console_UART_RX_Pin_INDEX))
#define ITEM_Console_UART_Baudrate			((Console_UART_DRV_ID<<16) | (Console_UART_Baudrate_INDEX))
#define ITEM_Console_UART_Print_Time_Stamp	((Console_UART_DRV_ID<<16) | (Console_UART_Print_Time_Stamp_INDEX))
/* LED */
#define ITEM_LED_Drives_LED	((LED_Drives_DRV_ID<<16) | (LED_Drives_LED_INDEX))
/* ONOFF KEY */
#define ITEM_ONOFF_Key_Use_Inner_ONOFF_Key	((ONOFF_Key_DRV_ID<<16) | (ONOFF_Key_Use_Inner_ONOFF_Key_INDEX))
#define ITEM_ONOFF_Key_Continue_Key_Function_After_Wake_Up	((ONOFF_Key_DRV_ID<<16) | (ONOFF_Key_Continue_Key_Function_After_Wake_Up_INDEX))
#define ITEM_ONOFF_Key_Key_Value	((ONOFF_Key_DRV_ID<<16) | (ONOFF_Key_Key_Value_INDEX))
#define ITEM_ONOFF_Key_Time_Press_Power_On	((ONOFF_Key_DRV_ID<<16) | (ONOFF_Key_Time_Press_Power_On_INDEX))
#define ITEM_ONOFF_Key_Time_Long_Press_Reset	((ONOFF_Key_DRV_ID<<16) | (ONOFF_Key_Time_Long_Press_Reset_INDEX))
#define ITEM_ONOFF_Key_Boot_Hold_Key_Func	((ONOFF_Key_DRV_ID<<16) | (ONOFF_Key_Boot_Hold_Key_Func_INDEX))
#define ITEM_ONOFF_Key_Boot_Hold_Key_Time_Ms	((ONOFF_Key_DRV_ID<<16) | (ONOFF_Key_Boot_Hold_Key_Time_Ms_INDEX))
#define ITEM_ONOFF_Key_Debounce_Time_Ms	((ONOFF_Key_DRV_ID<<16) | (ONOFF_Key_Debounce_Time_Ms_INDEX))
#define ITEM_ONOFF_Key_Reboot_After_Boot_Hold_Key_Clear_Paired_List	((ONOFF_Key_DRV_ID<<16) | (ONOFF_Key_Reboot_After_Boot_Hold_Key_Clear_Paired_List_INDEX))
/* LRADC KEY */
#define ITEM_LRADC_Keys_Key	((LRADC_Keys_DRV_ID<<16) | (LRADC_Keys_Key_INDEX))
#define ITEM_LRADC_Keys_LRADC_Ctrl	((LRADC_Keys_DRV_ID<<16) | (LRADC_Keys_LRADC_Ctrl_INDEX))
#define ITEM_LRADC_Keys_LRADC_Pull_Up	((LRADC_Keys_DRV_ID<<16) | (LRADC_Keys_LRADC_Pull_Up_INDEX))
#define ITEM_LRADC_Keys_LRADC_Key_Wake_Up	((LRADC_Keys_DRV_ID<<16) | (LRADC_Keys_Use_LRADC_Key_Wake_Up_INDEX))
#define ITEM_LRADC_Keys_LRADC_Value_Test	((LRADC_Keys_DRV_ID<<16) | (LRADC_Keys_LRADC_Value_Test_INDEX))
#define ITEM_LRADC_Keys_Debounce_Time_Ms	((LRADC_Keys_DRV_ID<<16) | (LRADC_Keys_Debounce_Time_Ms_INDEX))
/* GPIO KEY */
#define ITEM_GPIO_Keys_Key	((GPIO_Keys_DRV_ID<<16) | (GPIO_Keys_Key))
/* TAP KEY */
#define ITEM_TAP_KEY_Tap_Key_Control	((TAP_KEY_DRV_ID<<16) | (TAP_KEY_Tap_Tap_Key_Control_INDEX))
/* AUDIO */
#define ITEM_Audio_Settings_Out_Mode	((Audio_DRV_ID<<16) | (Audio_Settings_Audio_Out_Mode_INDEX))
#define ITEM_Audio_Settings_I2STX_Select_GPIO	((Audio_DRV_ID<<16) | (Audio_Settings_I2STX_Select_GPIO_INDEX))
#define ITEM_Audio_Settings_Channel_Select_Mode	((Audio_DRV_ID<<16) | (Audio_Settings_Channel_Select_Mode_INDEX))
#define ITEM_Audio_Settings_Channel_Select_GPIO	((Audio_DRV_ID<<16) | (Audio_Settings_Channel_Select_GPIO_INDEX))
#define ITEM_Audio_Settings_Channel_Select_LRADC	((Audio_DRV_ID<<16) | (Audio_Settings_Channel_Select_LRADC_INDEX))
#define ITEM_Audio_Settings_TWS_Alone_Audio_Channel	((Audio_DRV_ID<<16) | (Audio_Settings_TWS_Alone_Audio_Channel_INDEX))
#define ITEM_Audio_Settings_L_Speaker_Out	((Audio_DRV_ID<<16) | (Audio_Settings_L_Speaker_Out_INDEX))
#define ITEM_Audio_Settings_R_Speaker_Out	((Audio_DRV_ID<<16) | (Audio_Settings_R_Speaker_Out_INDEX))
#define ITEM_Audio_Settings_ADC_Bias_Setting	((Audio_DRV_ID<<16) | (Audio_Settings_ADC_Bias_Setting_INDEX))
#define ITEM_Audio_Settings_DAC_Bias_Setting	((Audio_DRV_ID<<16) | (Audio_Settings_DAC_Bias_Setting_INDEX))
#define ITEM_Audio_Settings_Keep_DA_Enabled_When_Play_Pause	((Audio_DRV_ID<<16) | (Audio_Settings_Keep_DA_Enabled_When_Play_Pause_INDEX))
#define ITEM_Audio_Settings_Disable_PA_When_Reconnect	((Audio_DRV_ID<<16) | (Audio_Settings_Disable_PA_When_Reconnect_INDEX))
#define ITEM_Audio_Settings_Extern_PA_Control	((Audio_DRV_ID<<16) | (Audio_Settings_Extern_PA_Control_INDEX))
#define ITEM_Audio_Settings_Large_Noise_Optimize_Enable	((Audio_DRV_ID<<16) | (Audio_Settings_Large_Noise_Optimize_Enable_INDEX))
#define ITEM_Audio_Settings_AntiPOP_Process_Disable	((Audio_DRV_ID<<16) | (Audio_Settings_AntiPOP_Process_Disable_INDEX))
#define ITEM_Audio_Settings_DMIC_Channel_Aligning	((Audio_DRV_ID<<16) | (Audio_Settings_DMIC_Channel_Aligning_INDEX))
#define ITEM_Audio_Settings_DMIC_Select_GPIO	((Audio_DRV_ID<<16) | (Audio_Settings_DMIC_Select_GPIO_INDEX))
#define ITEM_Audio_Settings_Enable_ANC	((Audio_DRV_ID<<16) | (Audio_Settings_Enable_ANC_INDEX))
#define ITEM_Audio_Settings_ANCDMIC_Select_GPIO	((Audio_DRV_ID<<16) | (Audio_Settings_ANCDMIC_Select_GPIO_INDEX))
#define ITEM_Audio_Settings_Record_Adc_Select	((Audio_DRV_ID<<16) | (Audio_Settings_Record_Adc_Select_INDEX))
#define ITEM_Audio_Settings_Enable_VMIC	((Audio_DRV_ID<<16) | (Audio_Settings_Enable_VMIC_INDEX))
#define ITEM_Audio_Settings_Hw_Aec_Select	((Audio_DRV_ID<<16) | (Audio_Settings_Hw_Aec_Select_INDEX))
#define ITEM_Audio_Settings_Tm_Adc_Select	((Audio_DRV_ID<<16) | (Audio_Settings_Tm_Adc_Select_INDEX))
#define ITEM_Audio_Settings_Mic_Config	((Audio_DRV_ID<<16) | (Audio_Settings_Mic_Config_INDEX))
/* BATTERY */
#define ITEM_Battery_Charge_Select_Charge_Mode	((Battery_Charge_DRV_ID<<16) | (Battery_Charge_Select_Charge_Mode_INDEX))
#define ITEM_Battery_Charge_Charge_Current	((Battery_Charge_DRV_ID<<16) | (Battery_Charge_Charge_Current_INDEX))
#define ITEM_Battery_Charge_Charge_Voltage	((Battery_Charge_DRV_ID<<16) | (Battery_Charge_Charge_Voltage_INDEX))
#define ITEM_Battery_Charge_Charge_Stop_Mode	((Battery_Charge_DRV_ID<<16) | (Battery_Charge_Charge_Stop_Mode_INDEX))
#define ITEM_Battery_Charge_Charge_Stop_Voltage	((Battery_Charge_DRV_ID<<16) | (Battery_Charge_Charge_Stop_Voltage_INDEX))
#define ITEM_Battery_Charge_Charge_Stop_Current	((Battery_Charge_DRV_ID<<16) | (Battery_Charge_Charge_Stop_Current_INDEX))
#define ITEM_Battery_Charge_Precharge_Stop_Voltage ((Battery_Charge_DRV_ID<<16) | (Battery_Charge_Precharge_Stop_Voltage_INDEX))
#define ITEM_Battery_Charge_Battery_Check_Period_Sec	((Battery_Charge_DRV_ID<<16) | (Battery_Charge_Battery_Check_Period_Sec_INDEX))
#define ITEM_Battery_Charge_Charge_Check_Period_Sec	((Battery_Charge_DRV_ID<<16) | (Battery_Charge_Charge_Check_Period_Sec_INDEX))
#define ITEM_Battery_Charge_Charge_Full_Continue_Sec	((Battery_Charge_DRV_ID<<16) | (Battery_Charge_Charge_Full_Continue_Sec_INDEX))
#define ITEM_Battery_Charge_Front_Charge_Full_Power_Off_Wait_Sec	((Battery_Charge_DRV_ID<<16) | (Battery_Charge_Front_Charge_Full_Power_Off_Wait_Sec_INDEX))
#define ITEM_Battery_Charge_DC5V_Detect_Debounce_Time_Ms	((Battery_Charge_DRV_ID<<16) | (Battery_Charge_DC5V_Detect_Debounce_Time_Ms_INDEX))
/* CHARGER BOX */
#define ITEM_Charger_Box_Enable_Charger_Box ((Charger_Box_DRV_ID<<16) | (Charger_Box_Enable_Charger_Box_INDEX))
#define ITEM_Charger_Box_DC5V_Pull_Down_Current ((Charger_Box_DRV_ID<<16) | (Charger_Box_DC5V_Pull_Down_Current_INDEX))
#define ITEM_Charger_Box_DC5V_Pull_Down_Hold_Ms ((Charger_Box_DRV_ID<<16) | (Charger_Box_DC5V_Pull_Down_Hold_Ms_INDEX))
#define ITEM_Charger_Box_Charger_Standby_Delay_Ms ((Charger_Box_DRV_ID<<16) | (Charger_Box_Charger_Standby_Delay_Ms_INDEX))
#define ITEM_Charger_Box_Charger_Standby_Voltage ((Charger_Box_DRV_ID<<16) | (Charger_Box_Charger_Standby_Voltage_INDEX))
#define ITEM_Charger_Box_Charger_Wake_Delay_Ms ((Charger_Box_DRV_ID<<16) | (Charger_Box_Charger_Wake_Delay_Ms_INDEX))
#define ITEM_Charger_Box_Enable_Battery_Recharge ((Charger_Box_DRV_ID<<16) | (Charger_Box_Enable_Battery_Recharge_INDEX))
#define ITEM_Charger_Box_Battery_Recharge_Threshold ((Charger_Box_DRV_ID<<16) | (Charger_Box_Battery_Recharge_Threshold_INDEX))
#define ITEM_Charger_Box_Charger_Box_Standby_Current ((Charger_Box_DRV_ID<<16) | (Charger_Box_Charger_Box_Standby_Current_INDEX))
#define ITEM_Charger_Box_DC5V_UART_Comm_Settings ((Charger_Box_DRV_ID<<16) | (Charger_Box_DC5V_UART_Comm_Settings_INDEX))
#define ITEM_Charger_Box_DC5V_IO_Comm_Settings ((Charger_Box_DRV_ID<<16) | (Charger_Box_DC5V_IO_Comm_Settings_INDEX))

/* BATTERY LEVEL */
#define ITEM_Battery_Level_Level ((Battery_Level_DRV_ID<<16) | (Battery_Level_Level_INDEX))
/* BATTERY LOW */
#define ITEM_Battery_Low_Battery_Too_Low_Voltage ((Battery_Low_DRV_ID<<16) | (Battery_Low_Battery_Too_Low_Voltage_INDEX))
#define ITEM_Battery_Low_Battery_Low_Voltage ((Battery_Low_DRV_ID<<16) | (Battery_Low_Battery_Low_Voltage_INDEX))
#define ITEM_Battery_Low_Battery_Low_Voltage_Ex ((Battery_Low_DRV_ID<<16) | (Battery_Low_Battery_Low_Voltage_Ex_INDEX))
#define ITEM_Battery_Low_Battery_Low_Prompt_Interval_Sec ((Battery_Low_DRV_ID<<16) | (Battery_Low_Battery_Low_Prompt_Interval_Sec_INDEX))


/*!
 * \brief 读取驱动配置数据
 * \n
 * \param item_key : 各驱动定义的item ID, 见driver_config.h
 * \param data : 保存配置数据
 * \param size : data 大小
 * \return
 *     成功: 数据长度
 * \n  失败: 0
 */
int cfg_get_by_key(uint32_t item_key, void *data, int size);


#endif  // ZEPHYR_INCLUDE_DRIVERS_DRIVER_ITEM_H_

