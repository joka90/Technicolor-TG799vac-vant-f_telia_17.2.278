/*
    Copyright 2000-2010 Broadcom Corporation

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


#define _BCMENET_LOCAL_

#include <linux/types.h>
#include <linux/delay.h>
#include <linux/mii.h>
#include <linux/stddef.h>
#include <linux/ctype.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <board.h>
#include "boardparms.h"
#include <bcm_map_part.h>
#include "bcm_intr.h"
#include "bcmenet.h"
#include "ethsw.h"
#include "bcmmii.h"
#include "bcmsw_runner.h"

#ifdef CONFIG_BCM96838
#include "phys_common_drv.h"
#endif

#include "hwapi_mac.h"
#include "bcmsw.h"
#include "bcmswshared.h"
#include <rdpa_api.h>
#include <rdpa_types.h>
#if defined(CONFIG_TECHNICOLOR_GPON_PATCH)
#include "ethsw_phy.h"
#endif

extern int wan_port_id;
extern spinlock_t bcm_extsw_access;
extern struct net_device* vnet_dev[];
extern bdmf_object_handle rdpa_cpu_obj;
static rdpa_emac_stat_t cached_rdpa_emac_stat[rdpa_emac__num_of];
/* Here enet only collect port stats for UNI port, then port number should be same as emac */
static rdpa_port_stat_t cached_rdpa_port_stat[rdpa_emac__num_of];
/* Cached stats per Ethernet virtual port - currently Runner stats are clear-on-read */
static struct net_device_stats cached_vport_stats[MAX_NUM_OF_VPORTS+1] = {[0 ... (MAX_NUM_OF_VPORTS)] = {0}};

void bcmsw_pmdio_rreg(int page, int reg, uint8 *data, int len)
{
    BCM_ENET_LINK_DEBUG("read op; page = %x; reg = %x; len = %d \n",
        (unsigned int) page, (unsigned int) reg, len);

    spin_lock_bh(&bcm_extsw_access);
    spin_unlock_bh(&bcm_extsw_access);

}
void bcmsw_pmdio_wreg(int page, int reg, uint8 *data, int len)
{
    BCM_ENET_LINK_DEBUG("write op; page = %x; reg = %x; len = %d \n",
        (unsigned int) page, (unsigned int) reg, len);
    BCM_ENET_LINK_DEBUG("given data = %02x %02x %02x %02x \n",
        data[0], data[1], data[2], data[3]);

    spin_lock_bh(&bcm_extsw_access);
    spin_unlock_bh(&bcm_extsw_access);
}

int ethsw_set_mac_hw(int port, PHY_STAT ps)
{
    bdmf_object_handle  port_obj = NULL;
    rdpa_if             rdpa_port;
    rdpa_emac_cfg_t     emac_cfg = {};
    bdmf_error_t        rc = BDMF_ERR_OK;
    rdpa_port_dp_cfg_t  port_cfg = {};


    rdpa_port = rdpa_port_map_from_hw_port(port, 0);
    if (rdpa_port == rdpa_if_none)
    {
        printk("\n\n\r **** invalid port %d - has no EMAC related\n", port);
        return -1;
    }

    rc = rdpa_port_get(rdpa_port, &port_obj);
    if (rc)
        return rc;

    rc = rdpa_port_cfg_get(port_obj, &port_cfg);
    if (rc != BDMF_ERR_OK)
    {
       printk("failed to rdpa_port_cfg_get rc=%d\n",rc);
       goto error;
    }

    if( port_cfg.emac == rdpa_emac_none )
    {
        printk("\n\n\r **** EMAC for port %d is not configured\n", port);
        return -1;
    }

    mac_hwapi_get_configuration(port_cfg.emac,&emac_cfg);


    if (ps.fdx)
        emac_cfg.full_duplex = 1;
    else
        emac_cfg.full_duplex = 0;

    if (ps.spd100)
        emac_cfg.rate = rdpa_emac_rate_100m;
    else if (ps.spd1000)
        emac_cfg.rate = rdpa_emac_rate_1g;
    else
        emac_cfg.rate = rdpa_emac_rate_10m;

    mac_hwapi_set_configuration(port_cfg.emac,&emac_cfg);
    mac_hwapi_set_rxtx_enable(port_cfg.emac,1,1);

error:
    bdmf_put(port_obj);
    return rc;
}

int bcmeapi_init_ext_sw_if(extsw_info_t *extSwInfo)
{
   ETHERNET_MAC_INFO *EnetInfo = EnetGetEthernetMacInfo();
   ETHERNET_MAC_INFO *info;
   uint32 sw_port, port_map;
   uint8 v8 = 0;

   /* Check if external switch is configured in boardparams */
   info = &EnetInfo[1];
   if (!((info->ucPhyType == BP_ENET_EXTERNAL_SWITCH) ||
         (info->ucPhyType == BP_ENET_SWITCH_VIA_INTERNAL_PHY)))
   {
      printk("No External switch connected\n");
      return -ENODEV;
   }
#if 0
   /* Runner external switch config is done through board init gatemakerpro_init()
    * Keeping the code in for later use - in case the same is not done through RDPA */
   {
      /* configure Runner for External switch */
      BL_SWITCH_CFG_DTE  xi_switch_cfg;
      BL_ERROR_DTE retVal;
      xi_switch_cfg.bridge_port_id        = CE_BL_BRIDGE_PORT_LAN_4;
      xi_switch_cfg.switch_hdr_type       = BRCM_HDR_OPCODE_0;
      xi_switch_cfg.remove_hdr_when_trap  = CE_STT_TRUE;
      retVal =  bl_api_cfg_ext_switch(&xi_switch_cfg);
      if ( retVal != CE_BL_NO_ERROR )
      {
         printk("\n\nERROR !! Runner External Switch cfg failed <%d>\n\n",(int)retVal);
      }
   }
#endif /* 0 */

/* Get the internal switch/runner MAC info */
   info = &EnetInfo[0];
   if (info->sw.port_map & (1<<extSwInfo->connected_to_internalPort))
   { /* Not needed but a valid check */
      unsigned long runner_phy_id = info->sw.phy_id[extSwInfo->connected_to_internalPort];
      PHY_STAT ps = {0};

      switch (runner_phy_id & (PHY_LNK_CFG_M << PHY_LNK_CFG_S))
      {

      case FORCE_LINK_10HD:
         ps.lnk = 1;
         break;

      case FORCE_LINK_10FD:
         ps.lnk = 1;
         ps.fdx = 1;
         break;

      case FORCE_LINK_100HD:
         ps.lnk = 1;
         ps.spd100 = 1;
         break;

      case FORCE_LINK_100FD:
         ps.lnk = 1;
         ps.spd100 = 1;
         ps.fdx = 1;
         break;

      case FORCE_LINK_1000FD:
         ps.lnk = 1;
         ps.spd1000 = 1;
         ps.fdx = 1;
         break;

      default:
         printk("Invalid Link/PHY config for internal port connected to external switch <0x%x>\n",(unsigned int)runner_phy_id);
         return -1;
      }

      /* Now set the MAC */
      ethsw_set_mac_hw(extSwInfo->connected_to_internalPort, ps);
   }

   info = &EnetInfo[1]; /* External Switch Info from Boardparams */
   sw_port = 0; /* Start with first port */
   port_map = info->sw.port_map;
   for (; port_map; sw_port++, port_map /= 2 )
   {
      /* Skip the switch ports which are not in the port_map */
      while ((port_map % 2) == 0)
      {
         port_map /= 2;
         sw_port++;
      }
      /* Enable RX and TX - these gets disabled in board driver (something different in Lilac based designs)*/
      v8 &= (~REG_PORT_CTRL_DISABLE);
   }

   return 0; /* Success */
}

