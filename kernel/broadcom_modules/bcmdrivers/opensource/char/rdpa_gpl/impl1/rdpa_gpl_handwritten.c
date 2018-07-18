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

/*
 * CPU interface
 */
#include <bdmf_interface.h>
#include <rdpa_types.h>

/* This map is to provide rdd bridge source port to rdpa_if */
#ifdef DSL
rdpa_if map_rdd_to_rdpa_if[] = {rdpa_if_wan0, rdpa_if_wan1, rdpa_if_lan0, rdpa_if_lan1,
    rdpa_if_lan2, rdpa_if_lan3, rdpa_if_lan4, rdpa_if_lan5,
    rdpa_if_lan6, rdpa_if_lan7, rdpa_if_switch, rdpa_if_none,
    rdpa_if_cpu, rdpa_if_none, rdpa_if_none, rdpa_if_none,
    rdpa_if_none, rdpa_if_wan0, rdpa_if_wan0, rdpa_if_wlan0};
#else
#if defined(CM3390)
rdpa_if map_rdd_to_rdpa_if[] = {rdpa_if_wan0, rdpa_if_lan0, rdpa_if_lan1, rdpa_if_lan2,
    rdpa_if_lan3, rdpa_if_lan4, rdpa_if_lan5, rdpa_if_lan6,
    rdpa_if_wlan0, rdpa_if_wan1, rdpa_if_none,
    rdpa_if_none, rdpa_if_cpu};
#else
rdpa_if map_rdd_to_rdpa_if[] = {rdpa_if_wan0, rdpa_if_lan0, rdpa_if_lan1, rdpa_if_lan2,
    rdpa_if_lan3, rdpa_if_lan4, rdpa_if_wan0, rdpa_if_wan0,
    rdpa_if_wlan0, rdpa_if_switch, rdpa_if_none,
    rdpa_if_none, rdpa_if_cpu};
#endif
#endif
EXPORT_SYMBOL(map_rdd_to_rdpa_if);

/** Enable CPU queue interrupt */
void (*f_rdpa_cpu_int_enable)(rdpa_cpu_port port, int queue);
EXPORT_SYMBOL(f_rdpa_cpu_int_enable);
void rdpa_cpu_int_enable(rdpa_cpu_port port, int queue)
{
    if (!f_rdpa_cpu_int_enable)
    {
        BDMF_TRACE_ERR("rdpa.ko is not loaded\n");
        return;
    }
    f_rdpa_cpu_int_enable(port, queue);
}
EXPORT_SYMBOL(rdpa_cpu_int_enable);

/** Disable CPU queue interrupt */
void (*f_rdpa_cpu_int_disable)(rdpa_cpu_port port, int queue);
EXPORT_SYMBOL(f_rdpa_cpu_int_disable);
void rdpa_cpu_int_disable(rdpa_cpu_port port, int queue)
{
    if (!f_rdpa_cpu_int_disable)
    {
        BDMF_TRACE_ERR("rdpa.ko is not loaded\n");
        return;
    }
    f_rdpa_cpu_int_disable(port, queue);
}
EXPORT_SYMBOL(rdpa_cpu_int_disable);

/** Clear CPU queue interrupt */
void (*f_rdpa_cpu_int_clear)(rdpa_cpu_port port, int queue);
EXPORT_SYMBOL(f_rdpa_cpu_int_clear);
void rdpa_cpu_int_clear(rdpa_cpu_port port, int queue)
{
    if (!f_rdpa_cpu_int_clear)
    {
        BDMF_TRACE_ERR("rdpa.ko is not loaded\n");
        return;
    }
    f_rdpa_cpu_int_clear(port, queue);
}
EXPORT_SYMBOL(rdpa_cpu_int_clear);

#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
/** Pull a single received packet from host queue. */
int (*f_rdpa_cpu_packet_get)(rdpa_cpu_port port, bdmf_index queue,
    bdmf_sysb *sysb, rdpa_cpu_rx_info_t *info);
EXPORT_SYMBOL(f_rdpa_cpu_packet_get);
int rdpa_cpu_packet_get(rdpa_cpu_port port, bdmf_index queue,
    bdmf_sysb *sysb, rdpa_cpu_rx_info_t *info)
{
    if (!f_rdpa_cpu_packet_get)
    {
        BDMF_TRACE_ERR("rdpa.ko is not loaded\n");
        return BDMF_ERR_STATE;
    }
    return f_rdpa_cpu_packet_get(port, queue, sysb, info);
}
EXPORT_SYMBOL(rdpa_cpu_packet_get);
#else
/** Pull a single received packet from host queue. */
int (*f_rdpa_cpu_packet_get)(rdpa_cpu_port port, bdmf_index queue,
    rdpa_cpu_rx_info_t *info);
