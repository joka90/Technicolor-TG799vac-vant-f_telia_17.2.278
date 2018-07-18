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

/* Description: Bluetooth Serial port driver for the BCM963XX. */

#include <linux/version.h>
#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/tty.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/serial.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/tty_flip.h>
#include <linux/slab.h>
#include <linux/completion.h>
#include <board.h>
#include <bcm_map_part.h>
#include <bcm_intr.h>
#include <linux/bcm_colors.h>
#ifdef __arm__
#include <mach/hardware.h>
#endif

#include <linux/serial_core.h>
#include "bt_serial_dma.h"

#include <asm/uaccess.h> /*copy_from_user*/
#include <linux/proc_fs.h>

/* Note on locking policy: All uart ops take the 
   port->lock, except startup shutdown and termios */

/******* Defines & Types *******/
#define BT_SERIAL_PRINT_CHARS       0
#define BT_SERIAL_LOOPBACK_ENABLE   0
#define BT_SERIAL_DUMP_REGS         0
#define BT_SERIAL_API_DEBUG         0
#define BT_SERIAL_MIN_BAUD          115200   /* Arbitrary value, this can be as low as 9600  */
#define BT_SERIAL_USE_HIGHRATE      0        /* Highrate allows for baudrates higher than 3.125Mbps */
#define BT_SERIAL_CALCULATE_DLHBR   1        /* 1 - Calculate dl/hbr values for any baudrates, 0 - use predefined values */

#undef BT_SER_DEBUG             /* undef it, just in case */
#if BT_SERIAL_API_DEBUG
#  define BT_SER_DEBUG(fmt, args...) printk( KERN_DEBUG "bt_serial: " fmt, ## args)
#else
#  define BT_SER_DEBUG(fmt, args...) /* not debugging: nothing */
#endif

#define UART_NR     1
#define BT_UART_PORT    66
#define BCM63XX_ISR_PASS_LIMIT  256
/* generate BT_UART_TXFIFOAEMPTYTHRESH interrupt if tx fifo level falls below this number.
 * Must define a constant becuase bt_serial_console_write needs this and
 * (BT_UART_REG(port))->fifocfg is not set until later. */
#define BT_UART_TXFIFOAEMPTYTHRESH      100 /* Threhsold indicates when TX FIFO has more room */
#define BT_UART_RXFIFOAFULLTHRESH    1 /* Threshold indicates when RX FIFO is not empty */
#define BT_UART_RXFIFOAFULLTHRESH_DMA  4 /* Threshold indicates when RX FIFO has the 4 byte hci packet header in it */
#define BT_UART_RXFIFOAFULLTHRESH_DMA_SWSLIP  5 /* Threshold indicates when RX FIFO has the 5 byte SLIP+hci packet header in it*/
#define BT_UART_MAX_PKT_HDR BT_UART_RXFIFOAFULLTHRESH_DMA_SWSLIP

#define BT_UART_REG(p) ((volatile BtUartCtrlRegs *) (p)->membase)

#ifndef IO_ADDRESS
#define IO_ADDRESS(x) (x)
#endif

#define BT_SERIAL_PROC_ENTRY_ROOT "driver/bt_serial"
#define BT_SERIAL_PROC_ENTRY "driver/bt_serial/xfer_mode"

typedef enum
{
   BT_SER_XFER_MODE_PIO = 0,
   BT_SER_XFER_MODE_DMA,
   BT_SER_XFER_MODE_DMA_SWSLIP,
   BT_SER_XFER_MODE_DMA_HWSLIP,
   BT_SER_XFER_MODE_DMA_HWSLIP_HWCRC,
   BT_SER_XFER_MODE_DMA_HWSLIP_HWCRC_WCHK,
   BT_SER_XFER_MODE_MAX,   
} BT_SERIAL_XFER_MODE;

typedef struct
{
   BT_SERIAL_XFER_MODE mode;
   char mode_desc[40];
} BT_SERIAL_XFER_MODE_MAP;

typedef struct  {
#ifdef BIG_ENDIAN 
   unsigned int seq_num:3;
   unsigned int ack_num:3;
   unsigned int data_integrity_check:1;
   unsigned int reliable_pkt:1;
   unsigned int pkt_type:4;
   unsigned int pkt_len:12;
   unsigned int header_checksum:8;
#else
   unsigned int header_checksum:8;
   unsigned int pkt_len:12;
   unsigned int pkt_type:4;
   unsigned int reliable_pkt:1;
   unsigned int data_integrity_check:1;
   unsigned int ack_num:3;
   unsigned int seq_num:3;
#endif
} BT_SERIAL_HCI_HDR;

/******* Private prototypes *******/
static void deinit_bt_serial_port(struct uart_port * port );
static int init_bt_serial_port( struct uart_port * port );
static unsigned int bt_serial_tx_empty( struct uart_port * port );   
static void bt_serial_set_mctrl( struct uart_port * port, unsigned int mctrl );
static unsigned int bt_serial_get_mctrl( struct uart_port * port );  
static void bt_serial_stop_tx( struct uart_port * port );    
static void bt_serial_start_tx( struct uart_port * port );   
static void bt_serial_stop_rx( struct uart_port * port );    
static void bt_serial_enable_ms( struct uart_port * port );  
static void bt_serial_break_ctl( struct uart_port * port, int break_state );  
static int bt_serial_startup( struct uart_port * port );    
static void bt_serial_shutdown( struct uart_port * port );   
static int bt_serial_set_baud_rate( struct uart_port *port, int baud );
static void bt_serial_set_termios( struct uart_port * port, struct ktermios *termios, struct ktermios *old );
static const char * bt_serial_type( struct uart_port * port );       
static void bt_serial_release_port( struct uart_port * port );
static int bt_serial_request_port( struct uart_port * port );
static void bt_serial_config_port( struct uart_port * port, int flags );
static int bt_serial_verify_port( struct uart_port * port, struct serial_struct *ser );
static void bt_serial_tx_chars_dma(struct uart_port *port);
static int rx_dma_comp_cb( struct uart_port * port, char * buff_address, unsigned int length );
static int tx_dma_comp_cb( struct uart_port * port, char * buff_address, unsigned int length );
static int get_payload_len( char * pkt_header );
static void bt_serial_set_xfer_mode( BT_SERIAL_XFER_MODE mode );
static ssize_t proc_write_mode(struct file *file, const char __user *buffer, unsigned long count, void *data);
static int proc_read_mode(char *buf, char **start, off_t offset, int len, int *unused_i, void *unused_v);
static int bt_serial_create_proc_entries( void );
static int bt_serial_remove_proc_entries( void );
void bt_serial_dump_regs(struct uart_port * port);

