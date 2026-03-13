#include <kernel.h>
#include <init.h>
#include <string.h>
#include <soc.h>
#include <crc.h>
#include <partition/partition.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(partition, CONFIG_LOG_DEFAULT_LEVEL);

static const struct partition_table *g_part_table;

void partition_dump(void)
{
	const struct partition_entry *part;
	char part_name[13];
	int i;

	printk("** Showing partition infomation **\n");
	printk("id  name      offset    type  seq   entry_offs  flag\n");

	for (i = 0; i < MAX_PARTITION_COUNT; i++) {
		part = &g_part_table->parts[i];

		/* name field to str */
		memcpy(part_name, part->name, 8);
		part_name[8] = '\0';

		if (strlen(part_name) && part->type) {
			printk("%-4d%-10s0x%-8x%-6d%-9d0x%-9x%-6d\n",
				i, part_name, part->offset, part->type,
				part->seq, part->entry_offs, part->flag);
		}
	}
}

#if 0
int partition_get_max_file_size(const struct partition_entry *part)
{
	if (!part)
		return -EINVAL;

	return (part->size - (part->file_offset - part->offset));
}

const struct partition_entry *partition_find_part(u32_t nor_phy_addr)
{
	const struct partition_entry *part;
	unsigned int i;

	 __ASSERT_NO_MSG(g_part_table);

	for (i = 0; i < MAX_PARTITION_COUNT; i++) {
		part = &g_part_table->parts[i];

		if ((part->offset <= nor_phy_addr) &&
			((part->offset + part->size) > nor_phy_addr)) {

			return part;
		}
	}

	return NULL;
}

const struct partition_entry *partition_get_part_by_id(u8_t part_id)
{
	 __ASSERT_NO_MSG(g_part_table);

	if (part_id >= MAX_PARTITION_COUNT)
		return NULL;

	return &g_part_table->parts[part_id];
}

const struct partition_entry *partition_get_current_part(void)
{
	u32_t cur_phy_addr;
	const boot_info_t *p_boot_info = soc_boot_get_info();

	 __ASSERT_NO_MSG(g_part_table);

	cur_phy_addr = p_boot_info->system_phy_addr;

	return partition_find_part(cur_phy_addr);
}

u8_t partition_get_current_mirror_id(void)
{
	const struct partition_entry *part;

	part = partition_get_current_part();
	if (part == NULL)
		return -1;

	return part->mirror_id;
}

u8_t partition_get_current_file_id(void)
{
	const struct partition_entry *part;

	part = partition_get_current_part();
	if (part == NULL)
		return -1;

	return part->file_id;
}

int partition_is_mirror_part(const struct partition_entry *part)
{
	u8_t mirror_id;

	 __ASSERT_NO_MSG(g_part_table);

	mirror_id = partition_get_current_mirror_id();

	if ((part->mirror_id != mirror_id) && (part->mirror_id != PARTITION_MIRROR_ID_INVALID)) {
		return 1;
	}

	return 0;
}

int partition_is_param_part(const struct partition_entry *part)
{
	 __ASSERT_NO_MSG(g_part_table);

	 if (!strncmp(part->name, PARTITION_NAME_PARAM0, sizeof(part->name)))
		return 1;

	return 0;
}

int partition_is_boot_part(const struct partition_entry *part)
{
	 __ASSERT_NO_MSG(g_part_table);

	if (part->type == PARTITION_TYPE_BOOT)
		return 1;

	if (!strncmp(part->name, PARTITION_NAME_BOOT0, sizeof(part->name)))
		return 1;

	return 0;
}
#endif

const struct partition_entry *partition_get_part(u8_t *part_name)
{
	const struct partition_entry *part;
	int i;

	 __ASSERT_NO_MSG(g_part_table);

	for (i = 0; i < MAX_PARTITION_COUNT; i++) {
		part = &g_part_table->parts[i];
		if (!strncmp(part->name, part_name, sizeof(part->name))) {
			return part;
		}
	}

	return NULL;
}

#if 0
const struct partition_entry *partition_get_temp_part(void)
{
	const struct partition_entry *temp_part;

	temp_part = partition_get_part(PARTITION_FILE_ID_OTA_TEMP);
	if (temp_part == NULL || (temp_part->type != PARTITION_TYPE_TEMP)) {
		LOG_ERR("cannot found temp part\n");
		return NULL;
	}

	return temp_part;
}

const struct partition_entry *partition_get_mirror_part(u8_t file_id)
{
	const struct partition_entry *part;
	int i;

	 __ASSERT_NO_MSG(g_part_table);

	for (i = 0; i < MAX_PARTITION_COUNT; i++) {
		part = &g_part_table->parts[i];
		if ((part->file_id == file_id) && partition_is_mirror_part(part)) {
			/* found */
			LOG_INF("found write part %d for file_id 0x%x\n", i, file_id);
			return part;
		}
	}

	return NULL;
}
#endif
const struct partition_entry *partition_get_stf_part(u8_t stor_id, u8_t file_id)
{
#if 0
	const struct partition_entry *part;
	int i;

	 __ASSERT_NO_MSG(g_part_table);

	for (i = 0; i < MAX_PARTITION_COUNT; i++) {
		part = &g_part_table->parts[i];
		if (((part->storage_id == stor_id) || (part->storage_id== STORAGE_ID_MAX)) && (part->file_id == file_id)) {
			/* found */
			LOG_INF("found write part %d for file_id 0x%x\n", i, file_id);
			return part;
		}
	}
#endif
	return NULL;
}


int partition_file_mapping(u8_t *part_name, u32_t vaddr)
{
	const struct partition_entry *part;
	int err, crc_is_enabled;

	part = partition_get_part(part_name);
	if (part == NULL) {
		LOG_ERR("cannot found part name %s\n", part_name);
		return -ENOENT;
	}

	crc_is_enabled = part->flag & PARTITION_FLAG_ENABLE_CRC ? 1 : 0;

	err = soc_memctrl_mapping(vaddr, part->offset, crc_is_enabled);
	if (err) {
		LOG_ERR("cannot mapping part name %s\n", part_name);
		return err;
	}

	return 0;
}


static int partition_init(const struct device *dev)
{
	g_part_table = (const struct partition_table *)soc_boot_get_part_tbl_addr();
	if (g_part_table == NULL ||
	    g_part_table->magic != PARTITION_TABLE_MAGIC) {
		LOG_ERR("partition table (%p) has wrong magic\n", g_part_table);
		goto failed;
	}

#if 0
	u32_t cksum;
	cksum = utils_crc32(0, (const uint8_t *)g_part_table, sizeof(struct partition_table) - 4);
	if (cksum != g_part_table->table_crc) {
		LOG_ERR("partition table (%p) checksum error\n", g_part_table);
		goto failed;
	}
#endif

	partition_dump();

	return 0;

failed:
	g_part_table = NULL;
	return 0;
}

SYS_INIT(partition_init, PRE_KERNEL_1, 60);