EXPORT_SYMBOL(f_rdpa_cpu_packet_get);
int rdpa_cpu_packet_get(rdpa_cpu_port port, bdmf_index queue,
    rdpa_cpu_rx_info_t *info)
{
    if (!f_rdpa_cpu_packet_get)
    {
        BDMF_TRACE_ERR("rdpa.ko is not loaded\n");
        return BDMF_ERR_STATE;
    }
    return f_rdpa_cpu_packet_get(port, queue, info);
}
EXPORT_SYMBOL(rdpa_cpu_packet_get);
#endif

/** Get the Time Of Day from the FW FIFO, by the ptp index */
int (*f_rdpa_cpu_ptp_1588_get_tod)(uint16_t ptp_index, uint32_t *tod_h,
    uint32_t *tod_l, uint16_t *local_counter_delta);
EXPORT_SYMBOL(f_rdpa_cpu_ptp_1588_get_tod);
int rdpa_cpu_ptp_1588_get_tod(uint16_t ptp_index, uint32_t *tod_h,
    uint32_t *tod_l, uint16_t *local_counter_delta)
{
    if (!f_rdpa_cpu_ptp_1588_get_tod)
    {
        BDMF_TRACE_ERR("rdpa.ko is not loaded\n");
        return BDMF_ERR_STATE;
    }
    return f_rdpa_cpu_ptp_1588_get_tod(ptp_index, tod_h, tod_l, local_counter_delta);
}
EXPORT_SYMBOL(rdpa_cpu_ptp_1588_get_tod);

/** similar to rdpa_cpu_send_sysb, but treats only ptp-1588 packets */
int (*f_rdpa_cpu_send_sysb_ptp)(bdmf_sysb sysb, const rdpa_cpu_tx_info_t *info);
EXPORT_SYMBOL(f_rdpa_cpu_send_sysb_ptp);
int rdpa_cpu_send_sysb_ptp(bdmf_sysb sysb, const rdpa_cpu_tx_info_t *info)
{
    if (!f_rdpa_cpu_send_sysb_ptp)
    {
        BDMF_TRACE_ERR("rdpa.ko is not loaded\n");
        return BDMF_ERR_STATE;
    }
    return f_rdpa_cpu_send_sysb_ptp(sysb, info);
}
EXPORT_SYMBOL(rdpa_cpu_send_sysb_ptp);

/** Send system buffer */
int (*f_rdpa_cpu_send_sysb)(bdmf_sysb sysb, const rdpa_cpu_tx_info_t *info);
EXPORT_SYMBOL(f_rdpa_cpu_send_sysb);
int rdpa_cpu_send_sysb(bdmf_sysb sysb, const rdpa_cpu_tx_info_t *info)
{
    if (!f_rdpa_cpu_send_sysb)
    {
        BDMF_TRACE_ERR("rdpa.ko is not loaded\n");
        return BDMF_ERR_STATE;
    }
    return f_rdpa_cpu_send_sysb(sysb, info);
}
EXPORT_SYMBOL(rdpa_cpu_send_sysb);

/** Send system buffer from WFD*/
int (*f_rdpa_cpu_send_wfd_to_bridge)(bdmf_sysb sysb, const rdpa_cpu_tx_info_t *info);
EXPORT_SYMBOL(f_rdpa_cpu_send_wfd_to_bridge);

int rdpa_cpu_send_wfd_to_bridge(bdmf_sysb sysb, const rdpa_cpu_tx_info_t *info)
{
    if (unlikely(!f_rdpa_cpu_send_wfd_to_bridge))
    {
        BDMF_TRACE_ERR("rdpa.ko is not loaded\n");
        return BDMF_ERR_STATE;
    }
    return f_rdpa_cpu_send_wfd_to_bridge(sysb, info);
}
EXPORT_SYMBOL(rdpa_cpu_send_wfd_to_bridge);

#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
/** Send system buffer to WAN */
int (*f_rdpa_cpu_tx_port_enet_wan)(bdmf_sysb sysb, uint32_t egress_queue);
EXPORT_SYMBOL(f_rdpa_cpu_tx_port_enet_wan);
int rdpa_cpu_tx_port_enet_wan(bdmf_sysb sysb, uint32_t egress_queue)
{
    return f_rdpa_cpu_tx_port_enet_wan(sysb, egress_queue);
}
EXPORT_SYMBOL(rdpa_cpu_tx_port_enet_wan);