int bcmeapi_ioctl_ethsw_clear_port_stats(struct ethswctl_data *e){
    uint32_t port = e->port;
    bdmf_object_handle port_obj = NULL;
   rdpa_port_dp_cfg_t  port_cfg = {};
   rdpa_port_stat_t  port_stat={};
    int rc = 0;

    if ((rdpa_emac0 + port) >= rdpa_emac__num_of)
    {
         BCM_ENET_ERROR("invalid lan port id %d \n", port);
         return -1;
    }

   /* clear local data */
   memset(&cached_rdpa_port_stat[rdpa_emac0 + port], 0, sizeof(rdpa_port_stat_t));

   /* clear counters in rdpa */
    bdmf_lock();

    rc = rdpa_port_get(rdpa_if_lan0 + port, &port_obj);
    if (rc)
       goto unlock_exit;

   rc = rdpa_port_cfg_get(port_obj, &port_cfg);
   if (rc != BDMF_ERR_OK)
   {
      printk("failed to rdpa_port_cfg_get rc=%d\n",rc);
      goto unlock_exit;
   }

   rdpa_port_stat_get(port_obj, &port_stat);

    unlock_exit:
    if (port_obj)
       bdmf_put(port_obj);

    bdmf_unlock();
    return rc;
}

int bcmeapi_ioctl_ethsw_get_port_stats(struct ethswctl_data *e)
{
    uint32_t port = e->port;
    bdmf_object_handle port_obj = NULL;
    rdpa_port_dp_cfg_t  port_cfg = {};
    rdpa_port_stat_t port_stat;
    struct rdpa_port_stats temp;
    int rc;

    BCM_ENET_INFO("Port = %2d", e->port);

    if ( (rdpa_emac0 + port) >= rdpa_emac__num_of )
    {
        BCM_ENET_ERROR("invalid lan port id %d \n", port);
        return -1;
    }

    bdmf_lock();

    rc = rdpa_port_get(rdpa_if_lan0 + port, &port_obj);
    if (rc)
        goto unlock_exit;

    rc = rdpa_port_cfg_get(port_obj, &port_cfg);
    if (rc != BDMF_ERR_OK)
    {
       printk("failed to rdpa_port_cfg_get rc=%d\n",rc);
       goto unlock_exit;
    }

    rdpa_port_stat_get(port_obj, &port_stat);

    /* rdpa counters are read and clear, need to store data into local cache */
    cached_rdpa_port_stat[rdpa_emac0 + port].rx_valid_pkt += port_stat.rx_valid_pkt;
    cached_rdpa_port_stat[rdpa_emac0 + port].rx_crc_error_pkt += port_stat.rx_crc_error_pkt;
    cached_rdpa_port_stat[rdpa_emac0 + port].rx_discard_1 += port_stat.rx_discard_1;
    cached_rdpa_port_stat[rdpa_emac0 + port].rx_discard_2 += port_stat.rx_discard_2;
    cached_rdpa_port_stat[rdpa_emac0 + port].bbh_drop_1 += port_stat.bbh_drop_1;
    cached_rdpa_port_stat[rdpa_emac0 + port].bbh_drop_2 += port_stat.bbh_drop_2;
    cached_rdpa_port_stat[rdpa_emac0 + port].bbh_drop_3 += port_stat.bbh_drop_3;
    cached_rdpa_port_stat[rdpa_emac0 + port].rx_discard_max_length += port_stat.rx_discard_max_length;
    cached_rdpa_port_stat[rdpa_emac0 + port].rx_discard_min_length += port_stat.rx_discard_min_length;
    cached_rdpa_port_stat[rdpa_emac0 + port].tx_valid_pkt += port_stat.tx_valid_pkt;
    cached_rdpa_port_stat[rdpa_emac0 + port].tx_discard += port_stat.tx_discard;
    cached_rdpa_port_stat[rdpa_emac0 + port].discard_pkt += port_stat.discard_pkt;

    temp.rx_valid_pkt = (unsigned long long)cached_rdpa_port_stat[rdpa_emac0 + port].rx_valid_pkt;
    temp.rx_crc_error_pkt = (unsigned long long)cached_rdpa_port_stat[rdpa_emac0 + port].rx_crc_error_pkt;
    temp.rx_discard_1 = (unsigned long long)cached_rdpa_port_stat[rdpa_emac0 + port].rx_discard_1;
    temp.rx_discard_2 = (unsigned long long)cached_rdpa_port_stat[rdpa_emac0 + port].rx_discard_2;
    temp.bbh_drop_1 = (unsigned long long)cached_rdpa_port_stat[rdpa_emac0 + port].bbh_drop_1;
    temp.bbh_drop_2 = (unsigned long long)cached_rdpa_port_stat[rdpa_emac0 + port].bbh_drop_2;
    temp.bbh_drop_3 = (unsigned long long)cached_rdpa_port_stat[rdpa_emac0 + port].bbh_drop_3;
    temp.rx_discard_max_length = (unsigned long long)cached_rdpa_port_stat[rdpa_emac0 + port].rx_discard_max_length;
    temp.rx_discard_min_length = (unsigned long long)cached_rdpa_port_stat[rdpa_emac0 + port].rx_discard_min_length;
    temp.tx_valid_pkt = (unsigned long long)cached_rdpa_port_stat[rdpa_emac0 + port].tx_valid_pkt;
    temp.tx_discard = (unsigned long long)cached_rdpa_port_stat[rdpa_emac0 + port].tx_discard;
    temp.discard_pkt = (unsigned long long)cached_rdpa_port_stat[rdpa_emac0 + port].discard_pkt;
    
    if (copy_to_user((void*)(&e->rdpa_port_stats_s), (void*)&temp, sizeof(struct rdpa_port_stats)))
    {
      rc = -EFAULT;
      goto unlock_exit;
    }

    unlock_exit:
    if (port_obj)
        bdmf_put(port_obj);

    bdmf_unlock();
    return rc;
}

int bcmeapi_ioctl_ethsw_clear_port_emac(struct ethswctl_data *e){
    uint32_t port = e->port;
    bdmf_object_handle port_obj = NULL;
    rdpa_port_dp_cfg_t  port_cfg = {};
    rdpa_emac_stat_t emac_stat;
    int rc = 0;

    if ((rdpa_emac0 + port) >= rdpa_emac__num_of)
    {
        BCM_ENET_ERROR("invalid lan port id %d \n", port);
        return -1;
    }

    /* clear local data */
   memset(&cached_rdpa_emac_stat[rdpa_emac0 + port], 0, sizeof(rdpa_emac_stat_t));

    /* clear counters in rdpa */
    bdmf_lock();

    rc = rdpa_port_get(rdpa_if_lan0 + port, &port_obj);
    if (rc)
        goto unlock_exit;

    rc = rdpa_port_cfg_get(port_obj, &port_cfg);
    if (rc != BDMF_ERR_OK)
    {
        printk("failed to rdpa_port_cfg_get rc=%d\n", rc);
        goto unlock_exit;
    }
    
   mac_hwapi_get_rx_counters (port_cfg.emac, &emac_stat.rx);
    
    mac_hwapi_get_tx_counters (port_cfg.emac, &emac_stat.tx);

unlock_exit:
    if (port_obj)
        bdmf_put(port_obj);

    bdmf_unlock();
    return rc;
}

