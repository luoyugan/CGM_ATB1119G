
#include <zephyr/types.h>
#include <strings.h>
#include <stddef.h>
#include <sys/types.h>
#include <device.h>
#include <sdfs.h>
#include <fs/fs.h>
#include <fs/fs_sys.h>
#include <storage/flash_map.h>
#include <partition/partition.h>
#include <linker/linker-defs.h>
#ifdef CONFIG_SD_FS_NAND_SD_STORAGE
#include "sdfs_nand_sd.h"
#endif

//#define K_SDFS_ADDR		(((unsigned int)_image_rom_start)+((unsigned int)_flash_used))
#define K_SDFS_ADDR		((unsigned int)_image_ksdfs_start)
	
static struct sd_file  g_sd_file_heap[CONFIG_SD_FILE_MAX];
static bool b_k_sdfs;


#define memcpy_flash_data memcpy

#if 0
#define sd_alloc k_malloc
#define sd_free k_free
#else
struct sd_file * sd_alloc(int size)
{
	int i;
	unsigned int key;
	key = irq_lock();
	for(i = 0; i < CONFIG_SD_FILE_MAX; i++){
		if(g_sd_file_heap[i].start == 0){
			g_sd_file_heap[i].start = 1; //use
			break;
		}
	}
	irq_unlock(key);
	if(i == CONFIG_SD_FILE_MAX)
		return NULL;
	else
		return &g_sd_file_heap[i];
}
void sd_free(struct sd_file * sd_file)
{
	unsigned int key;
	key = irq_lock();
	memset(sd_file, 0, sizeof(*sd_file));
	irq_unlock(key);
}
#endif

//#define CONFIG_SD_FS_VADDR_START g_vaddr_start
//static unsigned int g_vaddr_start = 0x0;



static struct sd_dir * sd_find_dir_by_addr(const char *filename, void *buf_size_32, uint32_t adfs_addr)
{
	int num, total, offset;
	struct sd_dir *sd_dir = buf_size_32;

	memcpy_flash_data(buf_size_32, (void *)adfs_addr, sizeof(*sd_dir));

	//printk("sd_dir->fname %s CONFIG_SD_FS_START 0x%x \n",sd_dir->fname,CONFIG_SD_FS_VADDR_START);
	if(memcmp(sd_dir->fname, "sdfs.bin", 8) != 0)
	{
		printk("sdfs.bin invalid, offset=0x%x\n", adfs_addr);
		return NULL;
	}
	total = sd_dir->offset;

	for(offset = adfs_addr + sizeof(*sd_dir), num = 0; num < total; offset += 32)
	{
		memcpy_flash_data(buf_size_32, (void *)offset, 32);
		//printk("%d,file=%s, size=0x%x\n", num, sd_dir->fname, sd_dir->size);
		if(strncasecmp(filename, sd_dir->fname, 12) == 0)
		{
			return sd_dir;
		}
		num++;
	}

	return NULL;
}

static struct sd_dir * sd_find_dir(const char *filename, void *buf_size_32)
{
	struct sd_dir * sd_d;
	if(b_k_sdfs){
		sd_d = sd_find_dir_by_addr(filename, buf_size_32, K_SDFS_ADDR);
		if(sd_d) {
			 sd_d->offset += K_SDFS_ADDR;
			return sd_d;
		}
	}
	sd_d = sd_find_dir_by_addr(filename, buf_size_32, CONFIG_SD_FS_VADDR_START);
	if(sd_d) {
		 sd_d->offset += CONFIG_SD_FS_VADDR_START;
		return sd_d;
	}
	return sd_d;
}
const char* const sd_volume_strs[] = {
	_SDFS_VOL_STRS
};

static const char *sd_get_part_type(const char *filename, u8_t *stor_id, u8_t *part)
{
	const char *pc;
	int i;
	*stor_id = STORAGE_ID_NOR;
	*part = 0;
	if(filename[0] != '/')
		return filename;

	filename += 1;
	pc = strchr(filename, ':');
	if(pc == NULL)  // /*|| pc[2] != '/'*/
		return NULL;
	for(i = 0; i < STORAGE_ID_MAX; i++){
		if(!strncmp(filename, sd_volume_strs[i], strlen(sd_volume_strs[i])))
			break;
	}
	if(i == STORAGE_ID_MAX)
		return NULL;
	*stor_id = i;
	*part = pc[1] - 'A';
	return pc+3;

}

struct sd_file * sd_fopen (const char *filename)
{
	struct sd_dir *sd_dir = NULL;
	uint8_t buf_size_32[32];
	struct sd_file *sd_file;
	u8_t stor_id, part;
	const char *fname;

