/****************************************************************************
 * <:copyright-BRCM:2014:DUAL/GPL:standard
 * 
 *    Copyright (c) 2014 Broadcom Corporation
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
// BCMFORMAT: notabs reindent:uncrustify:bcm_minimal_i3.cfg

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/dma-mapping.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <board.h>
#include <bcm_map_part.h>
#include <bcm_intr.h>
#ifdef __arm__
#include <mach/hardware.h>
#endif
#include "i2s.h"

/**************************************************************************** 
 *  i2s driver required data format:
 *  --------------------------------
 *  All samples that need to be sent out over the i2s bus need to be aligned
 *  to the MSB of a 32-bit word. The following diagrams show how 32, 24 and 
 *  16-bit samples need to be aligned ( X is a dummy byte, must be ZERO ):
 *
 *  32-bit LE data:
 *  0     1     2     3     4     5     6     7     8
 *  +-----+-----+-----+-----+-----+-----+-----+-----+
 *  | LSB |byte1|byte2| MSB | LSB |byte1|byte2| MSB |
 *  +-----+-----+-----+-----+-----+-----+-----+-----+
 *      Left Channel Data   |  Right Channel Data
 *  
 *  24-bit LE data:
 *  0     1     2     3     4     5     6     7     8
 *  +-----+-----+-----+-----+-----+-----+-----+-----+
 *  |  X  | LSB |byte1| MSB |  X  | LSB |byte1| MSB |
 *  +-----+-----+-----+-----+-----+-----+-----+-----+
 *      Left Channel Data   |  Right Channel Data
 *      
 *  16-bit LE data:
 *  0     1     2     3     4     5     6     7     8
 *  +-----+-----+-----+-----+-----+-----+-----+-----+
 *  |  X  |  X  | LSB | MSB |  X  |  X  | LSB | MSB |
 *  +-----+-----+-----+-----+-----+-----+-----+-----+
 *      Left Channel Data   |  Right Channel Data
 *
 *  32-bit BE data:
 *  0     1     2     3     4     5     6     7     8
 *  +-----+-----+-----+-----+-----+-----+-----+-----+
 *  | MSB |byte2|byte2| LSB | MSB |byte2|byte2| LSB |
 *  +-----+-----+-----+-----+-----+-----+-----+-----+
 *      Left Channel Data   |  Right Channel Data
 *  
 *  24-bit BE data:
 *  0     1     2     3     4     5     6     7     8
 *  +-----+-----+-----+-----+-----+-----+-----+-----+
 *  | MSB |byte1| LSB |  X  | MSB |byte1| LSB |  X  |
 *  +-----+-----+-----+-----+-----+-----+-----+-----+
 *      Left Channel Data   |  Right Channel Data
 *      
 *  16-bit BE data:
 *  0     1     2     3     4     5     6     7     8
 *  +-----+-----+-----+-----+-----+-----+-----+-----+
 *  | MSB | LSB |  X  |  X  | MSB | LSB |  X  |  X  |
 *  +-----+-----+-----+-----+-----+-----+-----+-----+
 *      Left Channel Data   |  Right Channel Data
 *
 ***************************************************************************/
 
/****************************************************************************
* Macro Definitions / DEFINES
****************************************************************************/
#define I2S_API_DEBUG   0
#define I2S_ISR_DEBUG   0
#define I2S_TASKLET_DEBUG  0
#define I2S_PROCESS_DESC_IN_ISR 0 
#define I2S_SINE_TEST   0
#if I2S_SINE_TEST
#define I2S_SINEPERIOD_NUM_SAMPLES   32
#endif

struct i2s_dma_desc
{
   dma_addr_t     dma_addr;         /* DMA address to be passed to h/w */  
   char *         buffer_addr;      /* Buffer address */
   unsigned int   dma_len;          /* Length of dma transfer */ 
   struct list_head tx_queue_entry;   
};

