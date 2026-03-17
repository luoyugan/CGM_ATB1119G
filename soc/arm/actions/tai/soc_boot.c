/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions atb111x family boot related infomation public interfaces.
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include "soc_boot.h"
#include <linker/linker-defs.h>

#define	SOC_BOOT_PARTITION_TABLE_OFFSET			(0xc00) /* The offset of flash mapping table within system parameter partition */
#define	SOC_BOOT_FIRMWARE_VERSION_OFFSET		(0x180) /* The offset of firmware version table within system parameter paritition */
#define	SOC_BOOT_INFOMATION_OFFSET				(0x200) /* The offset of boot infomation within system parameter paritition */

 /* The macro to define the address of system paremeter partition which copyed by bootloader stage */
#define SOC_BOOT_PARAM_PARITION_ADDRESS			(0x01000000)

uint32_t soc_boot_get_part_tbl_addr(void)
{
	return (SOC_BOOT_PARAM_PARITION_ADDRESS + SOC_BOOT_PARTITION_TABLE_OFFSET);
}

uint32_t soc_boot_get_fw_ver_addr(void)
{
	return (SOC_BOOT_PARAM_PARITION_ADDRESS + SOC_BOOT_FIRMWARE_VERSION_OFFSET);
}
static boot_info_t g_boot_info = {
	.mbrec_phy_addr = 0x0, /* The physical address of MBREC that current system running */
	.system_phy_addr= 0x2000, /* The physical address of SYSTEM that current using */
};

const boot_info_t *soc_boot_get_info(void)
{
	return  &g_boot_info;
	//return (const boot_info_t *)(SOC_BOOT_PARAM_PARITION_ADDRESS + SOC_BOOT_INFOMATION_OFFSET);
}

u32_t soc_boot_get_reboot_reason(void)
{
	const boot_info_t *p_boot_info = soc_boot_get_info();
	return p_boot_info->reboot_reason;
}

bool soc_boot_get_watchdog_is_reboot(void)
{
	const boot_info_t *p_boot_info = soc_boot_get_info();
	return !!p_boot_info->watchdog_reboot;
}