/******* Local Variables *******/
static struct proc_dir_entry *xfer_mode_entry;

BT_SERIAL_XFER_MODE_MAP xfer_mode_map[] = 
{
   { BT_SER_XFER_MODE_PIO                   , "Programmed I/O"},  
   { BT_SER_XFER_MODE_DMA                   , "DMA"},
   { BT_SER_XFER_MODE_DMA_SWSLIP            , "DMA & S/W SLIP"},
   { BT_SER_XFER_MODE_DMA_HWSLIP            , "DMA & H/W SLIP"},
   { BT_SER_XFER_MODE_DMA_HWSLIP_HWCRC      , "DMA & H/W SLIP & H/W TXCRC"},
   { BT_SER_XFER_MODE_DMA_HWSLIP_HWCRC_WCHK , "DMA & H/W SLIP & H/W TXCRC & RXCRC chk"},
   { BT_SER_XFER_MODE_MAX                   , "null"},
}; 

static BT_SERIAL_XFER_MODE xfer_mode = BT_SER_XFER_MODE_PIO;

static struct uart_ops bt_serial_pops = 
{
   .tx_empty     = bt_serial_tx_empty,
   .set_mctrl    = bt_serial_set_mctrl,
   .get_mctrl    = bt_serial_get_mctrl,
   .stop_tx      = bt_serial_stop_tx,
   .start_tx     = bt_serial_start_tx,
   .stop_rx      = bt_serial_stop_rx,
   .enable_ms    = bt_serial_enable_ms,
   .break_ctl    = bt_serial_break_ctl,
   .startup      = bt_serial_startup,
   .shutdown     = bt_serial_shutdown,
   .set_termios  = bt_serial_set_termios,
   .type         = bt_serial_type,
   .release_port = bt_serial_release_port,
   .request_port = bt_serial_request_port,
   .config_port  = bt_serial_config_port,
   .verify_port  = bt_serial_verify_port,
};

static struct uart_port bt_serial_ports[] = 
{
   {
      .membase    = (void *)IO_ADDRESS(BT_UART_PHYS_BASE),
      .mapbase    = BT_UART_PHYS_BASE,
      .iotype     = SERIAL_IO_MEM,
      .irq        = INTERRUPT_ID_UART2,
      .uartclk    = 50000000,
      .fifosize   = 1040,
      .ops        = &bt_serial_pops,
      .flags      = ASYNC_BOOT_AUTOCONF,
      .line       = 0,
   }
};

static struct uart_driver bt_serial_reg = 
{
   .owner            = THIS_MODULE,
   .driver_name      = "btserial",
   .dev_name         = "ttyBt",
   .major            = TTY_MAJOR,
   .minor            = 66,
   .nr               = UART_NR,
};

/******* Functions ********/

static int get_payload_len( char * pkt_header )
{
   BT_SERIAL_HCI_HDR * hci_hdr = (BT_SERIAL_HCI_HDR *)pkt_header; 
   int payload_len = hci_hdr->pkt_len;
   
   switch( xfer_mode )
   {                                   
      case BT_SER_XFER_MODE_DMA_SWSLIP:
         payload_len += 3; /* Add 2 bytes for CRC, 1 Byte for SLIP delim */
      break;
         
      case BT_SER_XFER_MODE_DMA_HWSLIP:           
      case BT_SER_XFER_MODE_DMA_HWSLIP_HWCRC:    
         payload_len += 2; /* Add 2 bytes for CRC */
      break;
       
      case BT_SER_XFER_MODE_DMA_HWSLIP_HWCRC_WCHK:
         payload_len += 3; /* Add 2 bytes for CRC, 1 Byte for CRC check */
      break;      
      
      default:
         /* Debug mode, hardocde payload to 10 bytes */
         payload_len = 10;
      break;
   }
   
   return payload_len;
}

/*
 * Enable ms
 */
static void bt_serial_enable_ms(struct uart_port *port)
{
   BT_SER_DEBUG(KERN_INFO "%s \n", __FUNCTION__);
}

/*
 * Get MCR register
 */
static unsigned int bt_serial_get_mctrl(struct uart_port *port)
{
   static unsigned int bt_serial_uart_mctrl = 0;
   
   BT_SER_DEBUG(KERN_INFO "%s \n", __FUNCTION__);   
   
   if( (BT_UART_REG(port))->MSR & BT_UART_MSR_CTS_STAT )
   { 
      bt_serial_uart_mctrl |= TIOCM_CTS;
   }
   
   if( (BT_UART_REG(port))->MSR & BT_UART_MSR_RTS_STAT )
   { 
      bt_serial_uart_mctrl |= TIOCM_RTS;
   }
   
   return bt_serial_uart_mctrl;
}

/*
 * Set MCR register
 */
static void bt_serial_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
   BT_SER_DEBUG(KERN_INFO "%s \n", __FUNCTION__);
   
   if( mctrl & TIOCM_RTS )
   {
      (BT_UART_REG(port))->MCR |= BT_UART_MCR_PROG_RTS;
   }
   else
   {
      (BT_UART_REG(port))->MCR &= ~BT_UART_MCR_PROG_RTS;   
   }   
}

/*
 * Set break state
 */
static void bt_serial_break_ctl(struct uart_port *port, int break_state)
{  
   BT_SER_DEBUG(KERN_INFO "%s \n", __FUNCTION__);
}

/*
 * Initialize serial port
 */
static int bt_serial_startup(struct uart_port *port)
{   
   unsigned long flags;
   
   spin_lock_irqsave(&port->lock, flags);
      
   init_bt_serial_port( port );
   
   spin_unlock_irqrestore(&port->lock, flags);  
    
   BT_SER_DEBUG(KERN_INFO "%s \n", __FUNCTION__);
   return 0;
}

/*
 * Shutdown serial port
 */
static void bt_serial_shutdown(struct uart_port *port)
{
   unsigned long flags;
   
   spin_lock_irqsave(&port->lock, flags);
   
   deinit_bt_serial_port(port);
   
   spin_unlock_irqrestore(&port->lock, flags);  

   BT_SER_DEBUG(KERN_INFO "%s \n", __FUNCTION__);    
}

