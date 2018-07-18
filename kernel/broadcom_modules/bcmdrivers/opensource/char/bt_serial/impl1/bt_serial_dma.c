/*
* <:copyright-BRCM:2013:GPL/GPL:standard
* 
*    Copyright (c) 2013 Broadcom Corporation
*    All Rights Reserved
* 
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License, version 2, as published by
* the Free Software Foundation (the "GPL").
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* 
* A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php, or by
* writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA 02111-1307, USA.
* 
* :>
*/
// BCMFORMAT: notabs reindent:uncrustify:bcm_minimal_i3.cfg

/* Description: Bluetooth Serial port driver DMA related functions for the BCM963XX. */
#include <linux/serial_core.h>
#include <linux/delay.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/amba/pl08x.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <board.h>
#include <bcm_map_part.h>
#include <bcm_intr.h>
#include <linux/bcm_colors.h>
#ifdef __arm__
#include <mach/hardware.h>
#endif
#include "bt_serial_dma.h"

/***************************** Defines & Types ******************************/
#define BT_UART_REG(p) ((volatile BtUartCtrlRegs *) (p)->membase)

#define BT_SERIAL_DMA_DEBUG      0

#undef BT_SER_DMA_DEBUG             /* undef it, just in case */
#if BT_SERIAL_DMA_DEBUG
#  define BT_SER_DMA_DEBUG(fmt, args...) printk( KERN_DEBUG "bt_serial_dma: " fmt, ## args)
#else
#  define BT_SER_DMA_DEBUG(fmt, args...) /* not debugging: nothing */
#endif

struct dma_xfer_desc
{
   dma_addr_t     dma_addr;         /* DMA address to be passed to h/w */  
   char *         buffer_addr;      /* Buffer address */
   unsigned int   buff_length;      /* Length of the dma buffer */
   unsigned int   xfer_length;      /* Length of dma transfer */
   enum dma_transfer_direction direction;
   struct uart_port * port;
   struct list_head tx_queue_entry;
};

struct bt_serial_dma_config
{
   dma_cap_mask_t    mask; 
   struct dma_chan   *tx_dma_chan;
   struct dma_chan   *rx_dma_chan;  
   struct dma_slave_config rx_slave_cfg;
   struct dma_slave_config tx_slave_cfg;  
   dma_complete_callback   rx_comp_cb;
   dma_complete_callback   tx_comp_cb;
};

/******************************* Prototypes *********************************/
static dma_cookie_t bt_serial_issue_dma_request( struct dma_xfer_desc * dma_request_desc );
static void bt_serial_dma_tx_cb(void *data);
static void bt_serial_dma_rx_cb(void *data);
static void init_tx_desc_queue(void);
static void deinit_tx_desc_queue(void);
static struct dma_xfer_desc * dequeue_tx_desc(void);
static void enqueue_tx_desc(struct dma_xfer_desc *); 

/***************************** Local Variables *******************************/
static struct bt_serial_dma_config bt_serial_dma_cfg;
static struct dma_xfer_desc tx_desc_queue_head;
static struct dma_xfer_desc * current_tx_dma_desc = NULL;
static struct dma_xfer_desc * current_rx_dma_desc = NULL;
static DEFINE_SPINLOCK(dma_req_issue_lock);
static DEFINE_SPINLOCK(tx_queue_lock);

/***************************** Static Functions ******************************/

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
   struct dma_xfer_desc * dma_request_desc = NULL;
   
   /* De-queue 1st descriptor */
   dma_request_desc = dequeue_tx_desc();  
   
   while( dma_request_desc )
   {
      /* Free descriptor and dma buffer*/    
      dma_unmap_single(dma_request_desc->port->dev, dma_request_desc->dma_addr, dma_request_desc->xfer_length, DMA_TO_DEVICE);            
      kfree(dma_request_desc->buffer_addr);     
      kfree(dma_request_desc);
      
      /* De-queue next descriptor */
      dma_request_desc = dequeue_tx_desc();  
   }
}

/* De-queue TX desc */
static struct dma_xfer_desc * dequeue_tx_desc(void)
{
   struct dma_xfer_desc * tempDesc = NULL;
   unsigned long flags;
   
   spin_lock_irqsave(&tx_queue_lock, flags);   
      
