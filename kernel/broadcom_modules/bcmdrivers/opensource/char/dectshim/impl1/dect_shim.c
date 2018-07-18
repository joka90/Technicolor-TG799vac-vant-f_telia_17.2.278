/****************************************************************************
* <:copyright-BRCM:2013:DUAL/GPL:standard
* 
*    Copyright (c) 2013 Broadcom Corporation
*    All Rights Reserved
* 
* Unless you and Broadcom execute a separate written software license
* agreement governing use of this software, this software is licensed
* to you under the terms of the GNU General Public License version 2
* (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
* with the following added to such license:
* 
*    As a special exception, the copyright holders of this software give
*    you permission to link this software with independent modules, and
*    to copy and distribute the resulting executable under terms of your
*    choice, provided that you also meet, for each linked independent
*    module, the terms and conditions of the license of that module.
*    An independent module is a module which is not derived from this
*    software.  The special exception does not apply to any modifications
*    of the software.
* 
* Not withstanding the above, under no circumstances may you combine
* this software in any way with any other Broadcom software provided
* under a license other than the GPL, without Broadcom's express prior
* written consent.
* 
* :>
*
* File Name  : dect_shim.c
*
* Description: This file contains Linux character device driver entry points
*              for the dectshim driver.
*
* Updates    : 03/04/2013  Alliu Created.
***************************************************************************/
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <asm/uaccess.h>
#include <linux/poll.h>
#include <linux/dma-mapping.h>
#include <linux/kernel.h>
#include <linux/delay.h>

#include <dectshimdrv.h>

#include <bcm_map_part.h>
#include <boardparms.h>
#include <boardparms_voice.h>

#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) 
#include <pmc_drv.h>
#include <BPCM.h>
#include <linux/dma-mapping.h>
#endif

/****************************************************************************
* Macro Definitions / DEFINES
****************************************************************************/
#define XFER_SIZE_160_BYTES
#define DECT_DMA_NUM_CHAN_MAX    4         
#define DECT_DMA_NUM_CHAN    DECT_DMA_NUM_CHAN_MAX         
#define DMA_LOOPBACK_CH0_CH1
#define DMA_UBUS_BYTES    8 

#define ALIGN_DATA(addr, boundary)       ((addr + boundary - 1) & ~(boundary - 1))

#ifdef XFER_SIZE_160_BYTES
  #define DECT_DMA_TRANSFER_SIZE_BYTES       160                                      /* 5ms tick, 80 Wideband-16bit samples                  */
  #define DECT_DMA_TX_TRANSFER_SIZE_BYTES    160                                      /* 5ms tick, 80 Wideband-16bit samples                  */
  #define DECT_DMA_BUFFER_SIZE_BYTES         (2 * DECT_DMA_TRANSFER_SIZE_BYTES )      /* Size for single direction, one slot, double buffered  */
  #define DECT_DMA_TX_BUFFER_SIZE_BYTES      (2 * DECT_DMA_TX_TRANSFER_SIZE_BYTES )   /* Size for single direction, one slot, double buffered  */
#endif

#define DECT_DMA_TRANSFER_SIZE_WORDS      (DECT_DMA_TRANSFER_SIZE_BYTES/4)     /* In 32-bit words                                       */
#define DECT_DMA_BUFFER_SIZE_WORDS        (DECT_DMA_BUFFER_SIZE_BYTES/4)       /* In 32-bit words                                       */
#define DECT_DMA_TX_TRANSFER_SIZE_WORDS   (DECT_DMA_TX_TRANSFER_SIZE_BYTES/4)  /* In 32-bit words                                       */
#define DECT_DMA_TX_BUFFER_SIZE_WORDS     (DECT_DMA_TX_BUFFER_SIZE_BYTES/4)    /* In 32-bit words                                       */
#define DECT_DMA_ALIGNMENT_OVERHEAD        DMA_UBUS_BYTES
#define DECT_DMA_PLL_DELAY_TIME            10

/* Need to restrict range of DMA memory for DECT DMA to within the first 256MB,
 * because DECT DMA can only access first 256MB of memory.
 * NOTE: This needs ZONE_DMA support to be compiled in the kernel */
#define DECT_DMA_VALID_DDR_ADDR_BITS       28

/****************************************************************************
* ProtoTypes
****************************************************************************/
int dectshim_open(struct inode *inode, struct file *filp);
int dectshim_release(struct inode *inode, struct file *filp);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
static long dectshim_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
#else
static int dectshim_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
#endif

static int dectI_dma_init(void);
static int dectI_dma_deinit(void);
static void dectI_pll_deinit(void);
static int dectI_pll_config(void);
static int dectI_dma_config(void);
static void dectI_dumpRegs(void);
static int dectI_Init( void );
static int dectI_Deinit( void );
static void dectI_dectblock_reset( void );

static int dectshimInit(unsigned long args);
static int dectshimDeinit(unsigned long args);
static int dectshimPLLConfig(unsigned long args);
static int dectshimDMAConfig(unsigned long args);
static int dectshimGetChs(unsigned long args);
static int dectshimGetVoiceParams( void );
static int dectshimGetVirtualAddr(unsigned long args);

typedef int (*FN_IOCTL) (unsigned long arg);
/****************************************************************************
* Variables
****************************************************************************/
typedef dma_addr_t   DMA_HANDLE;
DMA_HANDLE            dmaBufHandleTx, dmaBufHandleRx;
unsigned short           rxBufferArea = sizeof(unsigned long)*(DECT_DMA_NUM_CHAN * DECT_DMA_BUFFER_SIZE_BYTES/4) + (DECT_DMA_ALIGNMENT_OVERHEAD/4);
unsigned short           txBufferArea = sizeof(unsigned long)*(DECT_DMA_NUM_CHAN * DECT_DMA_BUFFER_SIZE_BYTES/4) + (DECT_DMA_ALIGNMENT_OVERHEAD/4);

volatile unsigned long *dectTxDMABuff; 
volatile unsigned long *dectRxDMABuff;

/* Pointer to DMA buffer start addresses */
volatile unsigned long * pDectTxDmaDDRStartAddr[DECT_DMA_NUM_CHAN];
volatile unsigned long * pDectRxDmaDDRStartAddr[DECT_DMA_NUM_CHAN];
volatile unsigned long * pDectTxDmaDDRStartAddrV[DECT_DMA_NUM_CHAN];
volatile unsigned long * pDectRxDmaDDRStartAddrV[DECT_DMA_NUM_CHAN];