int bcmeapi_ioctl_ethsw_get_port_emac(struct ethswctl_data *e)
{
    uint32_t port = e->port;
    bdmf_object_handle port_obj = NULL;
    rdpa_port_dp_cfg_t  port_cfg = {};
    rdpa_emac_stat_t emac_stat;
    struct emac_stats temp;
    int rc;

    BCM_ENET_INFO("Port = %2d", e->port);

    if ( (rdpa_emac0 + port) >= rdpa_emac__num_of )
    {
        BCM_ENET_ERROR("invalid lan port id %d \n", port);
        return -1;
    }

    bdmf_lock();

    rc = rdpa_port_get(rdpa_if_lan0 + port, &port_obj);
    if (rc )
        goto unlock_exit;

    rc = rdpa_port_cfg_get(port_obj, &port_cfg);
    if ( rc != BDMF_ERR_OK)
    {
       printk("failed to rdpa_port_cfg_get rc=%d\n",rc);
       goto unlock_exit;
    }

    mac_hwapi_get_rx_counters (port_cfg.emac, &emac_stat.rx);

    mac_hwapi_get_tx_counters (port_cfg.emac, &emac_stat.tx);
    
    /* rdpa counters are read and clear, need to store data into local cache */
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.byte += emac_stat.rx.byte;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.packet += emac_stat.rx.packet;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_64 += emac_stat.rx.frame_64;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_65_127 += emac_stat.rx.frame_65_127;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_128_255 += emac_stat.rx.frame_128_255;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_256_511 += emac_stat.rx.frame_256_511;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_512_1023 += emac_stat.rx.frame_512_1023;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_1024_1518 += emac_stat.rx.frame_1024_1518;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_1519_mtu += emac_stat.rx.frame_1519_mtu;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.unicast_packet += emac_stat.rx.unicast_packet;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.multicast_packet += emac_stat.rx.multicast_packet;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.broadcast_packet += emac_stat.rx.broadcast_packet;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.alignment_error += emac_stat.rx.alignment_error;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_length_error += emac_stat.rx.frame_length_error;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.code_error += emac_stat.rx.code_error;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.carrier_sense_error += emac_stat.rx.carrier_sense_error;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.fcs_error += emac_stat.rx.fcs_error;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.control_frame += emac_stat.rx.control_frame;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.pause_control_frame += emac_stat.rx.pause_control_frame;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.unknown_opcode += emac_stat.rx.unknown_opcode;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.undersize_packet += emac_stat.rx.undersize_packet;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.oversize_packet += emac_stat.rx.oversize_packet;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.fragments += emac_stat.rx.fragments;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.jabber += emac_stat.rx.jabber;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.overflow += emac_stat.rx.overflow;


    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.byte += emac_stat.tx.byte;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.packet += emac_stat.tx.packet;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.frame_64 += emac_stat.tx.frame_64;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.frame_65_127 += emac_stat.tx.frame_65_127;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.frame_128_255 += emac_stat.tx.frame_128_255;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.frame_256_511 += emac_stat.tx.frame_256_511;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.frame_512_1023 += emac_stat.tx.frame_512_1023;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.frame_1024_1518 += emac_stat.tx.frame_1024_1518;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.frame_1519_mtu += emac_stat.tx.frame_1519_mtu;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.fcs_error += emac_stat.tx.fcs_error;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.unicast_packet += emac_stat.tx.unicast_packet;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.multicast_packet += emac_stat.tx.multicast_packet;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.broadcast_packet += emac_stat.tx.broadcast_packet;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.excessive_collision += emac_stat.tx.excessive_collision;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.late_collision += emac_stat.tx.late_collision;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.single_collision += emac_stat.tx.single_collision;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.multiple_collision += emac_stat.tx.multiple_collision;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.total_collision += emac_stat.tx.total_collision;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.pause_control_frame += emac_stat.tx.pause_control_frame;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.deferral_packet += emac_stat.tx.deferral_packet;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.excessive_deferral_packet += emac_stat.tx.excessive_deferral_packet;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.jabber_frame += emac_stat.tx.jabber_frame;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.control_frame += emac_stat.tx.control_frame;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.oversize_frame += emac_stat.tx.oversize_frame;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.undersize_frame += emac_stat.tx.undersize_frame;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.fragments_frame += emac_stat.tx.fragments_frame;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.error += emac_stat.tx.error;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.underrun += emac_stat.tx.underrun;
    
   temp.rx_byte = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.byte;
   temp.rx_packet = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.packet;
   temp.rx_frame_64 = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_64;
   temp.rx_frame_65_127 = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_65_127;
   temp.rx_frame_128_255 = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_128_255;
   temp.rx_frame_256_511 = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_256_511;
   temp.rx_frame_512_1023 = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_512_1023;
   temp.rx_frame_1024_1518 = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_1024_1518;
   temp.rx_frame_1519_mtu    = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_1519_mtu;
   temp.rx_unicast_packet = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.unicast_packet;
   temp.rx_multicast_packet = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.multicast_packet;
   temp.rx_broadcast_packet = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.broadcast_packet;
   temp.rx_alignment_error = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.alignment_error;
   temp.rx_frame_length_error = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_length_error;
   temp.rx_code_error = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.code_error;
   temp.rx_carrier_sense_error = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.carrier_sense_error;
   temp.rx_fcs_error = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.fcs_error;
   temp.rx_undersize_packet = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.undersize_packet;
   temp.rx_oversize_packet = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.oversize_packet;
   temp.rx_fragments = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.fragments;
   temp.rx_jabber = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.jabber;
   temp.rx_overflow = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.overflow;
   temp.rx_control_frame = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.control_frame;
   temp.rx_pause_control_frame = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.pause_control_frame;
   temp.rx_unknown_opcode = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.unknown_opcode;

    temp.tx_byte = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.byte;
    temp.tx_packet = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.packet;
    temp.tx_frame_64 = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.frame_64;
    temp.tx_frame_65_127 = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.frame_65_127;
    temp.tx_frame_128_255 = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.frame_128_255;
    temp.tx_frame_256_511 = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.frame_256_511;
    temp.tx_frame_512_1023 = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.frame_512_1023;
    temp.tx_frame_1024_1518 = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.frame_1024_1518;
    temp.tx_frame_1519_mtu = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.frame_1519_mtu;
    temp.tx_fcs_error = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.fcs_error;
    temp.tx_unicast_packet = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.unicast_packet;
    temp.tx_multicast_packet = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.multicast_packet;
    temp.tx_broadcast_packet = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.broadcast_packet;
    temp.tx_total_collision = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.total_collision;
    temp.tx_jabber_frame = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.jabber_frame;
    temp.tx_oversize_frame = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.oversize_frame;
    temp.tx_undersize_frame = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.undersize_frame;
    temp.tx_fragments_frame = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.fragments_frame;
    temp.tx_error = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.error;
    temp.tx_underrun = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.underrun;
   temp.tx_excessive_collision = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.excessive_collision;
   temp.tx_late_collision = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.late_collision;
   temp.tx_single_collision = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.single_collision;
   temp.tx_multiple_collision = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.multiple_collision;
   temp.tx_pause_control_frame = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.pause_control_frame;
   temp.tx_deferral_packet = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.deferral_packet;
   temp.tx_excessive_deferral_packet = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.excessive_deferral_packet;
   temp.tx_control_frame = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.control_frame;

    if (copy_to_user((void*)(&e->emac_stats_s), (void*)&temp, sizeof(struct emac_stats)))
    {
      rc = -EFAULT;
      goto unlock_exit;
    }

    unlock_exit:
    if (port_obj)
      bdmf_put(port_obj);

    bdmf_unlock();
    return rc;
}

#ifdef CONFIG_BCM96838
int bcmeapi_ioctl_ethsw_port_pause_capability(struct ethswctl_data *e)
{
	int retval = 0;
    int rc = 0;
    uint32_t port = e->port;
    bdmf_object_handle port_obj = NULL;
    rdpa_port_flow_ctrl_t flowCtrl = {0};
    rdpa_port_dp_cfg_t  port_cfg = {};
    S_MAC_HWAPI_FLOW_CTRL  macFlowControl;

	BCM_ENET_INFO("Port = %2d", e->port);

	if ( (rdpa_emac0 + port) >= rdpa_emac__num_of )
	{
		BCM_ENET_ERROR("invalid lan port id %d \n", port);
		return -1;
	}


	bdmf_lock();

	rc = rdpa_port_get(rdpa_if_lan0 + port, &port_obj);
	if (rc )
		goto unlock_exit;

	rc = rdpa_port_cfg_get(port_obj, &port_cfg);
    if ( rc != BDMF_ERR_OK)
    {
       printk("failed to rdpa_port_cfg_get rc=%d\n",rc);
       goto unlock_exit;
    }

    mac_hwapi_get_flow_control(port_cfg.emac,&macFlowControl);

	if (e->type == TYPE_GET)
	{
		retval = macFlowControl.rxFlowEnable & macFlowControl.txFlowEnable;

		if (copy_to_user((void*)(&e->ret_val), (void*)&retval, sizeof(int))) {
			rc = -EFAULT;
			goto unlock_exit;
		}
	}
	else
	{
        struct net_device *dev = enet_phyport_to_vport_dev(port);
        if(NULL != dev)
		{
            memcpy(flowCtrl.src_address.b, dev->dev_addr, ETH_ALEN);
		}
		else
		{
            rc = -1;
            goto unlock_exit;
        }


        if (e->val == TRUE)
		{
			flowCtrl.rate = 100000000;
			flowCtrl.mbs = 250000;
			flowCtrl.threshold = 125000;
			macFlowControl.txFlowEnable = 1;
		}
		else
		{
			flowCtrl.rate = 0;
			flowCtrl.mbs = 0;
			flowCtrl.threshold = 0;
			macFlowControl.txFlowEnable = 0;
		}

        mac_hwapi_set_flow_control(port_cfg.emac,&macFlowControl);

		rc = rdpa_port_flow_control_set(port_obj, &flowCtrl);
		if (rc )
			goto unlock_exit;
	}


unlock_exit:
	if(port_obj)
		bdmf_put(port_obj);

	 bdmf_unlock();
	 return rc;
}

