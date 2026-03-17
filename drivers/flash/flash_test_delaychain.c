
#include <drivers/flash.h>
#include <drivers/spi.h>
#include <logging/log.h>
#include <soc.h>
#include <board_cfg.h>
#include "spi_flash.h"

static inline void test_setup_delaychain(struct spinor_info *sni, u8_t ns)
{
	struct acts_spi_reg *spi= (struct acts_spi_reg *)sni->spi.base;
	spi->ctrl = (spi->ctrl & ~(0xF << 16)) | (ns << 16);

	volatile int i = 100000;
	while (i--);

}
//#define TEST_READ_WRITE

#ifdef  TEST_READ_WRITE

#define TEST_ADDR 0x180000
#define TEST_ADDR_END  0x1c0000
#define TEST_SIZE	(1024*4)
static u32_t nor_test_buf[TEST_SIZE/4];
static u32_t nor_test_start= TEST_ADDR;
__ramfunc int test_read_write_try(struct device *dev, u8_t delay_chain)
{
	int ret, i;
	struct spinor_info *sni = (struct spinor_info *)(dev)->data;
	sni->spi.delay_chain = delay_chain;
	/* configure delay chain */
	test_setup_delaychain(sni, sni->spi.delay_chain);
	k_busy_wait(50);

	if(nor_test_start < TEST_ADDR || TEST_ADDR > TEST_ADDR_END)
		nor_test_start = TEST_ADDR;

	ret = flash_erase(dev, nor_test_start, TEST_SIZE);
	for(i = 0; i < TEST_SIZE/4; i++)
		nor_test_buf[i] = 	nor_test_start + i;
	
	ret = flash_write(dev, nor_test_start, nor_test_buf, TEST_SIZE);
	memset(nor_test_buf, 0 , TEST_SIZE);
	ret = flash_read(dev, nor_test_start, nor_test_buf, TEST_SIZE);
	if(ret){
		test_setup_delaychain(sni, CONFIG_SPI_FLASH_DELAY_CHAIN);
		printk("read write fail =0x%x\n", nor_test_start);
		return -1;
	}
	for(i = 0; i < TEST_SIZE/4; i++){
		if( nor_test_buf[i] != 	nor_test_start + i){
			test_setup_delaychain(sni, CONFIG_SPI_FLASH_DELAY_CHAIN);
			printk("read 0x%x != 0x%x\n", nor_test_start + i, nor_test_buf[i]);
			nor_test_start += TEST_SIZE;
			return -1;
		}
	}
	nor_test_start += TEST_SIZE;

	return 0;
}

#endif

__ramfunc u32_t test_delaychain_read_id(struct device *dev, u8_t delay_chain)
{
	u32_t nor_id, mid;
	struct spinor_info *sni = (struct spinor_info *)(dev)->data;

	sni->spi.delay_chain = delay_chain;

	/* configure delay chain */
	test_setup_delaychain(sni, sni->spi.delay_chain);
	k_busy_wait(50);
	nor_id = p_spinor_api->read_chipid(sni) & 0xffffff;

	mid = nor_id & 0xff;
	if ((mid == 0xff) || (mid == 0x00))
		return 0;
	
	return nor_id;
}

__ramfunc s32_t test_delaychain_try(struct device *dev, u8_t *ret_delaychain, u32_t chipid_ref)
{
	u32_t i, try_delaychain;
	bool match_flag = 0;
	u32_t nor_id_value_check;
	u32_t local_irq_save;

	local_irq_save = irq_lock();

	ret_delaychain[0] = 0;

	for (try_delaychain = 1; try_delaychain <= 15; try_delaychain++) {
		match_flag = 1;
		printk("try_delaychain :%d\n", try_delaychain);
		k_busy_wait(5000);
		#ifdef  TEST_READ_WRITE
		for (i = 0; i < 8; i++) {
			if(test_read_write_try(dev, try_delaychain)){
				match_flag = 0;
				break;
			}
		}
		#else
		for (i = 0; i < 64; i++) {
			nor_id_value_check = test_delaychain_read_id(dev, try_delaychain);
			if (nor_id_value_check != chipid_ref) {
				printk("read:0x%x @ %d\n", nor_id_value_check, try_delaychain);
				match_flag = 0;
				break;
			}
			
		}
		#endif

		ret_delaychain[try_delaychain] = match_flag;
	}

	test_delaychain_read_id(dev, CONFIG_SPI_FLASH_DELAY_CHAIN);

	irq_unlock(local_irq_save);

	return 0;
}


__ramfunc u8_t spinor_test_delaychain(void)
{
	u8_t ret_delaychain = 0;
	u8_t delaychain_flag[16];
	u8_t delaychain_total[16];
	u8_t start = 0, end, middle, i;
	u8_t expect_max_count_delay_chain = 0, max_count_delay_chain;
	u32_t freq;
	u32_t chipid_ref;
	struct device *test_nor_dev = device_get_binding(CONFIG_SPI_FLASH_NAME);

	printk("spinor test delaychain start\n");

	chipid_ref = test_delaychain_read_id(test_nor_dev, CONFIG_SPI_FLASH_DELAY_CHAIN);

	printk("chipid_ref : 0x%x\n", chipid_ref);

	memset(delaychain_total, 0x0, 16);

	//for (freq = 6; freq <= 222; freq += 6) {
		expect_max_count_delay_chain++;

	//	soc_freq_set_cpu_clk(0, freq);

	//	printk("set cpu freq : %d\n", freq);

		if (test_delaychain_try(test_nor_dev, delaychain_flag, chipid_ref) == 0) {
			for (i = 0; i < 16; i++)
				delaychain_total[i] += delaychain_flag[i];
		} else {
			printk("test_delaychain_try error!!\n");
			goto delay_chain_exit;
		}

	//}

	printk("delaychain_total : ");
	for (i = 0; i < 15; i++)
		printk("%d,", delaychain_total[i]);
	printk("%d\n", delaychain_total[15]);

	max_count_delay_chain = 0;
	for (i = 0; i < 16; i++) {
		if (delaychain_total[i] > max_count_delay_chain)
			max_count_delay_chain = delaychain_total[i];
	}

	for (i = 0; i < 16; i++) {
		if (delaychain_total[i] == max_count_delay_chain) {
			start = i;
			break;
		}
	}
	end = start;
	for (i = start + 1; i < 16; i++) {
		if (delaychain_total[i] != max_count_delay_chain)
			break;
		end = i;
	}

	if (max_count_delay_chain < expect_max_count_delay_chain) {
		printk("test delaychain max count is %d, less then expect %d!!\n",
			max_count_delay_chain, expect_max_count_delay_chain);
		goto delay_chain_exit;
	}

	if ((end - start + 1) < 3) {
		printk("test delaychain only %d ok!! too less!!\n", end - start + 1);
		goto delay_chain_exit;
	}

	middle = (start + end) / 2;
	printk("test delaychain pass, best delaychain is : %d\n\n", middle);

	ret_delaychain = middle;

	delay_chain_exit:

	return ret_delaychain;
}

__ramfunc int nor_test_delaychain(struct device *dev)
{
	printk("nor_test_delaychain\n");

	printk("delaychain : %d\n", spinor_test_delaychain());

	return 0;
}