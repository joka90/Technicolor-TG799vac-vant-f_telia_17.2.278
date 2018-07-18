/*
    Copyright 2004-2010 Broadcom Corporation

    <:label-BRCM:2011:DUAL/GPL:standard
    
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

#ifndef _BCMSW_DMA_H_
#define _BCMSW_DMA_H_

#include <bcm/bcmswapitypes.h>
#include <bcm/bcmswapistat.h>

/****************************************************************************
    Prototypes
****************************************************************************/
int  ethsw_save_port_state(void);
int  ethsw_restore_port_state(void);
void bcmeapi_ethsw_dump_page(int page);
int  bcmeapi_ethsw_dump_mib(int port, int type, int queue);
int  bcmeapi_ioctl_ethsw_port_tagreplace(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_port_tagmangle(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_port_tagmangle_matchvid(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_port_tagstrip(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_port_pause_capability(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_control(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_prio_control(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_vlan(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_pbvlan(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_port_irc_set(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_port_irc_get(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_port_erc_set(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_port_erc_get(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_cosq_config(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_cosq_sched(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_cosq_port_mapping(struct ethswctl_data *e);
void enet_park_rxdma_channel(int parkChan, int unpark);
void bcmeapi_ethsw_cosq_rxchannel_mapping(int *channel, int queue, int set);
int  bcmeapi_ioctl_ethsw_cosq_rxchannel_mapping(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_cosq_txchannel_mapping(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_cosq_txq_sel(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_clear_port_stats(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_clear_stats(uint32_t port_map);
int  bcmeapi_ioctl_ethsw_counter_get(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_port_default_tag_config(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_port_traffic_control(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_port_loopback(struct ethswctl_data *e, int phy_id);
int  bcmeapi_ioctl_ethsw_phy_mode(struct ethswctl_data *e, int phy_id);
int  bcmeapi_ioctl_ethsw_pkt_padding(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_port_jumbo_control(struct ethswctl_data *e); // bill
void fast_age_all(uint8_t age_static);
int bcmeapi_ioctl_ethsw_dscp_to_priority_mapping(struct ethswctl_data *e);
int bcmeapi_ioctl_ethsw_pcp_to_priority_mapping(struct ethswctl_data *e);
void ethsw_set_wanoe_portmap(uint16 wan_port_map);
int bcmeapi_ethsw_init(void);
int bcmeapi_ethsw_reinit(void);
void bcmeapi_ethsw_init_ports(void);
void bcmeapi_ethsw_init_hw(int unit, uint32_t map, int wanPort);
void bcmeapi_set_multiport_address(uint8_t* addr);
int ethsw_set_mac_hw2(int sw_port, int link, int speed, int duplex);
#define ethsw_set_mac_hw(sw_port, ps) ethsw_set_mac_hw2(sw_port, ps.lnk, ps.spd1000? 1000: (ps.spd100? 100:10), ps.fdx)
uint16_t ethsw_get_pbvlan(int port);
void ethsw_set_pbvlan(int port, uint16_t fwdMap);

int reset_switch(void);


#if (defined(CONFIG_BCM_ARL) || defined(CONFIG_BCM_ARL_MODULE))
int enet_hook_for_arl_access(void *ethswctl);
#endif

int bcmeapi_ioctl_debug_conf(struct ethswctl_data *e);
int write_vlan_table(bcm_vlan_t vid, uint32_t val32);
void enet_arl_write(uint8_t *mac, uint16_t vid, int val);
void bcmeapi_ethsw_set_stp_mode(unsigned int unit, unsigned int port, unsigned char stpState);
#ifdef REPORT_HARDWARE_STATS
int ethsw_get_hw_stats(int port, struct net_device_stats *stats);
#endif

int enet_arl_read( uint8_t *mac, uint32_t *vid, uint32_t *val );
int enet_arl_access_dump(void);
void enet_arl_dump_multiport_arl(void);
int remove_arl_entry(uint8_t *mac);
int remove_arl_entry_ext(uint8_t *mac);
int bcmeapi_init_ext_sw_if(extsw_info_t *extSwInfo);
#define bcmeapi_ioctl_ethsw_get_port_emac(e) 0 
#define bcmeapi_ioctl_ethsw_clear_port_emac(e) 0 

void ephy_adjust_afe(unsigned int phy_id);

#endif /* _BCMSW_DMA_H_ */