#ifndef READ_32
#define READ_32(a, r)			( *(volatile uint32_t*)&(r) = *(volatile uint32_t*) DEVICE_ADDRESS(a) )
#endif
#ifndef UNIMAC_READ32_REG
#define UNIMAC_READ32_REG(e,r,o) READ_32( UNIMAC_CONFIGURATION_UMAC_0_RDP_##r + (UNIMAC_CONF_EMAC_OFFSET * (e)) , (o) )
#endif

int bcmeapi_ioctl_ethsw_port_traffic_control(struct ethswctl_data *e, int phy_id)
{
    uint32_t port = e->port;
    bdmf_object_handle port_obj = NULL;
    rdpa_port_dp_cfg_t  port_cfg = {};
    S_HWAPI_MAC_STATUS macStatus;
    int rc = 0;

    BCM_ENET_INFO("Port = %2d, enable %2d", e->port, e->val);

    if ( (rdpa_emac0 + port) >= rdpa_emac__num_of )
    {
        BCM_ENET_ERROR("invalid lan port id %d \n", port);
        return -1;
    }

    bdmf_lock();

    rc = rdpa_port_get(rdpa_if_lan0 + port, &port_obj);
    if (rc )
        goto unlock_exit;

    rc = rdpa_port_cfg_get(port_obj, &port_cfg);
    if ( rc != BDMF_ERR_OK)
    {
       printk("failed to rdpa_port_cfg_get rc=%d\n",rc);
       goto unlock_exit;
    }

    if (e->type == TYPE_GET)
    {
#if 0    // It is not accurate for mac status
        retval = port_emac_cfg.enable;

        if (copy_to_user((void*)(&e->ret_val), (void*)&retval, sizeof(int))) {
            rc = -EFAULT;
            goto unlock_exit;
        }
#else
        int     mac_link_up;

        memset(&macStatus, 0x0, sizeof(S_HWAPI_MAC_STATUS));
        mac_hwapi_get_mac_status(port_cfg.emac,&macStatus);

        mac_link_up = macStatus.mac_link_stat;

        if (copy_to_user((void*)(&e->ret_val), (void*)&macStatus, sizeof(int))) {
            rc = -EFAULT;
        }
#endif
    }
    else
    {
        unsigned short bmcr;
        bool is_enable = FALSE;

        is_enable = (e->val)? 0 : 1;
#if defined(CONFIG_TECHNICOLOR_GPON_PATCH)
        /* GPON-10398: delete and add ONTENET on ALU ONT, ONT's eth port can not up again. 
         * rutGpon_enablePptpEthernetUni (e->val will be 0), rutGpon_disablePptpEthernetUni (e->val will be 3)
         *   ctrl_map: bit0 = rx_disable (1 = disable rx; 0 = enable rx)
         *             bit1 = tx_disable (1 = disable tx; 0 = enable tx)
         * following logic will take TRUE (1) as enable/PHY ON, FALSE (0) as disable/PHY DOWN
         */
		
        if(e->val == 3)
            is_enable = FALSE;
        else
            is_enable = TRUE;
        /* End GPON-10398 fix */
#endif

        mac_hwapi_set_rxtx_enable(port_cfg.emac, is_enable, is_enable);

        ethsw_phyport_rreg2(phy_id, MII_BMCR, &bmcr, 0);

        if (!is_enable)
        {
            bmcr |= BMCR_PDOWN;
            ethsw_phyport_wreg2(phy_id, MII_BMCR, &bmcr, 0);
        }
        else
        {
            bmcr &= ~BMCR_PDOWN;
            ethsw_phyport_wreg2(phy_id, MII_BMCR, &bmcr, 0);
        }
    }

unlock_exit:
    if(port_obj)
        bdmf_put(port_obj);

     bdmf_unlock();
     return rc;
}

int bcmeapi_ioctl_ethsw_port_loopback(struct ethswctl_data *e, int phy_id)
{
    uint32_t port = e->port;
    MAC_LPBK tmp = MAC_LPBK_NONE;
    int rc = 0;
    struct net_device* vnet_dev_lcl;
    int vportIdx;

    BCM_ENET_INFO("Port = %2d, val 0x%04x", e->port, e->val);


    if (rdpa_emac0 + port >= rdpa_emac__num_of)
        return -ERANGE;

    /*validate device is already initialized */
    vportIdx = LOGICAL_PORT_TO_VPORT(PHYSICAL_PORT_TO_LOGICAL_PORT(port, 0));
    if (vportIdx < 0 || vportIdx > MAX_NUM_OF_VPORTS)
    {
        return -ERANGE;
    }
    vnet_dev_lcl = vnet_dev[vportIdx];
    if (!vnet_dev_lcl)
    {
        return -ERANGE;
    }

    if (e->type == TYPE_GET)
    {
        int ret;

        mac_hwapi_get_loopback(rdpa_emac0 + port, &tmp);

        if (tmp == MAC_LPBK_NONE)
            ret = 0;
        else
            ret = 1;

        if (copy_to_user((void*)(&e->ret_val), (void*)&ret, sizeof(ret))) {
            rc = -EFAULT;
        }
    }
    else
    {
        if (e->val)
            tmp = MAC_LPBK_LOCAL;

        mac_hwapi_set_loopback(rdpa_emac0 + port, tmp);
    }

    return rc;
}


int bcmeapi_ioctl_ethsw_phy_autoneg_info(struct ethswctl_data *e, int phy_id)
{
    int rc = 0;
    int retval = 0;

    if (e->type == TYPE_GET)
    {
        unsigned short bmcr, bmsr, nway_advert, gig_ctrl, gig_status;

        ethsw_phyport_rreg2(phy_id, MII_BMCR, &bmcr, 0);
        ethsw_phyport_rreg2(phy_id, MII_BMSR, &bmsr, 0);

        if (bmcr == 0xffff || bmsr == 0x0000) {
            retval = -1;
        }

        /* Get the status not set the control */
        if (bmcr & BMCR_ANENABLE)
            e->autoneg_info = TRUE;
        else
            e->autoneg_info = FALSE;

        ethsw_phyport_rreg2(phy_id, MII_ADVERTISE, &nway_advert, 0);
        ethsw_phyport_rreg2(phy_id, MII_CTRL1000, &gig_ctrl, 0);
        ethsw_phyport_rreg2(phy_id, MII_STAT1000, &gig_status, 0);

        if (gig_ctrl & ADVERTISE_1000FULL)
            e->autoneg_ad |= AN_1000M_FULL;

        if (gig_ctrl & ADVERTISE_1000HALF)
            e->autoneg_ad |= AN_1000M_HALF;

        if (nway_advert & ADVERTISE_10HALF)
            e->autoneg_ad |= AN_10M_HALF;

        if (nway_advert & ADVERTISE_10FULL)
            e->autoneg_ad |= AN_10M_FULL;

        if (nway_advert & ADVERTISE_100HALF)
            e->autoneg_ad |= AN_100M_HALF;

        if (nway_advert & ADVERTISE_100FULL)
            e->autoneg_ad |= AN_100M_FULL;

        if (nway_advert & ADVERTISE_PAUSE_CAP)
            e->autoneg_ad |= AN_FLOW_CONTROL;

        // To be confirmed here
        e->autoneg_local = e->autoneg_ad;

        if (copy_to_user((void*)(&e->ret_val), (void*)&retval, sizeof(int))) {
            rc = -EFAULT;
        }
    }
    else
    {
        unsigned short bmcr;
        ethsw_phyport_rreg2(phy_id, MII_BMCR, &bmcr, 0);
        if (e->autoneg_info & AUTONEG_RESTART_MASK)
        {
            bmcr |= BMCR_ANRESTART;
            ethsw_phyport_wreg2(phy_id, MII_BMCR, &bmcr, 0);
        }
        else
        {
            if (e->autoneg_info & AUTONEG_CTRL_MASK)
                bmcr |= BMCR_ANENABLE;
            else
                bmcr &= ~BMCR_ANENABLE;
            ethsw_phyport_wreg2(phy_id, MII_BMCR, &bmcr, 0);
        }
    }
    return rc;
}

