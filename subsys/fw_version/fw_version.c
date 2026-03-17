#include <kernel.h>
#include <init.h>
#include <soc.h>
#include "fw_version.h"
#include <crc.h>

const struct fw_version *fw_version_get_current(void)
{
	const struct fw_version *ver =
		(struct fw_version *)soc_boot_get_fw_ver_addr();

	return ver;
}

void fw_version_dump(const struct fw_version *ver)
{
	printk("***  Current Firmware Version  ***\n");
	printk(" Firmware Version: 0x%08x\n", ver->version_code);
	printk("   System Version: 0x%08x\n", ver->system_version_code);
	printk("     Version Name: %s\n", ver->version_name);
	printk("       Board Name: %s\n", ver->board_name);
}

int fw_version_check(const struct fw_version *ver)
{
	uint32_t checksum;

	if (ver->magic != FIRMWARE_VERSION_MAGIC)
		return -1;

	checksum = utils_crc32(0, (const u8_t *)ver, sizeof(struct fw_version) - 4);

	if (ver->checksum != checksum)
		return -1;

	return 0;
}

#if 0
static int fw_version_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	const struct fw_version *ver = fw_version_get_current();

	if (fw_version_check(ver)) {
		printk("BAD firmware version !!!\n");
		return -1;
	}

	fw_version_dump(ver);

	return 0;
}

SYS_INIT(fw_version_init, APPLICATION, 1);
#endif