/** Send system buffer to LAN */
int (*f_rdpa_cpu_tx_port_enet_lan)(bdmf_sysb sysb, uint32_t egress_queue, uint32_t phys_port);
EXPORT_SYMBOL(f_rdpa_cpu_tx_port_enet_lan);
int rdpa_cpu_tx_port_enet_lan(bdmf_sysb sysb, uint32_t egress_queue, uint32_t phys_port)
{
    return f_rdpa_cpu_tx_port_enet_lan(sysb, egress_queue, phys_port);
}
EXPORT_SYMBOL(rdpa_cpu_tx_port_enet_lan);

/** Send system buffer to Flow Cache offload */
int (*f_rdpa_cpu_tx_flow_cache_offload)(bdmf_sysb sysb, uint32_t cpu_rx_queue, int dirty);
EXPORT_SYMBOL(f_rdpa_cpu_tx_flow_cache_offload);
int rdpa_cpu_tx_flow_cache_offload(bdmf_sysb sysb, uint32_t cpu_rx_queue, int dirty)
{
    return f_rdpa_cpu_tx_flow_cache_offload(sysb, cpu_rx_queue, dirty);
}
EXPORT_SYMBOL(rdpa_cpu_tx_flow_cache_offload);

/** Frees the given free index and returns a pointer to the associated System Buffer */
bdmf_sysb (*f_rdpa_cpu_return_free_index)(uint16_t free_index);
EXPORT_SYMBOL(f_rdpa_cpu_return_free_index);
bdmf_sysb rdpa_cpu_return_free_index(uint16_t free_index)
{
    return f_rdpa_cpu_return_free_index(free_index);
}
EXPORT_SYMBOL(rdpa_cpu_return_free_index);

/** Receive Ethernet system buffer */
int (*f_rdpa_cpu_host_packet_get_enet)(bdmf_index queue, bdmf_sysb *sysb, rdpa_if *src_port);
EXPORT_SYMBOL(f_rdpa_cpu_host_packet_get_enet);
int rdpa_cpu_host_packet_get_enet(bdmf_index queue, bdmf_sysb *sysb, rdpa_if *src_port)
{
    return f_rdpa_cpu_host_packet_get_enet(queue, sysb, src_port);
}
EXPORT_SYMBOL(rdpa_cpu_host_packet_get_enet);

void (*f_rdpa_cpu_tx_reclaim)(void);
EXPORT_SYMBOL(f_rdpa_cpu_tx_reclaim);
void rdpa_cpu_tx_reclaim(void)
{
    f_rdpa_cpu_tx_reclaim();
}
EXPORT_SYMBOL(rdpa_cpu_tx_reclaim);
#endif

/** Receive bulk ethernet system buffers for WFD */
int (*f_rdpa_cpu_wfd_bulk_fkb_get)(bdmf_index queue_id, unsigned int budget, void **rx_pkts, void *wfd_acc_info_p);
EXPORT_SYMBOL(f_rdpa_cpu_wfd_bulk_fkb_get);
int rdpa_cpu_wfd_bulk_fkb_get(bdmf_index queue_id, unsigned int budget, void **rx_pkts, void *wfd_acc_info_p)
{
    return f_rdpa_cpu_wfd_bulk_fkb_get(queue_id, budget, rx_pkts, wfd_acc_info_p);
}
EXPORT_SYMBOL(rdpa_cpu_wfd_bulk_fkb_get);

int (*f_rdpa_cpu_wfd_bulk_skb_get)(bdmf_index queue_id, unsigned int budget, void **rx_pkts, void *wfd_acc_info_p);
EXPORT_SYMBOL(f_rdpa_cpu_wfd_bulk_skb_get);
int rdpa_cpu_wfd_bulk_skb_get(bdmf_index queue_id, unsigned int budget, void **rx_pkts, void *wfd_acc_info_p)
{
    return f_rdpa_cpu_wfd_bulk_skb_get(queue_id, budget, rx_pkts, wfd_acc_info_p);
}
EXPORT_SYMBOL(rdpa_cpu_wfd_bulk_skb_get);