struct i2s_freq_map
{
   unsigned int freq;               /* Desired sampling frequency */
   unsigned int mclk_rate;          /* mclk/2*bclk = mclk_rate */
   unsigned int clk_sel;            /* The mclk frequency */
};

#undef I2S_DEBUG             /* undef it, just in case */
#if I2S_API_DEBUG
#  define I2S_DEBUG(fmt, args...) printk( KERN_DEBUG "bt_serial_dma: " fmt, ## args)
#else
#  define I2S_DEBUG(fmt, args...) /* not debugging: nothing */
#endif

/****************************************************************************
* Function Prototypes
****************************************************************************/
static long i2s_ioctl( struct file *flip, unsigned int command, unsigned long arg );
ssize_t i2s_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);    
static int i2s_open( struct inode *inode, struct file *filp );
static irqreturn_t i2s_dma_isr(int irq, void *dev_id);
struct i2s_freq_map * i2s_get_freq_map( unsigned int frequency );
static void deinit_tx_desc_queue(void);
static void init_tx_desc_queue(void);
void enqueue_pending_tx_desc(struct i2s_dma_desc * desc);
struct i2s_dma_desc * dequeue_pending_tx_desc(void);
struct i2s_dma_desc * get_free_dma_desc_buffer(int length);
void put_free_dma_desc_buffer( struct i2s_dma_desc * desc );


/****************************************************************************
* Local Variables
****************************************************************************/
static struct cdev     i2s_cdev;
static struct device   *i2s_device       = NULL;
static struct class    *i2s_cl           = NULL;
static struct i2s_dma_desc tx_desc_queue_head;
struct semaphore desc_available_sem;
static DEFINE_SPINLOCK(tx_queue_lock);
static DEFINE_MUTEX(i2s_cfg_mutex);
static int i2s_write_in_progress = 0;

#if !I2S_PROCESS_DESC_IN_ISR   
void i2s_dma_tasklet (unsigned long unused);
DECLARE_TASKLET(i2s_dma_tlet, i2s_dma_tasklet, 0);
#endif


static const struct file_operations i2s_fops = {
    .owner =    THIS_MODULE,
    .write =    i2s_write,
    .unlocked_ioctl = i2s_ioctl,
    .open =     i2s_open,
};

#if I2S_SINE_TEST
static unsigned int freq = 16000;
#else
static unsigned int freq = 44100;
#endif

struct i2s_freq_map freq_map[] =
{  
   { 16000  , 12 , I2S_CLK_25MHZ }, /*  Req Bclk: 1,024,000  , Actual Bclk: 1,041,667  */
   { 32000  , 12 , I2S_CLK_50MHZ }, /*  Req Bclk: 2,048,000  , Actual Bclk: 2,083,333  */
   { 44100  , 9  , I2S_CLK_50MHZ }, /*  Req Bclk: 2,822,400  , Actual Bclk: 2,777,778  */
   { 48000  , 8  , I2S_CLK_50MHZ }, /*  Req Bclk: 3,072,000  , Actual Bclk: 3,125,000  */
   { 96000  , 4  , I2S_CLK_50MHZ }, /*  Req Bclk: 6,144,000  , Actual Bclk: 6,250,000  */
   { 192000 , 2  , I2S_CLK_50MHZ }, /*  Req Bclk: 12,288,000 , Actual Bclk: 12,500,000 */
   { 384000 , 1  , I2S_CLK_50MHZ }, /*  Req Bclk: 24,576,000 , Actual Bclk: 25,000,000 */
   { 0      , 0  , 0             }, 
};                                                              

#if I2S_SINE_TEST
/* One, i2s-formatted, period of a sinewave sampled at 16-bits @ 16Khz Stereo */
unsigned int i2s_sine_16bit_16khz[I2S_SINEPERIOD_NUM_SAMPLES] = 
{      
   0x00000000, 0xFFFF0000, 0x188C0000, 0x188D0000, 0x2D5D0000, 0x2D5C0000, 0x3B440000, 0x3B440000, 
   0x40260000, 0x40270000, 0x3B440000, 0x3B440000, 0x2D5C0000, 0x2D5D0000, 0x188D0000, 0x188C0000, 
   0x00000000, 0xFFFF0000, 0xE7730000, 0xE7730000, 0xD2A30000, 0xD2A30000, 0xC4BC0000, 0xC4BC0000, 
   0xBFD90000, 0xBFD90000, 0xC4BC0000, 0xC4BC0000, 0xD2A30000, 0xD2A40000, 0xE7730000, 0xE7740000,
};
#endif


