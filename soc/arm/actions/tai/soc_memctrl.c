/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file memory controller interface
 */

#include <kernel.h>
#include <soc.h>
#include "soc_memctrl.h"

#define SOC_MEMCTRL_TEMP_MAP_CPU_ADDR 0x01c00000

void soc_memctrl_set_mapping(int idx, u32_t map_addr, u32_t nor_bus_addr)
{
	u32_t bus_addr;

	if (idx >= CACHE_MAPPING_ITEM_NUM)
		return;

	bus_addr = soc_memctrl_cpu_addr_to_bus_addr(map_addr);
	/* default: mapping size: 1MB */
	sys_write32((bus_addr & ~0xf) | 0x1, SPI_CACHE_MAPPING_ADDR0 + idx * 8);
	/*set entry must afer mapping set*/
	sys_write32(nor_bus_addr, SPI_CACHE_ADDR0_ENTRY + idx * 8);
}

int soc_memctrl_item_is_free(int idx)
{
	u32_t map_reg_val;

	map_reg_val = sys_read32(SPI_CACHE_MAPPING_ADDR0 + idx * 8);

	return (map_reg_val ? 0 : 1);
}

__ramfunc void soc_memctrl_cache_invalid(void)
{
	sys_write32(1, SPI0_CACHE_INVALIDATE);
	while ((sys_read32(SPI0_CACHE_INVALIDATE) & 0x1) == 1)
		;
}

int soc_memctrl_mapping(u32_t map_addr, u32_t nor_phy_addr, int enable_crc)
{
	u32_t nor_bus_addr;
	int i;

	if (nor_phy_addr % 0x1000)	{
		printk("invalid nor phy addr 0x%x\n", nor_phy_addr);
		return -EINVAL;
	}

	if (map_addr & (0x1ffff0))	{
		printk("invalid cpu addr 0x%x for mapping to 2MB, bit[4~20] must be 0\n", map_addr);
		return -EINVAL;
	}

	nor_bus_addr = nor_phy_addr;
	if (enable_crc) {
		nor_bus_addr |= (1 << 31);
	}

	for (i = CACHE_MAPPING_ITEM_NUM - 1; i >= 0; i--) {
		if (soc_memctrl_item_is_free(i)) {
			printk("slot=%d  to map nor phy addr 0x%x, map=x0%x\n", i, nor_phy_addr, map_addr);
			soc_memctrl_set_mapping(i, map_addr, nor_bus_addr);
			soc_memctrl_cache_invalid();
			return 0;
		}
	}

	printk("cannot found slot to map nor phy addr 0x%x\n", nor_phy_addr);
	return -ENOENT;
}

int soc_memctrl_unmapping(u32_t map_addr)
{
	u32_t bus_addr, map_reg_val;
	int i;

	bus_addr = soc_memctrl_cpu_addr_to_bus_addr(map_addr);

	for (i = CACHE_MAPPING_ITEM_NUM - 1; i >= 0; i--) {
		map_reg_val = sys_read32(SPI_CACHE_MAPPING_ADDR0 + i * 8);
		if (map_reg_val == 0)
			continue;

		if ((map_reg_val & ~0xF) == bus_addr) {
			sys_write32(0x0, SPI_CACHE_ADDR0_ENTRY + i * 8);
			sys_write32(0x0, SPI_CACHE_MAPPING_ADDR0 + i * 8);
			soc_memctrl_cache_invalid();
			return 0;
		}

	}

	printk("cannot found slot to map cpu addr 0x%x\n", map_addr);
	return -ENOENT;
}

void *soc_memctrl_create_temp_mapping(u32_t nor_phy_addr, int enable_crc)
{
	u32_t tmp_cpu_addr = SOC_MEMCTRL_TEMP_MAP_CPU_ADDR;
	int err;

	printk("%s: create temp mapping 0x%x, nor_phy_addr 0x%x, enable_crc %d\n", __func__,
		tmp_cpu_addr, nor_phy_addr, enable_crc);

	err = soc_memctrl_mapping(tmp_cpu_addr, nor_phy_addr, enable_crc);
	if (err < 0)
		return NULL;

	return (void *)tmp_cpu_addr;
}

void soc_memctrl_clear_temp_mapping(void *cpu_addr)
{
	soc_memctrl_unmapping((u32_t)cpu_addr);
}

__ramfunc void soc_memctrl_spicache_init(void)
{
	sys_write32(sys_read32(RMU_MRCR0) | (1 << 4), RMU_MRCR0);
	sys_write32(sys_read32(CMU_DEVCLKEN0) | (1 << 4), CMU_DEVCLKEN0);

	sys_write32(((1 << 0) | (1 << 5)), SPI0_CACHE_CTL);
}