/*
 * Release the memory region(s) being used by 'port'
 */
static void bt_serial_release_port(struct uart_port *port)
{
   BT_SER_DEBUG(KERN_INFO "%s \n", __FUNCTION__);   
}

/*
 * Request the memory region(s) being used by 'port'
 */
static int bt_serial_request_port(struct uart_port *port)
{
   BT_SER_DEBUG(KERN_INFO "%s \n", __FUNCTION__);
   return 0;
}

/*
 * verify the new serial_struct (for TIOCSSERIAL).
 */
static int bt_serial_verify_port(struct uart_port *port, struct serial_struct *ser)
{
   BT_SER_DEBUG(KERN_INFO "%s \n", __FUNCTION__);
   return 0;
}

/*
 * Disable tx transmission
 */
static void bt_serial_stop_tx(struct uart_port *port)
{
   BT_SER_DEBUG(KERN_INFO "%s \n", __FUNCTION__);
   (BT_UART_REG(port))->uart_int_en &= ~BT_UART_TXFIFOAEMPTY;
}

/*
 * Disable rx reception
 */
static void bt_serial_stop_rx(struct uart_port *port)
{
   BT_SER_DEBUG(KERN_INFO "%s \n", __FUNCTION__);   
   (BT_UART_REG(port))->uart_int_en &= ~BT_UART_RXFIFOAFULL; 
}

/*
 * Receive rx chars - Called from ISR
 */
static void bt_serial_rx_chars(struct uart_port *port)
{
   struct tty_struct *tty = port->state->port.tty;    
   unsigned int max_count = 256;
   int i=0;
   unsigned short status;
   unsigned char ch, flag = TTY_NORMAL;
   unsigned char pkt_header[BT_UART_MAX_PKT_HDR];
   unsigned int payload_bytes = 0;
   unsigned int header_bytes = 0;
   char * rx_buff = NULL;

   if( xfer_mode >= BT_SER_XFER_MODE_DMA )
   {  
      /* In DMA mode restrict direct FIFO reads to just the packet header */
      max_count = (BT_UART_REG(port))->RFL;
      
      /* Disable receive DMA, enable programmed FIFO reads */
      (BT_UART_REG(port))->HO_DMA_CTL &= ~BT_UART_DMA_CTL_DMA_EN;    
   }
   
   /* Interrupt due to content in RX FIFO -> clear BT_UART_RXFIFOEMPTY status bit */
   (BT_UART_REG(port))->uart_int_stat |= BT_UART_RXFIFOEMPTY;
   
   /* Local copy */
   status = (BT_UART_REG(port))->uart_int_stat;
   
   /* Keep reading from RX FIFO as long as it is not empty */
   while ( !(status & BT_UART_RXFIFOEMPTY) && i < max_count ) 
   {     
      ch = (BT_UART_REG(port))->uart_data;
      
      if( xfer_mode >= BT_SER_XFER_MODE_DMA )
      {
         pkt_header[i] = ch;
      }

      i++;      
      port->icount.rx++;
      
      /*
      * Note that the error handling code is
      * out of the main execution path
      */
      if (status & (BT_UART_RXBRKDET | BT_UART_RXPARITYERR)) 
      {
         if (status & BT_UART_RXBRKDET) 
         {
            status &= ~(BT_UART_RXPARITYERR);
            port->icount.brk++;
            if (uart_handle_break(port))
               goto ignore_char;
         } 
         else if (status & BT_UART_RXPARITYERR)
            port->icount.parity++;
         
         if (status & BT_UART_RXBRKDET)
            flag = TTY_BREAK;
         else if (status & BT_UART_RXPARITYERR)
            flag = TTY_PARITY;               
      }
      
      /* Check overflow conditions */
      if ((BT_UART_REG(port))->LSR & BT_UART_LSR_RX_OVERFLOW)
         port->icount.overrun++;                      
   
      if( xfer_mode == BT_SER_XFER_MODE_PIO )
      {         
         /* Add char to flip buffer */
         tty_insert_flip_char(tty, ch, flag);
      }
      
#if BT_SERIAL_PRINT_CHARS       
      printk(KERN_INFO "%s: Received char: %c\n", __FUNCTION, ch);
#endif        
   
   ignore_char:
      status = (BT_UART_REG(port))->uart_int_stat;
   }
   
   if( xfer_mode >= BT_SER_XFER_MODE_DMA )
   {
      /* Get number of bytes in header */
      header_bytes = max_count;
      
      /* Get number of bytes in payload - Skip first byte (SLIP delim) in SWSLIP mode */
      payload_bytes = get_payload_len( ( (xfer_mode == BT_SER_XFER_MODE_DMA_SWSLIP ) ? &pkt_header[1] : pkt_header ) );   
            
      /* Disable RFL interrupt - will be re-enabled at end of DMA*/
      /* FIXME: Do we need to do this? Setting of AFMODE_EN should block AFULL interrupt */
      (BT_UART_REG(port))->uart_int_en &= ~BT_UART_RXFIFOAFULL; 
      
      /* Enable receive DMA */
      (BT_UART_REG(port))->HO_DMA_CTL = (BT_UART_DMA_CTL_BURSTMODE_EN 
                                       | BT_UART_DMA_CTL_AFMODE_EN 
                                       | BT_UART_DMA_CTL_FASTMODE_EN
                                       | BT_UART_DMA_CTL_DMA_EN);    
                                       
      /* Allocate RX DMA buffer */
      rx_buff = kzalloc( (header_bytes+payload_bytes), GFP_NOWAIT);
      
      if( rx_buff )
      {
         /* Copy over pkt header */
         memcpy( rx_buff, pkt_header, max_count );
         
         /* Issue DMA */
         bt_serial_issue_rx_dma_request( port, rx_buff, header_bytes, payload_bytes ); 
      }
      else
      {
         printk(KERN_WARNING "%s: Cannot allocate buffer for DMA receive ", __FUNCTION__);               
      }
   }
   else
   {
      /* push flip buffer */
      tty_flip_buffer_push(tty);
   }
}

/*
 * Transmit tx chars
 */