/****************************************************************************
* Static functions
****************************************************************************/

/* Get free tx descriptor */
struct i2s_dma_desc * get_free_dma_desc_buffer(int length)
{
   struct i2s_dma_desc * new_desc = NULL;
         
   new_desc = kzalloc( sizeof(struct i2s_dma_desc), GFP_NOWAIT );
   if( new_desc )
   {
      new_desc->buffer_addr = kzalloc( length, GFP_NOWAIT );
      if( new_desc->buffer_addr )
      {
         new_desc->dma_len = length;
         I2S_DEBUG("%s: Allocated desc:0x%08x buffer:0x%08x\n", __FUNCTION__, 
                  (unsigned int)new_desc, (unsigned int)new_desc->buffer_addr);            
      }
      else
      {
         kfree(new_desc);
         new_desc = NULL;
      }
   }
   
   return new_desc;      
}

void put_free_dma_desc_buffer( struct i2s_dma_desc * desc )
{
   if( desc )
   {
#if I2S_ISR_DEBUG   
      I2S_DEBUG("%s: Freeing   desc:0x%08x buffer:0x%08x\n", __FUNCTION__, 
                  (unsigned int)desc, (unsigned int)desc->buffer_addr);                  
#endif
      kfree(desc->buffer_addr);
      kfree(desc);
   }         
}

/* De-queue TX desc */
struct i2s_dma_desc * dequeue_pending_tx_desc(void)
{
   struct i2s_dma_desc * tempDesc = NULL;
   unsigned long flags;
   
   spin_lock_irqsave(&tx_queue_lock, flags);   
      
   if( !list_empty(&tx_desc_queue_head.tx_queue_entry) )
   {
      /* Get first entry in list - Implements a queue */
      tempDesc = list_entry(tx_desc_queue_head.tx_queue_entry.next, struct i2s_dma_desc, tx_queue_entry);
      
      /* Delete entry from list */
      list_del(tx_desc_queue_head.tx_queue_entry.next);  
   }
   
   spin_unlock_irqrestore(&tx_queue_lock, flags);   
   
   return tempDesc;
}

/* Enqueue TX desc */
void enqueue_pending_tx_desc(struct i2s_dma_desc * desc)
{
   unsigned long flags;
   spin_lock_irqsave(&tx_queue_lock, flags);   
      
   /* Add entry at end of list - Implements a queue */
   list_add_tail( &desc->tx_queue_entry, &tx_desc_queue_head.tx_queue_entry );
   
   spin_unlock_irqrestore(&tx_queue_lock, flags);   
}

/* Initialize TX desc queue */
static void init_tx_desc_queue(void)
{
   unsigned long flags;
   
   spin_lock_irqsave(&tx_queue_lock, flags);   
   
   INIT_LIST_HEAD(&tx_desc_queue_head.tx_queue_entry);   
   
   spin_unlock_irqrestore(&tx_queue_lock, flags);   
}

/* De-Init TX desc queue */
static void deinit_tx_desc_queue(void)
{
   struct i2s_dma_desc * dma_request_desc = NULL;
   
   /* De-queue 1st descriptor */
   dma_request_desc = dequeue_pending_tx_desc();  
   
   while( dma_request_desc )
   {
      /* Free descriptor and dma buffer*/    
      dma_unmap_single(i2s_device, dma_request_desc->dma_addr, dma_request_desc->dma_len, DMA_TO_DEVICE); 
      put_free_dma_desc_buffer(dma_request_desc);           
            
      /* De-queue next descriptor */
      dma_request_desc = dequeue_pending_tx_desc();  
   }
}
      
