/*
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
*/
#ifndef _RDPA_CPU_HELPER_H
#define _RDPA_CPU_HELPER_H

#include "rdp_cpu_ring_defs.h"

#define SIZE_OF_RING_DESCRIPTOR sizeof(CPU_RX_DESCRIPTOR)

#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
#define RUNNER_SOURCE_PORT_PCI  19
#else
#define RUNNER_SOURCE_PORT_PCI  8
#endif

extern rdpa_if map_rdd_to_rdpa_if[];

#ifdef CM3390
#include "rdd_data_structure_auto.h"
#include "rdd_project_defs.h"
typedef void (*cpu_tx_dir_queue)(const rdpa_cpu_tx_info_t *info, void *raw_desc);

static void cpu_tx_ds_forward(const rdpa_cpu_tx_info_t *info, void *raw_desc)
{
    RDD_CPU_TX_DS_FORWARD_DESCRIPTOR_DTS *rnr_desc = (RDD_CPU_TX_DS_FORWARD_DESCRIPTOR_DTS *)raw_desc;

    rnr_desc->valid = 1;
    rnr_desc->command = 1;
    rnr_desc->src_bridge_port = info->port == rdpa_if_wan0 ? RDD_WAN0_VPORT : RDD_WAN1_VPORT;
    rnr_desc->payload_offset = info->data_offset;
    rnr_desc->packet_length = info->size;
    rnr_desc->ih_class = info->port == rdpa_if_wan0 ? 8 : 9;
    rnr_desc->buffer_number = (info->data >> 12) & 0xffff;
    rnr_desc->ddr = (info->data >> 29) & 0x1;
}
static void cpu_tx_us_forward(const rdpa_cpu_tx_info_t *info, void *raw_desc)
{
    RDD_CPU_TX_US_FORWARD_DESCRIPTOR_DTS *rnr_desc = (RDD_CPU_TX_US_FORWARD_DESCRIPTOR_DTS *)raw_desc;

    rnr_desc->valid = 1;
    rnr_desc->command = 1;

    /* TODO:Convert switch to map */
    if (info->port >= rdpa_if_ssid0  && info->port <= rdpa_if_ssid15)
    {
        rnr_desc->src_bridge_port = RDD_PCI_VPORT;
        rnr_desc->ssid = info->port - rdpa_if_ssid0;
        rnr_desc->ih_class = 2;
    }
    else
    {
        rnr_desc->src_bridge_port = info->port - rdpa_if_lan0 + RDD_VPORT_ID_1;
        rnr_desc->ih_class = info->port - rdpa_if_lan0 + 10;
    }
    rnr_desc->payload_offset = info->data_offset;
    rnr_desc->packet_length = info->size;
    rnr_desc->buffer_number = (info->data >> 12) & 0xffff;
    rnr_desc->ddr = (info->data >> 29) & 0x1;
}

static void cpu_tx_ds_egress(const rdpa_cpu_tx_info_t *info, void *raw_desc)
{
    RDD_CPU_TX_DS_EGRESS_DESCRIPTOR_DTS *rnr_desc = (RDD_CPU_TX_DS_EGRESS_DESCRIPTOR_DTS *)raw_desc;

    rnr_desc->valid = 1;
    rnr_desc->command = 1;
    if (info->port >= rdpa_if_ssid0  && info->port <= rdpa_if_ssid15)
    {
        rnr_desc->emac = RDD_PCI_VPORT;
        rnr_desc->ssid = info->port - rdpa_if_ssid0;
    }
    else
    {
        rnr_desc->emac = info->port - rdpa_if_lan0 + RDD_LAN0_VPORT;
    }
    rnr_desc->tx_queue = queue_id;
    rnr_desc->src_bridge_port = RDD_CPU_VPORT;
    rnr_desc->payload_offset = info->data_offset;
    rnr_desc->packet_length = info->size;
    rnr_desc->ih_class = info->port == rdpa_if_wan0 ? 8 : 9;
    rnr_desc->buffer_number = (info->data >> 12) & 0xffff;
    rnr_desc->ddr = (info->data >> 29) & 0x1;
}

