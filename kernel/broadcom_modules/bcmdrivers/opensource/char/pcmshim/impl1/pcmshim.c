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
***************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <asm/uaccess.h>

#include <pcmshimdrv.h>


/****************************************************************************
* Macro Definitions / DEFINES
****************************************************************************/
/* allocate 2k for each DMA buffer */
#define DMA_POOL_SIZE               (2000 * sizeof(uint8_t))
#define DMA_POOL_NAME               "bcm_pcm_dma_pool"
#define DMA_UBUS_BYTES              (8)


/****************************************************************************
* Local function declaration
****************************************************************************/
int  pcmshim_open(struct inode *inode, struct file *filp);
int  pcmshim_release(struct inode *inode, struct file *filp);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
long pcmshim_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
#else
int  pcmshim_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
#endif
static inline void pcmshim_mem_cleanup(void);
static inline int  pcmshim_mem_alloc(void);


/****************************************************************************
* Variables
****************************************************************************/
static struct device   *pcm_dma_device       = NULL;
static struct class    *pcm_dma_cl           = NULL;
static struct dma_pool *pcm_dma_pool         = NULL;
static struct cdev      pcm_dma_shim_cdev;
static int              pcm_dma_shim_open    = 0;

static const struct file_operations pcm_dma_shim_fops = {
    .owner =    THIS_MODULE,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
    .unlocked_ioctl = pcmshim_ioctl,
#else
    .ioctl =    pcmshim_ioctl,
#endif
    .open =     pcmshim_open,
    .release =  pcmshim_release,
};




/*****************************************************************************
*  FUNCTION:   pcmshim_mem_cleanup
*
*  PURPOSE:    Cleanup the buffer pool, if required
*
*  PARAMETERS: None
*
*  RETURNS:    Nothing.
*
*****************************************************************************/
static inline void pcmshim_mem_cleanup(void)
{
   if( pcm_dma_pool != NULL )
   {
      dma_pool_destroy( pcm_dma_pool );
      pcm_dma_pool = NULL;
   }
}


/*****************************************************************************
*  FUNCTION:   pcmshim_mem_alloc
*
*  PURPOSE:    Allocate the buffer pool, if possible
*
*  PARAMETERS: None
*
*  RETURNS:    0 on success, 1 otherwise
*
*****************************************************************************/
static inline int pcmshim_mem_alloc(void)
{
   if( pcm_dma_pool == NULL )
   {
      if( dma_supported(pcm_dma_device, DMA_BIT_MASK(64)) )
      {
         pcm_dma_device->coherent_dma_mask = DMA_BIT_MASK(64);
      }
      else if( dma_supported(pcm_dma_device, DMA_BIT_MASK(32)) )
      {
         pcm_dma_device->coherent_dma_mask = DMA_BIT_MASK(32);
      }
      else
      {
         printk(KERN_WARNING "%s: Unable to set DMA mask! Aborting.\n\n\n", __FUNCTION__);
         return 1;
      }

      pcm_dma_pool = dma_pool_create(DMA_POOL_NAME, pcm_dma_device, DMA_POOL_SIZE, DMA_UBUS_BYTES, 0);
   }

   return (pcm_dma_pool == NULL);
}


/*****************************************************************************
*  FUNCTION:   pcmshim_open
*
*  PURPOSE:    Handle the driver opening
*
*  PARAMETERS: inode - pointer to the kernel inode struct
*              filp  - pointer to the kernel file struct
*
*  RETURNS:    0 on success, error code otherwise
*
*****************************************************************************/
int pcmshim_open(struct inode *inode, struct file *filp)
{
   pcm_dma_shim_open++;
   return (0);
}


/*****************************************************************************
*  FUNCTION:   pcmshim_release
*
*  PURPOSE:    Handle the driver releasal
*
*  PARAMETERS: inode - pointer to the kernel inode struct
*              filp  - pointer to the kernel file struct
*
*  RETURNS:    0 on success, error code otherwise
*
*****************************************************************************/
int pcmshim_release(struct inode *inode, struct file *filp)
{
   pcm_dma_shim_open--;
   return (0);
}