volatile unsigned long * pDectTxDmaAHBStartAddr[DECT_DMA_NUM_CHAN] = { (unsigned long *) DECT_AHB_CHAN0_RX, 
                                                                     (unsigned long *) DECT_AHB_CHAN1_RX, 
                                                                     (unsigned long *) DECT_AHB_CHAN2_RX, 
                                                                     (unsigned long *) DECT_AHB_CHAN3_RX };
                                                                     
volatile unsigned long * pDectRxDmaAHBStartAddr[DECT_DMA_NUM_CHAN] = { (unsigned long *) DECT_AHB_CHAN0_TX, 
                                                                     (unsigned long *) DECT_AHB_CHAN1_TX, 
                                                                     (unsigned long *) DECT_AHB_CHAN2_TX, 
                                                                     (unsigned long *) DECT_AHB_CHAN3_TX };

volatile unsigned short * startCtl = (unsigned short *) DECT_STARTCTL;

struct device    *dect_device = NULL;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
static struct class     *dect_cl = NULL;
#endif

static struct cdev dectshim_cdev;

static const struct file_operations dectshim_fops = {
    .owner =    THIS_MODULE,

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
    .unlocked_ioctl = dectshim_ioctl,
#else
    .ioctl =    dectshim_ioctl,
#endif
    .open =     dectshim_open,
    .release =  dectshim_release,
};


static VOICE_BOARD_PARMS   voiceParams;
static int  dectShimInitialized = 0;


int dectshim_open(struct inode *inode, struct file *filp)
{
   printk("dectshim open device...\n");
   return (0);
}

int dectshim_release(struct inode *inode, struct file *filp)
{
   printk("dectshim close device...\n");
   return (0);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
static long dectshim_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#else
static int dectshim_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
   int ret = -EINVAL;
   FN_IOCTL IoctlFuncs[] = {
      dectshimInit,
      dectshimDeinit,
      dectshimPLLConfig,
      dectshimDMAConfig,
      dectshimGetChs,
      dectshimGetVirtualAddr
   };

   unsigned int cmdnr = _IOC_NR(cmd);

   printk("kernel::dectshim_ioctl: command %d\n", cmdnr);

   if ( cmdnr >= 0 && cmdnr < MAX_DECTSHIM_IOCTL_CMDS )
   {
      ret = (*IoctlFuncs[cmdnr]) (arg);
   }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
   return (long) ret;
#else
   return ret;
#endif
}

/*****************************************************************************
*
*  FUNCTION: dectI_Init
*
*  PURPOSE:
*      Intialize internal dect hw
*
*  PARAMETERS: none
*
*  RETURNS: 0 on SUCCESS
*
*  NOTES:
*
*****************************************************************************/
static int dectI_Init( void )
{
   int retVal = -1;

   #if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) 	
   BPCM_PWR_ZONE_N_CONTROL pwr_zone_ctrl;
   #endif
   
   if ( ( dectShimInitialized != 0 ) || 
   #if !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM963148) 
        OTP_REGS_GET_USER_BIT(OTP_DECT_DISABLE) || 
   #endif
        (voiceParams.numDectLines == 0) )
   {
      printk("DECT is disabled by OTP/boardparms or is initialized already\n");
      return ( 0 );
   }
   else 
   {

      #if defined(CONFIG_BCM963268) || defined(CONFIG_BCM96828)
      PERF->softResetB |= SOFT_RST_DECT;
      #endif
 
      #if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) 
      /* RESET DECT via BPCM ( Block Power Control Module ) */
      ReadBPCMRegister(PMB_ADDR_DECT_UBUS, BPCMRegOffset(zones[0].control), &pwr_zone_ctrl.Reg32);
      pwr_zone_ctrl.Bits.pwr_dn_req = 0;
      pwr_zone_ctrl.Bits.dpg_ctl_en = 1;
      pwr_zone_ctrl.Bits.pwr_up_req = 1;
      pwr_zone_ctrl.Bits.mem_pwr_ctl_en = 1;
      pwr_zone_ctrl.Bits.blk_reset_assert = 1;
      WriteBPCMRegister(PMB_ADDR_DECT_UBUS, BPCMRegOffset(zones[0].control), pwr_zone_ctrl.Reg32);
      mdelay ( 1 );
      #endif
      
      printk("DECT shim power up is done\n");
 
      /* Enable master clock to dect block */
      #if !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM963148) 
      PERF->blkEnables |= DECT_CLKEN;
      #endif

      DECT_CTRL->dect_shm_ctrl &= ~AHB_SWAP_MASK;  
      DECT_CTRL->dect_shm_ctrl |= AHB_SWAP_ACCESS;

      #if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) 
      DECT_CTRL->dect_shm_ctrl |= UBUS3_SWAP;
      DECT_CTRL->dect_shim_dcxo_ctrl0 = 0x80B27003;
        
      #else  
      DECT_CTRL->dect_shm_xtal_ctrl = 0x7F400E3E;
      #endif

      retVal = dectI_pll_config();
  
      dectI_dectblock_reset();

      /* Setup DECT DMA */
      retVal |= dectI_dma_init();

      /* Dump registers */
      dectI_dumpRegs();
 
      dectShimInitialized = 1;
   }

   return retVal;
}

/*****************************************************************************
*
*  FUNCTION: dectI_Deinit
*
*  PURPOSE:
*      De-Intialize internal dect hw
*
*  PARAMETERS: none
*
*  RETURNS: 0 on SUCCESS
*
*  NOTES:
*
*****************************************************************************/
static int dectI_Deinit( void )
{
   int retVal = -1;

   #if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) 
   BPCM_PWR_ZONE_N_CONTROL pwr_zone_ctrl;
   #endif
   
   if ( dectShimInitialized != 0 )
   {
      retVal = dectI_dma_deinit();
  
      dectI_pll_deinit();
  
      dectI_dectblock_reset();

      /* turn off shim layer */ 
      #if !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM963148) 
      PERF->blkEnables &= ~DECT_CLKEN;
      #endif

      dectShimInitialized = 0;

      #if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) 
      /* RESET DECT via BPCM ( Block Power Control Module ) */
      ReadBPCMRegister(PMB_ADDR_DECT_UBUS, BPCMRegOffset(zones[0].control), &pwr_zone_ctrl.Reg32);
      pwr_zone_ctrl.Bits.pwr_dn_req = 1;
      pwr_zone_ctrl.Bits.dpg_ctl_en = 1;
      pwr_zone_ctrl.Bits.pwr_up_req = 0;
      WriteBPCMRegister(PMB_ADDR_DECT_UBUS, BPCMRegOffset(zones[0].control), pwr_zone_ctrl.Reg32);
      #endif
   }
   else
   {
      printk("dectshim driver is not initialized...\n");
   }   

   return retVal;
}