static void bt_serial_tx_chars(struct uart_port *port)
{
   struct circ_buf *xmit = &port->state->xmit;

   (BT_UART_REG(port))->uart_int_en &= ~BT_UART_TXFIFOAEMPTY;

   if (port->x_char) 
   {
      /* Spin while TX FIFO still has characters to send */
      while (   ((BT_UART_REG(port))->LSR & BT_UART_LSR_TX_DATA_AVAIL) 
            && !((BT_UART_REG(port))->LSR & BT_UART_LSR_TX_HALT) 
            &&  ((BT_UART_REG(port))->MCR & BT_UART_MCR_TX_ENABLE)      );
       
      /* If TX stalls then return */
      if(  ((BT_UART_REG(port))->LSR & BT_UART_LSR_TX_HALT) || 
          !((BT_UART_REG(port))->MCR & BT_UART_MCR_TX_ENABLE) )
      {
         /* TX has stalled - Do nothing but drain buffer and return */
      }
      else
      {  
         /* TX FIFO is now empty - send character and return*/
         (BT_UART_REG(port))->uart_data = port->x_char;
      }
      port->icount.tx++;
      port->x_char = 0;
     
      return;
   }
   
   if (uart_circ_empty(xmit) || uart_tx_stopped(port)) 
   {
      bt_serial_stop_tx(port);
      return;
   }

   /* Write data until TX-FIFO is full OR circbuff is empty */
   while (!((BT_UART_REG(port))->uart_int_stat & BT_UART_TXFIFOFULL)) 
   { 
#if BT_SERIAL_PRINT_CHARS       
      printk(KERN_INFO "%s: Sent char: %c\n", __FUNCTION__, xmit->buf[xmit->tail]);
#endif        
      if( (BT_UART_REG(port))->MCR & BT_UART_MCR_TX_ENABLE )
         (BT_UART_REG(port))->uart_data = xmit->buf[xmit->tail]; 
      xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
      port->icount.tx++;
      if (uart_circ_empty(xmit))
         break;
   }

   if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
      uart_write_wakeup(port);

   if (uart_circ_empty(xmit))
      bt_serial_stop_tx(port);
   else 
   {
      /* We were not able to write all tx data to TX FIFO   *
       * Therefore enable interrupt for TX FIFO falling     *
       * below a threshold so that we can continue writing  *
       * rest of the circBuf                                */
      (BT_UART_REG(port))->uart_int_en |= BT_UART_TXFIFOAEMPTY ; 
   }
}

/*
 * Issue TX DMA request
 */
static void bt_serial_tx_chars_dma(struct uart_port *port)
{
   struct circ_buf *xmit = &port->state->xmit;
   unsigned int num_bytes = uart_circ_chars_pending(xmit);
   char * tx_buff = kzalloc(num_bytes, GFP_NOWAIT);
   int i=0;
      
   if( tx_buff )
   {
      while( !uart_circ_empty(xmit) && i < num_bytes ) 
      { 
#if BT_SERIAL_PRINT_CHARS       
         printk(KERN_INFO "%s: Sent char: %c\n", __FUNCTION__, xmit->buf[xmit->tail]);
#endif        
         tx_buff[i] = xmit->buf[xmit->tail]; 
         xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
         port->icount.tx++;
         i++;
      }   
         
      if( ((BT_UART_REG(port))->MCR & BT_UART_MCR_TX_ENABLE) )
      {
         /* Only Issue DMA request if TX is enabled */
         bt_serial_issue_tx_dma_request( port, tx_buff, num_bytes);
      }
   }
   else
   {
      printk(KERN_WARNING "%s: Cannot allocate buffer for DMA transmit ", __FUNCTION__);      
   }   
   
   /* Consumed all tx chars, stop further tx */
   bt_serial_stop_tx(port);
}


/*
 * Enable TX transmission
 */
static void bt_serial_start_tx(struct uart_port *port)
{  
   BT_SER_DEBUG(KERN_INFO "%s \n", __FUNCTION__);
   
   if( xfer_mode >= BT_SER_XFER_MODE_DMA )
   {
      bt_serial_tx_chars_dma(port);
   }  
   else
   {
      /* If TX is full, enable the TX FIFO almost empty interrupt and return */
      if( (BT_UART_REG(port))->uart_int_stat & BT_UART_TXFIFOFULL ) 
      {
         (BT_UART_REG(port))->uart_int_en |= BT_UART_TXFIFOAEMPTY;
      }
      else
      {
         /* IF TX FIFO is not full, attempt to write to it right away */
         bt_serial_tx_chars(port);
      }    
   }  
}

/*
 * bt_uart ISR
 */
static irqreturn_t bt_serial_int(int irq, void *dev_id)
{
   struct uart_port *port = dev_id;
   unsigned int status, pass_counter = BCM63XX_ISR_PASS_LIMIT;
   
   spin_lock(&port->lock);
   
   while ((status = (BT_UART_REG(port))->uart_int_stat & (BT_UART_REG(port))->uart_int_en)) 
   {  
      /* We have recieved a character in the RX FIFO */
      if (status & BT_UART_RXFIFOAFULL)
         bt_serial_rx_chars(port);
      
      if( xfer_mode == BT_SER_XFER_MODE_PIO )         
      {
         /* TX FIFO now has room */
         if (status & (BT_UART_TXFIFOAEMPTY))
            bt_serial_tx_chars(port);
      }
      
      if (pass_counter-- == 0)
         break;
   
   }
   
   /* Clear interrupt at peripheral level */
   (BT_UART_REG(port))->uart_int_stat = BT_UART_INT_MASK;
   
   spin_unlock(&port->lock);
   
#if !defined(CONFIG_ARM)
   // Clear the interrupt
   BcmHalInterruptEnable (irq);
#endif
   
   return IRQ_HANDLED;
}

/*
 * Check if TX FIFO is empty
 */
static unsigned int bt_serial_tx_empty(struct uart_port *port)
{
    BT_SER_DEBUG(KERN_INFO "%s \n", __FUNCTION__);
   //When no data is available -> TX FIFO is empty
    return (BT_UART_REG(port))->LSR & BT_UART_LSR_TX_DATA_AVAIL ? 0 : TIOCSER_TEMT;
}

/* 
 * Set bt uart baudrate registers
 */