int bcmeapi_ioctl_ethsw_phy_autoneg_cap_adv_set(struct ethswctl_data *e, int phy_id)
{
    unsigned short nway_advert;
    unsigned short gig_ctrl;
    unsigned short bmcr;
    unsigned short cap;

    ethsw_phyport_rreg2(phy_id, MII_ADVERTISE, &nway_advert, 0);
    ethsw_phyport_rreg2(phy_id, MII_CTRL1000, &gig_ctrl, 0);

    /* to mask all the phy capability */
    cap = (AN_FLOW_CONTROL | AN_10M_HALF | AN_10M_FULL | AN_100M_HALF | AN_100M_FULL |
        AN_1000M_HALF | AN_1000M_FULL);

    if ((e->autoneg_local & cap) == AN_FLOW_CONTROL)
    {/* If only flow control capability, keep the original speed and duplex capability to avoid port down */
        nway_advert |= ADVERTISE_PAUSE_CAP;
    }
    else /* If flow control capability + speed and duplex mode  or speed and duplex mode only */
    {
        nway_advert &= ~(ADVERTISE_10HALF | ADVERTISE_10FULL | ADVERTISE_100HALF |
            ADVERTISE_100FULL | ADVERTISE_PAUSE_CAP);
        gig_ctrl &= ~(ADVERTISE_1000HALF | ADVERTISE_1000FULL);

        if (e->autoneg_local & AN_10M_HALF)
            nway_advert |= ADVERTISE_10HALF;

        if (e->autoneg_local & AN_10M_FULL)
            nway_advert |= ADVERTISE_10FULL;

        if (e->autoneg_local & AN_100M_HALF)
            nway_advert |= ADVERTISE_100HALF;

        if (e->autoneg_local & AN_100M_FULL)
            nway_advert |= ADVERTISE_100FULL;

        if (e->autoneg_local & AN_1000M_FULL)
        {
            gig_ctrl |= ADVERTISE_1000FULL;
        }

        if (e->autoneg_local & AN_1000M_HALF)
        {
            gig_ctrl |= ADVERTISE_1000HALF;
        }

        if (e->autoneg_local & AN_FLOW_CONTROL)
        {
            nway_advert |= ADVERTISE_PAUSE_CAP;
        }
    }

    ethsw_phyport_wreg2(phy_id, MII_ADVERTISE, &nway_advert, 0);
    ethsw_phyport_wreg2(phy_id, MII_CTRL1000, &gig_ctrl, 0);

    ethsw_phyport_rreg2(phy_id, MII_BMCR, &bmcr, 0);
    bmcr |= BMCR_ANENABLE;
    ethsw_phyport_wreg2(phy_id, MII_BMCR, &bmcr, 0);

    if (e->autoneg_info & AUTONEG_RESTART_MASK)
    {
        bmcr |= BMCR_ANRESTART;
        ethsw_phyport_wreg2(phy_id, MII_BMCR, &bmcr, 0);
    }

    return 0;
}

int bcmeapi_ioctl_ethsw_link_status(struct ethswctl_data *e, int phy_id)
{
	int rc = 0;

	if (e->type == TYPE_GET)
	{
		int linkStatus;
		unsigned short bmsr;
		ethsw_phyport_rreg2(phy_id, MII_BMSR, &bmsr, 0);

		linkStatus = (bmsr & BMSR_LSTATUS) >> 2;
		if (copy_to_user((void*)(&e->status), (void*)&linkStatus, sizeof(int)))
			rc = -EFAULT;
	}
#if 0	     // Only worked in auto-config disabled status
	else
	{
		uint32_t port = e->port;
		bdmf_object_handle port_obj = NULL;
		rdpa_port_emac_cfg_t port_emac_cfg;

		BCM_ENET_INFO("Port = %2d, status %d, speed %d, duplex %d", e->port, e->status, e->speed, e->duplex);

		if ( (rdpa_emac0 + port) >= rdpa_emac__num_of )
		{
			BCM_ENET_ERROR("invalid lan port id %d \n", port);
			return -1;
		}

		bdmf_lock();

		rc = rdpa_port_get(rdpa_if_lan0 + port, &port_obj);
		if (rc )
			goto unlock_exit;

		rc = rdpa_port_emac_cfg_get(port_obj, &port_emac_cfg);
		if (rc )
			goto unlock_exit;

		port_emac_cfg.emac_param.full_duplex = e->duplex;
		port_emac_cfg.enable= e->status;
		if (e->speed == 10)
			port_emac_cfg.emac_param.rate= rdpa_emac_rate_10m;
		else if (e->speed == 100)
			port_emac_cfg.emac_param.rate= rdpa_emac_rate_100m;
		else if (e->speed == 1000)
			port_emac_cfg.emac_param.rate= rdpa_emac_rate_1g;

		rc = rdpa_port_emac_cfg_set(port_obj, &port_emac_cfg);
		if (rc )
			goto unlock_exit;

unlock_exit:
		if(port_obj)
			bdmf_put(port_obj);

		 bdmf_unlock();

	}
#endif

	return rc;
}


int bcmeapi_ioctl_ethsw_port_transparent_set(struct ethswctl_data *e)
{
   uint32_t port = e->port;
   bdmf_object_handle port_obj = NULL;
   rdpa_if rdpaif;
   rdpa_port_vlan_isolation_t   vlan_isolation;
   int rc = 0;

    rdpaif = rdpa_if_lan0 + port;
    if ( rdpa_emac0 + port>= rdpa_emac__num_of )
    {
        BCM_ENET_ERROR("invalid lan port id %d \n", port);
        return -1;
    }

    rc = rdpa_port_get(rdpaif, &port_obj);
    if (rc )
        goto unlock_exit;

    rc = rdpa_port_transparent_set(port_obj,e->transparent);
    if (rc )
               goto unlock_exit;


    rc =  rdpa_port_vlan_isolation_get(port_obj , &vlan_isolation);
    if (rc >= 0)
    {
        vlan_isolation.ds = !e->transparent;
        vlan_isolation.us = !e->transparent;
        rc = rdpa_port_vlan_isolation_set(port_obj, &vlan_isolation);
    }
    unlock_exit:
    if(port_obj)
        bdmf_put(port_obj);
    return rc;
}


int bcmeapi_ioctl_ethsw_port_vlanlist_set(struct ethswctl_data *e)
{
    uint32_t port = e->port;
    bdmf_object_handle port_obj = NULL;
    int rc = 0;
    bdmf_object_handle vlan_obj = NULL;
    char vlanName[BDMF_OBJ_NAME_LEN];
    bcm_vlan_t vid = e->vid & BCM_NET_VLAN_VID_M;
    struct net_device *dev = NULL;
    BDMF_MATTR(vlan_attrs, rdpa_vlan_drv());

    for(port=0;port<rdpa_emac__num_of;port++)
    {
        if(e->fwd_map & (1<<port))
            break;
    }

    if ((rdpa_emac0 + port) >= rdpa_emac__num_of )
    {
        BCM_ENET_ERROR( " Invalid port id %d \n", port);
        return -1;
    }

    dev = enet_phyport_to_vport_dev(port);
    if(NULL != dev)
        snprintf(vlanName, BDMF_OBJ_NAME_LEN, "vlan_%s_def", dev->name);
    else
        return -1;

    bdmf_lock();
    rc = rdpa_port_get(rdpa_if_lan0 + port, &port_obj);
    if (rc )
        goto unlock_exit;

    rdpa_vlan_get(vlanName,&vlan_obj);
    if(!vlan_obj)
    {
        rdpa_vlan_name_set(vlan_attrs,vlanName);
        rc = bdmf_new_and_set( rdpa_vlan_drv(), port_obj, vlan_attrs, &vlan_obj);
        if (rc )
            goto unlock_exit;
    }
    if(e->untag_map & (1<<port))
        rc = rdpa_vlan_vid_enable_set(vlan_obj,vid,0);
    else
        rc = rdpa_vlan_vid_enable_set(vlan_obj,vid,1);

    unlock_exit:
    if(port_obj)
        bdmf_put(port_obj);
    bdmf_unlock();

    return rc;
}