static void dectI_dectblock_reset( void )
{
   DECT_CTRL->dect_shm_ctrl |=  DECT_SOFT_RESET;
   mdelay ( 1 );
   DECT_CTRL->dect_shm_ctrl &=  ~DECT_SOFT_RESET;
   return;
}


/*****************************************************************************
*
*  FUNCTION: dectI_dma_init
*
*  PURPOSE:
*      Intialize dect DMA
*
*  PARAMETERS: none
*
*  RETURNS: 0 on SUCCESS
*
*  NOTES:
*
*****************************************************************************/
static int dectI_dma_init(void)
{      
   int retVal = -1;
   
   /* DECT DMA can only address memory upto 256M ( 28 bits ) */
#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) 
   if( dma_supported(dect_device, DMA_BIT_MASK(DECT_DMA_VALID_DDR_ADDR_BITS)) )
   {
      dect_device->coherent_dma_mask = DMA_BIT_MASK(DECT_DMA_VALID_DDR_ADDR_BITS);
   }
   else
   {
      printk(KERN_WARNING "%s: Unable to set DMA mask! Aborting.\n\n\n", __FUNCTION__);
      return retVal;
   }
#endif /* defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) */
   
   /* Allocate DMA memory */
   dectRxDMABuff = dma_alloc_coherent( dect_device, rxBufferArea, &dmaBufHandleRx, GFP_KERNEL );
   dectTxDMABuff = dma_alloc_coherent( dect_device, txBufferArea, &dmaBufHandleTx, GFP_KERNEL );

   printk("DECT DMA DDR Rx Physical Address   : 0x%08X\n", (unsigned int)dmaBufHandleRx);
   printk("DECT DMA DDR Rx Virtual  Address   : 0x%08X\n", (unsigned int)dectRxDMABuff);
   printk("DECT DMA DDR Rx Virtual  Addressptr: 0x%08X\n", (unsigned int)&dectRxDMABuff);   
   printk("DECT DMA DDR Tx Physical Address   : 0x%08X\n", (unsigned int)dmaBufHandleTx);
   printk("DECT DMA DDR Tx Virtual  Address   : 0x%08X\n", (unsigned int)dectTxDMABuff);
   printk("DECT DMA DDR Tx Virtual  Addressptr: 0x%08X\n", (unsigned int)&dectTxDMABuff);

   /* Initialize dect shim dma ctrl and dect buffer processing startCtl */
   DECT_DMA_CTRL->dect_shm_dma_ctrl = 0;

   /* Configure DMA registers */
   retVal = dectI_dma_config();

   return retVal;
}

