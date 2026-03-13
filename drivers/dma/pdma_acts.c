/*
 * Copyright (c) 2018 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT actions_acts_dma


#include <device.h>
#include <soc.h>
#include <drivers/dma.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(dma_acts, CONFIG_DMA_LOG_LEVEL);


#define DMA_ID_MEM				0
#define MAX_DMA_CH  10
#define DMA_CHAN(base, ch)		((struct acts_dma_chan_reg*)((base) +  (ch+1) * 0x100))
/* Maximum data sent in single transfer (Bytes) */
#define DMA_ACTS_MAX_DATA_ITEMS		0x7ffff

/*dma ctl register*/
#define DMA_CTL_SRC_TYPE_SHIFT			0
#define DMA_CTL_SRC_TYPE(x)			((x) << DMA_CTL_SRC_TYPE_SHIFT)
#define DMA_CTL_SRC_TYPE_MASK			DMA_CTL_SRC_TYPE(0x3f)
#define DMA_CTL_SAM_CONSTANT			(0x1 << 7)
#define DMA_CTL_DST_TYPE_SHIFT			8
#define DMA_CTL_DST_TYPE(x)			((x) << DMA_CTL_DST_TYPE_SHIFT)
#define DMA_CTL_DST_TYPE_MASK			DMA_CTL_DST_TYPE(0x3f)
#define DMA_CTL_DAM_CONSTANT			(0x1 << 15)


#define DMA_CTL_ADUDIO_TYPE_SHIFT			16
#define DMA_CTL_ADUDIO_TYPE(x)		((x) << DMA_CTL_ADUDIO_TYPE_SHIFT)
#define DMA_CTL_ADUDIO_TYPE_MASK	DMA_CTL_ADUDIO_TYPE(0x1)
#define DMA_CTL_ADUDIO_TYPE_INTER	DMA_CTL_ADUDIO_TYPE(0)
#define DMA_CTL_ADUDIO_TYPE_SEP		DMA_CTL_ADUDIO_TYPE(1)
#define DMA_CTL_TRM_SHIFT			17
#define DMA_CTL_TRM(x)				((x) << DMA_CTL_TRM_SHIFT)
#define DMA_CTL_TRM_MASK			DMA_CTL_TRM(0x1)
#define DMA_CTL_TRM_BURST8			DMA_CTL_TRM(0)
#define DMA_CTL_TRM_SINGLE			DMA_CTL_TRM(1)
#define DMA_CTL_RELOAD				(0x1 << 18)
#define DMA_CTL_TWS_SHIFT			20
#define DMA_CTL_TWS(x)				((x) << DMA_CTL_TWS_SHIFT)
#define DMA_CTL_TWS_MASK			DMA_CTL_TWS(0x3)
#define DMA_CTL_TWS_8BIT			DMA_CTL_TWS(2)
#define DMA_CTL_TWS_16BIT			DMA_CTL_TWS(1)
#define DMA_CTL_TWS_32BIT			DMA_CTL_TWS(0)

/*dma pending register*/
#define DMA_PD_TCIP(ch)				(1 << ch)
#define DMA_PD_HFIP(ch)				(1 << (ch+16))

/*dma irq enable register*/
#define DMA_IE_TCIP(ch)				(1 << ch)
#define DMA_IE_HFIP(ch)				(1 << (ch+16))


#define DMA_START_START				(0x1 << 0)



/* dma channel registers */
struct acts_dma_chan_reg {
	volatile uint32_t ctl;
	volatile uint32_t start;
	volatile uint32_t saddr0;
	volatile uint32_t saddr1;
	volatile uint32_t daddr0;
	volatile uint32_t daddr1;
	volatile uint32_t bc;
	volatile uint32_t rc;
};
struct acts_dma_reg {
	volatile uint32_t dma_ip;
	volatile uint32_t dma_ie;
};


typedef void (*dma_callback)(void *callback_arg, uint32_t channel,
			     int error_code);

struct dma_acts_channel {
	dma_callback cb;
	void   *cb_arg;
	uint32_t  complete_callback_en	: 1;
	uint32_t  channel_direction	: 3;
	uint32_t  reserved				: 28;

};

struct dma_acts_data {
	uint32_t base;
	int chan_num;
	struct dma_acts_channel channels[MAX_DMA_CH];
};

#define DEV_DATA(dev) \
	((struct dma_acts_data *const)(dev)->data)


static struct dma_acts_data dmac_data;


#if 0
static void dma_acts_dump_reg(struct dma_acts_data *ddev, uint32_t id)
{
	struct acts_dma_chan_reg *cregs = DMA_CHAN(ddev->base, id);

	LOG_INF("Using channel: %d", id);
	LOG_INF("  DMA_CTL:      0x%x", cregs->ctl);
	LOG_INF("  DMA_SADDR0:    0x%x", cregs->saddr0);
	LOG_INF("  DMA_SADDR1:    0x%x", cregs->saddr1);
	LOG_INF("  DMA_DADDR0:    0x%x", cregs->daddr0);
	LOG_INF("  DMA_DADDR1:    0x%x", cregs->daddr1);
	LOG_INF("  DMA_LEN:       0x%x", cregs->bc);
	LOG_INF("  DMA_RMAIN_LEN: 0x%x", cregs->rc);
}
#endif