/* Get frequency-parameter map */
struct i2s_freq_map * i2s_get_freq_map( unsigned int frequency )
{
   struct i2s_freq_map * freq_map_ptr = NULL;
   int i;
   
   for( i=0; freq_map[i].freq; i++ )
   {
      if( frequency == freq_map[i].freq )
      {
         freq_map_ptr = &freq_map[i];        
         break;
      }
   }

   I2S_DEBUG("%s: freq:%d mclk_rate:%d clk_sel:%d\n", __FUNCTION__, (int)frequency,
               (int)freq_map_ptr->mclk_rate, (int)freq_map_ptr->clk_sel);                  
      
   return freq_map_ptr;         
}

#if I2S_SINE_TEST
void i2s_copy_sine_data( char * buffer, unsigned int length )
{
   int i,j;
   unsigned int * sample_buffer_ptr = (unsigned int * )buffer;
   int num_iterations = (length/sizeof(unsigned int))/I2S_SINEPERIOD_NUM_SAMPLES;
   
   for( i=0; i<num_iterations; i++ )
   {
      for( j=0; j<I2S_SINEPERIOD_NUM_SAMPLES; j++ )
      {
         *sample_buffer_ptr = i2s_sine_16bit_16khz[j];
         sample_buffer_ptr++;
      }
   }
   
}
#endif

/* I2S ISR */
static irqreturn_t i2s_dma_isr(int irq, void *dev_id)
{  
#if I2S_PROCESS_DESC_IN_ISR   
   dma_addr_t dma_addr  = 0;
   unsigned int dma_len = 0;
   int eop = 0;    
   struct i2s_dma_desc * done_tx_desc;
#endif
   unsigned int int_status = I2S->intr;
      
#if I2S_ISR_DEBUG   
   I2S_DEBUG("%s: Intstat: 0x%08x\n", __FUNCTION__, (unsigned int)I2S->intr);   
#endif   
   
   /* Check if we got the right interrupt */
   if( int_status & I2S_DESC_OFF_INTR )
   {      
#if I2S_PROCESS_DESC_IN_ISR      
      /* Retrieve descriptor */
      dma_addr = I2S->desc_off_addr;
      dma_len  = I2S->desc_off_len;  
      eop = dma_len & I2S_DESC_EOP;    
      dma_len &= ~I2S_DESC_EOP;
      
#if I2S_ISR_DEBUG      
      I2S_DEBUG("%s: pst-clr OFF_DESC_LEVEL:%d\n", __FUNCTION__,  (int)(I2S->intr >> I2S_DESC_OFF_LEVEL_SHIFT) & I2S_DESC_LEVEL_MASK );          
#endif      

      /* Unmap DMA memory */
      if( dma_addr )
         dma_unmap_single(i2s_device, dma_addr, dma_len, DMA_TO_DEVICE);            
      
      /* Free buffer */
      done_tx_desc = dequeue_pending_tx_desc();
      
      /* Release desc and buffer */
      put_free_dma_desc_buffer(done_tx_desc);
      
      /* Check for end of packet */
      if( eop )
      {
         if( !( (I2S->intr >> I2S_DESC_IFF_LEVEL_SHIFT) & I2S_DESC_LEVEL_MASK ) )
         {
            /* Disable I2S interface if no RX descriptors in FIFO */
            //I2S->cfg &= ~I2S_ENABLE;   
         }
      }
           
      /* Give semaphore to indicate available descriptor space */
      up(&desc_available_sem);             
#else
      /* Mask Interrupt - Will be unmasked by tasklet */
      I2S->intr_en &= ~I2S_DESC_OFF_INTR_EN;
      
     /* Schedule tasklet to handle used descriptors */
      tasklet_schedule(&i2s_dma_tlet);
#endif      
   }
   else if ( int_status & I2S_DESC_OFF_OVERRUN_INTR 
          || int_status & I2S_DESC_IFF_UNDERRUN_INTR )
   {
      printk(KERN_WARNING  "%s: Underrun/Overruns detected 0x%08x\n", __FUNCTION__, int_status);      
   }
   
   /* Clear interrupt by writing 0 */
   I2S->intr &= ~I2S_INTR_MASK;
      
#if !defined(CONFIG_ARM)
   // Clear the interrupt
   BcmHalInterruptEnable (irq);
#endif
   
   return IRQ_HANDLED;   
}
 