/*****************************************************************************
*  FUNCTION:   pcmshim_ioctl
*
*  PURPOSE:    Handle the driver ioctl
*
*  PARAMETERS: filp  - pointer to the kernel file struct
*              cmd   - ioctl number
*              arg   - ioctl argument
*
*  RETURNS:    0 on success, error code otherwise
*
*****************************************************************************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
long pcmshim_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#else
int pcmshim_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
   int ret = -EINVAL;

   if(cmd == PCMSHIMIOCTL_GETBUF_CMD)
   {
      /* return the buffer */
      PPCMSHIMDRV_GETBUF_PARAM pUArg = (PPCMSHIMDRV_GETBUF_PARAM) arg;

      copy_to_user( &(pUArg->bufp), &(pcm_dma_pool), sizeof(struct dma_pool *) );
      ret = 0;
   }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
   return (long) ret;
#else
   return ret;
#endif
}


/*****************************************************************************
*  FUNCTION:   pcmshim_init
*
*  PURPOSE:    Handle the module initialization.
*
*  PARAMETERS: none
*
*  RETURNS:    0 on success, error code otherwise
*
*****************************************************************************/
static int __init pcmshim_init(void)
{
   printk(KERN_NOTICE "Loading PCM shim driver\n");

   /* create the character device */
   cdev_init(&pcm_dma_shim_cdev, &pcm_dma_shim_fops);
   cdev_add(&pcm_dma_shim_cdev, MKDEV(PCMSHIM_MAJOR, PCMSHIM_MINOR), 1);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
   /* create device class and device name */
   pcm_dma_cl = class_create(THIS_MODULE, PCMSHIM_DEVNAME);
   if(pcm_dma_cl == NULL)
   {
      printk(KERN_ALERT "Error creating device class\n");
      goto err_cdev_cleanup;
   }

   pcm_dma_device = device_create(pcm_dma_cl, NULL, MKDEV(PCMSHIM_MAJOR, PCMSHIM_MINOR), NULL, PCMSHIM_DEVNAME);
   if(pcm_dma_device == NULL)
   {
      printk(KERN_ALERT "Error creating device\n");
      goto err_class_cleanup;
   }
#endif

   if(pcmshim_mem_alloc())
   {
      printk(KERN_ALERT "Can't alloc DMA memory pool\n");
      goto err_device_cleanup;
   }

   return 0;

err_device_cleanup:
   device_destroy(pcm_dma_cl, MKDEV(PCMSHIM_MAJOR, PCMSHIM_MINOR));
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
err_class_cleanup:
   class_destroy(pcm_dma_cl);
err_cdev_cleanup:
   cdev_del(&pcm_dma_shim_cdev);
#endif
   return -1;
}


/*****************************************************************************
*  FUNCTION:   pcmshim_cleanup
*
*  PURPOSE:    Handle the module deinitialization.
*
*  PARAMETERS: none
*
*  RETURNS:    0 on success, error code otherwise
*
*****************************************************************************/
static void __exit pcmshim_deinit(void)
{
   printk(KERN_NOTICE "Cleaning up PCM shim driver\n");

   /* cleanup memory if required */
   pcmshim_mem_cleanup();

   /* delete the character device */
   cdev_del(&pcm_dma_shim_cdev);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
   /* destroy the shim device and device class */
   device_destroy(pcm_dma_cl, MKDEV(PCMSHIM_MAJOR, PCMSHIM_MINOR));
   class_destroy(pcm_dma_cl);
#endif

   printk(KERN_NOTICE "Unloaded PCM shim driver\n");
}


module_init(pcmshim_init);
module_exit(pcmshim_deinit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("PCM shim layer driver");