int bcmeapi_ioctl_ethsw_port_vlan_isolation_set(struct ethswctl_data *e)
{
   uint32_t port = e->port;
   bdmf_object_handle port_obj = NULL;
   rdpa_if rdpaif;
   rdpa_port_vlan_isolation_t   vlan_isolation;
   int rc = 0;

    rdpaif = rdpa_if_lan0 + port;
    if ( rdpa_emac0 + port>= rdpa_emac__num_of )
    {
        BCM_ENET_ERROR("invalid lan port id %d \n", port);
        return -1;
    }

    rc = rdpa_port_get(rdpaif, &port_obj);
    if (rc )
        goto unlock_exit;

    rc =  rdpa_port_vlan_isolation_get(port_obj , &vlan_isolation);
    if (rc >= 0)
    {
        vlan_isolation.ds = e->vlan_isolation.ds_enable;
        vlan_isolation.us = e->vlan_isolation.us_enable;
        rc = rdpa_port_vlan_isolation_set(port_obj, &vlan_isolation);
    }
    unlock_exit:
    if(port_obj)
        bdmf_put(port_obj);
    return rc;
}

int bcmeapi_ioctl_ethsw_port_irc_set(struct ethswctl_data *e)
{
    uint32_t port = e->port;
    bdmf_object_handle port_obj = NULL;
    int rc = 0;
    rdpa_port_flow_ctrl_t flowctl_cfg;
    char ifname[10]={0};
    struct net_device *realDev;

    BCM_ENET_INFO("Port = %2d, limit %ld, burst %ld", e->port, e->limit, e->burst_size);

    rc = rdpa_port_get(rdpa_if_lan0 + port, &port_obj);
    if (rc)
        goto unlock_exit;

    sprintf(ifname, "eth%d", port);
    realDev = dev_get_by_name(&init_net, ifname);
    
    if(NULL == realDev) {
        rc = -1;
        goto unlock_exit;
    }

    if(e->limit != 0)
        memcpy(flowctl_cfg.src_address.b, realDev->dev_addr, ETH_ALEN);
    else
        memset(flowctl_cfg.src_address.b, 0, ETH_ALEN);

    dev_put(realDev);
    flowctl_cfg.rate = e->limit*1000;            /* kbps -> bps per second */
    flowctl_cfg.mbs = e->burst_size*1000;    /* kbps-> bps */
    flowctl_cfg.threshold = e->burst_size*1000/2; /* bps, a half of mbs */

    rc = rdpa_port_flow_control_set(port_obj, &flowctl_cfg);

unlock_exit:
    if(port_obj)
        bdmf_put(port_obj);

    return rc;
}

int bcmeapi_ioctl_ethsw_port_irc_get(struct ethswctl_data *e)
{
    uint32_t port = e->port;
    bdmf_object_handle port_obj = NULL;
    rdpa_port_flow_ctrl_t flowctl_cfg;
    int rc = 0;

    BCM_ENET_INFO("Port = %2d", e->port);

    rc = rdpa_port_get(rdpa_if_lan0 + port, &port_obj);
    if (rc)
        goto unlock_exit;

    rc = rdpa_port_flow_control_get(port_obj, &flowctl_cfg);
    if(!rc) {
        e->limit = flowctl_cfg.rate*8/1000;  /* Bytes -> kbps */
        e->burst_size = flowctl_cfg.mbs*8/1000; /* Bytes -> kbps */
    }

unlock_exit:
    if(port_obj)
        bdmf_put(port_obj);

    return rc;
}

#define US_BC_RATE_LIMIT_DISABLE 0xFFFFFFFF
#define US_BC_RATE_LIMIT_METER_INDEX 2
static int cpu_meter_set(rdpa_cpu_reason_index_t reason_index, uint32_t rate,
    uint32_t meter_index, rdpa_if rdpaif)
{
    int rc = 0;
    rdpa_cpu_reason_index_t reason_cfg_idx = reason_index;
    rdpa_cpu_reason_cfg_t reason_cfg = {};
    bool is_reason_cfg_get_ok = FALSE;

    while (!rdpa_cpu_reason_cfg_get_next(rdpa_cpu_obj, &reason_cfg_idx))
    {
        rdpa_cpu_reason_cfg_get(rdpa_cpu_obj, &reason_cfg_idx, &reason_cfg);
        if (reason_cfg.meter == meter_index)
        {
            is_reason_cfg_get_ok = TRUE;
            break;
        }
    }

    /*if the meter is already there and we are not going to disable it, then just config
    meter rate is good enough as above, return quitely here*/
    if (is_reason_cfg_get_ok && rate != US_BC_RATE_LIMIT_DISABLE)
        return rc;
    if (!is_reason_cfg_get_ok)
        reason_cfg_idx = reason_index;
    if (rate == US_BC_RATE_LIMIT_DISABLE)
        reason_cfg.meter = BDMF_INDEX_UNASSIGNED;
    else
        reason_cfg.meter = meter_index;
    reason_cfg.queue = NETDEV_CPU_RX_QUEUE_ID;
    reason_cfg.meter_ports = rdpa_if_id(rdpaif);
    rc = rdpa_cpu_reason_cfg_set(rdpa_cpu_obj, &reason_cfg_idx, &reason_cfg);
    if (rc < 0)
        printk(KERN_ERR CARDNAME ": Error (%d) configuring CPU reason to meter\n", rc);
    return rc;
}

int bcmeapi_ioctl_ethsw_cpu_meter_set(struct ethswctl_data *e)
{
    uint32_t port = e->port;
    bdmf_object_handle port_obj = NULL;
    rdpa_if rdpaif;
    int rc = 0;
    rdpa_cpu_reason reason = (rdpa_cpu_reason)e->cpu_meter_rate_limit.meter_type;
    uint32_t rate = e->cpu_meter_rate_limit.rate_limit;
    uint32_t meter_index = BDMF_INDEX_UNASSIGNED;
    rdpa_cpu_meter_cfg_t meter_cfg = {};
    rdpa_dir_index_t dir_idx = {};
    rdpa_cpu_reason_index_t reason_cfg_idx = {};

    rdpaif = rdpa_if_lan0 + port;

    if ((rate < RDPA_CPU_METER_MIN_SR ||
        rate > RDPA_CPU_METER_MAX_SR ||
        rate % RDPA_CPU_METER_SR_QUANTA) &&
        rate != US_BC_RATE_LIMIT_DISABLE)
    {
        BCM_ENET_ERROR("invalid rate limit %d \n", rate);
        return -1;
    }
    /*currently only support broadcast, DPoE feature*/
    if (reason != rdpa_cpu_rx_reason_bcast)
    {
        BCM_ENET_ERROR("invalid meter reason %d \n", rate);
        return -1;
    }
    rc = rdpa_port_get(rdpaif, &port_obj);
    if (rc)
    {
        BCM_ENET_ERROR("invalid lan port id %d \n", port);
        goto unlock_exit;
    }

    /*To support rate limit per port, use couple meters, every meter for a single port
    so meter 2 to meter 5 will be used for this intention*/
    /*FIXME: use dynamic meter index allocation method once it's available*/
    meter_index = US_BC_RATE_LIMIT_METER_INDEX + port;
    dir_idx.index = meter_index;
    dir_idx.dir = rdpa_dir_us;
    if (rate != US_BC_RATE_LIMIT_DISABLE)
    {
        meter_cfg.sir = rate;
        rdpa_cpu_meter_cfg_set(rdpa_cpu_obj, &dir_idx , &meter_cfg);
    }

    reason_cfg_idx.dir = rdpa_dir_us;
    reason_cfg_idx.reason = reason;
    rc = cpu_meter_set(reason_cfg_idx, rate, meter_index, rdpaif);
unlock_exit:
    if(port_obj)
        bdmf_put(port_obj);
    return rc;
}