   if( !list_empty(&tx_desc_queue_head.tx_queue_entry) )
   {
      /* Get first entry in list - Implements a queue */
      tempDesc = list_entry(tx_desc_queue_head.tx_queue_entry.next, struct dma_xfer_desc, tx_queue_entry);
      
      /* Delete entry from list */
      list_del(tx_desc_queue_head.tx_queue_entry.next);  
   }
   
   spin_unlock_irqrestore(&tx_queue_lock, flags);   
   
   return tempDesc;
}

/* Enqueue TX desc */
static void enqueue_tx_desc(struct dma_xfer_desc * desc)
{
   unsigned long flags;
   spin_lock_irqsave(&tx_queue_lock, flags);   
      
   /* Add entry at end of list - Implements a queue */
   list_add_tail( &desc->tx_queue_entry, &tx_desc_queue_head.tx_queue_entry );
   
   spin_unlock_irqrestore(&tx_queue_lock, flags);   
}

/* Callback function for TX DMA channel - Called in tasklet context */
static void bt_serial_dma_tx_cb(void *data)
{
   unsigned long flags;
   struct dma_xfer_desc * dma_request_desc = (struct dma_xfer_desc *)data;
   struct dma_xfer_desc * next_dma_request_desc = NULL;
   
   BT_SER_DMA_DEBUG("%s: DESC_ADDR:0x%08x, DMA_ADDR:0x%08x, CPU_ADDR:0x%08x, buff_len:%d\n",
                     __FUNCTION__,
                     (unsigned int)dma_request_desc,
                     (unsigned int)dma_request_desc->dma_addr,
                     (unsigned int)dma_request_desc->buffer_addr,
                     (int)dma_request_desc->buff_length );
                     
   /* TX DMA completed */  
   /* If in HWSLIP mode, indicate end of TX Packet */ 
   if( (BT_UART_REG(dma_request_desc->port))->LCR & BT_UART_LCR_SLIP )
   {
      /* Wait for TX FIFO Empty. FIXME: This could be overkill, sim and rtl indicates this should be done */
      while( !((BT_UART_REG(dma_request_desc->port))->LSR & BT_UART_LSR_TX_IDLE) 
           || ((BT_UART_REG(dma_request_desc->port))->LSR & BT_UART_LSR_TX_DATA_AVAIL) );
      
      /* Wait for TX_PACKET_READY to be 1 */
      while( !((BT_UART_REG(dma_request_desc->port))->LSR & BT_UART_LSR_TX_PACKET_RDY) );
      
      /* Indicate end of TX packet */
      (BT_UART_REG(dma_request_desc->port))->FCR &= ~BT_UART_FCR_TX_PACKET;            
   }     
   
   /* Unmap DMA memory */
   dma_unmap_single(dma_request_desc->port->dev, dma_request_desc->dma_addr, dma_request_desc->xfer_length, DMA_TO_DEVICE);
   
   /* Call tx complete callback */
   bt_serial_dma_cfg.tx_comp_cb( dma_request_desc->port, dma_request_desc->buffer_addr, dma_request_desc->buff_length );
   
   /* Free descriptor */
   kfree(dma_request_desc);
   
   /* Check if we have any pending tx dma desc in the queue */
   spin_lock_irqsave(&dma_req_issue_lock, flags);
   next_dma_request_desc = dequeue_tx_desc();
   if( next_dma_request_desc )
   {
      if( bt_serial_issue_dma_request(next_dma_request_desc) > 0 )
         current_tx_dma_desc = next_dma_request_desc;
      else
         current_tx_dma_desc = NULL;
   }
   else
      current_tx_dma_desc = NULL;
   spin_unlock_irqrestore(&dma_req_issue_lock, flags);   
   
}

