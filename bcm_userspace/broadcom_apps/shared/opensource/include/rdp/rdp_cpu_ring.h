/*
   Copyright (c) 2013 Broadcom Corporation
   All Rights Reserved

    <:label-BRCM:2013:DUAL/GPL:standard

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

/******************************************************************************/
/*                                                                            */
/* File Description:                                                          */
/*                                                                            */
/* This file contains the implementation of the Runner CPU ring interface     */
/*                                                                            */
/******************************************************************************/

#ifndef _RDP_CPU_RING_H_
#define _RDP_CPU_RING_H_

#ifndef RDP_SIM

#if defined(__KERNEL__) || defined(_CFE_)

/*****************************************************************************/
/*                                                                           */
/* Include files                                                             */
/*                                                                           */
/*****************************************************************************/
#include<bcm_pkt_lengths.h>

#ifdef _CFE_
#include "bl_os_wraper.h"
#endif
#include "rdpa_types.h"
#include "rdpa_cpu_basic.h"
#include "rdd.h"
#include <bcm_mm.h>
#include "rdp_cpu_ring_defs.h"

/*****************************************************************************/
/*                                                                           */
/* defines and structures                                                    */
/*                                                                           */
/*****************************************************************************/


#ifdef _CFE_

#include "lib_malloc.h"
#include "cfe_iocb.h"
#define RDP_CPU_RING_MAX_QUEUES	1
#define RDP_WLAN_MAX_QUEUES		0

#elif defined(__KERNEL__)

#include "rdpa_cpu.h"
#include "bdmf_system.h"
#include "bdmf_shell.h"
#include "bdmf_dev.h"

#define RDP_CPU_RING_MAX_QUEUES		RDPA_CPU_MAX_QUEUES
#define RDP_WLAN_MAX_QUEUES			RDPA_WLAN_MAX_QUEUES

extern const bdmf_attr_enum_table_t rdpa_cpu_reason_enum_table;

#endif

typedef enum
{
	sysb_type_skb,
	sysb_type_fkb,
	sysb_type_raw,
}cpu_ring_sysb_type;

typedef enum
{
	type_cpu_rx,
	type_pci_tx
}cpu_ring_type;


typedef struct
{
	uint8_t*                        data_ptr;
	uint16_t                        packet_size;
	uint16_t                        flow_id;
	uint16_t						reason;
	uint16_t					    src_bridge_port;
    uint16_t                        dst_ssid;
    uint16_t						wl_metadata; 
    uint16_t                        ptp_index;    
}
CPU_RX_PARAMS;

typedef void* (sysbuf_alloc_cb)(cpu_ring_sysb_type, uint32_t *pDataPtr);
typedef void (sysbuf_free_cb)(void* packetPtr);

typedef void* t_sysb_ptr;


typedef union
{
	CPU_RX_DESCRIPTOR cpu_rx;
	//PCI_TX_DESCRIPTOR pci_tx; not ready yet
}
RING_DESC_UNION;

#define MAX_BUFS_IN_CACHE 32 
typedef struct
{
	uint32_t				ring_id;
	uint32_t				admin_status;
	uint32_t				num_of_entries;
	uint32_t				size_of_entry;
	uint32_t				packet_size;
	cpu_ring_sysb_type		buff_type;
	RING_DESC_UNION*		head;
	RING_DESC_UNION*		base;
	RING_DESC_UNION*		end;
	uint32_t               buff_cache_cnt;
	uint32_t*              buff_cache;
}
RING_DESCTIPTOR;

/*array of possible rings private data*/
#define D_NUM_OF_RING_DESCRIPTORS (RDP_CPU_RING_MAX_QUEUES + RDP_WLAN_MAX_QUEUES)


int rdp_cpu_ring_create_ring(uint32_t ringId,uint32_t entries, cpu_ring_sysb_type buff_type,
                            uint32_t*  ring_head);

int rdp_cpu_ring_delete_ring(uint32_t ringId);

int rdp_cpu_ring_read_packet_copy(uint32_t ringId, CPU_RX_PARAMS* rxParams);

int rdp_cpu_ring_get_queue_size(uint32_t ringId);

int rdp_cpu_ring_get_queued(uint32_t ringId);

int rdp_cpu_ring_flush(uint32_t ringId);

int rdp_cpu_ring_not_empty(uint32_t ringId);

#endif /* if defined(__KERNEL__) || defined(_CFE_) */

#else
#include "rdp_cpu_ring_sim.h"
#endif /* RDP_SIM */

#endif /* _RDP_CPU_RING_H_ */