static int bt_serial_set_baud_rate( struct uart_port *port, int baud )
{
#if BT_SERIAL_CALCULATE_DLHBR   
   unsigned int extraCyc, intDiv;
   
   BT_SER_DEBUG(KERN_INFO "%s: requested baud: %d  \n", __FUNCTION__, baud);

#if BT_SERIAL_USE_HIGHRATE
   /* Calculate the integer divider */
   intDiv = port->uartclk / baud ;
   (BT_UART_REG(port))->dlbr = 256 - intDiv;
   (BT_UART_REG(port))->MCR |= BT_UART_MCR_HIGH_RATE;      
#else      
   /* Calculate the integer divider */
   intDiv = port->uartclk / ( 16*baud );
   (BT_UART_REG(port))->dlbr = 256 - intDiv;
   (BT_UART_REG(port))->MCR &= ~BT_UART_MCR_HIGH_RATE;         
   
   /* Calculate the extra cycles of uartclk required to full-  *
    * -fill the bit timing requirements for required baudrate  */
   extraCyc = ( port->uartclk / baud ) - intDiv * 16;   
   if( extraCyc )
   {
      /* Equally distribute the extraCycles at the start and end of bit interval */
      (BT_UART_REG(port))->dhbr = ( extraCyc/2 | (extraCyc/2) << 4 ) + extraCyc % 2;
   }
   
   if( extraCyc > 16 )
   {
      printk(KERN_WARNING "bt_serial_set_baud_rate: Cannot set required extra cycles %d ", extraCyc);      
   }
   
   BT_SER_DEBUG(KERN_INFO "dlbr: 0x%08x, dhbr: 0x%08x \n", (unsigned int)(BT_UART_REG(port))->dlbr, 
                                                           (unsigned int)(BT_UART_REG(port))->dhbr );
#endif /* BT_SERIAL_USE_HIGHRATE */

#else         

   /* Use predefined values */
   switch( baud )
   {              
      case 3000000:
      {
         (BT_UART_REG(port))->dhbr = BT_UART_DHBR_3000K;
         (BT_UART_REG(port))->dlbr = BT_UART_DLBR_3000K;
      }
    
    case 115200: 
    default:       
      {
         (BT_UART_REG(port))->dhbr = BT_UART_DHBR_115200;
         (BT_UART_REG(port))->dlbr = BT_UART_DLBR_115200;
         baud = 115200;
      }
    break;
   } 
#endif /* BT_SERIAL_CALCULATE_DLHBR */

   BT_SER_DEBUG(KERN_INFO "%s: Setting baudrate to: %d\n", __FUNCTION__, baud);
   
   return 0;             
}

/*
 * Set terminal options
 */
static void bt_serial_set_termios(struct uart_port *port, 
    struct ktermios *termios, struct ktermios *old)
{
   unsigned long flags;
   unsigned int baud;
   
   BT_SER_DEBUG(KERN_INFO "%s \n", __FUNCTION__);
   
   spin_lock_irqsave(&port->lock, flags);
   
   /* Disable RX/TX */
   (BT_UART_REG(port))->LCR &= ~(BT_UART_LCR_RXEN | BT_UART_LCR_TXOEN);
      
   /* Ask the core to return selected baudrate value ( bps ) */
#if BT_SERIAL_USE_HIGHRATE  
   baud = uart_get_baud_rate(port, termios, old, BT_SERIAL_MIN_BAUD, port->uartclk/8); 
#else
   baud = uart_get_baud_rate(port, termios, old, BT_SERIAL_MIN_BAUD, port->uartclk/16); 
#endif   
   
   /* Set baud rate registers */
   bt_serial_set_baud_rate(port, baud);
   
   /* Set stop bits */
   if (termios->c_cflag & CSTOPB)
      (BT_UART_REG(port))->LCR |= BT_UART_LCR_STB;
   else
      (BT_UART_REG(port))->LCR &= ~(BT_UART_LCR_STB);
   
   /* Set Parity */
   if (termios->c_cflag & PARENB) 
   {
      (BT_UART_REG(port))->LCR |= BT_UART_LCR_PEN;
      if (!(termios->c_cflag & PARODD))
         (BT_UART_REG(port))->LCR |= BT_UART_LCR_EPS;
   }
   
   /* Update the per-port timeout */
   uart_update_timeout(port, termios->c_cflag, baud);
   
   /* Unused in this driver */
   port->read_status_mask = 0;
   port->ignore_status_mask = 0;        
    
   /* Finally, re-enable the transmitter and receiver */
   (BT_UART_REG(port))->LCR |= (BT_UART_LCR_RXEN | BT_UART_LCR_TXOEN);
      
   spin_unlock_irqrestore(&port->lock, flags);   
}

/*
 * Check serial type
 */
static const char *bt_serial_type(struct uart_port *port)
{
    BT_SER_DEBUG(KERN_INFO "%s \n", __FUNCTION__);   
    return port->type == BT_UART_PORT ? "BT_UART" : NULL;
}

/*
 * Configure/autoconfigure the port.
 */
static void bt_serial_config_port(struct uart_port *port, int flags)
{
   BT_SER_DEBUG(KERN_INFO "%s \n", __FUNCTION__);
   if (flags & UART_CONFIG_TYPE) 
   {
      port->type = BT_UART_PORT;
   }
}

/*
 * Initialize serial port registers.
 */