#if !I2S_PROCESS_DESC_IN_ISR     
void i2s_dma_tasklet(unsigned long unused)
{
   dma_addr_t dma_addr  = 0;
   unsigned int dma_len = 0;
   int eop = 0;    
   int i=0;
   struct i2s_dma_desc * done_tx_desc;
   unsigned int int_status = I2S->intr;

   /* Loop until OFF fifo level drops to zero or for I2S_DESC_FIFO_DEPTH cycles */
   while( ( (int)(int_status >> I2S_DESC_OFF_LEVEL_SHIFT) & I2S_DESC_LEVEL_MASK ) && i < I2S_DESC_FIFO_DEPTH )         
   {
#if I2S_TASKLET_DEBUG      
      I2S_DEBUG("%s: pre-clr OFF_DESC_LEVEL:%d\n", __FUNCTION__,  (int)(I2S->intr >> I2S_DESC_OFF_LEVEL_SHIFT) & I2S_DESC_LEVEL_MASK );          
#endif    
  
      /* Retrieve descriptor */
      dma_addr = I2S->desc_off_addr;
      dma_len  = I2S->desc_off_len;  
      eop = dma_len & I2S_DESC_EOP;    
      dma_len &= ~I2S_DESC_EOP;
      
#if I2S_TASKLET_DEBUG      
      I2S_DEBUG("%s: pst-clr OFF_DESC_LEVEL:%d\n", __FUNCTION__,  (int)(int_status >> I2S_DESC_OFF_LEVEL_SHIFT) & I2S_DESC_LEVEL_MASK );          
#endif      
   
      /* Unmap DMA memory */
      if( dma_addr )
         dma_unmap_single(i2s_device, dma_addr, dma_len, DMA_TO_DEVICE);            
      
      /* Free buffer */
      done_tx_desc = dequeue_pending_tx_desc();
      
      /* Release desc and buffer */
      put_free_dma_desc_buffer(done_tx_desc);
      
      /* Check for end of packet */
      if( eop )
      {
         if( !( (int_status >> I2S_DESC_IFF_LEVEL_SHIFT) & I2S_DESC_LEVEL_MASK ) )
         {
            /* Disable I2S interface if no RX descriptors in FIFO */
            //I2S->cfg &= ~I2S_ENABLE;   
         }
      }
           
      /* Give semaphore to indicate available descriptor space */
      up(&desc_available_sem);                
      
      /* Increment loop counter */
      i++;
      
      /* Update interrupt status */
      int_status = I2S->intr;
   }
   
   /* Unmask interrupt */
   I2S->intr_en |= I2S_DESC_OFF_INTR_EN;
}
#endif

