/*
    <:copyright-BRCM:2013:DUAL/GPL:standard
    
       Copyright (c) 2013 Broadcom Corporation
       All Rights Reserved
    
    Unless you and Broadcom execute a separate written software license
    agreement governing use of this software, this software is licensed
    to you under the terms of the GNU General Public License version 2
    (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
    with the following added to such license:
    
       As a special exception, the copyright holders of this software give
       you permission to link this software with independent modules, and
       to copy and distribute the resulting executable under terms of your
       choice, provided that you also meet, for each linked independent
       module, the terms and conditions of the license of that module.
       An independent module is a module which is not derived from this
       software.  The special exception does not apply to any modifications
       of the software.
    
    Not withstanding the above, under no circumstances may you combine
    this software in any way with any other Broadcom software provided
    under a license other than the GPL, without Broadcom's express prior
    written consent.
    
    :> 

*/

#include "spidevices.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
#else
typedef unsigned long uintptr_t;
#endif // LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)

#define MOCA_RD(x)    ((((struct moca_platform_data *)priv->pdev->dev.platform_data)->useSpi == 0) ? \
                       (*((volatile uint32_t *)((unsigned long)(x) | KSEG1))) : \
                       ((uint32_t)kerSysBcmSpiSlaveReadReg32(0, (uint32_t)(x))))

#define MOCA_RD8(x, y) ((((struct moca_platform_data *)priv->pdev->dev.platform_data)->useSpi == 0) ? \
                        (*(y) = *((volatile unsigned char *)((unsigned long)(x) | KSEG1))) : \
                        (kerSysBcmSpiSlaveRead(0, (unsigned long)(x), y, 1)))

#define MOCA_WR(x,y)   do { ((((struct moca_platform_data *)priv->pdev->dev.platform_data)->useSpi == 0) ? \
                            (*((volatile uint32_t *)((unsigned long)(x) | KSEG1))) = (y) : \
                            kerSysBcmSpiSlaveWriteReg32(0, (uint32_t)(x), (y))); } while(0)

#define MOCA_WR8(x,y)    do { ((((struct moca_platform_data *)priv->pdev->dev.platform_data)->useSpi == 0) ? \
                               (*((volatile unsigned char *)((unsigned long)(x) | KSEG1))) = (unsigned char)(y) : \
                               kerSysBcmSpiSlaveWrite(0, (unsigned long)(x), (y), 1)); } while(0)

#define MOCA_WR16(x,y)   do { ((((struct moca_platform_data *)priv->pdev->dev.platform_data)->useSpi == 0) ? \
                               (*((volatile unsigned short *)((unsigned long)(x) | KSEG1))) = (unsigned short)(y) : \
                               kerSysBcmSpiSlaveWrite(0, (unsigned long)(x), (y), 2)); } while(0)

#define I2C_RD(x)		MOCA_RD(x)
#define I2C_WR(x, y)		MOCA_WR(x, y)

static void bogus_release(struct device *dev)
{
}

static struct moca_platform_data moca_data = {
	.macaddr_hi =		0x00000102,
	.macaddr_lo =		0x03040000,

	.bcm3450_i2c_base =	0x10000180,
	.bcm3450_i2c_addr =	0x70,

	.hw_rev =		0,
	.rf_band =		MOCA_BAND_HIGHRF,
	.chip_id =		0,
#if defined(CONFIG_BCM_6802_MoCA)
	.useDma           = 0,
	.useSpi           = 1,
#else
	.useDma           = 1,
	.useSpi           = 0,
#endif
#ifdef CONFIG_SMP
	.smp_processor_id = 1,
#endif
};

static struct resource moca_resources[] = {
	[0] = {
		.start =	0x10d00000,
		.end =		0x10da1453,
		.flags =	IORESOURCE_MEM,
	},
	[1] = {
		.start =	50,
		.end =		50,
		.flags =	IORESOURCE_IRQ,
	},
};

static struct platform_device moca_plat_dev = {
	.name =			"bmoca",
	.id =			0,
	.num_resources =	ARRAY_SIZE(moca_resources),
	.resource =		moca_resources,
	.dev = {
		.platform_data = &moca_data,
		.release =	bogus_release,
	},
};