static int init_bt_serial_port( struct uart_port * port )
{  
   unsigned int temp;
   
   /* Disable TX/RX */
   (BT_UART_REG(port))->LCR = 0;
   (BT_UART_REG(port))->MCR = 0;
   
   /* Assign HC data */
   (BT_UART_REG(port))->ptu_hc = BT_UART_PTU_HC_DATA;
   
   /* Route btuart signals out on uart2 pins */
   UART1->prog_out |= ARMUARTEN;
   
   /* Disable/Clear interrupts */
   (BT_UART_REG(port))->uart_int_stat = BT_UART_INT_MASK;
   (BT_UART_REG(port))->uart_int_en = BT_UART_INT_MASK_DISABLE;
   
   /* Set baudrate - 115200bps based on a 50Mhz clock */
   (BT_UART_REG(port))->dhbr = BT_UART_DHBR_115200;
   (BT_UART_REG(port))->dlbr = BT_UART_DLBR_115200;
   
   /* Config FCR */
   (BT_UART_REG(port))->FCR = 0;
   
   /* Config LCR - 1 Stop bit */
   (BT_UART_REG(port))->LCR = BT_UART_LCR_STB;
   
   /* Config MCR - Enable baud rate adjustment */
   (BT_UART_REG(port))->MCR = BT_UART_MCR_BAUD_ADJ_EN;
   
#if BT_SERIAL_LOOPBACK_ENABLE
   /* Config MCR - Set loopback */
   (BT_UART_REG(port))->MCR |= BT_UART_MCR_LOOPBACK;
#endif   
   
   /* Set TX-almost-empty and RX-almost-full thresholds*/
   (BT_UART_REG(port))->TFL = BT_UART_TXFIFOAEMPTYTHRESH;
      
   /* Set Recieve FIFO flow control register */
   (BT_UART_REG(port))->RFC = BT_UART_RFC_NO_FC_DATA;
   
   /* Set escape character register */
   (BT_UART_REG(port))->ESC = BT_UART_ESC_NO_SLIP_DATA;
   
   /* Clear DMA packet lengths, Enable TX DMAs*/
   if( xfer_mode >= BT_SER_XFER_MODE_DMA )
   {
      (BT_UART_REG(port))->HOPKT_LEN  = 0;
      (BT_UART_REG(port))->HIPKT_LEN  = 0;
                                       
      (BT_UART_REG(port))->HI_DMA_CTL =  BT_UART_DMA_CTL_BURSTMODE_EN 
                                       | BT_UART_DMA_CTL_AFMODE_EN 
                                       | BT_UART_DMA_CTL_FASTMODE_EN
                                       | BT_UART_DMA_CTL_DMA_EN;     
                                                                     
      (BT_UART_REG(port))->HO_BSIZE   = BT_UART_HO_BSIZE_DATA;
      (BT_UART_REG(port))->HI_BSIZE   = BT_UART_HI_BSIZE_DATA;
   }

   /* Set thresholds for the RX Almost Full Interrupt + SLIP settings (if enabled) */
   switch( xfer_mode )
   {
      case BT_SER_XFER_MODE_PIO:
         (BT_UART_REG(port))->RFL = BT_UART_RXFIFOAFULLTHRESH;
      break;
      
      case BT_SER_XFER_MODE_DMA_SWSLIP:
         (BT_UART_REG(port))->RFL = BT_UART_RXFIFOAFULLTHRESH_DMA_SWSLIP;
      break;   
               
      case BT_SER_XFER_MODE_DMA_HWSLIP_HWCRC_WCHK:       
         (BT_UART_REG(port))->LCR |= BT_UART_LCR_SLIP_RX_CRC;
      case BT_SER_XFER_MODE_DMA_HWSLIP_HWCRC:      
         (BT_UART_REG(port))->LCR |= BT_UART_LCR_SLIP_TX_CRC; 
      case BT_SER_XFER_MODE_DMA_HWSLIP:
         (BT_UART_REG(port))->LCR |= BT_UART_LCR_SLIP;               
         (BT_UART_REG(port))->FCR |= BT_UART_FCR_SLIP_RESYNC;  
         (BT_UART_REG(port))->ESC = BT_UART_ESC_SLIP_DATA;                                
         (BT_UART_REG(port))->RFL = BT_UART_RXFIFOAFULLTHRESH_DMA;   
      break;
         
      default:
         (BT_UART_REG(port))->RFL = BT_UART_RXFIFOAFULLTHRESH_DMA;   
      break;
   }
       
   /* Clear RX FIFO */
   while( !((BT_UART_REG(port))->uart_int_stat & BT_UART_RXFIFOEMPTY) )
   {
      temp = (BT_UART_REG(port))->uart_data;
   }
      
   /* Config LCR - RX/TX enables */
   (BT_UART_REG(port))->LCR |= BT_UART_LCR_RXEN | BT_UART_LCR_TXOEN;
   
   /* Config MCR - TX state machine enable */
   (BT_UART_REG(port))->MCR |= BT_UART_MCR_TX_ENABLE;
   
   /* Flush TX FIFO */
   while(   ((BT_UART_REG(port))->LSR & BT_UART_LSR_TX_DATA_AVAIL) 
      && !((BT_UART_REG(port))->LSR & BT_UART_LSR_TX_HALT)  );
         
#if BT_SERIAL_DUMP_REGS  
   bt_serial_dump_regs(port);    
#endif   

   /* Finally, clear status and enable interrupts on RX FIFO over a threshold */
   (BT_UART_REG(port))->uart_int_stat = BT_UART_INT_MASK;
   (BT_UART_REG(port))->uart_int_en = BT_UART_RXFIFOAFULL;
   
   return 0;
}