void *(*f_rdpa_cpu_data_get)(int rdpa_cpu_type);
EXPORT_SYMBOL(f_rdpa_cpu_data_get);
void *rdpa_cpu_data_get(int rdpa_cpu_type)
{
    return f_rdpa_cpu_data_get(rdpa_cpu_type);
}
EXPORT_SYMBOL(rdpa_cpu_data_get);

/** Send raw packet */
int (*f_rdpa_cpu_send_raw)(void *data, uint32_t length, const rdpa_cpu_tx_info_t *info);
EXPORT_SYMBOL(f_rdpa_cpu_send_raw);
int rdpa_cpu_send_raw(void *data, uint32_t length, const rdpa_cpu_tx_info_t *info)
{
    if (!f_rdpa_cpu_send_raw)
    {
        BDMF_TRACE_ERR("rdpa.ko is not loaded\n");
        return BDMF_ERR_STATE;
    }
    return f_rdpa_cpu_send_raw(data, length, info);
}
EXPORT_SYMBOL(rdpa_cpu_send_raw);

/** Map from HW port to rdpa_if */
rdpa_if (*f_rdpa_port_map_from_hw_port)(int hw_port, bdmf_boolean emac_only);
EXPORT_SYMBOL(f_rdpa_port_map_from_hw_port);
rdpa_if rdpa_port_map_from_hw_port(int hw_port, bdmf_boolean emac_only)
{
    if (!f_rdpa_port_map_from_hw_port)
    {
        BDMF_TRACE_ERR("rdpa.ko is not loaded\n");
        return rdpa_if_none;
    }
    return f_rdpa_port_map_from_hw_port(hw_port, emac_only);
}
EXPORT_SYMBOL(rdpa_port_map_from_hw_port);

int (*f_rdpa_cpu_queue_not_empty)(rdpa_cpu_port port, bdmf_index queue);
EXPORT_SYMBOL(f_rdpa_cpu_queue_not_empty);
int rdpa_cpu_queue_not_empty(rdpa_cpu_port port, bdmf_index queue)
{
    if (!f_rdpa_cpu_queue_not_empty)
    {
        BDMF_TRACE_ERR("rdpa.ko is not loaded\n");
        return BDMF_ERR_STATE;
    }
    return f_rdpa_cpu_queue_not_empty(port, queue);
}
EXPORT_SYMBOL(rdpa_cpu_queue_not_empty);

/** Send EPON Dgasp */
int (*f_rdpa_cpu_send_epon_dying_gasp)(bdmf_sysb sysb, const rdpa_cpu_tx_info_t *info);
EXPORT_SYMBOL(f_rdpa_cpu_send_epon_dying_gasp);
int rdpa_cpu_send_epon_dying_gasp(bdmf_sysb sysb, const rdpa_cpu_tx_info_t *info)
{
    if (!f_rdpa_cpu_send_epon_dying_gasp)
    {
        BDMF_TRACE_ERR("rdpa.ko is not loaded\n");
        return BDMF_ERR_STATE;
    }
    return f_rdpa_cpu_send_epon_dying_gasp(sysb, info);
}
EXPORT_SYMBOL(rdpa_cpu_send_epon_dying_gasp);

int (*f_rdpa_cpu_is_per_port_metering_supported)(rdpa_cpu_reason reason);
EXPORT_SYMBOL(f_rdpa_cpu_is_per_port_metering_supported);
int rdpa_cpu_is_per_port_metering_supported(rdpa_cpu_reason reason)
{
    if (!f_rdpa_cpu_is_per_port_metering_supported)
    {
        BDMF_TRACE_ERR("rdpa.ko is not loaded\n");
        return BDMF_ERR_STATE;
    }
    return f_rdpa_cpu_is_per_port_metering_supported(reason);
}
EXPORT_SYMBOL(rdpa_cpu_is_per_port_metering_supported);

rdpa_ports (*f_rdpa_ports_all_lan)(void);
EXPORT_SYMBOL(f_rdpa_ports_all_lan);
rdpa_ports rdpa_ports_all_lan()
{
    if (!f_rdpa_ports_all_lan)
    {
        BDMF_TRACE_ERR("rdpa.ko is not loaded\n");
        return BDMF_ERR_STATE;
    }
    return f_rdpa_ports_all_lan();
}
EXPORT_SYMBOL(rdpa_ports_all_lan);

void (*f_rdpa_cpu_rx_dump_packet)(char *name, rdpa_cpu_port port,
    bdmf_index queue, rdpa_cpu_rx_info_t *info, uint32_t dst_ssid);