/* Callback function for RX DMA channel - Called in tasklet context  */
static void bt_serial_dma_rx_cb(void *data)
{
   unsigned long flags;
   struct dma_xfer_desc * dma_request_desc = (struct dma_xfer_desc *)data;
   
   BT_SER_DMA_DEBUG("%s: DESC_ADDR:0x%08x, DMA_ADDR:0x%08x, CPU_ADDR:0x%08x, buff_len:%d\n",
                     __FUNCTION__,
                     (unsigned int)dma_request_desc,
                     (unsigned int)dma_request_desc->dma_addr,
                     (unsigned int)dma_request_desc->buffer_addr,
                     (int)dma_request_desc->buff_length );
                     
   /* Set current desc as null */
   spin_lock_irqsave(&dma_req_issue_lock, flags);   
   current_rx_dma_desc = NULL;
   spin_unlock_irqrestore(&dma_req_issue_lock, flags);   
   
   /* Unmap DMA memory */
   dma_unmap_single(dma_request_desc->port->dev, dma_request_desc->dma_addr, dma_request_desc->xfer_length, DMA_FROM_DEVICE);
   
   /* Call rx complete callback */
   bt_serial_dma_cfg.rx_comp_cb( dma_request_desc->port, dma_request_desc->buffer_addr, dma_request_desc->buff_length );
   
   /* Free descriptor */
   kfree(dma_request_desc);
}

/* Common dma_request function */
static dma_cookie_t bt_serial_issue_dma_request( struct dma_xfer_desc * dma_request_desc )
{
   struct dma_async_tx_descriptor *desc;
   struct scatterlist sg;
   struct dma_chan * chan;
   dma_cookie_t dma_handle;
   dma_async_tx_callback callback;
   int dma_length = dma_request_desc->xfer_length;

   /* Get dma channel, set call backs and transfer lengths */
   if( dma_request_desc->direction == DMA_MEM_TO_DEV )
   {
      /* TX DMA request */
      chan = bt_serial_dma_cfg.tx_dma_chan;
      callback = bt_serial_dma_tx_cb;
      (BT_UART_REG(dma_request_desc->port))->HIPKT_LEN = dma_length;

      /* If in HWSLIP mode, indicate start of TX Packet */  
      if( (BT_UART_REG(dma_request_desc->port))->LCR & BT_UART_LCR_SLIP )
      {
         /* Wait for TX_PACKET_RDY to be 0 */
         while( (BT_UART_REG(dma_request_desc->port))->LSR & BT_UART_LSR_TX_PACKET_RDY );
         
         /* Indicate start of new TX Packet */
         (BT_UART_REG(dma_request_desc->port))->FCR |= BT_UART_FCR_TX_PACKET;          
      }     
   }
   else
   {
      /* RX DMA request */
      chan = bt_serial_dma_cfg.rx_dma_chan;
      callback = bt_serial_dma_rx_cb;     
      
      /* Adjust dma_length to account for already received pkt header */
      dma_length -= (dma_request_desc->dma_addr - dma_request_desc->dma_addr);
      (BT_UART_REG(dma_request_desc->port))->HOPKT_LEN = dma_length;
   }
   
   /* Map SG lists */
   sg_init_table(&sg, 1);
   
   /* MUST sg length to zero, as transfer length is controlled by peripheral     */
   /* Note: DMAC debug prints will show transfer lengths of 0 bytes, ignore them */
   sg_dma_len(&sg) = 0; 
   sg_set_page(&sg, pfn_to_page(PFN_DOWN(dma_request_desc->dma_addr)),
          0, offset_in_page(dma_request_desc->dma_addr));
   sg_dma_address(&sg) = dma_request_desc->dma_addr;
   
   /* Get dma descriptor */
   desc = dmaengine_prep_slave_sg(chan, &sg, 1, dma_request_desc->direction, 0);
         
   if (!desc) {
      printk(KERN_ERR "Couldn't prepare DMA slave!\n");
      return -ENXIO;
   }

   /* Set callback */
   desc->callback = callback;
   desc->callback_param = dma_request_desc;

   /* Submit DMA */
   dma_handle = dmaengine_submit((struct dma_async_tx_descriptor *)desc);
   
   BT_SER_DMA_DEBUG("%s: DESC_ADDR:0x%08x, DMA_ADDR:0x%08x, CPU_ADDR:0x%08x, buff_len:%d, Dir:%d\n",
                     __FUNCTION__,
                     (unsigned int)dma_request_desc,
                     (unsigned int)dma_request_desc->dma_addr,
                     (unsigned int)dma_request_desc->buffer_addr,
                     (int)dma_length,
                     (int)dma_request_desc->direction );
                     
   /* Issue DMA request */
   dma_async_issue_pending(chan);
   
   return (dma_handle); 
}  

