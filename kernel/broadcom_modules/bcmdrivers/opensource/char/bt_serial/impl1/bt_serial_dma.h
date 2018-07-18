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

#ifndef __BT_SERIAL_DMA_H
#define __BT_SERIAL_DMA_H

#include <linux/serial_core.h>

typedef int (*dma_complete_callback)(struct uart_port * port, char * buff_address, unsigned int length);
int bt_serial_dma_init( dma_complete_callback rx_callback, dma_complete_callback tx_callback );
int bt_serial_dma_deinit(void);
int bt_serial_issue_rx_dma_request( struct uart_port * port, char * rx_buff, unsigned int hdr_length, unsigned int payload_length );
int bt_serial_issue_tx_dma_request( struct uart_port * port, char * tx_buff, unsigned int length );
void bt_serial_dma_halt_tx( struct uart_port * port );
void bt_serial_dma_halt_rx( struct uart_port * port );

#endif