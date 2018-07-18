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

#ifndef _BCMSW_H_
#define _BCMSW_H_

#include <bcm/bcmswapitypes.h>
#include <bcm/bcmswapistat.h>
#include "bcmenet_common.h"
#if !defined(RDPA_PLATFORM)
#include "bcmsw_dma.h"
#else
#include "bcmsw_runner.h"
#endif
#include "bcm_map_part.h"

void fast_age_port(uint8_t port, uint8_t age_static);
int  ethsw_counter_collect(uint32_t port_map, int discard);
void bcmeapi_reset_mib_ext(void);
void fast_age_all_ext(uint8_t age_static);
void extsw_wreg_wrap(int page, int reg, void *vptr, int len);
void extsw_rreg_wrap(int page, int reg, void *vptr, int len);
#define DATA_TYPE_HOST_ENDIAN   (0x00<<24)
#define DATA_TYPE_BYTE_STRING   (0x01<<24)
#define DATA_TYPE_VID_MAC       (0x02<<24)
#define DATA_TYPE_MIB_COUNT     (0x03<<24)
void extsw_wreg(int page, int reg, uint8 *data, int len);
void extsw_rreg(int page, int reg, uint8 *data, int len);
void extsw_set_wanoe_portmap(uint16 wan_port_map);
void extsw_fast_age_port(uint8 port, uint8 age_static);
#if defined(CONFIG_BCM_EXT_SWITCH)
void extsw_init_config(void);
#else
#define extsw_init_config() {}
#endif
int remove_arl_entry_wrapper(void *ptr);
int bcmsw_dump_mib_ext(int port, int type);
int bcmsw_set_multiport_address_ext(uint8_t* addr);
int bcmeapi_ioctl_set_multiport_address(struct ethswctl_data *e);
#ifdef NETDEV_STATS_MASK_ALL
int bcmsw_get_hw_stats(int port, int extswitch, struct net_device_stats *stats, unsigned long mask);
#else
int bcmsw_get_hw_stats(int port, int extswitch, struct net_device_stats *stats);
#endif
int enet_arl_read_ext( uint8_t *mac, uint32_t *vid, uint32_t *val );
void enet_arl_write_ext( uint8_t *mac, uint16_t vid, uint32_t val );
int enet_arl_access_dump_ext(void);
void enet_arl_dump_ext_multiport_arl(void);
int bcmeapi_ioctl_extsw_port_jumbo_control(struct ethswctl_data *e);
int bcmsw_enable_hw_switching(void);
int bcmsw_disable_hw_switching(void);
int bcmeapi_ioctl_extsw_control(struct ethswctl_data *e);
int bcmeapi_ioctl_extsw_prio_control(struct ethswctl_data *e);
int sf2_prio_control(struct ethswctl_data *e);
int sf2_pause_drop_ctrl(struct ethswctl_data *e);
int bcmeapi_ioctl_extsw_cosq_sched(struct ethswctl_data *e);
int sf2_cosq_sched(struct ethswctl_data *e);
void bcmsw_dump_page_ext(int page);
int bcmeapi_ioctl_ethsw_pid_to_priority_mapping(struct ethswctl_data *e);
int  bcmeapi_ioctl_ethsw_cos_priority_method_config(struct ethswctl_data *e);
int bcmeapi_ioctl_extsw_pid_to_priority_mapping(struct ethswctl_data *e);
int bcmeapi_ioctl_extsw_pcp_to_priority_mapping(struct ethswctl_data *e);
int bcmeapi_ioctl_extsw_dscp_to_priority_mapping(struct ethswctl_data *e);
int bcmeapi_ioctl_extsw_cos_priority_method_config(struct ethswctl_data *e);
int bcmeapi_ioctl_extsw_cosq_port_mapping(struct ethswctl_data *e);
int bcmeapi_ioctl_extsw_config_acb(struct ethswctl_data *e);
int  enet_ioctl_ethsw_dos_ctrl(struct ethswctl_data *e);
int bcmeapi_ioctl_extsw_port_erc_config(struct ethswctl_data *e);
int bcmeapi_ioctl_extsw_port_shaper_config(struct ethswctl_data *e);
int bcmeapi_ioctl_extsw_pbvlan(struct ethswctl_data *e);
void bcmeapi_reset_mib(void);
void bcmsw_port_mirror_get(int *enable, int *mirror_port, unsigned int *ing_pmap, unsigned int *eg_pmap, unsigned int *blk_no_mrr);
void bcmsw_port_mirror_set(int enable, int mirror_port, unsigned int ing_pmap, unsigned int eg_pmap, unsigned int blk_no_mrr);
void extsw_set_pbvlan(int port, uint16_t fwdMap);
uint16_t extsw_get_pbvlan(int port);

#if defined(CONFIG_BCM_SWITCH_PORT_TRUNK_SUPPORT)
void extsw_port_trunk_init(void);
void bcmsw_port_trunk_set(unsigned int hash_sel);
void bcmsw_port_trunk_get(int *enable, unsigned int *hash_sel, unsigned int *grp0_pmap, unsigned int *grp1_pmap);
#else
#define extsw_port_trunk_init() {}
#define bcmsw_port_trunk_set() {}
#define bcmsw_port_trunk_get() {}
#endif /* CONFIG_BCM_SWITCH_PORT_TRUNK_SUPPORT */

/* Crossbar related defines */
#if defined(CONFIG_5x3_CROSSBAR_SUPPORT) /* 5x3 Crossbar  */
#define BCMENET_CROSSBAR_MAX_INT_PORTS       (BP_MAX_CROSSBAR_INT_PORTS) 
#define BCMENET_CROSSBAR_MAX_EXT_PORTS       (BP_MAX_CROSSBAR_EXT_PORTS) 
#else  
      /* 4x2 Crossbar - * note that BP_xxx always keeps the maximum 
       * number supported to make BP * independent of chip */
#define BCMENET_CROSSBAR_MAX_INT_PORTS       (BP_MAX_CROSSBAR_INT_PORTS-1) 
#define BCMENET_CROSSBAR_MAX_EXT_PORTS       (BP_MAX_CROSSBAR_EXT_PORTS-1) 
#endif

/* This represents the total number of PHY ports
 * i.e. 8 LAN ports + 1 IMP + Crossbar ports 4/5 = 13/14 */
#define BCMENET_MAX_PHY_PORTS     (BP_CROSSBAR_PORT_BASE+BCMENET_CROSSBAR_MAX_EXT_PORTS) 

#define SF2_VALID_CB_INT_PORT(x) ( ((x) >= 0) && ((x) < BCMENET_CROSSBAR_MAX_INT_PORTS) )
#define SF2_VALID_CB_EXT_PORT(x) ( ((x) >= 0) && ((x) < BCMENET_CROSSBAR_MAX_EXT_PORTS) )

#endif /* _BCMSW_H_ */