/*****************************************************************************
*
*  FUNCTION: dectI_dma_config
*
*  PURPOSE:
*      Configure dect DMA
*
*  PARAMETERS: none
*
*  RETURNS: 0 on SUCCESS
*
*  NOTES:
*
*****************************************************************************/
static int dectI_dma_config(void)
{
   int i,j;
   
   /* Calculate DMA DDR start addresses */
   for ( i=0; i < DECT_DMA_NUM_CHAN; i++ )
   {
      /* Align DMA buffers along 64-bit boundaries */
      pDectTxDmaDDRStartAddr[i]  = (volatile unsigned long *) ( (unsigned long *) dmaBufHandleTx + i * DECT_DMA_BUFFER_SIZE_WORDS );
      pDectRxDmaDDRStartAddr[i]  = (volatile unsigned long *) ( (unsigned long *) dmaBufHandleRx + i * DECT_DMA_BUFFER_SIZE_WORDS );                   
      pDectTxDmaDDRStartAddrV[i] = (volatile unsigned long *) ( (unsigned long *) dectTxDMABuff  + i * DECT_DMA_BUFFER_SIZE_WORDS );
      pDectRxDmaDDRStartAddrV[i] = (volatile unsigned long *) ( (unsigned long *) dectRxDMABuff  + i * DECT_DMA_BUFFER_SIZE_WORDS );                   
   }

#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) 
   /* Setup DDR Start addresses -- Must convert to Physical addresses*/                  
   DECT_DMA_CTRL->dect_shm_dma_ddr_saddr_tx_s0 = ((unsigned int)pDectTxDmaDDRStartAddr[0] );
   DECT_DMA_CTRL->dect_shm_dma_ddr_saddr_rx_s0 = ((unsigned int)pDectRxDmaDDRStartAddr[0] );
#if (DECT_DMA_NUM_CHAN > 1)
   DECT_DMA_CTRL->dect_shm_dma_ddr_saddr_tx_s1 = ((unsigned int)pDectTxDmaDDRStartAddr[1] );
   DECT_DMA_CTRL->dect_shm_dma_ddr_saddr_rx_s1 = ((unsigned int)pDectRxDmaDDRStartAddr[1] );
#if (DECT_DMA_NUM_CHAN > 2)
   DECT_DMA_CTRL->dect_shm_dma_ddr_saddr_tx_s2 = ((unsigned int)pDectTxDmaDDRStartAddr[2] );
   DECT_DMA_CTRL->dect_shm_dma_ddr_saddr_rx_s2 = ((unsigned int)pDectRxDmaDDRStartAddr[2] );
#if (DECT_DMA_NUM_CHAN > 3)
   DECT_DMA_CTRL->dect_shm_dma_ddr_saddr_tx_s3 = ((unsigned int)pDectTxDmaDDRStartAddr[3] );
   DECT_DMA_CTRL->dect_shm_dma_ddr_saddr_rx_s3 = ((unsigned int)pDectRxDmaDDRStartAddr[3] );
#endif
#endif
#endif

   DECT_DMA_CTRL->dect_shm_dma_ahb_saddr_tx_s01  = ((unsigned int)pDectTxDmaAHBStartAddr[0] - (unsigned int)DECT_AHB_REG_PHYS_BASE) << 16;  /* Slot 0 */
   DECT_DMA_CTRL->dect_shm_dma_ahb_saddr_rx_s01  = ((unsigned int)pDectRxDmaAHBStartAddr[0] - (unsigned int)DECT_AHB_REG_PHYS_BASE) << 16;  /* Slot 0 */
#if (DECT_DMA_NUM_CHAN > 1)
   DECT_DMA_CTRL->dect_shm_dma_ahb_saddr_tx_s01 |= ((unsigned int)pDectTxDmaAHBStartAddr[1] - (unsigned int)DECT_AHB_REG_PHYS_BASE);         /* Slot 1 */
   DECT_DMA_CTRL->dect_shm_dma_ahb_saddr_rx_s01 |= ((unsigned int)pDectRxDmaAHBStartAddr[1] - (unsigned int)DECT_AHB_REG_PHYS_BASE);         /* Slot 1 */
#if (DECT_DMA_NUM_CHAN > 2)
   DECT_DMA_CTRL->dect_shm_dma_ahb_saddr_tx_s23  = ((unsigned int)pDectTxDmaAHBStartAddr[2] - (unsigned int)DECT_AHB_REG_PHYS_BASE) << 16;  /* Slot 2 */
   DECT_DMA_CTRL->dect_shm_dma_ahb_saddr_rx_s23  = ((unsigned int)pDectRxDmaAHBStartAddr[2] - (unsigned int)DECT_AHB_REG_PHYS_BASE) << 16;  /* Slot 2 */
#if (DECT_DMA_NUM_CHAN > 3)
   DECT_DMA_CTRL->dect_shm_dma_ahb_saddr_tx_s23 |= ((unsigned int)pDectTxDmaAHBStartAddr[3] - (unsigned int)DECT_AHB_REG_PHYS_BASE);         /* Slot 3 */
   DECT_DMA_CTRL->dect_shm_dma_ahb_saddr_rx_s23 |= ((unsigned int)pDectRxDmaAHBStartAddr[3] - (unsigned int)DECT_AHB_REG_PHYS_BASE);         /* Slot 3 */
#endif
#endif
#endif

#else  //6362, 63268

   DECT_DMA_CTRL->dect_shm_dma_ddr_saddr_tx_s0 = ((unsigned int)pDectTxDmaDDRStartAddr[0] & (unsigned int)0x1FFFFFFF );
   DECT_DMA_CTRL->dect_shm_dma_ddr_saddr_rx_s0 = ((unsigned int)pDectRxDmaDDRStartAddr[0] & (unsigned int)0x1FFFFFFF );
#if (DECT_DMA_NUM_CHAN > 1)
   DECT_DMA_CTRL->dect_shm_dma_ddr_saddr_tx_s1 = ((unsigned int)pDectTxDmaDDRStartAddr[1] & (unsigned int)0x1FFFFFFF );
   DECT_DMA_CTRL->dect_shm_dma_ddr_saddr_rx_s1 = ((unsigned int)pDectRxDmaDDRStartAddr[1] & (unsigned int)0x1FFFFFFF );
#if (DECT_DMA_NUM_CHAN > 2)
   DECT_DMA_CTRL->dect_shm_dma_ddr_saddr_tx_s2 = ((unsigned int)pDectTxDmaDDRStartAddr[2] & (unsigned int)0x1FFFFFFF );
   DECT_DMA_CTRL->dect_shm_dma_ddr_saddr_rx_s2 = ((unsigned int)pDectRxDmaDDRStartAddr[2] & (unsigned int)0x1FFFFFFF );
#if (DECT_DMA_NUM_CHAN > 3)
   DECT_DMA_CTRL->dect_shm_dma_ddr_saddr_tx_s3 = ((unsigned int)pDectTxDmaDDRStartAddr[3] & (unsigned int)0x1FFFFFFF );
   DECT_DMA_CTRL->dect_shm_dma_ddr_saddr_rx_s3 = ((unsigned int)pDectRxDmaDDRStartAddr[3] & (unsigned int)0x1FFFFFFF );
#endif
#endif
#endif

   DECT_DMA_CTRL->dect_shm_dma_ahb_saddr_tx_s01  = ((unsigned int)pDectTxDmaAHBStartAddr[0] - (unsigned int)DECT_AHB_SHARED_RAM_BASE) << 16;  /* Slot 0 */
   DECT_DMA_CTRL->dect_shm_dma_ahb_saddr_rx_s01  = ((unsigned int)pDectRxDmaAHBStartAddr[0] - (unsigned int)DECT_AHB_SHARED_RAM_BASE) << 16;  /* Slot 0 */
#if (DECT_DMA_NUM_CHAN > 1)
   DECT_DMA_CTRL->dect_shm_dma_ahb_saddr_tx_s01 |= ((unsigned int)pDectTxDmaAHBStartAddr[1] - (unsigned int)DECT_AHB_SHARED_RAM_BASE);         /* Slot 1 */
   DECT_DMA_CTRL->dect_shm_dma_ahb_saddr_rx_s01 |= ((unsigned int)pDectRxDmaAHBStartAddr[1] - (unsigned int)DECT_AHB_SHARED_RAM_BASE);         /* Slot 1 */
#if (DECT_DMA_NUM_CHAN > 2)
   DECT_DMA_CTRL->dect_shm_dma_ahb_saddr_tx_s23  = ((unsigned int)pDectTxDmaAHBStartAddr[2] - (unsigned int)DECT_AHB_SHARED_RAM_BASE) << 16;  /* Slot 2 */
   DECT_DMA_CTRL->dect_shm_dma_ahb_saddr_rx_s23  = ((unsigned int)pDectRxDmaAHBStartAddr[2] - (unsigned int)DECT_AHB_SHARED_RAM_BASE) << 16;  /* Slot 2 */
#if (DECT_DMA_NUM_CHAN > 3)
   DECT_DMA_CTRL->dect_shm_dma_ahb_saddr_tx_s23 |= ((unsigned int)pDectTxDmaAHBStartAddr[3] - (unsigned int)DECT_AHB_SHARED_RAM_BASE);         /* Slot 3 */
   DECT_DMA_CTRL->dect_shm_dma_ahb_saddr_rx_s23 |= ((unsigned int)pDectRxDmaAHBStartAddr[3] - (unsigned int)DECT_AHB_SHARED_RAM_BASE);         /* Slot 3 */
#endif
#endif
#endif

#endif  //end of #if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) 

   /* Setup DMA transfer size in terms of 32-bit words*/
   DECT_DMA_CTRL->dect_shm_dma_xfer_size_tx = (   (DECT_DMA_TX_TRANSFER_SIZE_WORDS) << 24     /* Slot0 */
                                                | (DECT_DMA_TX_TRANSFER_SIZE_WORDS) << 16     /* Slot1 */
                                                | (DECT_DMA_TX_TRANSFER_SIZE_WORDS) << 8      /* Slot2 */
                                                | (DECT_DMA_TX_TRANSFER_SIZE_WORDS) << 0 );   /* Slot3 */

   DECT_DMA_CTRL->dect_shm_dma_xfer_size_rx = (   (DECT_DMA_TRANSFER_SIZE_WORDS) << 24        /* Slot0 */
                                                | (DECT_DMA_TRANSFER_SIZE_WORDS) << 16        /* Slot1 */
                                                | (DECT_DMA_TRANSFER_SIZE_WORDS) << 8         /* Slot2 */
                                                | (DECT_DMA_TRANSFER_SIZE_WORDS) << 0 );      /* Slot3 */


   /* Setup total size of dma buff per slot in terms of DMA transfer size */
   DECT_DMA_CTRL->dect_shm_dma_buf_size_tx = (    ( DECT_DMA_TX_BUFFER_SIZE_WORDS / DECT_DMA_TX_TRANSFER_SIZE_WORDS ) << 24    /* Slot0 */
                                                | ( DECT_DMA_TX_BUFFER_SIZE_WORDS / DECT_DMA_TX_TRANSFER_SIZE_WORDS ) << 16    /* Slot1 */
                                                | ( DECT_DMA_TX_BUFFER_SIZE_WORDS / DECT_DMA_TX_TRANSFER_SIZE_WORDS ) << 8     /* Slot2 */
                                                | ( DECT_DMA_TX_BUFFER_SIZE_WORDS / DECT_DMA_TX_TRANSFER_SIZE_WORDS ) << 0 );  /* Slot3 */

   DECT_DMA_CTRL->dect_shm_dma_buf_size_rx = (    ( DECT_DMA_BUFFER_SIZE_WORDS / DECT_DMA_TRANSFER_SIZE_WORDS ) << 24      /* Slot0 */
                                                | ( DECT_DMA_BUFFER_SIZE_WORDS / DECT_DMA_TRANSFER_SIZE_WORDS ) << 16      /* Slot1 */
                                                | ( DECT_DMA_BUFFER_SIZE_WORDS / DECT_DMA_TRANSFER_SIZE_WORDS ) << 8       /* Slot2 */
                                                | ( DECT_DMA_BUFFER_SIZE_WORDS / DECT_DMA_TRANSFER_SIZE_WORDS ) << 0 );    /* Slot3 */

   /* clear DMA tx and rx counter */ 
   DECT_DMA_CTRL->dect_shm_dma_xfer_cntr_tx  = 0;
   DECT_DMA_CTRL->dect_shm_dma_xfer_cntr_rx  = 0;
   
   /* Setup DMA burst size */    
   DECT_DMA_CTRL->dect_shm_dma_ctrl &= ~MAX_BURST_CYCLE_MASK;
   DECT_DMA_CTRL->dect_shm_dma_ctrl |= ( 8 << MAX_BURST_CYCLE_SHIFT );

   /* Give priority to RX DMA */
   DECT_DMA_CTRL->dect_shm_dma_ctrl |= DMA_RX_TRIG_FIRST;
      
   /* Use 16-bit swap for DMA */  
   DECT_DMA_CTRL->dect_shm_dma_ctrl &= ~DMA_SUBWORD_SWAP_MASK;
   #if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) 
   DECT_DMA_CTRL->dect_shm_dma_ctrl |= DMA_SWAP_8_BIT;
   #else
   DECT_DMA_CTRL->dect_shm_dma_ctrl |= DMA_SWAP_16_BIT;
   #endif

   /* Prime TX and RX */
   for ( i=0; i<DECT_DMA_NUM_CHAN; i++ )
   {
      for ( j=0; j<DECT_DMA_BUFFER_SIZE_WORDS; j++ )
      {
         *(pDectTxDmaDDRStartAddrV[i] + j) =  0x00000000; //0x01234567;
         *(pDectRxDmaDDRStartAddrV[i] + j) =  0x00000000; //0x01010101;
         #if !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM963148) 
         *(pDectTxDmaAHBStartAddr[i] + j) =  0x00000000; //0x02020202;
         *(pDectRxDmaAHBStartAddr[i] + j) =  0x00000000; //0xFEDCBA98;
         #else
         *( (unsigned int *)(IO_ADDRESS((unsigned int)(pDectTxDmaAHBStartAddr[i]))) + j ) =  0x00000000; //0x02122232;
         *( (unsigned int *)(IO_ADDRESS((unsigned int)(pDectRxDmaAHBStartAddr[i]))) + j ) =  0x00000000; //0xFEDCBA98; 
         #endif   
      }
   }
   
   DECT_CTRL->dect_shm_irq_status |= DECT_CTRL->dect_shm_irq_status;

   /* Enable channels */
   DECT_DMA_CTRL->dect_shm_dma_ctrl |= ( RX_EN_0 | TX_EN_0 );  
#if (DECT_DMA_NUM_CHAN > 1)
   DECT_DMA_CTRL->dect_shm_dma_ctrl |= ( RX_EN_1 | TX_EN_1 );  
#if (DECT_DMA_NUM_CHAN > 2)
   DECT_DMA_CTRL->dect_shm_dma_ctrl |= ( RX_EN_2 | TX_EN_2 );  
#if (DECT_DMA_NUM_CHAN > 3)
   DECT_DMA_CTRL->dect_shm_dma_ctrl |= ( RX_EN_3 | TX_EN_3 ); 
#endif 
#endif 
#endif

   return 0;
}