#if BT_SERIAL_DUMP_REGS  
void bt_serial_dump_regs(struct uart_port * port)
{ 
   printk(KERN_INFO "ptu_hc        : 0x%08x @ 0x%08x\n", (unsigned int)(BT_UART_REG(port))->ptu_hc        , (unsigned int)&(BT_UART_REG(port))->ptu_hc       );
   printk(KERN_INFO "uart_data     : 0x%08x @ 0x%08x\n", (unsigned int)(BT_UART_REG(port))->uart_data     , (unsigned int)&(BT_UART_REG(port))->uart_data    );  
   printk(KERN_INFO "uart_int_stat : 0x%08x @ 0x%08x\n", (unsigned int)(BT_UART_REG(port))->uart_int_stat , (unsigned int)&(BT_UART_REG(port))->uart_int_stat);  
   printk(KERN_INFO "uart_int_en   : 0x%08x @ 0x%08x\n", (unsigned int)(BT_UART_REG(port))->uart_int_en   , (unsigned int)&(BT_UART_REG(port))->uart_int_en  );
   printk(KERN_INFO "dhbr          : 0x%08x @ 0x%08x\n", (unsigned int)(BT_UART_REG(port))->dhbr          , (unsigned int)&(BT_UART_REG(port))->dhbr         ); 
   printk(KERN_INFO "dlbr          : 0x%08x @ 0x%08x\n", (unsigned int)(BT_UART_REG(port))->dlbr          , (unsigned int)&(BT_UART_REG(port))->dlbr         );
   printk(KERN_INFO "ab0           : 0x%08x @ 0x%08x\n", (unsigned int)(BT_UART_REG(port))->ab0           , (unsigned int)&(BT_UART_REG(port))->ab0          ); 
   printk(KERN_INFO "FCR           : 0x%08x @ 0x%08x\n", (unsigned int)(BT_UART_REG(port))->FCR           , (unsigned int)&(BT_UART_REG(port))->FCR          ); 
   printk(KERN_INFO "ab1           : 0x%08x @ 0x%08x\n", (unsigned int)(BT_UART_REG(port))->ab1           , (unsigned int)&(BT_UART_REG(port))->ab1          ); 
   printk(KERN_INFO "LCR           : 0x%08x @ 0x%08x\n", (unsigned int)(BT_UART_REG(port))->LCR           , (unsigned int)&(BT_UART_REG(port))->LCR          ); 
   printk(KERN_INFO "MCR           : 0x%08x @ 0x%08x\n", (unsigned int)(BT_UART_REG(port))->MCR           , (unsigned int)&(BT_UART_REG(port))->MCR          );
   printk(KERN_INFO "LSR           : 0x%08x @ 0x%08x\n", (unsigned int)(BT_UART_REG(port))->LSR           , (unsigned int)&(BT_UART_REG(port))->LSR          );
   printk(KERN_INFO "MSR           : 0x%08x @ 0x%08x\n", (unsigned int)(BT_UART_REG(port))->MSR           , (unsigned int)&(BT_UART_REG(port))->MSR          );
   printk(KERN_INFO "RFL           : 0x%08x @ 0x%08x\n", (unsigned int)(BT_UART_REG(port))->RFL           , (unsigned int)&(BT_UART_REG(port))->RFL          ); 
   printk(KERN_INFO "TFL           : 0x%08x @ 0x%08x\n", (unsigned int)(BT_UART_REG(port))->TFL           , (unsigned int)&(BT_UART_REG(port))->TFL          ); 
   printk(KERN_INFO "RFC           : 0x%08x @ 0x%08x\n", (unsigned int)(BT_UART_REG(port))->RFC           , (unsigned int)&(BT_UART_REG(port))->RFC          );
   printk(KERN_INFO "ESC           : 0x%08x @ 0x%08x\n", (unsigned int)(BT_UART_REG(port))->ESC           , (unsigned int)&(BT_UART_REG(port))->ESC          ); 
   printk(KERN_INFO "HOPKT_LEN     : 0x%08x @ 0x%08x\n", (unsigned int)(BT_UART_REG(port))->HOPKT_LEN     , (unsigned int)&(BT_UART_REG(port))->HOPKT_LEN    ); 
   printk(KERN_INFO "HIPKT_LEN     : 0x%08x @ 0x%08x\n", (unsigned int)(BT_UART_REG(port))->HIPKT_LEN     , (unsigned int)&(BT_UART_REG(port))->HIPKT_LEN    ); 
   printk(KERN_INFO "HO_DMA_CTL    : 0x%08x @ 0x%08x\n", (unsigned int)(BT_UART_REG(port))->HO_DMA_CTL    , (unsigned int)&(BT_UART_REG(port))->HO_DMA_CTL   ); 
   printk(KERN_INFO "HI_DMA_CTL    : 0x%08x @ 0x%08x\n", (unsigned int)(BT_UART_REG(port))->HI_DMA_CTL    , (unsigned int)&(BT_UART_REG(port))->HI_DMA_CTL   ); 
   printk(KERN_INFO "HO_BSIZE      : 0x%08x @ 0x%08x\n", (unsigned int)(BT_UART_REG(port))->HO_BSIZE      , (unsigned int)&(BT_UART_REG(port))->HO_BSIZE     ); 
   printk(KERN_INFO "HI_BSIZE      : 0x%08x @ 0x%08x\n", (unsigned int)(BT_UART_REG(port))->HI_BSIZE      , (unsigned int)&(BT_UART_REG(port))->HI_BSIZE     );    
}
#endif   

/*
 * RX DMA complete callback - Copy data to flip buffer
 */
static int rx_dma_comp_cb( struct uart_port * port, char * buff_address, unsigned int length )
{
   int i = 0;
   struct tty_struct *tty = port->state->port.tty;    
   unsigned char flag = TTY_NORMAL;
      
   /* Copy data */
   for( i=0; i< length; i++ )
   {     
      tty_insert_flip_char(tty, buff_address[i], flag);
   }
   
   /* Free DMA buffer */
   kfree(buff_address);    

   /* Send buffer to tty */
   tty_flip_buffer_push(tty);
   
   /* Disable RX DMA */
   (BT_UART_REG(port))->HO_DMA_CTL &=  ~(BT_UART_DMA_CTL_BURSTMODE_EN 
                                       | BT_UART_DMA_CTL_AFMODE_EN 
                                       | BT_UART_DMA_CTL_FASTMODE_EN
                                       | BT_UART_DMA_CTL_DMA_EN);          

   /* Re-enable RFL interrupt */
   (BT_UART_REG(port))->uart_int_en |= BT_UART_RXFIFOAFULL; /* FIXME: This might be too late if tasklet is delayed */   
   
   return 0;
}

/*
 * TX DMA complete callback
 */
static int tx_dma_comp_cb( struct uart_port * port, char * buff_address, unsigned int length )
{
   /* Free DMA buffer */
   kfree(buff_address); 
      
   return 0;
}

/*
 * Set data transfer mode.
 */

static void bt_serial_set_xfer_mode( BT_SERIAL_XFER_MODE mode )
{
   unsigned long flags[UART_NR];
   int i;
   unsigned int uart_active = 0;
      
   /* Disable ports */
   for (i = 0; i < UART_NR; i++) 
   {
      spin_lock_irqsave(&((&bt_serial_ports[i])->lock),flags[i]);   
      uart_active |= (BT_UART_REG(&bt_serial_ports[i]))->LCR & (BT_UART_LCR_RXEN | BT_UART_LCR_TXOEN); 
      uart_active |= (BT_UART_REG(&bt_serial_ports[i]))->MCR & BT_UART_MCR_TX_ENABLE; 
   }

   /* Only change mode if uart is not active */
   if( uart_active )
   {
      printk(KERN_ERR "%s: xfer_mode change not allowed when UART is active\n", __FUNCTION__);  
   }
   else
   {   
      /* Set xfer_mode */
      xfer_mode = mode;
   }
      
   /* Enable ports */
   for (i = 0; i < UART_NR; i++) 
   {      
      spin_unlock_irqrestore(&((&bt_serial_ports[i])->lock),flags[i]);      
   }  
}