/* Handles DMA interrupts and dispatches to the individual channel */
static void dma_acts_isr(void *arg)
{
	uint32_t id = (uint32_t) arg;
	struct dma_acts_data *ddev = &dmac_data;
	struct acts_dma_reg *gregs = (struct acts_dma_reg *)ddev->base;
	struct dma_acts_channel *chan = &ddev->channels[id];
	uint32_t hf_pending, tc_pending;

	if (id >= ddev->chan_num)
		return;

	hf_pending = DMA_PD_HFIP(id) &
		  gregs->dma_ip & gregs->dma_ie;

	tc_pending = DMA_PD_TCIP(id) &
		  gregs->dma_ip & gregs->dma_ie;

	/* clear pending */
	gregs->dma_ip = tc_pending | hf_pending;

	/* process full complete callback */
	if ((tc_pending || hf_pending) && chan->complete_callback_en &&
	    chan->cb) {
		chan->cb(chan->cb_arg, id, 0);

	}

}


/* Configure a channel */
static int dma_acts_config(struct device *dev, uint32_t channel,
			   struct dma_config *config)
{
	struct dma_acts_data *ddev = DEV_DATA(dev);
	struct dma_acts_channel *chan  = &ddev->channels[channel];
	struct acts_dma_chan_reg *cregs = DMA_CHAN(ddev->base, channel);
	struct dma_block_config *head_block = config->head_block;
	uint32_t ctl;
	int data_width = 0;

	if (channel >= ddev->chan_num) {
		LOG_ERR("DMA error:ch=%d > dma max chan=%d\n",channel,
		       ddev->chan_num);

		return -EINVAL;
	}

	if (head_block->block_size > DMA_ACTS_MAX_DATA_ITEMS) {
		LOG_ERR("DMA error: Data size too big: %d",
		       head_block->block_size);
		return -EINVAL;
	}

	if (config->complete_callback_en || config->error_callback_en) {
		chan->cb = config->dma_callback;
		chan->cb_arg = config->callback_arg;
		chan->complete_callback_en = config->complete_callback_en;
	} else {
		chan->cb = NULL;
	}

	cregs->saddr0 = (uint32_t)head_block->source_address;
	cregs->daddr0 = (uint32_t)head_block->dest_address;
	cregs->bc = (uint32_t)head_block->block_size;

	chan->channel_direction = config->channel_direction;

	if (config->channel_direction == MEMORY_TO_PERIPHERAL) {
		ctl = DMA_CTL_SRC_TYPE(DMA_ID_MEM) |
		      DMA_CTL_DST_TYPE(config->dma_slot) |
		      DMA_CTL_DAM_CONSTANT;
	} else if (config->channel_direction == PERIPHERAL_TO_MEMORY)  {
		ctl = DMA_CTL_SRC_TYPE(config->dma_slot) |
		      DMA_CTL_SAM_CONSTANT |
		      DMA_CTL_DST_TYPE(DMA_ID_MEM);
	} else {
		ctl = DMA_CTL_SRC_TYPE(DMA_ID_MEM) |
		      DMA_CTL_DST_TYPE(DMA_ID_MEM);
	}
	/** extern for actions dma interleaved mode */
	if (config->reserved == 1 && config->channel_direction == MEMORY_TO_PERIPHERAL) {
		cregs->saddr1 = (uint32_t)head_block->source_address;
		ctl |= DMA_CTL_ADUDIO_TYPE_SEP;
	}else if(config->reserved == 1 && config->channel_direction == PERIPHERAL_TO_MEMORY) {
		cregs->daddr1 = (uint32_t)head_block->dest_address;
		ctl |= DMA_CTL_ADUDIO_TYPE_SEP;
	}

	if (config->source_data_size) {
		data_width = config->source_data_size;
	}

	if (config->dest_data_size) {
		data_width = config->dest_data_size;
	}

	if (head_block->source_reload_en || head_block->dest_reload_en) {
		ctl |= DMA_CTL_RELOAD;
	}

	switch (data_width) {
	case 2:
		ctl |= DMA_CTL_TWS_16BIT;
		break;
	case 4:
		ctl |= DMA_CTL_TWS_32BIT;
		break;
	case 1:
	default:
		ctl |= DMA_CTL_TWS_8BIT;
		break;
	}
	cregs->ctl = ctl;
	return 0;

}

static int dma_acts_start(struct device *dev, uint32_t channel)
{

	struct dma_acts_data *ddev = DEV_DATA(dev);
	struct dma_acts_channel *chan  = &ddev->channels[channel];
	struct acts_dma_chan_reg *cregs = DMA_CHAN(ddev->base, channel);
	struct acts_dma_reg *gregs = (struct acts_dma_reg *)ddev->base;

	uint32_t key;
	
	if (channel >= ddev->chan_num) {
		return -EINVAL;
	}
	key = irq_lock();	
	/* clear old irq pending */
	gregs->dma_ip = DMA_PD_TCIP(channel) | DMA_PD_HFIP(channel);

	gregs->dma_ie &= ~( DMA_IE_TCIP(channel) | DMA_IE_HFIP(channel));

	/* enable dma channel full complete irq? */
	if (chan->complete_callback_en) {
		gregs->dma_ie |= DMA_IE_TCIP(channel) ;
	}

	/* start dma transfer */
	cregs->start |= DMA_START_START;
	irq_unlock(key);

	return 0;
}