#if defined(CONFIG_TECHNICOLOR_GPON_PATCH)
int bcmeapi_ioctl_ethsw_phy_mode(struct ethswctl_data *e, int phy_id)
{
    uint16_t v16;
    uint16_t mii_autoneg_adv_reg;
    uint16_t mii_1000t_ctrl_reg;

    e->ret_val = -1;
    BCM_ENET_DEBUG("Given physical port %d", e->port);
    if (e->port >= EPHY_PORTS) {
        BCM_ENET_DEBUG("Invalid port %d ", e->port);
        return -EINVAL;
    }

    BCM_ENET_DEBUG("Given phy id %d", phy_id);
    if ((e->port < EPHY_PORTS) && !IsPhyConnected(phy_id)) {
        BCM_ENET_DEBUG("port %d: No Phy", e->port);
        return -EINVAL;
    }

    if (e->type == TYPE_GET) {
        ethsw_phy_rreg(phy_id, MII_BMCR, &v16);
        if (v16 & BMCR_ANENABLE)
            e->speed = 0;
        else if (v16 & BMCR_SPEED1000)
            e->speed = 1000;
        else if (v16 & BMCR_SPEED100)
            e->speed = 100;
        else
            e->speed = 10;

        if (v16 & BMCR_FULLDPLX)
            e->duplex = 1;
        else
            e->duplex = 0;
    } else {

#define MII_AR_SPEED_MASK		0x01E0
#define MII_AR_DEFAULT_CAP_MASK		0x0DE0
#define MII_ATLX_CR_1000T_SPEED_MASK	0x0300
#define MII_ATLX_CR_1000T_DEFAULT_CAP_MASK	0x0300

	/* Read the MII Auto-Neg Advertisement Register (Address 4). */
	mii_autoneg_adv_reg = MII_AR_DEFAULT_CAP_MASK;

	/* Read the MII 1000Base-T Control Register (Address 9). */
	mii_1000t_ctrl_reg = MII_ATLX_CR_1000T_DEFAULT_CAP_MASK;

	/*
     * First we clear all the 10/100 mb speed bits in the Auto-Neg
 	 * Advertisement Register (Address 4) and the 1000 mb speed bits in
 	 * the  1000Base-T Control Register (Address 9).
 	 */
	mii_autoneg_adv_reg &= ~MII_AR_SPEED_MASK;
	mii_1000t_ctrl_reg &= ~MII_ATLX_CR_1000T_SPEED_MASK;


        ethsw_phy_rreg(phy_id, MII_BMCR, &v16);
        if (e->speed == 0) {
            v16 |= BMCR_ANENABLE;
	    mii_autoneg_adv_reg |= ADVERTISE_ALL;
	    mii_1000t_ctrl_reg |= (ADVERTISE_1000FULL | ADVERTISE_1000HALF);
	}
	
        else {
            v16 &= (~BMCR_ANENABLE);
            if (e->speed == 1000) {
                v16 |= BMCR_SPEED1000;
             /* If one of the link partners disables auto]negotiation, for 1000BT the link can not be
               established. IEEE did not define 1000BT parallel detection function. Although force two
               PHYs to 1000BT mode (disable auto]negotiation by 0x0[12] =0, force speed by 0x0=0x8140
               and manually configure one as master and the other as slave), the link can be established.
               We still suggest using auto]negotiation to build 1000BT link as standard configuration. */
		v16 |= BMCR_ANENABLE;

		if (e->duplex)
			mii_1000t_ctrl_reg |= ADVERTISE_1000FULL;
		else
			mii_1000t_ctrl_reg |= ADVERTISE_1000HALF;

            }
            else {
                v16 &= (~BMCR_SPEED1000);
                if (e->speed == 100) {
			v16 |= BMCR_SPEED100;
			if (e->duplex)
				mii_autoneg_adv_reg |= ADVERTISE_100FULL;
			else
				mii_autoneg_adv_reg |= ADVERTISE_10HALF;
		} else {
			v16 &= (~BMCR_SPEED100);
			if (e->duplex)
				mii_autoneg_adv_reg |= ADVERTISE_10FULL;
			else
				mii_autoneg_adv_reg |= ADVERTISE_10HALF;
		}
		
            }
            if (e->duplex)
                v16 |= BMCR_FULLDPLX;
            else
                v16 &= (~BMCR_FULLDPLX);
        }

	/* flow control fixed to enable all */
	mii_autoneg_adv_reg |= (ADVERTISE_PAUSE_ASYM | ADVERTISE_PAUSE_CAP);

        ethsw_phy_wreg(phy_id, MII_ADVERTISE, &mii_autoneg_adv_reg);

        ethsw_phy_wreg(phy_id, MII_CTRL1000, &mii_1000t_ctrl_reg);

	v16 |= BMCR_RESET;
        ethsw_phy_wreg(phy_id, MII_BMCR, &v16);
    }

    e->ret_val = 0;
    return BCM_E_NONE;
}
#else
int bcmeapi_ioctl_ethsw_phy_mode(struct ethswctl_data *e, int phy_id)
{
    BCM_ENET_DEBUG("Given physical port %d", e->port);

    if (e->type == TYPE_GET)
    {
        PHY_RATE curr_phy_rate;
        
        curr_phy_rate = PhyGetLineRateAndDuplex(e->port);

        switch (curr_phy_rate)
        {
            case PHY_RATE_10_FULL:
                e->duplex = 1;
                e->speed  = 10;
                break;
            case PHY_RATE_10_HALF:
                e->duplex = 0;
                e->speed  = 10;
                break;
            case PHY_RATE_100_FULL:
                e->duplex = 1;
                e->speed  = 100;
                break;
            case PHY_RATE_100_HALF:
                e->duplex = 0;
                e->speed  = 100;
                break;
            case PHY_RATE_1000_FULL:
                e->duplex = 1;
                e->speed  = 1000;
                break;
            case PHY_RATE_1000_HALF:
                e->duplex = 0;
                e->speed  = 1000;
                break;
            case PHY_RATE_LINK_DOWN:
            default:
                e->duplex = 0;
                e->speed  = 0;
        }
    }
    else
    {
        unsigned short bmcr;

        ethsw_phyport_rreg2(phy_id, MII_BMCR, &bmcr, 0);
        bmcr &= ~(BMCR_SPEED1000 | BMCR_FULLDPLX | BMCR_ANRESTART |
          BMCR_ANENABLE | BMCR_SPEED100);

        if (e->speed == 0)
        {
            bmcr |= (BMCR_ANENABLE | BMCR_ANRESTART);
        }
        else
        {
            if (e->speed == 1000)
            {
                bmcr |= BMCR_SPEED1000;
            }
            else if (e->speed == 100)
            {
                bmcr |= BMCR_SPEED100;
            }
            bmcr |= (e->duplex == 1) ? BMCR_FULLDPLX : 0;
        }

        ethsw_phyport_wreg2(phy_id, MII_BMCR, &bmcr, 0);
    }

    e->ret_val = 0;
    return BCM_E_NONE;
}
#endif

int bcmeapi_ioctl_ethsw_port_erc_set(struct ethswctl_data *e)
{
    uint32_t port = e->port;
    bdmf_object_handle port_obj = NULL;
    rdpa_port_tm_cfg_t  port_tm_cfg;
    int rc = 0;

    BCM_ENET_INFO("Port = %2d, limit %ld, burst %ld", e->port, e->limit, e->burst_size);

    rc = rdpa_port_get(rdpa_if_lan0 + port, &port_obj);
    if (rc)
        goto unlock_exit;

    rc = rdpa_port_tm_cfg_get(port_obj,&port_tm_cfg);
    if (rc)
        goto unlock_exit;

    if (port_tm_cfg.sched)
    {
        rdpa_tm_rl_cfg_t  tm_rl_cfg = {0};
        tm_rl_cfg.af_rate = e->limit*1000;
        rc = rdpa_egress_tm_rl_set(port_tm_cfg.sched,&tm_rl_cfg);
    }

    unlock_exit:
    if (port_obj)
        bdmf_put(port_obj);

    return rc;
}