/* i2s_open: Basic register initialization */
static int i2s_open( struct inode *inode, struct file *filp )
{
   int ret = 0;
   struct i2s_freq_map * freq_map_ptr = NULL;
   
   I2S_DEBUG("%s\n", __FUNCTION__);
   
   /* Acquire configuration mutex */
   mutex_lock(&i2s_cfg_mutex);
   
   if( !i2s_write_in_progress )
   {         
      /* Disable I2S interface */
      I2S->cfg &= ~I2S_ENABLE;   
      
      /* Clear and disable I2S interrupts ( by writing 0 ) */
      I2S->intr &= ~I2S_INTR_MASK;
      I2S->intr_en = 0;   
         
      /* Clear DMA interrupt thresholds */   
      I2S->intr_iff_thld = 0;   
      I2S->intr_off_thld = 0;
         
      /* Setup I2S as follows ( Fs = sampling frequency ):                    *
       * 64Fs BCLK, leftChannel=0, rightchannel=1, falling BCLK,LRCLK low for *
       * left, Data delayed by 1 BCLK from LRCLK transition, MSB justified    */
      I2S->cfg |= I2S_OUT_R;                                                       
      I2S->cfg &= ~I2S_OUT_L;                                                    
      I2S->cfg |= 2 << I2S_SCLKS_PER_1FS_DIV32_SHIFT;                            
      I2S->cfg |= I2S_BITS_PER_SAMPLE_32 << I2S_BITS_PER_SAMPLE_SHIFT;         
      I2S->cfg &= ~I2S_SCLK_POLARITY;                                         
      I2S->cfg &= ~I2S_LRCK_POLARITY;                                         
      I2S->cfg |= I2S_DATA_ALIGNMENT;                                         
      I2S->cfg &= ~I2S_DATA_JUSTIFICATION;                                    
      I2S->cfg |= I2S_DATA_ENABLE;                                            
      I2S->cfg |= I2S_CLOCK_ENABLE;                                              
          
      /* Set DMA interrupt thresholds */   
      I2S->intr_iff_thld = 0;   
      I2S->intr_off_thld = 0;
           
      /* Enable off interrupts - interrupt when output fifo level is over 0 */
      I2S->intr_en &= ~I2S_DESC_INTR_TYPE_SEL;      
      I2S->intr_en |= I2S_DESC_OFF_INTR_EN;
      
      /* Based on sampling frequency, choose MCLK and *
       * select divide ratio for required BCLK        */
      freq_map_ptr = i2s_get_freq_map( freq );
      
      if( freq_map_ptr )
      {
         I2S->cfg &= I2S_MCLK_CLKSEL_CLR_MASK;
         I2S->cfg |= freq_map_ptr->mclk_rate << I2S_MCLK_RATE_SHIFT;
         I2S->cfg |= freq_map_ptr->clk_sel   << I2S_CLKSEL_SHIFT;     
         
         /* Initialize semaphore */
         sema_init(&desc_available_sem, I2S_DESC_FIFO_DEPTH); 
                  
         /* Initialize pending descriptor queue */
         init_tx_desc_queue();      
         ret = 0;
      }
      else
      {
         printk(KERN_ERR "%s: Unsupported frequency %d\n", __FUNCTION__, freq);
         ret = -EINVAL; 
      }
   }
   else
   {
      ret = -EBUSY;  
   }
   
   /* Release configuration mutex */
   mutex_unlock(&i2s_cfg_mutex);      

   return ret;   
}
 