static int dma_acts_stop(struct device *dev, uint32_t channel)
{
	struct dma_acts_data *ddev = DEV_DATA(dev);
	struct acts_dma_chan_reg *cregs = DMA_CHAN(ddev->base, channel);
	struct acts_dma_reg *gregs = (struct acts_dma_reg *)ddev->base;
	uint32_t key;

	if (channel >= ddev->chan_num) {
		return -EINVAL;
	}
	key = irq_lock();
	gregs->dma_ie &= ~( DMA_IE_TCIP(channel) | DMA_IE_HFIP(channel));
	/* clear old irq pending */
	gregs->dma_ip = DMA_PD_TCIP(channel) | DMA_PD_HFIP(channel);	

	/* disable reload brefore stop dma */
	cregs->ctl &= ~DMA_CTL_RELOAD;	
	cregs->start &= ~DMA_START_START;
	irq_unlock(key);

	return 0;

}

static int dma_acts_reload(struct device *dev, uint32_t channel,
			   uint32_t src, uint32_t dst, size_t size)
{

	struct dma_acts_data *ddev = DEV_DATA(dev);
	struct acts_dma_chan_reg *cregs = DMA_CHAN(ddev->base, channel);
	uint32_t key;

	if (channel >= ddev->chan_num) {
		return -EINVAL;
	}
	key = irq_lock();
	cregs->saddr0 = src;
	cregs->daddr0 = dst;
	cregs->bc = size;
	irq_unlock(key);
	return 0;

}

static int dma_acts_get_status(struct device *dev, uint32_t channel,
			       struct dma_status *stat)
{
	struct dma_acts_data *ddev = DEV_DATA(dev);
	struct acts_dma_chan_reg *cregs = DMA_CHAN(ddev->base, channel);
	struct dma_acts_channel *chan  = &ddev->channels[channel];

	if (channel >= ddev->chan_num || stat == NULL) {
		return -EINVAL;
	}

	if (cregs->start) {
		stat->busy = true;
		stat->pending_length = cregs->rc;
	} else {
		stat->busy = false;
		stat->pending_length = 0;
	}
	stat->dir = chan->channel_direction;
	return 0;
}

static int dma_acts_request(const struct device *dev, uint32_t channel)
{
	return channel;
}
static void dma_acts_free(const struct device *dev, uint32_t channel)
{

}
				   

DEVICE_DECLARE(dma_acts_0);

#define DMA_ACTS_IRQ_CONNECT(n)						 \
	do {								 \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(0, n, irq),		 \
			    DT_INST_IRQ_BY_IDX(0, n, priority),		 \
			    dma_acts_isr, n, 0);	 \
		irq_enable(DT_INST_IRQ_BY_IDX(0, n, irq));		 \
		data->chan_num++; \
	} while (0)

static int dma_acts_init(struct device *dev)
{
	struct dma_acts_data *data = DEV_DATA(dev);

	data->base = DT_INST_REG_ADDR(0);
	acts_clock_peripheral_enable(DT_INST_PROP(0, clkid));
	acts_reset_peripheral(DT_INST_PROP(0, rstid));
	data->chan_num = 0;
#if DT_INST_IRQ_HAS_IDX(0, 0)
	DMA_ACTS_IRQ_CONNECT(0);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 1)
	DMA_ACTS_IRQ_CONNECT(1);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 2)
	DMA_ACTS_IRQ_CONNECT(2);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 3)
	DMA_ACTS_IRQ_CONNECT(3);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 4)
	DMA_ACTS_IRQ_CONNECT(4);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 5)
	DMA_ACTS_IRQ_CONNECT(5);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 6)
	DMA_ACTS_IRQ_CONNECT(6);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 7)
	DMA_ACTS_IRQ_CONNECT(7);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 8)
	DMA_ACTS_IRQ_CONNECT(8);
#endif
#if DT_INST_IRQ_HAS_IDX(0, 9)
	DMA_ACTS_IRQ_CONNECT(9);
#endif
	printk("dma-num=%d\n", data->chan_num);
	return 0;
}


static const struct dma_driver_api dma_acts_api = {
	.config = dma_acts_config,
	.start = dma_acts_start,
	.stop = dma_acts_stop,
	.reload = dma_acts_reload,
	.get_status = dma_acts_get_status,
	.request = dma_acts_request,
	.free = dma_acts_free, 

};

DEVICE_AND_API_INIT(dma_acts_0, DT_INST_LABEL(0), &dma_acts_init,
		    &dmac_data, NULL, POST_KERNEL,
		    1, &dma_acts_api);