/*****************************************************************************
*
*  FUNCTION: dectI_dma_deinit
*
*  PURPOSE:
*      deinit dect DMA
*
*  PARAMETERS: none
*
*  RETURNS: 0 on SUCCESS
*
*  NOTES:
*
*****************************************************************************/
static int dectI_dma_deinit(void)
{

   *startCtl = 0; 
   
   mdelay( 20 );  
   
   DECT_DMA_CTRL->dect_shm_dma_ctrl &= ~( DMA_RX_INT_TRIG_EN | DMA_TX_INT_TRIG_EN ); 
   
   /* Stop/clear DMA engine */
   DECT_DMA_CTRL->dect_shm_dma_ctrl = DMA_CLEAR;
   DECT_DMA_CTRL->dect_shm_dma_ctrl = 0;
       
   
   /* Clear DMA done bits */
   DECT_CTRL->dect_shm_irq_status |= 0x00000030;
   
   /* Free DMA memory */
   dma_free_coherent( dect_device, rxBufferArea, (void *)dectRxDMABuff, dmaBufHandleRx );
   dma_free_coherent( dect_device, txBufferArea, (void *)dectTxDMABuff, dmaBufHandleTx );                        

   
   return 0;
}

static void dectI_pll_deinit(void)
{
   #if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) 
   DECT_CTRL->dect_shm_pll_reg_1 &= ~PLL_PWRON;
   #else
   DECT_CTRL->dect_shm_pll_reg_1 |= PLL_DECT_PWRDWN |      \
                                    PLL_REFCOMP_PWRDOWN |  \
                                    PLL_NDIV_PWRDOWN |     \
                                    PLL_CH1_PWRDOWN |      \
                                    PLL_CH2_PWRDOWN |      \
                                    PLL_CH3_PWRDOWN |      \
                                    PLL_ARESET |           \
                                    PLL_DRESET;
   #endif
   return;
}


