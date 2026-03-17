/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Actions atb111x family boot related infomation public interfaces.
 */

 /* The macro to define the mapping address of SPI0 cache which configured by bootloader stage when boot from SPI-NOR solution */
#define SOC_BOOT_NOR_MAPPING_ADDRESS			(0x08000000)

/* The macro to define the mapping address of SPI1 cache for PSRAM which configured by bootloader stage */
#define SOC_BOOT_PSRAM_MAPPING_ADDRESS			(0x0c000000)

/* The total size of ROM data that range from TEXT to RODATA sections */
extern uint32_t z_rom_data_size;

#define	SOC_BOOT_FIRMWARE_VERSION_OFFSET		(0x180)	/* The offset of firmware version table within system parameter paritition */

/*!
 * struct boot_info_t
 * @brief The boot infomation that transfer from bootloader to system.
 */
typedef struct {
	uint32_t mbrec_phy_addr; /* The physical address of MBREC that current system running */
	uint32_t param_phy_addr; /* The physical address of PARAM that current system using */
	uint32_t system_phy_addr; /* The physical address of SYSTEM that current using */
	uint32_t reboot_reason; /* Reboot reason */
	uint32_t watchdog_reboot : 1; /* The reboot event that occured from bootrom watchdog expired */
	uint32_t is_mirror : 1; /* The indicator of launching system is a mirror partition */
} boot_info_t;

/* @brief The function to get the address of current partition table */
uint32_t soc_boot_get_part_tbl_addr(void);

/* @brief The function to get the address of current firmware version */
uint32_t soc_boot_get_fw_ver_addr(void);

/* @brief The function to get the current boot infomation  */
const boot_info_t *soc_boot_get_info(void);

/* @brief The function to get the reboot reason */
u32_t soc_boot_get_reboot_reason(void);

/* @brief The function to get the indicator of watchdog reboot */
bool soc_boot_get_watchdog_is_reboot(void);