/*************************** Non-Static Functions ****************************/
int bt_serial_issue_rx_dma_request( struct uart_port * port, char * rx_buff, unsigned int hdr_length, unsigned int payload_length )
{
   unsigned long flags;
   struct dma_xfer_desc * dma_request_desc = (struct dma_xfer_desc *)kzalloc(sizeof(struct dma_xfer_desc), GFP_NOWAIT);
   
   if( dma_request_desc )
   {                 
      /* Map DMA region */
      dma_request_desc->dma_addr = dma_map_single(port->dev, (void *)(rx_buff+hdr_length), payload_length,
                                                         DMA_FROM_DEVICE);
      if( dma_mapping_error(port->dev, dma_request_desc->dma_addr) )
      {
         printk(KERN_ERR "dma_map_single Rx failed\n");
         kfree(dma_request_desc);
         return -ENOMEM;
      }           
      
      /* Fill out descriptor */
      dma_request_desc->direction = DMA_DEV_TO_MEM;   
      dma_request_desc->buffer_addr = rx_buff;  
      dma_request_desc->buff_length = payload_length + hdr_length;
      dma_request_desc->xfer_length = payload_length;    
      dma_request_desc->port = port;      
                  
      /* Set current RX desc pointer */
      spin_lock_irqsave(&dma_req_issue_lock, flags);
      
      if( bt_serial_issue_dma_request( dma_request_desc ) > 0 )
      {
         current_rx_dma_desc = dma_request_desc;
      }
      
      spin_unlock_irqrestore(&dma_req_issue_lock, flags);   
      
      return( 0 );
      
   }
   else
   {
      printk(KERN_ERR "Couldn't get free dma_xfer_desc!\n");
      return -ENOMEM;
   }
}

int bt_serial_issue_tx_dma_request( struct uart_port * port, char * tx_buff, unsigned int length )
{
   unsigned long flags;
   
   struct dma_xfer_desc * dma_request_desc = (struct dma_xfer_desc *)kzalloc(sizeof(struct dma_xfer_desc), GFP_NOWAIT);
   
   if( dma_request_desc )
   {           
      /* Map DMA region */          
      dma_request_desc->dma_addr = dma_map_single(port->dev, (void *)tx_buff, length, DMA_TO_DEVICE);
      
      if( dma_mapping_error(port->dev, dma_request_desc->dma_addr) )
      {
         printk(KERN_ERR "dma_map_single Rx failed\n");
         kfree(dma_request_desc);
         return -ENOMEM;
      }           
      
      /* Fill out descriptor */
      dma_request_desc->direction = DMA_MEM_TO_DEV;   
      dma_request_desc->buffer_addr = tx_buff;  
      dma_request_desc->buff_length = length;
      dma_request_desc->xfer_length = length;
      dma_request_desc->port = port;      
      
      /* Check if tx dma has already been issued, if so then queue desc */
      spin_lock_irqsave(&dma_req_issue_lock, flags);
      if( !current_tx_dma_desc )
      {
         if( bt_serial_issue_dma_request( dma_request_desc ) > 0 )
         {
            current_tx_dma_desc = dma_request_desc;
         }
      }
      else
      {
         enqueue_tx_desc( dma_request_desc );         
      }     
      spin_unlock_irqrestore(&dma_req_issue_lock, flags);   
            
      return 0;
   }
   else
   {
      printk(KERN_ERR "Couldn't get free dma_xfer_desc!\n");
      return -ENOMEM;
   }
}
   
void bt_serial_dma_halt_tx( struct uart_port * port )
{
   /* FIXME: Make this port specific */
   /* Stop all TX DMA transfers */
   dmaengine_terminate_all(bt_serial_dma_cfg.tx_dma_chan);        
   /* FIXME: Free all tx descriptors for this port */
}

void bt_serial_dma_halt_rx( struct uart_port * port )
{
   /* FIXME: Make this port specific */
   /* Stop all RX DMA transfers */
   dmaengine_terminate_all(bt_serial_dma_cfg.rx_dma_chan);        
   /* FIXME: Free all rx descriptors for this port */
}