/* i2s_write: Send data via DMA */
ssize_t i2s_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)    
{
   char * user_buff = (char*)buf;
   unsigned int byte_count = count;
   struct i2s_dma_desc * new_desc;
   unsigned int xfer_length;
               
   /* Release configuration mutex */
   mutex_lock(&i2s_cfg_mutex);
   
   if( i2s_write_in_progress )
      return -EBUSY;
   else
      i2s_write_in_progress = 1;
      
   /* Release configuration mutex */
   mutex_unlock(&i2s_cfg_mutex);         
              
#if I2S_SINE_TEST
   /* Adjust byte_count so that we can stuff the maximum number of sine wave periods in a dma buffer */
   byte_count =  I2S_DMA_BUFF_MAX_LEN - (I2S_DMA_BUFF_MAX_LEN%(I2S_SINEPERIOD_NUM_SAMPLES * sizeof(unsigned int)));                 
#endif   

   /* Submit dma transfers */
   while( byte_count )
   {
      I2S_DEBUG("%s: Count:%d\n", __FUNCTION__, byte_count); 
      
      /* Ensure we only continue if we have descriptor space */
      if(down_interruptible(&desc_available_sem))
         return -ERESTARTSYS;   
      
      /* Calculate transfer length */
      xfer_length = ((byte_count>I2S_DMA_BUFF_MAX_LEN)?I2S_DMA_BUFF_MAX_LEN:byte_count);
      new_desc = get_free_dma_desc_buffer(xfer_length);
      if( new_desc )
      {        
         I2S_DEBUG("%s: Bytes Left cur:%d\n", __FUNCTION__, byte_count); 
      
#if I2S_SINE_TEST
         i2s_copy_sine_data(new_desc->buffer_addr, byte_count);
#else
         /* Copy over user data */
         copy_from_user (new_desc->buffer_addr, user_buff, new_desc->dma_len);
#endif   
         
         /* Increment data pointer */
         user_buff += new_desc->dma_len;
         
#if !I2S_SINE_TEST
         /* Adjust count */
         byte_count -= new_desc->dma_len;
#endif   

         I2S_DEBUG("%s: Bytes Left rem:%d\n", __FUNCTION__, byte_count);          
         
         /* Map dma buffer */
         I2S_DEBUG("%s: First data word:0x%08x\n", __FUNCTION__, *(unsigned int*)new_desc->buffer_addr);          
         new_desc->dma_addr = dma_map_single(i2s_device, new_desc->buffer_addr, new_desc->dma_len, DMA_TO_DEVICE);
         if( dma_mapping_error(i2s_device, new_desc->dma_addr) )
         {
            printk(KERN_ERR "%s:dma_map_single Tx failed\n", __FUNCTION__);
            return -ENOMEM;
         }   
         
         /* Write descriptor */
         enqueue_pending_tx_desc(new_desc);
         I2S->desc_iff_len = (byte_count>0)? new_desc->dma_len : (new_desc->dma_len | I2S_DESC_EOP) ;
         I2S->desc_iff_addr = new_desc->dma_addr;      
                           
         I2S_DEBUG("%s: IFF_DESC_LEVEL:%d\n", __FUNCTION__,  (int)(I2S->intr >> I2S_DESC_IFF_LEVEL_SHIFT) & I2S_DESC_LEVEL_MASK ); 
         I2S_DEBUG("%s: OFF_DESC_LEVEL:%d\n", __FUNCTION__,  (int)(I2S->intr >> I2S_DESC_OFF_LEVEL_SHIFT) & I2S_DESC_LEVEL_MASK );          
                  
         /* Release configuration mutex */
         mutex_lock(&i2s_cfg_mutex);
         
         /* Enable I2S interface */
         I2S->cfg |= I2S_ENABLE; 
         
         /* Release configuration mutex */
         mutex_unlock(&i2s_cfg_mutex);         
      }
      else
      {
         printk(KERN_ERR "%s:Couldnt get free tx descriptor\n", __FUNCTION__);
         return -ENOMEM; 
      }
   }
   
   /* Release configuration mutex */
   mutex_lock(&i2s_cfg_mutex);
   
   /* Write complete */
   i2s_write_in_progress = 0;
      
   /* Release configuration mutex */
   mutex_unlock(&i2s_cfg_mutex);         
   
   *f_pos += count;
   return count;
}
   
static long i2s_ioctl( struct file *flip, unsigned int command, unsigned long arg )
{
   struct i2s_freq_map * freq_map_ptr = NULL;
   int ret = 0;

   I2S_DEBUG("%s: Cmd: %d\n", __FUNCTION__, command);

   /* Aqcuire configuration mutex */
   mutex_lock(&i2s_cfg_mutex);      
   
   if( !i2s_write_in_progress )
   {         
      if( command == I2S_SAMPLING_FREQ_SET_IOCTL )
      {            
         if( (I2S->cfg & I2S_DATA_ENABLE) && (I2S->cfg & I2S_CLOCK_ENABLE) )
         {
            /* Lookup frequency map */
            freq_map_ptr = i2s_get_freq_map( arg );
            
            if( freq_map_ptr )
            {            
               I2S->cfg &= I2S_MCLK_CLKSEL_CLR_MASK;
               I2S->cfg |= freq_map_ptr->mclk_rate << I2S_MCLK_RATE_SHIFT;
               I2S->cfg |= freq_map_ptr->clk_sel   << I2S_CLKSEL_SHIFT;  
               
               freq = freq_map_ptr->freq;  
                                  
               I2S_DEBUG("%s: Setting Sampling Freq:%d\n", __FUNCTION__,  freq_map_ptr->freq );       
               ret = 0;
            }
            else
            {
               printk(KERN_ERR "%s: Unsupported frequency %d\n", __FUNCTION__, (int)arg);
               ret = -EINVAL; 
            }             
         }
         else
         {
            printk(KERN_ERR "%s: Driver not initialized\n", __FUNCTION__);      
            ret = -EINVAL; 
         }     
      }
      else
      {
         //printk(KERN_ERR "%s: Unsupported command %d\n", __FUNCTION__, command);
         ret = -EINVAL; 
      } 
   }       
   else
   {
      ret = -EBUSY;       
   }
           
   /* Release configuration mutex */
   mutex_unlock(&i2s_cfg_mutex); 
                 
   return ret; 
}
    