static void cpu_tx_us_egress(const rdpa_cpu_tx_info_t *info, void *raw_desc)
{
    RDD_CPU_TX_US_EGRESS_DESCRIPTOR_DTS *rnr_desc = (RDD_CPU_TX_US_EGRESS_DESCRIPTOR_DTS *)raw_desc;
    rnr_desc->valid = 1;
    rnr_desc->command = 1;
    rnr_desc->wan_flow = info->flow;
    rnr_desc->tx_queue = info->queue_id;
}

static cpu_tx_dir_queue cpu_tx_array[2][2] =
{
    {cpu_tx_ds_egress, cpu_tx_us_egress},
    {cpu_tx_ds_forward, cpu_tx_us_forward}
};

inline void rdpa_cpu_tx_pd_set(const rdpa_cpu_tx_info_t *info, void *raw_desc)
{
    int dir = info->port == rdpa_if_wan0 | info->port == rdpa_if_wan1;
    cpu_tx_array[info->method][dir](info, raw_desc);
}
#endif

static inline rdpa_if rdpa_cpu_rx_srcport_to_rdpa_if(uint16_t rdd_srcport, int flow_id)
{
#ifndef BRCM_FTTDP
    /* Special case for wifi packets: if src_port is PCI then need to set
     * SSID */
    return (rdd_srcport == RUNNER_SOURCE_PORT_PCI) ? rdpa_if_ssid0 +
        flow_id : map_rdd_to_rdpa_if[rdd_srcport];
#else
    switch (rdd_srcport)
    {
    case 0:
        return rdpa_if_wan0;
    /* .. upto number-of-lan-ifs + 1 */
    case 1 ... rdpa_if_lan_max - rdpa_if_lan0 + 1 + 1:
        return rdpa_if_lan0 + rdd_srcport - 1;
    default:
        return rdpa_if_none;
    }
#endif
}

inline int rdpa_cpu_rx_pd_get(void *raw_desc /* Input */, rdpa_cpu_rx_info_t *rx_pd/* Output */)

{
    CPU_RX_DESCRIPTOR *p_desc = (CPU_RX_DESCRIPTOR *)raw_desc;
    CPU_RX_DESCRIPTOR rx_desc = {};


    /* p_desc is in uncached mem so reading 32bits at a time into
     cached mem improves performance will be change to BurstBank read later*/
    rx_desc.word2 = p_desc->word2;
    if (rx_desc.ownership == OWNERSHIP_HOST)
    {
        rx_pd->data = (uint32_t)PHYS_TO_CACHED(rx_desc.word2);

        rx_desc.word0 = p_desc->word0;
        rx_pd->size = rx_desc.packet_length;
        rx_pd->src_port = rdpa_cpu_rx_srcport_to_rdpa_if(rx_desc.source_port,
                        rx_desc.flow_id);
        rx_pd->reason_data = rx_desc.flow_id;

        rx_desc.word1 = p_desc->word1;
        rx_pd->reason = (rdpa_cpu_reason) rx_desc.reason;
        rx_pd->dest_ssid = rx_desc.dst_ssid;
        rx_desc.word3 = p_desc->word3;
        rx_pd->wl_metadata = rx_desc.wl_metadata;
        rx_pd->ptp_index = p_desc->ip_sync_1588_idx;

        return 0;
    }

    return BDMF_ERR_NO_MORE;
}

inline void rdpa_cpu_ring_rest_desc(volatile void *raw_desc, void *data)
{
    volatile CPU_RX_DESCRIPTOR *p_desc = (volatile CPU_RX_DESCRIPTOR *)raw_desc;

    /* assign the buffer address to ring and set the ownership to runner
     * by clearing  bit 31 which is used as ownership flag */
    p_desc->word2 = swap4bytes(((VIRT_TO_PHYS(data)) & 0x7fffffff));
}

inline int rdpa_cpu_ring_not_empty(const void *raw_desc)
{
    CPU_RX_DESCRIPTOR *p_desc = (CPU_RX_DESCRIPTOR *)raw_desc;

#if defined(CONFIG_BCM963138) || defined(_BCM963138_) || defined(CONFIG_BCM963148) || defined(_BCM963148_)
        unsigned long *pul1;

        pul1 = ((unsigned long *)p_desc) + 2;
        return *pul1 & 0x00000080;
#else
    return (p_desc->ownership == OWNERSHIP_HOST);
#endif
}
#endif