	fname = sd_get_part_type(filename, &stor_id, &part);
	if(fname == NULL){
		printk("sdfs file %s invalid\n", filename);
		return NULL;
	}
	printk("sdfs:stor_id=%d, p=%d\n",stor_id, part);
	if(stor_id == STORAGE_ID_NOR)
		sd_dir = sd_find_dir(fname, (void *)buf_size_32);
#ifdef CONFIG_SD_FS_NAND_SD_STORAGE
	else
		sd_dir = nand_sd_find_dir(stor_id, part, fname, (void *)buf_size_32);
#endif
	if(sd_dir == NULL)
	{
		printk("%s no this file %s\n", __FUNCTION__, filename);
		return NULL;
	}

	sd_file = sd_alloc(sizeof(*sd_file));
	if(sd_file == NULL)
	{
		printk("%s malloc(%d) failed\n", __FUNCTION__, (int)sizeof(*sd_file));
		return NULL;
	}

	sd_file->start = sd_dir->offset;
	sd_file->size = sd_dir->size;
	sd_file->readptr = sd_file->start;
	sd_file->file_id = part;
	sd_file->storage_id = stor_id;
	return sd_file;
}

void sd_fclose(struct sd_file *sd_file)
{
	sd_free(sd_file);
}

int sd_fread(struct sd_file *sd_file, void *buffer, int len)
{
	unsigned int size_in_512, read_size;

	if ((sd_file->readptr - sd_file->start + len) > sd_file->size)
	{
		len = sd_file->size - (sd_file->readptr - sd_file->start);
	}
	if(len <= 0)
		return 0;

	read_size = len;

#ifdef CONFIG_SD_FS_NAND_SD_STORAGE
	if(sd_file->storage_id != STORAGE_ID_NOR)
		return nand_sd_sd_fread(sd_file->storage_id, sd_file, buffer, read_size);
#endif

	size_in_512 = 512 - (sd_file->readptr % 512);
	size_in_512 = size_in_512 > len ? len : size_in_512;
	if(size_in_512 > 0)
	{
		memcpy_flash_data(buffer, (void *)sd_file->readptr, size_in_512);

		buffer = (uint8_t *)buffer + size_in_512;
		sd_file->readptr += size_in_512;
		len -= size_in_512;
	}

	for(; len > 0; buffer = (uint8_t *)buffer + size_in_512, sd_file->readptr += size_in_512, len -= size_in_512)
	{
		size_in_512 = len > 512 ? 512 : len;

		memcpy_flash_data(buffer, (void *)sd_file->readptr, size_in_512);
	}

	return read_size;
}

int sd_ftell(struct sd_file *sd_file)
{
    return (sd_file->readptr - sd_file->start);
}

int sd_fseek(struct sd_file *sd_file, int offset, unsigned char whence)
{
	if (whence == FS_SEEK_SET)
	{
		if (offset > sd_file->size)
			return -1;

		sd_file->readptr = sd_file->start + offset;
		return 0;
	}

	if (whence == FS_SEEK_CUR)
	{
		if(sd_file->readptr + offset < sd_file->start
			|| sd_file->readptr + offset > sd_file->start + sd_file->size)
		{
			return -1;
		}

		sd_file->readptr += offset;
		return 0;
	}

	if (whence == FS_SEEK_END)
	{
		if(offset > 0 || offset + sd_file->size < 0)
			return -1;

		sd_file->readptr = sd_file->start + sd_file->size + offset;
		return 0;
	}

	return -EINVAL;
}

int sd_fsize(const char *filename)
{
	struct sd_file *fd = sd_fopen(filename);
	int file_size;

	if (!fd) {
		return -EINVAL;
	}

	file_size = fd->size;

	sd_fclose(fd);

	return file_size;
}

int sd_fmap(const char *filename, void** addr, int* len)
{
	struct sd_file *fd = sd_fopen(filename);
	if (!fd) {
		return -EINVAL;
	}
	if(fd->storage_id != STORAGE_ID_NOR)
		return -EINVAL;


	if (addr)
		*addr = (void *)fd->start;

	if (len)
		*len = fd->size;

	sd_fclose(fd);

	return 0;
}