#endif // CONFIG_BCM96838

static inline int IsValidRunnerPort(int port)
{
#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
    /* 63138 can only have a WAN port on Runner -
       need to put in this cluge because RDPA and Ethernet Driver do not work well together when External Switch is in place.
       Hate to do this - but no other option;
       Without this check - If Runner EMAC_0 is not configured as WAN - blindly we will consider this port as LAN */
    return IsLogPortWan(PHYSICAL_PORT_TO_LOGICAL_PORT(port, 0)) ? 0 : 1;
#else
    return 0;
#endif
}

void bcmeapi_reset_mib(void)
{
    memset(&cached_vport_stats[0], 0, sizeof(cached_vport_stats));
    return;
}

int ethsw_get_hw_stats(int port, struct net_device_stats *stats)
{
   bdmf_object_handle port_obj = NULL;
   rdpa_port_dp_cfg_t port_cfg;
   rdpa_emac_stat_t emac_stat;
   struct net_device* vnet_dev_lcl;
   int vportIdx;
#if defined(ENET_GPON_CONFIG) || defined(ENET_EPON_CONFIG)
   rdpa_gem_stat_t gem_stat;
   rdpa_iptv_stat_t iptv_stat;
   bdmf_object_handle gem = NULL;
   bdmf_object_handle iptv = NULL;
#endif /* ENET_GPON_CONFIG or ENET_EPON_CONFIG */

   struct net_device_stats *port_stats;
   int rc;

   /*validate device is already initialized first device is the bcmsw*/
   vportIdx = LOGICAL_PORT_TO_VPORT(PHYSICAL_PORT_TO_LOGICAL_PORT(port, 0)); /* Runner is always unit=0 */
   if (vportIdx < 0 || vportIdx > MAX_NUM_OF_VPORTS)
   {
       return 0;
   }
   vnet_dev_lcl = vnet_dev[vportIdx];
   if (!vnet_dev_lcl)
   {
      return 0;
   }

   /*take the stats pointer*/
   port_stats = &cached_vport_stats[vportIdx];

   rc = IsValidRunnerPort(port);
   if (rc)
       goto port_stat_exit; /* Return the cached stats as last seen for this port */


   /*now we accumulate our counters into p_port_stats since our HW is clear on read*/

   BCM_ENET_INFO("Port = %2d", port);


#if defined(ENET_GPON_CONFIG) || defined(ENET_EPON_CONFIG) 
/* FIXME : At some point this should be changed and driven with a call to check if this device
 * in Enet driver configured as WAN - see code snippet below in else IsLogPortWan() */
   if (port == GPON_PORT_ID || port == EPON_PORT_ID)
   {
      while ((gem = bdmf_get_next(rdpa_gem_drv(), gem, NULL)))
      {
         rc = rdpa_gem_stat_get(gem, &gem_stat);
         if (rc)
            goto gem_exit;

         port_stats->rx_bytes += gem_stat.rx_bytes;
         port_stats->rx_packets += gem_stat.rx_packets;
         port_stats->rx_dropped += gem_stat.rx_packets_discard;
         port_stats->tx_bytes += gem_stat.tx_bytes;
         port_stats->tx_packets += gem_stat.tx_packets;
         port_stats->tx_dropped += gem_stat.tx_packets_discard;
      }

gem_exit:
      if (gem)
         bdmf_put(gem);

      rc = rdpa_iptv_get(&iptv);
      if (rc)
         goto iptv_exit;

      rc = rdpa_iptv_iptv_stat_get(iptv, &iptv_stat);
      if (rc)
         goto iptv_exit;

      port_stats->multicast += iptv_stat.rx_valid_pkt;

      iptv_exit:
      if (iptv)
         bdmf_put(iptv);
   }
   else
#endif /* ENET_GPON_CONFIG or ENET_EPON_CONFIG */
   {
      rdpa_if rdpaif = rdpa_if_lan0 + port;
      /* Check if the runner Ethernet port is configured as WAN */
      if (IsLogPortWan(PHYSICAL_PORT_TO_LOGICAL_PORT(port, 0))) {
          rdpaif = rdpa_if_wan0;
      }
      rc = rdpa_port_get(rdpaif, &port_obj);
      if (rc)
         goto port_unlock_exit;

      rc = rdpa_port_cfg_get(port_obj, &port_cfg);
      if (rc || port_cfg.emac == rdpa_emac_none)
          goto port_unlock_exit;

      mac_hwapi_get_rx_counters(port_cfg.emac,&emac_stat.rx);
      mac_hwapi_get_tx_counters(port_cfg.emac,&emac_stat.tx);

      port_stats->collisions += emac_stat.tx.total_collision;
      port_stats->multicast += emac_stat.rx.multicast_packet;
      port_stats->rx_bytes += emac_stat.rx.byte;
      port_stats->rx_packets += emac_stat.rx.packet;
      port_stats->rx_crc_errors += emac_stat.rx.fcs_error;
      port_stats->rx_errors += emac_stat.rx.alignment_error +
                               emac_stat.rx.code_error +
                               emac_stat.rx.frame_length_error ;
      port_stats->rx_length_errors += emac_stat.rx.frame_length_error;
      port_stats->tx_bytes += emac_stat.tx.byte;
      port_stats->tx_errors += emac_stat.tx.error;
      port_stats->tx_packets += emac_stat.tx.packet;

#if defined(CONFIG_BCM_KF_EXTSTATS)
      port_stats->tx_multicast_packets += emac_stat.tx.multicast_packet;
#endif
      port_unlock_exit:
      if (port_obj)
         bdmf_put(port_obj);
   }

port_stat_exit:
   /*copy the stats to the output pointer*/
   memcpy(stats,port_stats,sizeof(*stats));

   return rc;
}

int bcmeapi_ioctl_ethsw_sal_dal_set (struct ethswctl_data *e)
{
   uint32_t port = e->port;
   bdmf_object_handle port_obj = NULL;
   rdpa_if rdpaif;
   rdpa_port_dp_cfg_t cfg;
   int rc = 0;

   if(e->unit) /*unit 0 - emacs, unit 1 - wan*/
   {
      rdpaif = rdpa_if_wan0;
   }
   else
   {
      rdpaif = rdpa_if_lan0 + port;
      if ( rdpa_emac0 + port>= rdpa_emac__num_of )
      {
         BCM_ENET_ERROR("invalid lan port id %d \n", port);
         return -1;
      }
   }
   rc = rdpa_port_get(rdpaif, &port_obj);
   if (rc )
      goto unlock_exit;
   rc = rdpa_port_cfg_get(port_obj , &cfg);
   if (rc >= 0)
   {
      cfg.sal_enable = e->  sal_dal_en;
      cfg.dal_enable = e->  sal_dal_en;
      rc = rdpa_port_cfg_set(port_obj, &cfg);
   }
   unlock_exit:
   if(port_obj)
      bdmf_put(port_obj);
   return rc;
}

int bcmeapi_ioctl_ethsw_mtu_set(struct ethswctl_data *e)
{
    int rc = 0;
   rdpa_emac emac;

   for (emac = rdpa_emac0; emac < rdpa_emac__num_of; emac++)
   {
       if (!enet_phyport_to_vport_dev(emac - rdpa_emac0))
           continue;

       mac_hwapi_set_tx_max_frame_len(emac, e->mtu);
   }

   return rc; 
}
/*
 * input:
 *        e->unit is switch unit #
 *        e->port is physical hw port
 * output:
 *        returns rdpa_if in e->val
 */
int bcmeapi_ioctl_ethsw_phys_port_to_rdpa_port(struct ethswctl_data *e)
{
    int logical_port = PHYSICAL_PORT_TO_LOGICAL_PORT(e->port, e->unit);

    if (logical_port == wan_port_id) {
        e->val = rdpa_if_wan0;
    }
    else {
#if defined(CONFIG_BCM_EXT_SWITCH)
       e->val = rdpa_physical_port_to_rdpa_if(LOGICAL_PORT_TO_PHYSICAL_PORT(logical_port));
#else
       e->val = LOGICAL_PORT_TO_PHYSICAL_PORT(logical_port) + rdpa_if_lan0;
#endif
    }
    return 0;
}