/*
 * proc entry write handler.
 */
static ssize_t proc_write_mode(struct file *file, const char __user *buffer,
                                  unsigned long count, void *data)
{
   char mode;
      
   printk(KERN_INFO "%s: count: %d\n", __FUNCTION__, (int)count);
   
   copy_from_user(&mode, buffer, 1);
   mode -= '0';
   
   if( mode < BT_SER_XFER_MODE_MAX && mode >= BT_SER_XFER_MODE_PIO )
   {
      bt_serial_set_xfer_mode( (BT_SERIAL_XFER_MODE)mode);
   }     

   return count;
}

/*
 * proc entry read handler.
 */
static int proc_read_mode(char *buf, char **start, off_t offset, 
                          int len, int *unused_i, void *unused_v)
{
   int outlen = 0;   
   int i;
   
   len = sprintf(buf,"Current Transfer Mode: %d - %s\n", xfer_mode, xfer_mode_map[xfer_mode].mode_desc);
   buf += len;
   outlen += len;
   
   len = sprintf(buf,"Available Modes:\n");
   buf += len;
   outlen += len;
   
   for( i=BT_SER_XFER_MODE_PIO; i< BT_SER_XFER_MODE_MAX; i++ )
   {
      len = sprintf(buf,"%d - %s\n", i, xfer_mode_map[i].mode_desc );
      buf += len;
      outlen += len;
   }
         
   return outlen;
}

/*
 * Create proc entry
 */
static int bt_serial_create_proc_entries( void )
{
   int ret;
   
   proc_mkdir(BT_SERIAL_PROC_ENTRY_ROOT, NULL);
   
   xfer_mode_entry = create_proc_entry(BT_SERIAL_PROC_ENTRY, 0666, NULL);  
   if(xfer_mode_entry != NULL) 
   {
      xfer_mode_entry->read_proc = proc_read_mode;
      xfer_mode_entry->write_proc = proc_write_mode;  
   }  
   else
   {
      ret = -ENOMEM;    
   }
   
   return ret;
}

/*
 * Remove proc entry
 */
static int bt_serial_remove_proc_entries( void )
{
   remove_proc_entry(BT_SERIAL_PROC_ENTRY, NULL);
   return 0;   
}

/*
 * bt_uart module init
 */
static int __init bt_serial_init(void)
{
   int ret;
   int i;
   
   printk(KERN_INFO "Bluetooth Serial: Driver $Revision: 1.00 $\n");
   
   /* Register driver with serial core */
   ret = uart_register_driver(&bt_serial_reg);
   
   if( ret < 0 )
      goto out;      
      
   /* Initialize PL081 based DMA */
   ret = bt_serial_dma_init( rx_dma_comp_cb, tx_dma_comp_cb);      

   if( ret < 0 )
      goto out;         
   
   for (i = 0; i < UART_NR; i++) 
   {
      /* Register port with serial core */
      ret = uart_add_one_port(&bt_serial_reg, &bt_serial_ports[i]);
      
      if( ret < 0 )
         goto out;        
          
#if defined(CONFIG_ARM)
      /* for ARM it will always rearm!! */
      ret = BcmHalMapInterruptEx((FN_HANDLER)bt_serial_int,
                             (unsigned int)&bt_serial_ports[i], 
                             ((struct uart_port *)&bt_serial_ports[i])->irq,
                             "bt_serial", INTR_REARM_YES,
                             INTR_AFFINITY_TP1_IF_POSSIBLE);
#else
      ret = BcmHalMapInterruptEx((FN_HANDLER)bt_serial_int,
                             (unsigned int)&bt_serial_ports[i],
                             ((struct uart_port *)&bt_serial_ports[i])->irq,
                             "bt_serial", INTR_REARM_NO,
                             INTR_AFFINITY_TP1_IF_POSSIBLE);
#endif    
      if (ret != 0)
      {
         printk(KERN_WARNING "init_bt_serial_port: failed to register "
                             "intr %d rv=%d\n", ((struct uart_port *)&bt_serial_ports[i])->irq, ret);
         goto out;      
      }
      else
      {
         BT_SER_DEBUG(KERN_INFO "init_bt_serial_port: Successfully registered " 
                             "intr %d \n", ((struct uart_port *)&bt_serial_ports[i])->irq);              
      }
   }
   
   /* Create proc entries */
   bt_serial_create_proc_entries();
   
   goto init_ok;     
      
out:
   uart_unregister_driver(&bt_serial_reg);
init_ok:       
   return ret;
}

/*
 * De-initialize serial port registers
 */
static void deinit_bt_serial_port(struct uart_port * port )
{
   /* Disable and clear interrupts */
   (BT_UART_REG(port))->uart_int_en = 0;   
   (BT_UART_REG(port))->uart_int_stat = BT_UART_INT_MASK;
   
   /* Stop all DMAs */
   (BT_UART_REG(port))->HI_DMA_CTL = 0;
   (BT_UART_REG(port))->HO_DMA_CTL = 0;   
   
   /* Config LCR - RX/TX disables */
   (BT_UART_REG(port))->LCR &= ~(BT_UART_LCR_RXEN | BT_UART_LCR_TXOEN);
   
   /* Config MCR - TX state machine disable */
   (BT_UART_REG(port))->MCR &= ~(BT_UART_MCR_LOOPBACK | BT_UART_MCR_TX_ENABLE);
   
}

/*
 * bt_uart module de-init
 */
static void __exit bt_serial_exit(void)
{
   int i;   
   
   for (i = 0; i < UART_NR; i++) 
   {    
#if !defined(CONFIG_ARM)
      BcmHalInterruptDisable((unsigned int)((struct uart_port *)&bt_serial_ports[i])->irq);
#endif
      free_irq( ((struct uart_port *)&bt_serial_ports[i])->irq, &bt_serial_ports[i]);     
   }      
   /* Init PL081 DMA settings */
   bt_serial_dma_deinit();
      
   /* Unregister with serial core */
   uart_unregister_driver(&bt_serial_reg);
   
   /* Remove proc entries */
   bt_serial_remove_proc_entries();

}

module_init(bt_serial_init);
module_exit(bt_serial_exit);

MODULE_DESCRIPTION("BCM63XX serial port driver $Revision: 3.00 $");
MODULE_LICENSE("GPL");