int bt_serial_dma_deinit(void)
{
   unsigned long flags;
   struct dma_xfer_desc * tmp_tx_dma_desc = NULL;
   struct dma_xfer_desc * tmp_rx_dma_desc = NULL;
   
   /* Stop all DMA transfers */
   dmaengine_terminate_all(bt_serial_dma_cfg.tx_dma_chan);
   dmaengine_terminate_all(bt_serial_dma_cfg.rx_dma_chan);  
   
   /* Release all DMA channels */
   dma_release_channel(bt_serial_dma_cfg.tx_dma_chan);
   dma_release_channel(bt_serial_dma_cfg.rx_dma_chan);
   
   /* De-init tx descriptor queue and free pending descriptors */
   deinit_tx_desc_queue();

   spin_lock_irqsave(&dma_req_issue_lock, flags); 
   tmp_tx_dma_desc = current_tx_dma_desc;
   tmp_rx_dma_desc = current_rx_dma_desc;
   current_tx_dma_desc = NULL;
   current_rx_dma_desc = NULL;
   spin_unlock_irqrestore(&dma_req_issue_lock, flags);   
   
   /* Free current descriptors */
   if( tmp_tx_dma_desc )  
   {
      dma_unmap_single(tmp_tx_dma_desc->port->dev, tmp_tx_dma_desc->dma_addr, tmp_tx_dma_desc->xfer_length, DMA_TO_DEVICE);            
      kfree(tmp_tx_dma_desc->buffer_addr);      
      kfree(tmp_tx_dma_desc);
   }
   
   if( tmp_rx_dma_desc )   
   {
      dma_unmap_single(tmp_rx_dma_desc->port->dev, tmp_rx_dma_desc->dma_addr, tmp_rx_dma_desc->xfer_length, DMA_FROM_DEVICE);            
      kfree(tmp_rx_dma_desc->buffer_addr);      
      kfree(tmp_rx_dma_desc);
   }     
   
   return 0;   
}

int bt_serial_dma_init( dma_complete_callback rx_callback, dma_complete_callback tx_callback )
{
   int ret = -ENOMEM;
   
   /* Save callback */
   bt_serial_dma_cfg.rx_comp_cb = rx_callback;
   bt_serial_dma_cfg.tx_comp_cb = tx_callback;
   
   /* reserve DMA channnels */
   dma_cap_zero(bt_serial_dma_cfg.mask);
   dma_cap_set(DMA_SLAVE, bt_serial_dma_cfg.mask);
   
   /* reserve rx */
   bt_serial_dma_cfg.rx_dma_chan = dma_request_channel(bt_serial_dma_cfg.mask, pl08x_filter_id,
         PL081_DMA_CHAN_BT_UART_RX);   
   if (!bt_serial_dma_cfg.rx_dma_chan) 
   {
      printk(KERN_ERR "Couldn't get BT_UART RX DMA channel!\n");
      return -ENXIO;
   }
            
   /* reserve tx */
   bt_serial_dma_cfg.tx_dma_chan = dma_request_channel(bt_serial_dma_cfg.mask, pl08x_filter_id,
         PL081_DMA_CHAN_BT_UART_TX);  
   if (!bt_serial_dma_cfg.tx_dma_chan) 
   {
      printk(KERN_ERR "Couldn't get BT_UART TX DMA channel!\n");
      ret = -ENXIO;
      goto err_release_rx;
   }
               
   /* Configure rx channel */
   memset(&bt_serial_dma_cfg.rx_slave_cfg, 0, sizeof(struct dma_slave_config));
   bt_serial_dma_cfg.rx_slave_cfg.direction = DMA_DEV_TO_MEM;
   bt_serial_dma_cfg.rx_slave_cfg.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
   bt_serial_dma_cfg.rx_slave_cfg.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
   bt_serial_dma_cfg.rx_slave_cfg.dst_maxburst = 0;
   bt_serial_dma_cfg.rx_slave_cfg.src_maxburst = 16;
   bt_serial_dma_cfg.rx_slave_cfg.src_addr = (dma_addr_t)&(((BtUartCtrlRegs * )BT_UART_PHYS_BASE)->uart_data); 
   bt_serial_dma_cfg.rx_slave_cfg.device_fc = true;
   ret = dmaengine_slave_config( bt_serial_dma_cfg.rx_dma_chan, &bt_serial_dma_cfg.rx_slave_cfg ); 
   if( ret < 0 )  
   {
      printk(KERN_ERR "Couldn't configure BT_UART TX DMA channel!\n");
      ret = -ENXIO;
      goto err_release_tx;
   }
      
   /* Configure tx channel */
   memset(&bt_serial_dma_cfg.tx_slave_cfg, 0, sizeof(struct dma_slave_config));
   bt_serial_dma_cfg.tx_slave_cfg.direction = DMA_MEM_TO_DEV;
   bt_serial_dma_cfg.tx_slave_cfg.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
   bt_serial_dma_cfg.tx_slave_cfg.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
   bt_serial_dma_cfg.tx_slave_cfg.dst_maxburst = 16;
   bt_serial_dma_cfg.tx_slave_cfg.src_maxburst = 0;
   bt_serial_dma_cfg.tx_slave_cfg.dst_addr = (dma_addr_t)&(((BtUartCtrlRegs * )BT_UART_PHYS_BASE)->uart_data); 
   bt_serial_dma_cfg.tx_slave_cfg.device_fc = true;
   ret = dmaengine_slave_config( bt_serial_dma_cfg.tx_dma_chan, &bt_serial_dma_cfg.tx_slave_cfg ); 
   if( ret < 0 )
   {
      printk(KERN_ERR "Couldn't configure BT_UART TX DMA channel!\n");
      ret = -ENXIO;
      goto err_release_tx;
   }
      
   /* Initialize tx descriptor queue */
   init_tx_desc_queue();
   
   return 0;   

err_release_tx:
   dma_release_channel(bt_serial_dma_cfg.tx_dma_chan);
err_release_rx:
   dma_release_channel(bt_serial_dma_cfg.rx_dma_chan);      
   
   return ret;                                        
}


