#ifdef DSL_MOCA
static struct moca_platform_data moca1_data = {
	.macaddr_hi       = 0x00000102,
	.macaddr_lo       = 0x03040000,

	.bcm3450_i2c_base = 0x10000180,
	.bcm3450_i2c_addr = 0x70,

	.hw_rev	=		0,
	.chip_id =		0,
	
	.rf_band =		MOCA_BAND_WANRF,

	.useDma           = 0,
	.useSpi           = 1,
#ifdef CONFIG_SMP
	.smp_processor_id = 1,
#endif
};

static struct resource moca1_resources[] = {
	[0] = {
			.start = 0x10d00000,
			.end   = 0x10da1453,
			.flags = IORESOURCE_MEM,
	},
	[1] = {
			.start = 32,
			.end   = 32,
			.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device moca1_plat_dev = {
	.name          = "bmoca",
	.id            = 10,
	.num_resources = ARRAY_SIZE(moca1_resources),
	.resource      = moca1_resources,
	.dev           = {
		.platform_data = &moca1_data,
		.release       = bogus_release,
							},
};
#endif

static uint32_t moca_irq_status(struct moca_priv_data *priv, int flush)
{
	uint32_t stat = 0;
	uint32_t reg;
	unsigned long flags;

	/* We can't read moca core regs unless the core's clocks are on. */
	spin_lock_irqsave(&priv->clock_lock, flags);
	reg = MOCA_RD(0x10000010);

	if ((reg & 0x180) == 0x180)
	{
		stat = MOCA_RD(0x10da1414);
		if(flush == FLUSH_IRQ) {
			MOCA_WR(0x10da1414, stat);
			MOCA_RD(0x10da1414);
		}
		if(flush == FLUSH_DMA_ONLY) {
			MOCA_WR(0x10da1414, stat & M2H_NEXTCHUNK);
			MOCA_RD(0x10da1414);
		}
		if(flush == FLUSH_REQRESP_ONLY) {
			MOCA_WR(0x10da1414, stat & ( M2H_REQ | M2H_RESP) );
			MOCA_RD(0x10da1414);
		}
	}
	spin_unlock_irqrestore(&priv->clock_lock, flags);

	return(stat);
}

static void moca_enable_irq(struct moca_priv_data *priv)
{
#if defined(CONFIG_BCM_6802_MoCA)
	kerSysMocaHostIntrEnable();
#else
	enable_irq(priv->irq);
#endif
}

static void moca_disable_irq(struct moca_priv_data *priv)
{
#if defined(CONFIG_BCM_6802_MoCA)
	kerSysMocaHostIntrDisable();
#else
	if(in_interrupt())
		disable_irq_nosync(priv->irq);
	else
		disable_irq(priv->irq);
#endif
}

static void moca_hw_reset(struct moca_priv_data *priv)
{
	unsigned long flags;

	/* Reset moca cpu first */
	MOCA_UNSET(0x10000010, 0x200);
    udelay(20);
    /* Reset sys */
	spin_lock_irqsave(&priv->clock_lock, flags);
	MOCA_UNSET(0x10000010, 0x300);
    udelay(20);    
    /* Reset MOCA */
	MOCA_UNSET(0x10000010, 0x380);    
	spin_unlock_irqrestore(&priv->clock_lock, flags);
}

static uint32_t moca_get_vco(struct moca_priv_data *priv)
{
    uint32_t misc_strap_bus;
    uint32_t vco;
    
	/* Find the VCO frequency value */
	misc_strap_bus = MOCA_RD(0x10001814);
	misc_strap_bus &= 0xf8000000;
	misc_strap_bus >>= 27;
    
	switch (misc_strap_bus) {
		case 27:
		case 26:
		case 25:
		case 24:
		case  4:
			vco = 1200;
			break;

		case 23:
		case 15:
		case 14:
		case 13:
		case 12:
		case 11:
		case  8:
			vco = 1800;
			break;

		case 20:
			vco = 2400;
			break;

		case  9:
			vco = 1350;
			break; 

		case  7:
		case  6:
		case  5:
		case  1:
			vco = 2000;
			break;

		default:
			printk(KERN_ERR "MOCA: UNSUPPORTED STRAP BUS SETTING 0x%x\n",
				misc_strap_bus);
		case 30:
		case 29:
		case 28:
		case 22:
		case 21:
		case 10:
		case  3:
		case  2:
		case  0:
			vco = 1600;
			break;
	}

    return(vco);
}

#define MDIV9_FACTOR  222
#define MDIV10_FACTOR 200//225

static uint32_t moca_get_misc_clk_strap(struct moca_priv_data *priv)
{
	uint32_t vco;
	uint32_t ret_val;
	uint8_t  mdiv10, mdiv9;
	
//    	uint32_t misc_strap_bus;
    
//	misc_strap_bus = MOCA_RD(0x10001814);
//	misc_strap_bus &= 0xf8000000;
//	misc_strap_bus >>= 27;

	vco = moca_get_vco(priv);

	mdiv9 = vco / MDIV9_FACTOR;
	if ((vco % MDIV9_FACTOR) >= (MDIV9_FACTOR / 2))
		mdiv9++;
   
	mdiv10 = vco / MDIV10_FACTOR;
	if ((vco % MDIV10_FACTOR) >= (MDIV10_FACTOR / 2))
		mdiv10++;

	if (priv->continuous_power_tx_mode)
		mdiv10 = vco / 100;

	ret_val = (mdiv10 << 8) | mdiv9;

//	printk(KERN_INFO "MOCA %d: MIPS PLL VCO = %u, VCOFreq = %u\n", 
//		priv->pdev->id, misc_strap_bus, vco);
//	printk(KERN_INFO "MOCA %d: PHY Clk Div Factor = %u,  CPU Clk Div Factor = %u\n", 
//		priv->pdev->id, mdiv10, mdiv9);

	return(ret_val);
}

static unsigned int moca_get_phy_freq(struct moca_priv_data *priv)
{
	unsigned int x = moca_get_misc_clk_strap(priv);

	x = x >> 8;

	return( ((moca_get_vco(priv)*2)/ x + 1) / 2); 
}

static unsigned int moca_get_cpu_freq(struct moca_priv_data *priv)
{
	unsigned int x = moca_get_misc_clk_strap(priv);

	x = x & 0xff;

	return( ((moca_get_vco(priv)*2)/ x + 1) / 2); 
}


/* Must have dev_mutex when calling this function */
static void moca_hw_init(struct moca_priv_data *priv, int action)
{
#if defined(DSL_MOCA)
	int      ret;
	uint16_t moca_gpio_pin;
#endif
	uint32_t misc_moca_clk_strap = 0x0c09;
	unsigned long flags;

	moca_hw_reset(priv);

#if defined(DSL_MOCA)
	if (action == MOCA_ENABLE)
		misc_moca_clk_strap = moca_get_misc_clk_strap(priv);
#endif
	/* MISC moca clk strap */
	MOCA_WR(0x1000181c, misc_moca_clk_strap);
	MOCA_WR(0x10001820, 0x01);
	MOCA_RD(0x10001820);
	msleep(100);
	MOCA_WR(0x10001820, 0x00);
	MOCA_RD(0x10001820);

	/* release core from reset */
	spin_lock_irqsave(&priv->clock_lock, flags);
	MOCA_SET(0x10000010, 0x180);
	MOCA_RD(0x10000010);
	spin_unlock_irqrestore(&priv->clock_lock, flags);

#if defined(DSL_MOCA)
	ret = BpGetMoCALedGpio( &moca_gpio_pin ) ;

	if ( BP_SUCCESS == ret ) {
		unsigned long flags;

		spin_lock_irqsave(&bcm_gpio_spinlock, flags);
		moca_gpio_pin &= BP_GPIO_NUM_MASK;
		MOCA_SET(0x10000098, (1 << moca_gpio_pin));
		MOCA_SET(0x10000084, (1 << moca_gpio_pin));
		MOCA_UNSET(0x100000a4, 0xf00);

		if (action == MOCA_DISABLE)
		{
			/* turn off the MoCA LED */
			if ((moca_gpio_pin & BP_ACTIVE_MASK) == BP_ACTIVE_HIGH)
				MOCA_SET(0x100000a4, 0xb00);
			else
				MOCA_SET(0x100000a4, 0x800);
		}
		spin_unlock_irqrestore(&bcm_gpio_spinlock, flags);
	}
#endif

	/* enable and clear all h2m/m2h interrupts */
	MOCA_WR(0x10da1400, priv->h2m_req_bit[1] | priv->h2m_resp_bit[1] );
	MOCA_RD(0x10da1400);
	MOCA_WR(0x10da1408, priv->h2m_req_bit[1] | priv->h2m_resp_bit[1] );
	MOCA_RD(0x10da1408);

	MOCA_WR(0x10da140c, M2H_REQ | M2H_RESP | M2H_ASSERT | M2H_WDT_CPU1 |
		M2H_NEXTCHUNK);
	MOCA_RD(0x10da140c);
	MOCA_WR(0x10da1414, M2H_REQ | M2H_RESP | M2H_ASSERT | M2H_WDT_CPU1 |
		M2H_NEXTCHUNK);
	MOCA_RD(0x10da1414);
}

static void moca_ringbell(struct moca_priv_data *priv, uint32_t mask)
{
	MOCA_WR(0x10da1404, mask);
}

static uint32_t moca_start_mips(struct moca_priv_data *priv, unsigned int cpuid)
{
#if 0

#define OFF_TEST_MUX_SEL	0x000a142c

	// Enable test ports
	printk ("moca: Configuring Test ports.\n");
	MOCA_UNSET(0x100000b8, 0x7); //GPIO->GPIOBaseMode &= 0xfffffff8;
	MOCA_SET(0x100000b8, 0x2);   //GPIO->GPIOBaseMode |= 0x02;
	MOCA_WR(0x100000a8, 0x400);  //GPIO->TestControl = 0x400;
	MOCA_WR(0x10000014, 0x680);  //PERF->diagControl = 0x680;

   // To enable (most) of outputs as GPIOs, set to 0x30. It will  give accessibility to the following GPIOs
   // 0-4, 8, 9, 11, 13, 14, 16-23, 26-29, 31
	MOCA_WR(0x10001830, 0x3f);   //MISC->miscGpioDiagOverlay = 0x3f;  

	MOCA_UNSET(priv->base + OFF_TEST_MUX_SEL, 0xf); //MoCA_BLOCK->extras.testMuxSel &= 0xfffffff0;
	MOCA_SET(priv->base + OFF_TEST_MUX_SEL, 0x1); //MoCA_BLOCK->extras.testMuxSel |= 0x1;
	// Enable test ports end
#endif


	MOCA_SET(0x10000010, 1 << 9);
	return(0);
}

static void moca_m2m_xfer(struct moca_priv_data *priv,
	uint32_t dst, uint32_t src, uint32_t ctl)
{
	uint32_t status;

	MOCA_WR(priv->base + priv->m2m_src_offset, src);
	MOCA_WR(priv->base + priv->m2m_dst_offset, dst);
	MOCA_WR(priv->base + priv->m2m_status_offset, 0);
	MOCA_RD(priv->base + priv->m2m_status_offset);
	MOCA_WR(priv->base + priv->m2m_cmd_offset, ctl);

	do {
		status = MOCA_RD(priv->base + priv->m2m_status_offset);
	} while(status == 0);

#if 0
	//printk("%s: transfer args: %08x %08x %08x\n", __FUNCTION__, src, dst, ctl);
	printk("%s: regs: %08x %08x %08x %08x\n", __FUNCTION__,
		MOCA_RD(priv->base + priv->m2m_status_offset),
		MOCA_RD(priv->base + priv->m2m_src_offset), 
		MOCA_RD(priv->base + priv->m2m_dst_offset), 
		MOCA_RD(priv->base + priv->m2m_cmd_offset));
	if(status & (3 << 29))
		printk(KERN_WARNING "%s: bad status %08x "
			"(s/d/c %08x %08x %08x)\n", __FUNCTION__,
			status, src, dst, ctl);
#endif    
}

static void moca_write_mem(struct moca_priv_data *priv,
	uint32_t dst_offset, void *src, unsigned int len)
{
	struct moca_platform_data *pd = priv->pdev->dev.platform_data;

	if((dst_offset >= priv->cntl_mem_offset+priv->cntl_mem_size) ||
		((dst_offset + len) > priv->cntl_mem_offset+priv->cntl_mem_size)) {
		printk(KERN_WARNING "%s: copy past end of cntl memory: %08x\n",
			__FUNCTION__, dst_offset);
		return;
	}

	if ( 1 == pd->useDma )
	{
		dma_addr_t pa;

		pa = dma_map_single(&priv->pdev->dev, src, len, DMA_TO_DEVICE);
		mutex_lock(&priv->copy_mutex);
		moca_m2m_xfer(priv, dst_offset + priv->data_mem_offset, (uint32_t)pa, len | M2M_WRITE);
		mutex_unlock(&priv->copy_mutex);
		dma_unmap_single(&priv->pdev->dev, pa, len, DMA_TO_DEVICE);
	}
	else
	{
		uintptr_t addr = (uintptr_t)priv->base + priv->data_mem_offset + dst_offset;
		uint32_t *data = src;
		int i;

		mutex_lock(&priv->copy_mutex);
		for(i = 0; i < len; i += 4, addr += 4, data++)
			MOCA_WR(addr, be32_to_cpu(*data));
		MOCA_RD(addr - 4);	/* flush write */
		mutex_unlock(&priv->copy_mutex);
	}
}

static void moca_read_mem(struct moca_priv_data *priv,
	void *dst, uint32_t src_offset, unsigned int len)
{
	struct moca_platform_data *pd = priv->pdev->dev.platform_data;
    
	if((src_offset >= priv->cntl_mem_offset+priv->cntl_mem_size) ||
		((src_offset + len) > priv->cntl_mem_offset+priv->cntl_mem_size)) {
		printk(KERN_WARNING "%s: copy past end of cntl memory: %08x\n",
			__FUNCTION__, src_offset);
		return;
	}

	if ( 1 == pd->useDma )
	{
		dma_addr_t pa;

		pa = dma_map_single(&priv->pdev->dev, dst, len, DMA_FROM_DEVICE);
		mutex_lock(&priv->copy_mutex);
		moca_m2m_xfer(priv, (uint32_t)pa, src_offset + priv->data_mem_offset, len | M2M_READ);
		mutex_unlock(&priv->copy_mutex);
		dma_unmap_single(&priv->pdev->dev, pa, len, DMA_FROM_DEVICE);
	}
	else
	{
		uintptr_t addr = priv->data_mem_offset + src_offset;
		uint32_t *data = dst;
		int i;

		mutex_lock(&priv->copy_mutex);
		for(i = 0; i < len; i += 4, addr += 4, data++)
			*data = cpu_to_be32(MOCA_RD((uintptr_t)priv->base + addr));
		mutex_unlock(&priv->copy_mutex);
	}
}

static void moca_write_sg(struct moca_priv_data *priv,
	uint32_t dst_offset, struct scatterlist *sg, int nents)
{
	int j;
	uintptr_t addr = priv->data_mem_offset + dst_offset;
	struct moca_platform_data *pd = priv->pdev->dev.platform_data;

	dma_map_sg(&priv->pdev->dev, sg, nents, DMA_TO_DEVICE);

	mutex_lock(&priv->copy_mutex);
	for(j = 0; j < nents; j++)
	{
		if ( 1 == pd->useDma )
		{
		    // printk("XXX copying page %d, PA %08x\n", j, (int)sg[j].dma_address);
			moca_m2m_xfer(priv, addr, (uint32_t)sg[j].dma_address, 
				sg[j].length | M2M_WRITE);

			addr += sg[j].length;
		}
		else
		{
			unsigned long *data = (void *)phys_to_virt(sg[j].dma_address);

			kerSysBcmSpiSlaveWriteBuf(0, ((unsigned long)priv->base) + addr, data, sg[j].length, 4);
			addr += sg[j].length;
		}
	}
	mutex_unlock(&priv->copy_mutex);

	dma_unmap_sg(&priv->pdev->dev, sg, nents, DMA_TO_DEVICE);
}

/* NOTE: this function is not tested */
#if 0
static void moca_read_sg(struct moca_priv_data *priv,
	uint32_t src_offset, struct scatterlist *sg, int nents)
{
	int j;
	uintptr_t addr = priv->data_mem_offset + src_offset;

	dma_map_sg(&priv->pdev->dev, sg, nents, DMA_FROM_DEVICE);

	mutex_lock(&priv->copy_mutex);
	for(j = 0; j < nents; j++) {
#if 0 //USE_DMA
		 printk("XXX copying page %d, PA %08x\n", j, (int)sg[j].dma_address);
		moca_m2m_xfer(priv, addr, (uint32_t)sg[j].dma_address,
			sg[j].length | M2M_READ);

		addr += sg[j].length;
#else
		uint32_t *data = (void *)phys_to_virt(sg[j].dma_address);
		unsigned int len = sg[j].length;
		int i;

		for(i = 0; i < len; i += 4, addr += 4, data++) {
			*data = cpu_to_be32(
				MOCA_RD((uintptr_t)priv->base + addr));
			//printk("MoCA READ: AD 0x%x  = 0x%x (0x%x)\n", (priv->base + addr), MOCA_RD((uintptr_t)priv->base + addr), *data);
		 }
#endif
	}
	mutex_unlock(&priv->copy_mutex);

	dma_unmap_sg(&priv->pdev->dev, sg, nents, DMA_FROM_DEVICE);
}
#endif

static void moca_read_mac_addr(struct moca_priv_data *priv, uint32_t * hi, uint32_t * lo)
{
	struct net_device * pdev ;
	char					 mocaName[7] ;

	if (priv == NULL)
		sprintf (mocaName, "moca%u", 0) ;
	else
		sprintf (mocaName, "moca%u", priv->pdev->id) ;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 29)
	pdev = dev_get_by_name ( &init_net, mocaName ) ;
#else
	pdev = dev_get_by_name ( mocaName ) ;
#endif

	if ((pdev != NULL) && (lo != NULL) && (hi != NULL)) {
		mac_to_u32(hi, lo, pdev->dev_addr);
	}
}


#if defined(DSL_MOCA)

/*
 * This helper function was added to allow the enet driver to compile in
 * consumer environment for 68xx profiles.
 */
void moca_get_fc_bits(void * arg, unsigned long *moca_fc_reg)
{
	struct moca_priv_data *     priv;
	struct moca_platform_data * pMocaData;
	unsigned long               flags;

	if (arg == NULL) {
		return;
	}

	priv = (struct moca_priv_data *) arg;
	pMocaData = (struct moca_platform_data *)priv->pdev->dev.platform_data;

	*moca_fc_reg = 0;
	if (priv != NULL)
	{
		/* We can't read moca core regs unless the core's clocks are on. */
		spin_lock_irqsave(&priv->clock_lock, flags);
		if (priv->running) {
			*moca_fc_reg = MOCA_RD(priv->sideband_gmii_fc_offset);
		}
		spin_unlock_irqrestore(&priv->clock_lock, flags);
	}
}

#endif /* DSL_MOCA */


extern void bcmenet_register_moca_fc_bits_cb(void cb(void *, unsigned long *), int isWan, void * arg);

static int  hw_specific_init( struct moca_priv_data *priv )
{
#ifdef DSL_MOCA
	struct moca_platform_data *pMocaData;
	pMocaData = (struct moca_platform_data *)priv->pdev->dev.platform_data;

	/* fill in the hw_rev field */
	pMocaData->chip_id = MOCA_RD(0x10000000);
	pMocaData->hw_rev = HWREV_MOCA_20;

	bcmenet_register_moca_fc_bits_cb(
		moca_get_fc_bits, pMocaData->useSpi ? 1 : 0, (void *)priv);
#endif

	return 0;
}

static void moca_3450_write(struct moca_priv_data *priv, u8 addr, u32 data)
{
	if (((struct moca_platform_data *)priv->pdev->dev.platform_data)->useSpi == 0)
		bcm3450_write_reg(addr, data);
	else
		moca_3450_write_i2c(priv, addr, data);
}

static u32 moca_3450_read(struct moca_priv_data *priv, u8 addr)
{
	if (((struct moca_platform_data *)priv->pdev->dev.platform_data)->useSpi == 0)
		return(bcm3450_read_reg(addr));
	else
		return(moca_3450_read_i2c(priv, addr));
}

/*
 * PM STUBS
 */

struct clk *clk_get(struct device *dev, const char *id)
{
	return NULL;
}

int clk_enable(struct clk *clk)
{
	return 0;
}

void clk_disable(struct clk *clk)
{
}

void clk_put(struct clk *clk)
{
}