/*****************************************************************************
*
*  FUNCTION: dectI_pll_config
*
*  PURPOSE:
*      configure dect PLL settings
*
*  PARAMETERS: none
*
*  RETURNS: 0 on SUCCESS
*
*  NOTES:
*
*****************************************************************************/
static int dectI_pll_config(void)
{
   int i;
   
   printk("configure dect PLL \n");
   
   #if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) 
   DECT_CTRL->dect_shm_pll_reg_1 |= PLL_PWRON;
   mdelay( 1 ); 
   DECT_CTRL->dect_shm_pll_reg_1 |= PLL_RESETB;
   #else
   DECT_CTRL->dect_shm_pll_reg_1 &= ~(PLL_DECT_PWRDWN | PLL_REFCOMP_PWRDOWN | PLL_NDIV_PWRDOWN | PLL_CH1_PWRDOWN | PLL_CH2_PWRDOWN | PLL_CH3_PWRDOWN);
   DECT_CTRL->dect_shm_pll_reg_0 = 0x210064C0; //0x390C6560;   // Set Cz=0b11
   DECT_CTRL->dect_shm_pll_reg_1 &= ~PLL_VCO_RNG;
   DECT_CTRL->dect_shm_pll_reg_2 = 0x60000000;
   DECT_CTRL->dect_shm_pll_reg_3 = 0x11181808;
   
   DECT_CTRL->dect_shm_pll_reg_1 &= ~PLL_ARESET;
   #endif
   
   for ( i=0; i<100; i++ )
   {
      if ( DECT_CTRL->dect_shm_irq_status & ( 1 << DECT_SHM_IRQ_PLL_PHASE_LOCK ) )
      {
         /* PLL is locked */
         printk("####################SUCCESS: DECT PLL IS LOCKED#################\n");
         printk("dect shm irq status: 0x%08x, i: %d\n", DECT_CTRL->dect_shm_irq_status, i);
         break;
      }
      mdelay( DECT_DMA_PLL_DELAY_TIME );
   }
   
   if (( i == 100 ) && ((DECT_CTRL->dect_shm_irq_status & ( 1 << DECT_SHM_IRQ_PLL_PHASE_LOCK ))!=0x80))
   {
      /* Error PLL not locked */
      printk("###FAILURE#FAILURE: DECT PLL IS NOT LOCKED##FAILURE#FAILURE###\n");      
      return -1;
   }
   else
   {      
      /* PLL locked, bring Digital Portion out of reset */
      #if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) 
      DECT_CTRL->dect_shm_pll_reg_1 |= PLL_P_RESETB;
      DECT_CTRL->dect_shm_pll_reg_1 &= ~(PLL_PWRDWN_CH1 | PLL_PWRDWN_CH2 | PLL_PWRDWN_CH3 );
      #else
      DECT_CTRL->dect_shm_pll_reg_1 &= ~PLL_DRESET;
      #endif
   }

   printk("DECT PLL init completed successfully\n");

   DECT_CTRL->dect_shm_ctrl |= DECT_PLL_REF_DECT | DECT_CLK_CORE_DECT | DECT_CLK_OUT_XTAL | 
                              PHCNT_CLK_SRC_PLL |
                              PCM_PULSE_COUNT_ENABLE | DECT_PULSE_COUNT_ENABLE;

   DECT_CTRL->dect_shm_ctrl &= ~DECT_CLK_OUT_PLL;
   
   /* Route undivided  dectclk of 10.368Mhz to PCM DPLL */
   #if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) 	
   DECT_CTRL->dect_shim_pcm_ctrl = 0;
   #endif

   return 0;
}