static int sd_fs_init(const struct device *dev)
{
	int err;
	struct sd_dir sd_dir;
	printk("sdfs: init mapping to 0x%x, koff=0x%x\n", CONFIG_SD_FS_VADDR_START, K_SDFS_ADDR);
	memcpy_flash_data(&sd_dir, (void *)K_SDFS_ADDR, sizeof(sd_dir));
	if(memcmp(sd_dir.fname, "sdfs.bin", 8) == 0){
		printk("ksdfs.bin ok\n");
		b_k_sdfs = true;
	}else{
		b_k_sdfs = false;
	}
	err = partition_file_mapping(PARTITION_NAME_SDFS, CONFIG_SD_FS_VADDR_START);
	if (err) {
		printk("sdfs: cannot mapping part file_id %d", PARTITION_FILE_ID_SDFS);
		return -1;
	}

	return 0;
}

SYS_INIT(sd_fs_init, PRE_KERNEL_1, 80);


#ifdef CONFIG_FILE_SYSTEM

static int sdfs_open(struct fs_file_t *zfp, const char *file_name, fs_mode_t flags)
{
	struct sd_file * sdf;
	if (zfp == NULL || file_name == NULL) {
		return -EINVAL;
	}

	if (zfp->filep) {
		/* file has been opened */
		return -EEXIST;
	}
	sdf = sd_fopen(file_name);
	if(sdf == NULL)
		return -EINVAL;

	zfp->filep = (void *)sdf;
	return 0;
}

static int sdfs_close(struct fs_file_t *zfp)
{
	if (zfp == NULL) {
		return -EINVAL;
	}
	if (zfp->filep) {
		sd_fclose((struct sd_file *)zfp->filep);
		zfp->filep = NULL;
	} else {
		return -EIO;
	}
	return 0;
}
static ssize_t sdfs_read(struct fs_file_t *zfp, void *ptr, size_t size)
{
	if (zfp == NULL || ptr == NULL) {
		return -EINVAL;
	}
	return sd_fread((struct sd_file *)zfp->filep, ptr, size);
}


static int sdfs_seek(struct fs_file_t *zfp, off_t offset, int whence)
{
	if (!zfp) {
		return -EINVAL;
	}
	return sd_fseek((struct sd_file *)zfp->filep, offset, whence);
}

static off_t sdfs_tell(struct fs_file_t *zfp)
{
	if (!zfp) {
		return -EINVAL;
	}
	return sd_ftell((struct sd_file *)zfp->filep);
}
static int sdfs_stat(struct fs_mount_t *mountp,
		      const char *path, struct fs_dirent *entry)
{
	int ret;
	if (mountp == NULL || path == NULL || entry == NULL) {
		return -EINVAL;
	}
	ret = sd_fsize(path);
	if(ret < 0) {
		printk("%s not exist\n", path);
		return -EINVAL;
	}
	entry->type = FS_DIR_ENTRY_FILE;
	entry->size = ret;
	return 0;
}

static int sdfs_statvfs(struct fs_mount_t *mountp,
			 const char *path, struct fs_statvfs *stat)
{
	if (mountp == NULL || path == NULL || stat == NULL) {
		return -EINVAL;
	}
	memset(stat, 0, sizeof(struct fs_statvfs));
	stat->f_bsize = 512;
	return 0;
}

static int sdfs_mount(struct fs_mount_t *mountp)
{
	u8_t stor_id, part;
	const char *fname;
	const struct partition_entry *parti;

	if (mountp == NULL) {
		return -EINVAL;
	}
	fname = sd_get_part_type(mountp->mnt_point, &stor_id, &part);
	if(fname == NULL){
		printk("sdfs mount fail,%s\n", mountp->mnt_point);
		return -EINVAL;
	}
	parti = partition_get_stf_part(stor_id, part+PARTITION_FILE_ID_SDFS_PART_BASE);
	if(parti == NULL){
		printk("sdfs mount get parit fail,%s\n", mountp->mnt_point);
		return -EINVAL;
	}

	return 0;
}

static int sdfs_unmount(struct fs_mount_t *mountp)
{
	if (mountp == NULL) {
		return -EINVAL;
	}
	return 0;
}


/* File system interface */
const struct fs_file_system_t sdfs_fs = {
	.open = sdfs_open,
	.close = sdfs_close,
	.read = sdfs_read,
	.lseek = sdfs_seek,
	.tell = sdfs_tell,
	.mount = sdfs_mount,
	.unmount = sdfs_unmount,
	.stat = sdfs_stat,
	.statvfs = sdfs_statvfs,

};

static int fs_sdfs_init(const struct device *dev)
{
	int ret;
	ret = fs_register(FS_SDFS, &sdfs_fs);
	printk("sdfs fs_register=%d\n", ret);
	return 0;
}


SYS_INIT(fs_sdfs_init, POST_KERNEL, 99);

#endif