EXPORT_SYMBOL(f_rdpa_cpu_rx_dump_packet);
void rdpa_cpu_rx_dump_packet(char *name, rdpa_cpu_port port,
    bdmf_index queue, rdpa_cpu_rx_info_t *info, uint32_t dst_ssid)
{
    if (!f_rdpa_cpu_rx_dump_packet)
    {
        BDMF_TRACE_ERR("rdpa.ko is not loaded\n");
        return;
    }
    f_rdpa_cpu_rx_dump_packet(name, port, queue, info, dst_ssid);
    return;
}
EXPORT_SYMBOL(rdpa_cpu_rx_dump_packet);

/** Run time mapping from HW port to rdpa_if using array */
rdpa_if (*f_rdpa_physical_port_to_rdpa_if)(rdpa_physical_port port);
EXPORT_SYMBOL(f_rdpa_physical_port_to_rdpa_if);
rdpa_if rdpa_physical_port_to_rdpa_if(rdpa_physical_port port)
{
    if (!f_rdpa_physical_port_to_rdpa_if)
    {
        BDMF_TRACE_ERR("rdpa.ko is not loaded\n");
        return rdpa_if_none;
    }
    return f_rdpa_physical_port_to_rdpa_if(port);
}
EXPORT_SYMBOL(rdpa_physical_port_to_rdpa_if);

int (*f_rdpa_egress_tm_queue_id_by_lan_port_queue)(rdpa_if port, int queue, uint32_t *queue_id);
EXPORT_SYMBOL(f_rdpa_egress_tm_queue_id_by_lan_port_queue);

int rdpa_egress_tm_queue_id_by_lan_port_queue(rdpa_if port, int queue, uint32_t *queue_id)
{
    if (!f_rdpa_egress_tm_queue_id_by_lan_port_queue)
        return BDMF_ERR_STATE;
    return f_rdpa_egress_tm_queue_id_by_lan_port_queue(port, queue, queue_id);
}
EXPORT_SYMBOL(rdpa_egress_tm_queue_id_by_lan_port_queue);


int (*f_rdpa_egress_tm_queue_id_by_wan_flow_index)(int *wan_flow, int ind, uint32_t *queue_id);
EXPORT_SYMBOL(f_rdpa_egress_tm_queue_id_by_wan_flow_index);

int rdpa_egress_tm_queue_id_by_wan_flow_index(int *wan_flow, int ind, uint32_t *queue_id)
{
    if (!f_rdpa_egress_tm_queue_id_by_wan_flow_index)
        return BDMF_ERR_STATE;
    return f_rdpa_egress_tm_queue_id_by_wan_flow_index(wan_flow, ind, queue_id);
}
EXPORT_SYMBOL(rdpa_egress_tm_queue_id_by_wan_flow_index);

int (*f_rdpa_egress_tm_cfg_queue_threshold_by_tcont_queue)(int port_id, int priority, int min_threshold,
    int max_threshold);
EXPORT_SYMBOL(f_rdpa_egress_tm_cfg_queue_threshold_by_tcont_queue);

int rdpa_egress_tm_cfg_queue_threshold_by_tcont_queue(int port_id, int priority, int min_threshold, int max_threshold)
{
    if (!f_rdpa_egress_tm_cfg_queue_threshold_by_tcont_queue)
        return BDMF_ERR_STATE;
    return f_rdpa_egress_tm_cfg_queue_threshold_by_tcont_queue(port_id, priority, min_threshold, max_threshold);
}
EXPORT_SYMBOL(rdpa_egress_tm_cfg_queue_threshold_by_tcont_queue);

int (*f_rdpa_egress_tm_cfg_queue_threshold_by_lan_port_queue)(int port_id, int priority, int min_threshold,
    int max_threshold);
EXPORT_SYMBOL(f_rdpa_egress_tm_cfg_queue_threshold_by_lan_port_queue);

int rdpa_egress_tm_cfg_queue_threshold_by_lan_port_queue(int port_id, int priority, int min_threshold,
    int max_threshold)
{
    if (!f_rdpa_egress_tm_cfg_queue_threshold_by_lan_port_queue)
        return BDMF_ERR_STATE;
    return f_rdpa_egress_tm_cfg_queue_threshold_by_lan_port_queue(port_id, priority, min_threshold, max_threshold);
}
EXPORT_SYMBOL(rdpa_egress_tm_cfg_queue_threshold_by_lan_port_queue);