/*****************************************************************************
*
*  FUNCTION: dectI_dumpRegs
*
*  PURPOSE:
*      dump dect registers
*
*  PARAMETERS: none
*
*  RETURNS: 0 on SUCCESS
*
*  NOTES:
*
*****************************************************************************/
static void dectI_dumpRegs(void)
{
   printk("DECT_CTRL->dect_shm_pll_reg_0   : 0x%08X\n", (unsigned int)DECT_CTRL->dect_shm_pll_reg_0);
   printk("DECT_CTRL->dect_shm_pll_reg_1   : 0x%08X\n", (unsigned int)DECT_CTRL->dect_shm_pll_reg_1);
   printk("DECT_CTRL->dect_shm_pll_reg_2   : 0x%08X\n", (unsigned int)DECT_CTRL->dect_shm_pll_reg_2);
   printk("DECT_CTRL->dect_shm_pll_reg_3   : 0x%08X\n", (unsigned int)DECT_CTRL->dect_shm_pll_reg_3);
   printk("dect_shm_dma_ddr_saddr_tx_s0    : 0x%08x\n", (unsigned int)DECT_DMA_CTRL->dect_shm_dma_ddr_saddr_tx_s0   );
   printk("dect_shm_dma_ddr_saddr_tx_s1    : 0x%08x\n", (unsigned int)DECT_DMA_CTRL->dect_shm_dma_ddr_saddr_tx_s1   );
   printk("dect_shm_dma_ddr_saddr_tx_s2    : 0x%08x\n", (unsigned int)DECT_DMA_CTRL->dect_shm_dma_ddr_saddr_tx_s2   );
   printk("dect_shm_dma_ddr_saddr_tx_s3    : 0x%08x\n", (unsigned int)DECT_DMA_CTRL->dect_shm_dma_ddr_saddr_tx_s3   );
   printk("dect_shm_dma_ddr_saddr_rx_s0    : 0x%08x\n", (unsigned int)DECT_DMA_CTRL->dect_shm_dma_ddr_saddr_rx_s0   );   
   printk("dect_shm_dma_ddr_saddr_rx_s1    : 0x%08x\n", (unsigned int)DECT_DMA_CTRL->dect_shm_dma_ddr_saddr_rx_s1   );
   printk("dect_shm_dma_ddr_saddr_rx_s2    : 0x%08x\n", (unsigned int)DECT_DMA_CTRL->dect_shm_dma_ddr_saddr_rx_s2   ); 
   printk("dect_shm_dma_ddr_saddr_rx_s3    : 0x%08x\n", (unsigned int)DECT_DMA_CTRL->dect_shm_dma_ddr_saddr_rx_s3   );   
   printk("dect_shm_dma_ahb_saddr_tx_s01   : 0x%08x\n", (unsigned int)DECT_DMA_CTRL->dect_shm_dma_ahb_saddr_tx_s01  ); 
   printk("dect_shm_dma_ahb_saddr_tx_s23   : 0x%08x\n", (unsigned int)DECT_DMA_CTRL->dect_shm_dma_ahb_saddr_tx_s23  ); 
   printk("dect_shm_dma_ahb_saddr_rx_s01   : 0x%08x\n", (unsigned int)DECT_DMA_CTRL->dect_shm_dma_ahb_saddr_rx_s01  );    
   printk("dect_shm_dma_ahb_saddr_rx_s23   : 0x%08x\n", (unsigned int)DECT_DMA_CTRL->dect_shm_dma_ahb_saddr_rx_s23  ); 
   printk("dect_shm_dma_offset_addr_tx_s01 : 0x%08x\n", (unsigned int)DECT_DMA_CTRL->dect_shm_dma_offset_addr_tx_s01);
   printk("dect_shm_dma_offset_addr_rx_s01 : 0x%08x\n", (unsigned int)DECT_DMA_CTRL->dect_shm_dma_offset_addr_rx_s01);
   printk("dect_shm_dma_xfer_size_tx       : 0x%08x\n", (unsigned int)DECT_DMA_CTRL->dect_shm_dma_xfer_size_tx      );
   printk("dect_shm_dma_xfer_size_rx       : 0x%08x\n", (unsigned int)DECT_DMA_CTRL->dect_shm_dma_xfer_size_rx      );
   printk("dect_shm_dma_buf_size_tx        : 0x%08x\n", (unsigned int)DECT_DMA_CTRL->dect_shm_dma_buf_size_tx       );
   printk("dect_shm_dma_buf_size_rx        : 0x%08x\n", (unsigned int)DECT_DMA_CTRL->dect_shm_dma_buf_size_rx       );
   printk("dect_shm_dma_xfer_cntr_tx       : 0x%08x\n", (unsigned int)DECT_DMA_CTRL->dect_shm_dma_xfer_cntr_tx      );
   printk("dect_shm_dma_xfer_cntr_rx       : 0x%08x\n", (unsigned int)DECT_DMA_CTRL->dect_shm_dma_xfer_cntr_rx      );
   printk("dect_shm_dma_ctrl               : 0x%08x\n", (unsigned int)DECT_DMA_CTRL->dect_shm_dma_ctrl              );
}

/*****************************************************************************
*
*  FUNCTION: dectShimDrvInit
*
*  PURPOSE:
*      This function is called when dect shim driver is loaded.
*
*  PARAMETERS: none
*
*  RETURNS: 
*  NOTES:
*
*****************************************************************************/
void dectShimDrvInit(void)
{
   dev_t devId;

   printk("Initialize DECT Shim layer....\n");

   devId = MKDEV(DECTSHIM_MAJOR, DECTSHIM_MINOR);

   cdev_init(&dectshim_cdev, &dectshim_fops);
   cdev_add(&dectshim_cdev, devId, 1);

   dectshimGetVoiceParams();
  
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
   if((dect_cl = class_create(THIS_MODULE, "dectshim")) == NULL)
   {
      return -1;
   }
   if((dect_device = device_create(dect_cl, NULL, devId, NULL, "dectshim")) == NULL)
   {
      class_destroy(dect_cl);
      return -1;
   }
#endif

   dectI_Init();

   return;
}

/*****************************************************************************
*
*  FUNCTION: dectShimDrvCleanup
*
*  PURPOSE:
*      This function is called when dect shim driver is unloaded.
*
*  PARAMETERS: none
*
*  RETURNS: 
*  NOTES:
*
*****************************************************************************/
void dectShimDrvCleanup(void)
{
   dev_t devId;

   dectI_Deinit();

   devId = MKDEV(DECTSHIM_MAJOR, DECTSHIM_MINOR);
   cdev_del(&dectshim_cdev);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
    device_destroy(dect_cl, devId);
    class_destroy(dect_cl);
#endif

   printk("Uninitialized DECT shim layer...\n");

   return;
}