static int __init i2s_init(void) 
{
   int ret;
   dev_t devId;
   
   /* Generate device id */
   devId = MKDEV(I2S_MAJOR, 0);
   
   /* Register char driver region */
   register_chrdev_region(devId, 1, "i2s");
   
   /* Create class and device ( /sys entries ) */
   i2s_cl = class_create(THIS_MODULE, "i2s");
   if(i2s_cl == NULL)
   {
      printk(KERN_ERR "Error creating device class\n");
      goto err_cdev_cleanup;
   }

   i2s_device = device_create(i2s_cl, NULL, devId, NULL, "i2s");
   if(i2s_device == NULL)
   {
      printk(KERN_ERR "Error creating device\n");
      goto err_class_cleanup;
   }
   
   /* Init the character device */
   cdev_init(&i2s_cdev, &i2s_fops);
   i2s_cdev.owner = THIS_MODULE;
   ret = cdev_add(&i2s_cdev, devId, 1);

   if( ret )
   {
      printk(KERN_ERR "Error %d adding i2s driver", ret);
      goto err_device_cleanup;
   }
   else
   {
      printk(KERN_ALERT "i2s registered\n");
   }
   
#if defined(CONFIG_ARM)
   /* for ARM it will always rearm!! */
   ret = BcmHalMapInterruptEx((FN_HANDLER)i2s_dma_isr,
                          0, 
                          INTERRUPT_ID_I2S,
                          "i2s_dma", INTR_REARM_YES,
                          INTR_AFFINITY_DEFAULT);
#else
   ret = BcmHalMapInterruptEx((FN_HANDLER)i2s_dma_isr,
                          0, 
                          INTERRUPT_ID_I2S,
                          "i2s_dma", INTR_REARM_NO,
                          INTR_AFFINITY_DEFAULT);
#endif    

   if (ret != 0)
   {
      printk(KERN_ERR "i2s_init: failed to register "
                          "intr %d rv=%d\n", INTERRUPT_ID_I2S, ret);
      goto err_device_cleanup;      
   }   
   
   return 0;
   
err_device_cleanup:
   device_destroy(i2s_cl, devId);
err_class_cleanup:
   class_destroy(i2s_cl);
err_cdev_cleanup:
   cdev_del(&i2s_cdev);
   return -1;
   
}
 
static void __exit i2s_exit(void) 
{
   dev_t devId;
   
   /* deinit queue */
   deinit_tx_desc_queue();
   
   /* Generate device id */
   devId = MKDEV(I2S_MAJOR, 0);

   /* Delete cdev */
   cdev_del(&i2s_cdev);

   /* destroy the shim device and device class */
   device_destroy(i2s_cl, devId);
   class_destroy(i2s_cl);
   
   /* Unregister chrdev region */
   unregister_chrdev_region(devId, 1);
   
   printk(KERN_ALERT "i2s unregistered\n");
}
 
module_init(i2s_init);
module_exit(i2s_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("I2S Driver");



         
         

      
            
      
      
      
     
      