// int bt_serial_issue_tx_dma( dma_addr_t dma_addr, unsigned int length )
// {
//    struct dma_xfer_desc
//    struct dma_async_tx_descriptor *desc;
//    struct scatterlist sg;
//    
//    /* Map SG lists */
//    sg_init_table(&sg, 1);
//    sg_dma_len(&sg) = length;
//    sg_set_page(&sg, pfn_to_page(PFN_DOWN(dma_addr)),
//           xfer_len, offset_in_page(dma_addr));
//    sg_dma_address(&sg) = dma_addr;
//    desc = dmaengine_prep_slave_sg(bt_serial_dma_cfg.tx_dma_chan, &sg, 1, 
//          bt_serial_dma_cfg.tx_slave_cfg.direction, 0);
//          
//    if (!desc) {
//       printk(KERN_ERR "Couldn't prepare DMA slave!\n");
//       return -ENXIO;
//    }
// 
//    /* Set callback */
//    desc->callback = bt_serial_dma_tx_cb;
//    desc->callback_param = dma_addr;
// 
//    /* Submit DMA */
//    dmaengine_submit((struct dma_async_tx_descriptor *)desc);
// }
// 
// int bt_serial_issue_rx_dma( dma_addr_t dma_addr, unsigned int length )
// {
//    struct dma_async_tx_descriptor *desc;
//    struct scatterlist sg;
//    
//    /* Map SG lists */
//    sg_init_table(&sg, 1);
//    sg_dma_len(&sg) = length;
//    sg_set_page(&sg, pfn_to_page(PFN_DOWN(dma_addr)),
//           xfer_len, offset_in_page(dma_addr));
//    sg_dma_address(&sg) = dma_addr;
//    desc = dmaengine_prep_slave_sg(bt_serial_dma_cfg.rx_dma_chan, &sg, 1, 
//          bt_serial_dma_cfg.rx_slave_cfg.direction, 0);
//          
//    if (!desc) {
//       printk(KERN_ERR "Couldn't prepare DMA slave!\n");
//       return -ENXIO;
//    }
// 
//    /* Set callback */
//    desc->callback = bt_serial_dma_rx_cb;
//    desc->callback_param = dma_addr;
// 
//    /* Submit DMA */
//    dmaengine_submit((struct dma_async_tx_descriptor *)desc);
// }  