/*****************************************************************************
*
*  FUNCTION: dectshimInit
*
*  PURPOSE:
*      This function is called through the dectshim Ioctl. It will kick off 
*      The DECT shim DMA.
*
*  PARAMETERS: args
*
*  RETURNS: 
*  NOTES:
*
*****************************************************************************/
static int dectshimInit(unsigned long args)
{

   (void) args;

   #if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) 
   if ( 0 ) //OTP_REGS_GET_USER_BIT(OTP_DECT_DISABLE) )
   #else
   if ( OTP_REGS_GET_USER_BIT(OTP_DECT_DISABLE) )	
   #endif
   {
      return ( 0 );
   }
   
   if ( dectShimInitialized == 0 )
   {
      /* This should never be the case */
      dectI_Init();
   }
   else 
   {
      dectI_dectblock_reset();

      DECT_DMA_CTRL->dect_shm_dma_ctrl |= ( DMA_RX_INT_TRIG_EN | DMA_TX_INT_TRIG_EN ); 
  
      if( DECT_DMA_CTRL->dect_shm_dma_offset_addr_tx_s01 == 0x00000000 )
      {
         printk("Kick dma Tx once\n");
         (DECT_DMA_CTRL)->dect_shm_dma_ctrl |=  ( DMA_TX_REG_TRIG_EN ); 
         (DECT_DMA_CTRL)->dect_shm_dma_ctrl &= ~( DMA_TX_REG_TRIG_EN );      
      }
      if( DECT_DMA_CTRL->dect_shm_dma_offset_addr_rx_s01 == 0x00000000 )
      {
         (DECT_DMA_CTRL)->dect_shm_dma_ctrl |=  ( DMA_RX_REG_TRIG_EN ); 
         (DECT_DMA_CTRL)->dect_shm_dma_ctrl &= ~( DMA_RX_REG_TRIG_EN );      
         printk("Kick dma Rx once\n");
      }
   }
   
   return 0;
}

/*****************************************************************************
*
*  FUNCTION: dectshimDeinit
*
*  PURPOSE:
*      This function is called through the dectshim Ioctl. It will stop the  
*      DECT buffer processing and stop the DECT shim DMA.
*
*  PARAMETERS: args
*
*  RETURNS: 
*  NOTES:
*
*****************************************************************************/
static int dectshimDeinit(unsigned long args)
{
   (void) args;

   #if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) 
   if ( 0 ) //OTP_REGS_GET_USER_BIT(OTP_DECT_DISABLE) )
   #else
   if ( OTP_REGS_GET_USER_BIT(OTP_DECT_DISABLE) )	
   #endif
   {
      return ( 0 );
   }

   *startCtl = 0; 
   dectI_dectblock_reset();
   
   mdelay( 20 );  
   
   DECT_DMA_CTRL->dect_shm_dma_ctrl &= ~( DMA_RX_INT_TRIG_EN | DMA_TX_INT_TRIG_EN ); 
   
   return 0;
}

static int dectshimPLLConfig(unsigned long args)
{
   return 0;
}

static int dectshimDMAConfig(unsigned long args)
{
   return 0;
}

/*****************************************************************************
*
*  FUNCTION: dectshimGetChs
*
*  PURPOSE:
*      This function is called through the dectshim Ioctl. It will return the
*      the number of DECT channels supported.  The number of DECT channels is
*      defined in boardparms_voice.c.
*
*  PARAMETERS: args
*
*  RETURNS: 
*  NOTES:
*
*****************************************************************************/
static int dectshimGetChs(unsigned long args)
{
   unsigned int endptCount = 0;
   PDECTSHIMDRV_CHANNELCOUNT_PARAM  pUArg = (PDECTSHIMDRV_CHANNELCOUNT_PARAM)args;

   printk("dectshimGetchs called\n");
   #if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) 
   if ( 0 ) //OTP_REGS_GET_USER_BIT(OTP_DECT_DISABLE) )
   #else
   if ( OTP_REGS_GET_USER_BIT(OTP_DECT_DISABLE) )	
   #endif
   {
      /* Force the number of DECT line to zero if DECT is disabled in the chip such as 6361 */
      copy_to_user( &(pUArg->channel_count), &(endptCount), sizeof(voiceParams.numDectLines) );
   } 
   else
   {
      copy_to_user( &(pUArg->channel_count), &(voiceParams.numDectLines), sizeof(voiceParams.numDectLines) );
   }
   return 0;
}

static int dectshimGetVoiceParams( void )
{
   char boardIdStr[BP_BOARD_ID_LEN];
   char baseBoardIdStr[BP_BOARD_ID_LEN];

   /* We don't have the voice parameters yet.
   ** First we need to get the board ID from the main BOARD_PARAMETERS table.
   ** Once we get the board ID we will use it as index to the voice tables */
   if ( BpGetVoiceBoardId(boardIdStr) == BP_SUCCESS )
   {
      printk("Obtained board id string (%s)\n", boardIdStr );
   }
   else
   {
      printk("Failed to obtain board id string !!!\n" );
      printk("*** Please verify that board configuration id is properly set up on the CFE prompt !!! \n" );
      return (-1);
   }

   if ( BpGetBoardId(baseBoardIdStr) == BP_SUCCESS )
   {
      printk("Obtained Base board id string (%s)", baseBoardIdStr );
   }
   else
   {
      printk("Failed to obtain Base board id string !!!");
      printk("   *** Please verify that Base board configuration id is properly set up on the CFE prompt !!! \n");
      return (-1);
   }

   /* Get the voice voice parameters based on the board id string */
   if ( BpGetVoiceParms( boardIdStr, &voiceParams, baseBoardIdStr ) == BP_SUCCESS )
   {
      printk( "Successfully obtained voice parameters\n" );
   }
   else
   {
      printk("Failed to obtain voice parameters !!!\n" );
      return (-1);
   }

   return 0;
}

/*****************************************************************************
*
*  FUNCTION: dectshimGetVirtualAddr
*
*  PURPOSE:
*      This function is called through the dectshim Ioctl. It will return the
*      DMA DDR virtual Addresses.
*
*  PARAMETERS: args
*
*  RETURNS: 
*  NOTES:
*
*****************************************************************************/
static int dectshimGetVirtualAddr(unsigned long args)
{
   PDECTSHIMDRV_DMAADDR_PARAM  pUArg = (PDECTSHIMDRV_DMAADDR_PARAM)args;

   printk("dectshimGetVirtualAddr called\n");

   copy_to_user( &(pUArg->ddrAddrTxV), &(dectTxDMABuff), sizeof(unsigned long) );
   copy_to_user( &(pUArg->ddrAddrRxV), &(dectRxDMABuff), sizeof(unsigned long) );
   
   return 0;
}
