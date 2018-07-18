/*
   <:copyright-BRCM:2010:DUAL/GPL:standard

   Copyright (c) 2010 Broadcom Corporation
   All Rights Reserved

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


//**************************************************************************
// File Name  : bcmenet.c
//
// Description: This is Linux network driver for Broadcom Ethernet controller
//
//**************************************************************************

#define VERSION     "0.1"
#define VER_STR     "v" VERSION " " __DATE__ " " __TIME__

#define _BCMENET_LOCAL_
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/in.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/init.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/mii.h>
#include <linux/skbuff.h>
#include <linux/kthread.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/kmod.h>
#include <linux/rtnetlink.h>
#include "linux/if_bridge.h"
#include <net/arp.h>
#include <board.h>
#include <spidevices.h>
#include <bcmnetlink.h>
#include <bcm_intr.h>
#include "linux/bcm_assert_locks.h"
#include <linux/bcm_realtime.h>
#include <linux/stddef.h>
#include <asm/atomic.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/nbuff.h>

#include <net/net_namespace.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0)
#include <linux/module.h>
#endif
#include <linux/version.h>


#if defined(_CONFIG_BCM_BPM)
#include <linux/gbpm.h>
#include "bpm.h"
#endif

#include "bcmmii.h"
#include "bcmenet.h"
#include "ethsw_phy.h"
#include "ethsw.h"
#include "bcmsw.h"
#include "pktCmf_public.h"
#include "eth_pwrmngt.h"

#include "bcmsw_api.h"
#include "bcmswaccess.h"
#include "bcmSpiRes.h"
#include "bcmswshared.h"
#include "ethswdefs.h"
#include "bcm_chip_arch.h"

#if defined(_CONFIG_BCM_INGQOS)
#include <linux/iqos.h>
#include "ingqos.h"
#endif

#if defined(CONFIG_BCM_GMAC)
#include <bcmgmac.h>
#endif

#include "bcm_pinmux.h"
#include "bp_defs.h"

#ifdef CONFIG_BLOG
static unsigned long bcmenet_get_xmit_mark(void *dev, int priority, unsigned long uMark);
#endif

/* Technicolor: START: allow for crossbar ports to be selectively used as WAN(0) or LAN(1) -- default to WAN
 *  If a external crossbar port is associated with both a WAN(Runner) port and a LAN(Switch) port, then this
 *  bitmask is used to determine which port the external crossbar port should be associated to.
 *
 *  Each bit represents one of the external crossbar ports:
 *  Bit0: AE/SerDes (SFP)
 *  Bit1: GPHY4
 *  Bit2: RGMII3
 *  BIT3: MII
 *  Bit4: GPHY3
*/
unsigned char LanWanMux =  0x00;
/* Technicolor: END: */

#ifdef BRCM_FTTDP
#define G9991_DEBUG_RDPA_PORT rdpa_if_lan29 /* Hardcoded in FW */
/* Debug port taken from BP: MGMT_PORT must be configured in BP if port exists
   on physical board */
int g9991_bp_debug_port = -1;
#endif
#if defined(CONFIG_BCM96838) || defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
#include "bcmenet_runner_inline.h"
#endif

#if defined(SUPPORT_ETHTOOL)
#include "bcmenet_ethtool.h"
#endif

#ifdef CONFIG_BCM_PTP_1588
#include "bcmenet_ptp_1588.h"
#endif
/* vnet_dev[0] is bcmsw device not attached to any physical port */
#define port_id_from_dev(dev) ((dev->base_addr == vnet_dev[0]->base_addr) ? MAX_TOTAL_SWITCH_PORTS : \
        ((BcmEnet_devctrl *)netdev_priv(dev))->sw_port_id)
extern int kerSysGetMacAddress(unsigned char *pucaMacAddr, unsigned long ulId);
static int bcm63xx_enet_open(struct net_device * dev);
static int bcm63xx_enet_close(struct net_device * dev);
static void bcm63xx_enet_timeout(struct net_device * dev);
static void bcm63xx_enet_poll_timer(unsigned long arg);
static int bcm63xx_enet_xmit(pNBuff_t pNBuff, struct net_device * dev);
static inline int bcm63xx_enet_xmit2(struct net_device *dev, EnetXmitParams *pParam);
#ifdef NETDEV_STATS_MASK_ALL
static struct net_device_stats * bcm63xx_enet_query_masked(struct net_device * dev, unsigned long mask);
#endif
static struct net_device_stats * bcm63xx_enet_query(struct net_device * dev);
static int bcm63xx_enet_change_mtu(struct net_device *dev, int new_mtu);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 1)
static int bcm63xx_enet_rx_thread(void *arg);
#else
static int bcm63xx_enet_poll_napi(struct napi_struct *napi, int budget);
#endif
static uint32 bcm63xx_rx(void *ptr, uint32 budget);
static int bcm_set_soft_switching(int swPort, int type);
static int bcm_set_mac_addr(struct net_device *dev, void *p);
static int bcm63xx_init_dev(BcmEnet_devctrl *pDevCtrl);
static int bcm63xx_uninit_dev(BcmEnet_devctrl *pDevCtrl);
static void __exit bcmenet_module_cleanup(void);
static int __init bcmenet_module_init(void);
static int bcm63xx_enet_ioctl(struct net_device *dev, struct ifreq *rq, int cmd);
int __init bcm63xx_enet_probe(void);
static int bcm_mvtags_len(char *ethHdr);
static void bcmenet_update_pbvlan_all_bridge(void);

/* Sanity checks for user configured DMA parameters */
#if (CONFIG_BCM_DEF_NR_RX_DMA_CHANNELS > ENET_RX_CHANNELS_MAX)
#error "ERROR - Defined RX DMA Channels greater than MAX"
#endif
#if (CONFIG_BCM_DEF_NR_TX_DMA_CHANNELS > ENET_TX_CHANNELS_MAX)
#error "ERROR - Defined TX DMA Channels greater than MAX"
#endif

#if (ENET_RX_CHANNELS_MAX > 4)
#error "Overlaying channel and pDevCtrl into context param needs rework; check CONTEXT_CHAN_MASK "
#endif

/* FRV: Direct release of buffers from eth back to wl */
#ifdef CONFIG_TCH_KF_WLAN_PERF
RecycleFuncP bcm63xx_eth_wl_recycle_hook;
EXPORT_SYMBOL(bcm63xx_eth_wl_recycle_hook);

void (*bcm63xx_eth_wl_recycle_fast)(struct sk_buff *skb);
EXPORT_SYMBOL(bcm63xx_eth_wl_recycle_fast);
#endif /* CONFIG_TCH_KF_WLAN_PERF */
/* FRV: Direct release of buffers from eth back to wl */


struct kmem_cache *enetSkbCache;

static DECLARE_COMPLETION(poll_done);
static atomic_t poll_lock = ATOMIC_INIT(1);
static int poll_pid = -1;
struct net_device* vnet_dev[MAX_NUM_OF_VPORTS+1] = {[0 ... (MAX_NUM_OF_VPORTS)] = NULL};
int vport_to_logicalport[MAX_NUM_OF_VPORTS + 1] = {-1, 0, 1, 2, 3, 4, 5, 6, 7};
int logicalport_to_vport[MAX_TOTAL_SWITCH_PORTS] = {[0 ... (MAX_TOTAL_SWITCH_PORTS-1)] = -1};
#if defined(BCM_ENET_CB_WAN_PORT_LNX_INTF_SUPPORT)
int cbport_to_vportIdx[BCMENET_CROSSBAR_MAX_EXT_PORTS] = {[0 ... (BCMENET_CROSSBAR_MAX_EXT_PORTS-1)] = -1};
#endif
PHY_STAT ext_phy_stat[BP_MAX_ENET_MACS][MAX_SWITCH_PORTS][BCMENET_CROSSBAR_MAX_EXT_PORTS] =
            { [0 ... (BP_MAX_ENET_MACS - 1)][0 ... (MAX_SWITCH_PORTS - 1)][0 ... (BCMENET_CROSSBAR_MAX_EXT_PORTS - 1)] = {0}};
int vport_cnt;  /* number of vports: bitcount of Enetinfo.sw.port_map */

static unsigned int consolidated_portmap;

#if defined(ENET_EPON_CONFIG)
static const char* eponif_name = "epon0";
static const char* oamif_name = "oam";
struct net_device* eponifid_to_dev[MAX_EPON_IFS] = {[0 ... (MAX_EPON_IFS-1)] = NULL};
struct net_device* oam_dev;
void* oam_tx_func = NULL;
void* epon_data_tx_func = NULL;
static int create_epon_vport(const char *name);
static int delete_all_epon_vports(void);
EXPORT_SYMBOL(eponifid_to_dev);
EXPORT_SYMBOL(oam_dev);
EXPORT_SYMBOL(oam_tx_func);
EXPORT_SYMBOL(epon_data_tx_func);
#endif
#if defined(ENET_GPON_CONFIG)
#define UNASSIGED_IFIDX_VALUE (-1)
#define MAX_GPON_IFS_PER_GEM  (5)
static int gem_to_gponifid[MAX_GEM_IDS][MAX_GPON_IFS_PER_GEM];
static int freeid_map[MAX_GPON_IFS] = {0};
static int default_gemid[MAX_GPON_IFS] = {0};
struct net_device* gponifid_to_dev[MAX_GPON_IFS] = {[0 ... (MAX_GPON_IFS-1)] = NULL};
static int create_gpon_vport(char *name);
static int delete_gpon_vport(char *ifname);
static int delete_all_gpon_vports(void);
static int set_get_gem_map(int op, char *ifname, int ifnum, uint8 *pgem_map_arr);
static void dumpGemIdxMap(uint8 *pgem_map_arr);
static void initGemIdxMap(uint8 *pgem_map_arr);
static int set_mcast_gem_id(uint8 *pgem_map_arr);
#endif

#ifdef RDPA_VPORTS
static int delete_all_rdpa_vports(void);
static int create_rdpa_vports(void);
static int create_rdpa_vport(int sid, int physical_port, char *devname);
static struct net_device *rdpa_vport_to_dev[rdpa_if__number_of];
#endif


#if defined(CONFIG_BCM_SWITCH_PORT_TRUNK_SUPPORT)
uint32 trunk_grp[BCM_SW_MAX_TRUNK_GROUPS] = {[0 ... (BCM_SW_MAX_TRUNK_GROUPS-1)] = 0};
#endif

static const struct net_device_ops bcm96xx_netdev_ops = {
    .ndo_open   = bcm63xx_enet_open,
    .ndo_stop   = bcm63xx_enet_close,
    .ndo_start_xmit   = (HardStartXmitFuncP)bcm63xx_enet_xmit,
    .ndo_set_mac_address  = bcm_set_mac_addr,
    .ndo_do_ioctl   = bcm63xx_enet_ioctl,
    .ndo_tx_timeout   = bcm63xx_enet_timeout,
    .ndo_get_stats      = bcm63xx_enet_query,
#ifdef NETDEV_STATS_MASK_ALL
    .ndo_get_stats_masked = bcm63xx_enet_query_masked,
#endif
    .ndo_change_mtu     = bcm63xx_enet_change_mtu
};


#ifdef BCM_ENET_DEBUG_BUILD
/* Debug Variables */
/* Number of pkts received on each channel */
static int ch_pkts[ENET_RX_CHANNELS_MAX] = {0};
/* Number of times there are no rx pkts on each channel */
static int ch_no_pkts[ENET_RX_CHANNELS_MAX] = {0};
static int ch_no_bds[ENET_RX_CHANNELS_MAX] = {0};
/* Number of elements in ch_serviced debug array */
#define NUM_ELEMS 4000
/* -1 indicates beginning of an rx(). The bit31 indicates whether a pkt
   is received on that channel or not */
static unsigned int ch_serviced[NUM_ELEMS] = {0};
static int dbg_index;
#define NEXT_INDEX(index) ((++index) % NUM_ELEMS)
#define ISR_START 0xFF
#define WRR_RELOAD 0xEE
#endif
#define LLID_SINGLE 0

extsw_info_t extSwInfo = {
    .switch_id = 0,
    .brcm_tag_type = 0,
    .present = 0,
    .connected_to_internalPort = -1,
};

int bcmenet_in_init_dev = 0;

enet_global_var_t global = {
    .extPhyMask = 0,
    .dump_enable = 0,
    .net_device_stats_from_hw = {0},
    .pVnetDev0_g = NULL
};

/* Indicate whether the external PHY or external switch's PHY is connected */
static int bcmenet_use_ext_phy;

/* Record the return value from bcmeapi_map_interrupt(pDevCtrl) about if
    interrupt is used for internal and external PHY handling */
static int phy_int_mapped;

/* Indicate if port status polling is needed based on the phy_int_mapped */
static int force_link_check;

struct semaphore bcm_ethlock_switch_config;
struct semaphore bcm_link_handler_config;

spinlock_t bcm_ethlock_phy_access;
spinlock_t bcm_extsw_access;
atomic_t phy_read_ref_cnt = ATOMIC_INIT(0);
atomic_t phy_write_ref_cnt = ATOMIC_INIT(0);

#ifdef DYING_GASP_API

/* OAMPDU Ethernet dying gasp message */
unsigned char dg_ethOam_frame[64] = {
    1, 0x80, 0xc2, 0, 0, 2,
    0, 0,    0,    0, 0, 0, /* Fill Src MAC at the time of sending, from dev */
    0x88, 0x9,
    3, /* Subtype */
    5, /* code for DG frame */
    'B', 'R', 'O', 'A', 'D', 'C', 'O', 'M',
    ' ', 'B', 'C', 'G',

};

/* Socket buffer and buffer pointer for msg */
struct sk_buff *dg_skbp;

/* Flag indicates we're in Dying Gasp and powering down - don't clear once set */
int dg_in_context=0;

#endif

/* Delete all the virtual eth ports */
static void delete_vport(void)
{
    int port;

    synchronize_net();

    for (port = 1; port <= vport_cnt; port++)
    {
        if (vnet_dev[port] != NULL)
        {
#ifdef SEPARATE_MAC_FOR_WAN_INTERFACES
            if(memcmp(vnet_dev[0]->dev_addr, vnet_dev[port]->dev_addr, ETH_ALEN)) {
                kerSysReleaseMacAddress(vnet_dev[port]->dev_addr);
            }
#endif
            unregister_netdev(vnet_dev[port]);
            free_netdev(vnet_dev[port]);
            vnet_dev[port] = NULL;
        }
    }
}

/* Indirection just to print the caller line number in case of error */
#define bcmenet_set_dev_mac_addr(a,b) _bcmenet_set_dev_mac_addr(a,b,__LINE__)

static int _bcmenet_set_dev_mac_addr(struct net_device *dev, struct sockaddr *pSockAddr, int callerLine)
{
    int retVal;
    pSockAddr->sa_family = dev->type; /* set the sa_family same as dev type to avoid error */
    if (netif_running(dev)) {
        //dev_close(dev); /* ifconfig ethX down - MAC address change is not allowed on running device */
        // clear_bit === ifconfig ethX down ; close & open are unnecessary and costly
        clear_bit(__LINK_STATE_START, &dev->state);
        if (rtnl_is_locked()) {
            retVal = dev_set_mac_address(dev, pSockAddr);
        }
        else {
            rtnl_lock();
            retVal = dev_set_mac_address(dev, pSockAddr);
            rtnl_unlock();
        }
        //dev_open(dev); /* ifconfig ethX up */
        set_bit(__LINK_STATE_START, &dev->state);
        if (retVal) {
            printk("%d:ERROR<%d> - setting MAC address for dev %s\n",callerLine,retVal,dev->name);
        }
    }
    else {

        if (rtnl_is_locked()) {
            retVal = dev_set_mac_address(dev, pSockAddr);
        }
        else {
            rtnl_lock();
            retVal = dev_set_mac_address(dev, pSockAddr);
            rtnl_unlock();
        }
        if (retVal) {
            printk("%d:ERROR<%d> - setting MAC address for dev %s\n",callerLine,retVal,dev->name);
        }
    }
    return retVal;
}


#if defined(_CONFIG_ENET_BCM_TM)
static int enet_bcm_tm_free(uint16_t length, void *param_p)
{
    enet_bcm_tm_param_t *enetParam_p = param_p;

    global.pVnetDev0_g->stats.tx_dropped++;
//    pParam->vstats->tx_dropped++;

    nbuff_flushfree((pNBuff_t)enetParam_p->key);

    return 0;
}

static int enet_bcm_tm_register(int port, struct net_device *dev)
{
    bcmFun_t *bcmFun;
    bcmTmDrv_arg_t tm;

    enet_bcm_tm_enqueue = bcmFun_get(BCM_FUN_ID_TM_ENQUEUE);
    BCM_ASSERT(enet_bcm_tm_enqueue != NULL);

    bcmFun = bcmFun_get(BCM_FUN_ID_TM_REGISTER);
    BCM_ASSERT(bcmFun != NULL);

    tm.phy = BCM_TM_DRV_PHY_TYPE_ETH;
    tm.port = port;
    tm.nbrOfQueues = ENET_BCM_TM_NBR_OF_QUEUES;
    tm.nbrOfEntries = ENET_BCM_TM_NBR_OF_ENTRIES;
    tm.paramSize = sizeof(enet_bcm_tm_param_t);
    tm.txCallbackFunc = enet_bcm_tm_xmit;
    tm.freeCallbackFunc = enet_bcm_tm_free;

    return bcmFun(&tm);
}
#endif

/* Create virtual eth ports: one for each physical switch port except
   for the GPON port */
static int create_vport(void)
{
    struct net_device *dev;
    struct sockaddr sockaddr;
    int status, vport_id, logical_port;
    int *map_p, sw_num=0, sw_pmap[BP_MAX_ENET_MACS];
    PHY_STAT phys;
    BcmEnet_devctrl *pDevCtrl = NULL;
    BcmEnet_devctrl *pVnetDev0 = (BcmEnet_devctrl *) netdev_priv(vnet_dev[0]);
    int phy_id, phy_conn;
    char vethPortmap[MAX_NUM_OF_VPORTS];
    int npm = sizeof(vethPortmap)/sizeof(vethPortmap[0]);

    char *phy_devName;
    int unit = 0, physical_port = 0;
    unsigned long port_flags;
    int cb_port = BP_CROSSBAR_NOT_DEFINED; /* Initialize to invalid value */
#if defined(BCM_ENET_CB_WAN_PORT_LNX_INTF_SUPPORT)
    char *phy_devNameOrig; /* used to store the default phy_devName for the group of crossbar ports */
#endif /* */
    /* separate out the portmap for internal and external switch */
#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
    /* For 63138/63148 - start creating Linux interfaces from the Runner(Internal Switch) Port first */
    sw_pmap[0] = consolidated_portmap & SW_PORTMAP_M;
    sw_pmap[1] = consolidated_portmap & SW_PORTMAP_EXT_M;
#else
    /* For older chips - start creating Linux interfaces from the External Switch Port first - to stay backward compatible */
    sw_pmap[0] = consolidated_portmap & SW_PORTMAP_EXT_M;
    sw_pmap[1] = consolidated_portmap & SW_PORTMAP_M;
#endif
    if (vport_cnt > MAX_NUM_OF_VPORTS
        || BpGetVethPortmap_tch(&npm, vethPortmap))
        return -1;

    phys.lnk = 0;
    phys.fdx = 0;
    phys.spd1000 = 0;
    phys.spd100 = 0;

    sw_num = 0;
    map_p = &sw_pmap[sw_num]; /* Start with first switch */

    for ( vport_id = 1; vport_id <= vport_cnt; vport_id++)
    {
        logical_port = vethPortmap[vport_id-1];
        if (logical_port < 0 || logical_port >= MAX_TOTAL_SWITCH_PORTS)
            continue;

        /* Initialize the vport <--> phyport mapping tables */
        vport_to_logicalport[vport_id] = logical_port;
        logicalport_to_vport[logical_port] = vport_id;

        BCM_ASSERT(logical_port < MAX_TOTAL_SWITCH_PORTS);

        if (!bcmeapi_should_create_vport(logical_port))
        {
            (*map_p) /= 2;
            logical_port ++;
            vport_cnt--;
            continue;
        }

        physical_port = LOGICAL_PORT_TO_PHYSICAL_PORT(logical_port);
        unit = LOGICAL_PORT_TO_UNIT_NUMBER(logical_port);
        phy_id = pVnetDev0->EnetInfo[unit].sw.phy_id[physical_port];
        if(IsExtPhyId(phy_id) || IsExternalSwitchUnit(unit))
        {
            bcmenet_use_ext_phy = TRUE;
        }
        phy_conn = pVnetDev0->EnetInfo[unit].sw.phyconn[physical_port];
        phy_devName = pVnetDev0->EnetInfo[unit].sw.phy_devName[physical_port];
        port_flags = pVnetDev0->EnetInfo[unit].sw.port_flags[physical_port];
        /* TBD - Why we did not take care of crossbar ports ??*/

#if defined(CONFIG_BCM_SWITCH_PORT_TRUNK_SUPPORT)
        if ( IsPortTrunkGrpEnabled(port_flags) )
        {
            int grp_no = PortTrunkGroup(port_flags);
            int mstr_logical_port = (trunk_grp[grp_no] & TRUNK_GRP_MASTER_PORT_MASK) >> TRUNK_GRP_MASTER_PORT_SHIFT;
            if (trunk_grp[grp_no])
            {
                /* Already created interface for this LAG group */
                vnet_dev[vport_id] = vnet_dev[LOGICAL_PORT_TO_VPORT(mstr_logical_port)];
                trunk_grp[grp_no] |= (TRUNK_GRP_PORT_MASK & (1<<logical_port));
                /* Before we bail out, let underlying layer Runner/Internal-switch do what it needs
                 * for this LAN port under trunk; Note that trunking awareness is only in the bcmenet
                 * for load balancing and the actual switch; Other layers are not aware of this.
                 * For Runner platforms, it is important to create the port & TM objects */
                dev = vnet_dev[vport_id];
                pDevCtrl = netdev_priv(dev);
                pDevCtrl->sw_port_id        = logical_port; /* Temporary set logical port */
                bcmeapi_create_vport(dev);
                pDevCtrl->sw_port_id        = mstr_logical_port; /* Revert the earlier change */
                continue;
            }
            /* Set logical port and Master port in LAG group */
            trunk_grp[grp_no] = ( (TRUNK_GRP_PORT_MASK & (1<<logical_port)) | (logical_port << TRUNK_GRP_MASTER_PORT_SHIFT) );
        }
#endif /* CONFIG_BCM_SWITCH_PORT_TRUNK_SUPPORT */
#ifdef BRCM_FTTDP
        if (IsPortMgmt(port_flags))
        {
            if (g9991_bp_debug_port != -1)
                printk("g9991_bp_debug_port already configured on port %d\n", g9991_bp_debug_port);

            g9991_bp_debug_port = physical_port;
        }
#endif

#if defined(BCM_ENET_CB_WAN_PORT_LNX_INTF_SUPPORT)
        phy_devNameOrig = phy_devName; /* Store original device name for later use */
        cb_port = enet_get_first_crossbar_port(logical_port); 
        do
        {
            if (cb_port >= 0 && cb_port < BCMENET_CROSSBAR_MAX_EXT_PORTS) /* Range check */
            {
                if (pVnetDev0->EnetInfo[unit].sw.crossbar[cb_port].phy_devName != PHY_DEVNAME_NOT_DEFINED)
                { /* Use the name specific to this crossbar port */
                    phy_devName = pVnetDev0->EnetInfo[unit].sw.crossbar[cb_port].phy_devName;
                }
                else
                { /* Use the default or generic name */
                    phy_devName = phy_devNameOrig;
                }
            }
#endif /* */
        dev = alloc_etherdev(sizeof(BcmEnet_devctrl));
        if (dev == NULL) {
            printk("%s: dev alloc failed \n", __FUNCTION__);
            delete_vport();
            return -ENOMEM;
        }

        pDevCtrl = netdev_priv(dev);
        memset(pDevCtrl, 0, sizeof(BcmEnet_devctrl));

        /* Set the pDevCtrl->dev to dev */
        pDevCtrl->dev = dev;

        if (phy_devName != PHY_DEVNAME_NOT_DEFINED)
        {
            dev_alloc_name(dev, phy_devName);
        }
        else
        {
            dev_alloc_name(dev, dev->name);
        }

        SET_MODULE_OWNER(dev);

        dev->netdev_ops             = vnet_dev[0]->netdev_ops;
#if defined(SUPPORT_ETHTOOL)
        dev->ethtool_ops            = vnet_dev[0]->ethtool_ops;
#endif
        dev->priv_flags             |= vnet_dev[0]->priv_flags;
        dev->base_addr              = logical_port;

        dev->features               = vnet_dev[0]->features;
        dev->vlan_features          = vnet_dev[0]->vlan_features;
        /* Switch port id of this interface */
        pDevCtrl->sw_port_id        = logical_port; /* This is logical port number */

#if defined(CONFIG_BCM_PKTRUNNER_GSO)
            /* currently GSO is supported only on LAN ports, and since eth0 can be used as WAN
             * we disable gso support on wan port.
             */ 
            if(logical_port == 0)
            {
                printk("++++ disabling GSO on logical_port=0 dev=%s\n", dev->name);
                dev->features &= ~(NETIF_F_SG | NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_TSO | NETIF_F_TSO6 | NETIF_F_UFO);
                dev->vlan_features &= ~(NETIF_F_SG | NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_TSO | NETIF_F_TSO6 | NETIF_F_UFO);
            }
#endif

        status = bcmeapi_create_vport(dev);
        if (status)
        {
            /*cancel the device creation*/
            free_netdev(dev);
            return status;
        }

        netdev_path_set_hw_port(dev, logical_port, BLOG_ENETPHY);
        status = register_netdev(dev);
        if (status != 0)
        {
            unregister_netdev(dev);
            free_netdev(dev);
            return status;
        }
        vnet_dev[vport_id] = dev;

        /* The vport_id specifies the unique id of virtual eth interface */
        pDevCtrl->vport_id = vport_id;
            pDevCtrl->cb_ext_port = cb_port; /* Attach crossbar port only if virtual devices are created 
                                                for crossbar otherwise it should be BP_CROSSBAR_NOT_DEFINED */

        /* Set the default tx queue to 0 */
        pDevCtrl->default_txq = 0;
        pDevCtrl->use_default_txq = 0;
        memmove(sockaddr.sa_data, vnet_dev[0]->dev_addr, ETH_ALEN);
        BCM_ENET_DEBUG("phy_id = %d", phy_id);
        if (IsWanPort(phy_id)) {
            pVnetDev0->wanPort |= 1 << logical_port;
            BCM_ENET_DEBUG("Getting MAC for WAN port %d", logical_port);
#ifdef SEPARATE_MAC_FOR_WAN_INTERFACES
            status = kerSysGetMacAddress(dev->dev_addr, dev->ifindex);
            if (status == 0) {
                memmove(sockaddr.sa_data, dev->dev_addr, ETH_ALEN);
            }
#endif
            dev->priv_flags |= IFF_WANDEV;
#if !defined (CONFIG_BCM96838)
            dev->priv_flags &= ~IFF_HW_SWITCH;
#endif
        }
        bcmenet_set_dev_mac_addr(dev, &sockaddr);
#if defined(CONFIG_BCM_GMAC)
        /* GMAC port is only on the internal switch  */
        if ( gmac_is_gmac_supported() && !IsExternalSwitchPort(logical_port) && gmac_is_gmac_port(physical_port))
        {
            if (IsWanPort(phy_id)) {
                gmac_set_wan_port( 1 );
            }
            pVnetDev0->gmacPort |= 1 << logical_port;
            BCM_ENET_DEBUG("Setting gmac port %d phy id %d to gmacPort %d", physical_port, logical_port, pVnetDev0->gmacPort);
        }
#endif

        /* Note: The parameter vport_id should be the vport_id-1. The ethsw_set_mac
           maps it to physical port id */
        if(pVnetDev0->unit == 0)
        {
                ethsw_set_mac(logical_port, phys);  
        }

        if(IsExternalSwitchPort(logical_port))
        {
            dev->priv_flags |= IFF_EXT_SWITCH;
        }
        if (IsPortSoftSwitching(port_flags))
        {
            bcm_set_soft_switching(logical_port, TYPE_ENABLE);
        }
#if defined(_CONFIG_ENET_BCM_TM)
        enet_bcm_tm_register(logical_port, dev);
#endif
        printk("%s: <%s sw port: %d> <Logical : %02d> PHY_ID <0x%08x : 0x%02x> MAC : %02X:%02X:%02X:%02X:%02X:%02X\n",
               dev->name,
               (LOGICAL_PORT_TO_UNIT_NUMBER(logical_port)?"Ext":"Int"),LOGICAL_PORT_TO_PHYSICAL_PORT(logical_port), logical_port,
               phy_id, phy_id&BCM_PHY_ID_M,
               dev->dev_addr[0], dev->dev_addr[1], dev->dev_addr[2], dev->dev_addr[3], dev->dev_addr[4], dev->dev_addr[5]);
#if defined(BCM_ENET_CB_WAN_PORT_LNX_INTF_SUPPORT)
            /* Could check if cb_port is valid but should be OK as this function is not called many times */
            if (cb_port != BP_CROSSBAR_NOT_DEFINED) /* Valid crossbar port - store vport_id */
            {
                cbport_to_vportIdx[cb_port] = vport_id;
            }
            cb_port = enet_get_next_crossbar_port(logical_port, cb_port);
            if (cb_port != BP_CROSSBAR_NOT_DEFINED) /* More crossbar ports for this logical port */
            {
                vport_id++; /* Move on to the next vport index */
                vport_to_logicalport[vport_id] = logical_port; /* store the new vport to logical port mapping */
            }
        } while(cb_port != BP_CROSSBAR_NOT_DEFINED);
#endif /* */
    }
    return 0;
}

#undef OFFSETOF
#define OFFSETOF(STYPE, MEMBER)     ((size_t) &((STYPE *)0)->MEMBER)

static inline int bcm_mvtags_len(char *ethHdr)
{
    unsigned int end_offset = 0;
    struct vlan_ethhdr *vhd;
    BcmEnet_hdr* bhd;
    uint16 brcm_type;

    bhd = (BcmEnet_hdr*)ethHdr;
    brcm_type = ntohs(bhd->brcm_type);
    if (brcm_type == BRCM_TYPE)
    {
        end_offset += BRCM_TAG_LEN;
        bhd = (BcmEnet_hdr *)((int)bhd + BRCM_TAG_LEN);
        brcm_type = ntohs(bhd->brcm_type);
    }

#ifndef BRCM_FTTDP
    /* FTTDP inband control traffic also uses 0x888A, but is not a switch tag. Assume they don't exist together */
    if (brcm_type == BRCM_TYPE2)
    {
        end_offset += BRCM_TAG_TYPE2_LEN;
    }
#endif

#ifdef VLAN_TAG_FFF_STRIP
    vhd = (struct vlan_ethhdr*)(ethHdr + end_offset);
    if((ntohs(vhd->h_vlan_proto) == VLAN_TYPE) &&
            (ntohs(vhd->h_vlan_TCI) & VLAN_VID_MASK) == 0xFFF)
    {
        end_offset += VLAN_HLEN;
    }
#endif

    return end_offset;
}

/*
 *  This is a modified version of eth_type_trans(), for taking care of
 *  Broadcom Tag with Ethernet type BRCM_TYPE [0x8874].
 */

unsigned short bcm_type_trans(struct sk_buff *skb, struct net_device *dev)
{
    struct ethhdr *eth;
    unsigned char *rawp;
    unsigned int end_offset = 0, from_offset = 0;
    uint16 *to, *end, *from;
    unsigned int hdrlen = sizeof(struct ethhdr);

    skb_reset_mac_header(skb);

    end_offset = bcm_mvtags_len(skb->data);
    if (end_offset)
    {
        from_offset = OFFSETOF(struct ethhdr, h_proto);

        to = (uint16*)(skb->data + from_offset + end_offset) - 1;
        end = (uint16*)(skb->data + end_offset) - 1;
        from = (uint16*)(skb->data + from_offset) - 1;

        while ( to != end )
            *to-- = *from--;
    }

    skb_set_mac_header(skb, end_offset);

    hdrlen += end_offset;

    skb_pull(skb, hdrlen);
    eth = (struct ethhdr *)skb_mac_header(skb);

    if(*eth->h_dest&1)
    {
        if(memcmp(eth->h_dest,dev->broadcast, ETH_ALEN)==0)
            skb->pkt_type=PACKET_BROADCAST;
        else
            skb->pkt_type=PACKET_MULTICAST;
    }

    /*
     *  This ALLMULTI check should be redundant by 1.4
     *  so don't forget to remove it.
     *
     *  Seems, you forgot to remove it. All silly devices
     *  seems to set IFF_PROMISC.
     */

    else if(1 /*dev->flags&IFF_PROMISC*/)
    {
        if(memcmp(eth->h_dest,dev->dev_addr, ETH_ALEN))
            skb->pkt_type=PACKET_OTHERHOST;
    }

    if (ntohs(eth->h_proto) >= 1536)
        return eth->h_proto;

    rawp = skb->data;

    /*
     *  This is a magic hack to spot IPX packets. Older Novell breaks
     *  the protocol design and runs IPX over 802.3 without an 802.2 LLC
     *  layer. We look for FFFF which isn't a used 802.2 SSAP/DSAP. This
     *  won't work for fault tolerant netware but does for the rest.
     */
    if (*(unsigned short *)rawp == 0xFFFF)
        return htons(ETH_P_802_3);

    /*
     *  Real 802.2 LLC
     */
    return htons(ETH_P_802_2);
}

/* --------------------------------------------------------------------------
Name: bcm63xx_enet_open
Purpose: Open and Initialize the EMAC on the chip
-------------------------------------------------------------------------- */
static int bcm63xx_enet_open(struct net_device * dev)
{
    BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);

    if (dev != vnet_dev[0])
    {
        if ((vnet_dev[0]->flags & IFF_UP) == 0)
            return -ENETDOWN;

        bcmeapi_add_dev_queue(dev);
        netif_start_queue(dev);
        return 0;
    }

    ASSERT(pDevCtrl != NULL);
    TRACE(("%s: bcm63xx_enet_open\n", dev->name));

    bcmeapi_open_dev(pDevCtrl, dev);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 1)
    /* enet does not use NAPI in 3.4 */
#else
    /* napi_enable must be called before the interrupts are enabled
       if an interrupt comes in before napi_enable is called the napi
       handler will not run and the interrupt will not be re-enabled */
    napi_enable(&pDevCtrl->napi);
#endif

    netif_start_queue(dev);


    return 0;
}

/* --------------------------------------------------------------------------
Name: bcm63xx_enet_close
Purpose: Stop communicating with the outside world
Note: Caused by 'ifconfig ethX down'
-------------------------------------------------------------------------- */
static int bcm63xx_enet_close(struct net_device * dev)
{
    BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);

    if (dev != vnet_dev[0])
    {
        netif_stop_queue(dev);
        return 0;
    }

    ASSERT(pDevCtrl != NULL);
    TRACE(("%s: bcm63xx_enet_close\n", dev->name));

    bcmeapi_del_dev_intr(pDevCtrl);

    netif_stop_queue(dev);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 1)
    /* enet does not use NAPI in 3.4 */
#else
    napi_disable(&pDevCtrl->napi);
#endif

    return 0;
}

/* --------------------------------------------------------------------------
Name: bcm63xx_enet_timeout
Purpose:
-------------------------------------------------------------------------- */
static void bcm63xx_enet_timeout(struct net_device * dev)
{
    ASSERT(dev != NULL);
    TRACE(("%s: bcm63xx_enet_timeout\n", dev->name));

    dev->trans_start = jiffies;
    netif_wake_queue(dev);
}

#if defined(REPORT_HARDWARE_STATS) && defined(CONFIG_BCM_SWITCH_PORT_TRUNK_SUPPORT)
static struct net_device_stats g_stats_2;
/* --------------------------------------------------------------------------
    Name: bcmenet_add_stats
 Purpose: Adds the stats from *s into *d and returns total stats
-------------------------------------------------------------------------- */
static void bcmenet_add_stats(struct net_device_stats *d, struct net_device_stats *s)
{
    d->rx_packets += s->rx_packets ;
    d->tx_packets += s->tx_packets ;
    d->rx_bytes += s->rx_bytes ;
    d->tx_bytes += s->tx_bytes ;
    d->rx_errors += s->rx_errors ;
    d->tx_errors += s->tx_errors ;
    d->rx_dropped += s->rx_dropped ;
    d->tx_dropped += s->tx_dropped ;
    d->multicast += s->multicast ;

    #if defined(CONFIG_BCM_KF_EXTSTATS)
    d->tx_multicast_packets += s->tx_multicast_packets ;
    d->rx_multicast_bytes += s->rx_multicast_bytes ;
    d->tx_multicast_bytes += s->tx_multicast_bytes ;
    d->rx_broadcast_packets += s->rx_broadcast_packets ;
    d->tx_broadcast_packets += s->tx_broadcast_packets ;
    d->rx_unknown_packets += s->rx_unknown_packets ;
    #endif

    d->collisions += s->collisions ;
    d->rx_length_errors += s->rx_length_errors ;
    d->rx_over_errors += s->rx_over_errors ;
    d->rx_crc_errors += s->rx_crc_errors ;
    d->rx_frame_errors += s->rx_frame_errors ;
    d->rx_fifo_errors += s->rx_fifo_errors ;
    d->rx_missed_errors += s->rx_missed_errors ;
    d->tx_aborted_errors += s->tx_aborted_errors ;
    d->tx_carrier_errors += s->tx_carrier_errors ;
    d->tx_fifo_errors += s->tx_fifo_errors ;
    d->tx_heartbeat_errors += s->tx_heartbeat_errors ;
    d->tx_window_errors += s->tx_window_errors ;
    d->rx_compressed += s->rx_compressed ;
    d->tx_compressed += s->tx_compressed ;
}
#endif

/* --------------------------------------------------------------------------
Name: bcm63xx_enet_query
Purpose: Return the current statistics. This may be called with the card
open or closed.
-------------------------------------------------------------------------- */
#ifdef NETDEV_STATS_MASK_ALL
static struct net_device_stats *
bcm63xx_enet_query(struct net_device * dev)
{
    return bcm63xx_enet_query_masked(dev, NETDEV_STATS_MASK_ALL);
}
#endif

static struct net_device_stats *
#ifdef NETDEV_STATS_MASK_ALL
bcm63xx_enet_query_masked(struct net_device * dev, unsigned long mask)
#else
bcm63xx_enet_query(struct net_device * dev)
#endif
{
#ifdef REPORT_HARDWARE_STATS
    int port, log_port, extswitch;

    if (dev->base_addr == vnet_dev[0]->base_addr) /* bcmsw device */
    {
        return &(((BcmEnet_devctrl *)netdev_priv(dev))->stats);
    }

    log_port = port_id_from_dev(dev);

#ifdef RDPA_VPORTS
    /* XXX: As for now, no HW statistics for SID ports */
    if (log_port == SID_PORT_ID)
        return &(((BcmEnet_devctrl *)netdev_priv(dev))->stats);
#endif

#if defined(ENET_EPON_CONFIG)
    if(log_port == EPON_PORT_ID)
    {
        return &(((BcmEnet_devctrl *)netdev_priv(dev))->stats);
    }
#endif

    port = LOGICAL_PORT_TO_PHYSICAL_PORT(log_port);
    extswitch = IsExternalSwitchPort(log_port);

    if ((port < 0) || (port >= MAX_SWITCH_PORTS)) {
        printk("Invalid port <phy - %d : log - %d>, so stats will not be correct \n", port, log_port);
    }else {
        struct net_device_stats *stats = &global.net_device_stats_from_hw;
        BcmEnet_devctrl *pDevCtrl = (BcmEnet_devctrl *)netdev_priv(dev);
        uint32 rxDropped = 0, txDropped = 0;
        /* reset stats before collecting - in case the called API does not provide anything */
        memset(stats,0,sizeof(*stats));
#ifdef NETDEV_STATS_MASK_ALL
        bcmsw_get_hw_stats(port, extswitch, stats, mask);
#else
        bcmsw_get_hw_stats(port, extswitch, stats);
#endif

#if defined(CONFIG_BCM_SWITCH_PORT_TRUNK_SUPPORT)
        {
            unsigned long trunk_pmap = 0;
            struct net_device_stats *p_stats_2 = &g_stats_2;
            int grp_no = 0;
            if (extswitch) { /* Trunking support only for external switch */
                for (grp_no=0; grp_no < BCM_SW_MAX_TRUNK_GROUPS; grp_no++)
                {
                    if ( (trunk_grp[grp_no] & TRUNK_GRP_PORT_MASK) & (1<<log_port) ) { /* Port is part of trunk group_0 */
                        trunk_pmap = trunk_grp[grp_no] & TRUNK_GRP_PORT_MASK; /* Get the logical pmap */
                        break;
                    }
                }
                trunk_pmap = LOGICAL_PORTMAP_TO_PHYSICAL_PORTMAP(trunk_pmap);
                trunk_pmap &= ~(1<<port); /* Remove the original port for which we already got the stats */
            }
            while( (port = ffs(trunk_pmap)) != 0 ) {
                port--; /* ffs returns 1-based */
                memset(p_stats_2,0,sizeof(*p_stats_2));
#ifdef NETDEV_STATS_MASK_ALL
                bcmsw_get_hw_stats(port, extswitch, p_stats_2, mask);
#else
                bcmsw_get_hw_stats(port, extswitch, p_stats_2);
#endif
                trunk_pmap &= ~(1<<port);
                /* Copy over to cummulative stats */
                bcmenet_add_stats(stats, p_stats_2);
            }
            /* Restore the value of port back to original - in case it gets used later */
            port = LOGICAL_PORT_TO_PHYSICAL_PORT(log_port);
        }
#endif /* !CONFIG_BCM_SWITCH_PORT_TRUNK_SUPPORT */

        /* Add the dropped packets in software */
        stats->rx_dropped += pDevCtrl->stats.rx_dropped;
        stats->tx_dropped += pDevCtrl->stats.tx_dropped;

        bcmeapi_EthGetStats(log_port, &rxDropped, &txDropped);

        stats->rx_dropped += rxDropped;
        stats->tx_dropped += txDropped;
    }
    return &global.net_device_stats_from_hw;
#else
    return &(((BcmEnet_devctrl *)netdev_priv(dev))->stats);
#endif
}

static int bcm63xx_enet_change_mtu(struct net_device *dev, int new_mtu)
{
    int max_mtu = ENET_MAX_MTU_PAYLOAD_SIZE;
    if (new_mtu < ETH_ZLEN || new_mtu > max_mtu)
        return -EINVAL;
    dev->mtu = new_mtu;
#if defined (CONFIG_BCM96838)
    {
        struct ethswctl_data e;
        e.mtu = new_mtu;
        bcmeapi_ioctl_ethsw_mtu_set(&e);
    }
#endif

    return 0;
}


/*
 * handle_link_status_change
 */
void link_change_handler(int port, int cb_port, int linkstatus, int speed, int duplex)
{
    IOCTL_MIB_INFO *mib;
    int mask, vport;
    struct net_device *dev = vnet_dev[0];
    struct net_device *pNetDev;
    BcmEnet_devctrl *priv = (BcmEnet_devctrl *)netdev_priv(dev);
    int linkMask;
    int sw_port;
#if defined(CONFIG_BCM_ETH_PWRSAVE)
    int phyId = 0; // 0 is a valid phy_id but not an external phy_id. So we are OK initializing it to 0.

    if (!IsExternalSwitchPort(port))
    {
        phyId = priv->EnetInfo[0].sw.phy_id[LOGICAL_PORT_TO_PHYSICAL_PORT(port)];
    }

    ethsw_phy_pll_up(0);
#endif
    if (port >= MAX_TOTAL_SWITCH_PORTS)
    {
        return;
    }

    down(&bcm_link_handler_config);

    sw_port  = port;
    vport    = LOGICAL_PORT_TO_VPORT(port);
    mask     = 1 << port;
    /* Boundary condition check */
    /* This boundary check condition will fail for the internal switch port that is connected to
       the external switch as the vport would be -1. This is normal and expected */
    if ( vport >= 0 || vport < (sizeof(vnet_dev)/sizeof(vnet_dev[0])))
        pNetDev  = vnet_dev[vport];
    else
        pNetDev = NULL;
#if defined(BCM_ENET_CB_WAN_PORT_LNX_INTF_SUPPORT)
    if (pNetDev) 
    {
        BcmEnet_devctrl *cb_priv;
        cb_priv = (BcmEnet_devctrl *)netdev_priv(pNetDev);
        if (cb_port == BP_CROSSBAR_NOT_DEFINED && cb_priv->cb_ext_port != BP_CROSSBAR_NOT_DEFINED)
        {   /* This may be a port on LAN */
            cb_port = cb_priv->cb_ext_port;
        }
        if (cb_port != BP_CROSSBAR_NOT_DEFINED && cb_port >= 0 && cb_port < BCMENET_CROSSBAR_MAX_EXT_PORTS)
        {
            vport = cbport_to_vportIdx[cb_port];
            pNetDev  = vnet_dev[vport];
            cb_priv = (BcmEnet_devctrl *)netdev_priv(pNetDev);
            cb_priv->linkState = linkstatus; /* Store the link status in the network device itself */
            printk("vport %s for logical port <%d> cb <%d>\n",pNetDev->name,port,cb_port);

            if (cb_priv->sw_port_id != port)
            {
                printk("ERROR - cannot find vport for logical port <%d>\n",port);
                pNetDev = NULL; /* This will error out later */
            }
        }
    }
#endif /* BCM_ENET_CB_WAN_PORT_LNX_INTF_SUPPORT */
    linkMask = linkstatus << port;
    vport   -= 1; /* -1 for ethsw_set_mac */

    if ( NULL == pNetDev )
    {
        up(&bcm_link_handler_config);
        return;
    }

    if ((priv->linkState & mask) != linkMask) {
        BCM_ENET_LINK_DEBUG("port=%x; vport=%x", port, vport);

        mib = &((BcmEnet_devctrl *)netdev_priv(pNetDev))->MibInfo;
        if (linkstatus) {
#if defined(CONFIG_BCM_ETH_PWRSAVE)
            /* Link is up, so de-isolate the Phy  */
            if (IsExtPhyId(phyId)) {
                ethsw_isolate_phy(phyId, 0);
            }
#endif

#if defined(CONFIG_BCM_SWITCH_PORT_TRUNK_SUPPORT)
            {
                int grp_no = 0;
                for (; grp_no < BCM_SW_MAX_TRUNK_GROUPS; grp_no++)
                {
                    if (trunk_grp[grp_no] & mask) {
                        /* This port is part of LAG Group */
                        if (priv->linkState & trunk_grp[grp_no] & TRUNK_GRP_PORT_MASK) {
                            printk((KERN_CRIT "%s (%s switch port: %d) (Logical Port: %d) Link UP %d mbps %s duplex\n"),
                                    pNetDev->name, (LOGICAL_PORT_TO_UNIT_NUMBER(port)?"Ext":"Int"),LOGICAL_PORT_TO_PHYSICAL_PORT(port), port, speed, duplex?"full":"half");
                            printk((KERN_CRIT "Skipping Link UP - one of other LAG member is already UP <0x%04x>\n"),(unsigned int)(priv->linkState & trunk_grp[grp_no] & TRUNK_GRP_PORT_MASK));
                            priv->linkState |= mask;
                            up(&bcm_link_handler_config);
                            return;
                        }
                    }
                }
            }
#endif /* CONFIG_BCM_SWITCH_PORT_TRUNK_SUPPORT */

            /* Just set a flag for EEE because a 1 second delay is required */
            priv->eee_enable_request_flag[0] |= (1<<sw_port);

            if (speed == 1000)
            {
                mib->ulIfSpeed = SPEED_1000MBIT;
            }
            else if (speed == 100)
            {
                mib->ulIfSpeed = SPEED_100MBIT;
            }
            else if (speed == 200)
            {
                mib->ulIfSpeed = SPEED_200MBIT;
            }
            else
            {
                mib->ulIfSpeed = SPEED_10MBIT;
            }

            bcmeapi_EthSetPhyRate(port, 1, mib->ulIfSpeed, pNetDev->priv_flags & IFF_WANDEV);

/* TCH Modification : Keep the default MTU (1500) set in ether_setup even when Jumbo frames are enabled */
#if 0
#if defined (CONFIG_BCM_JUMBO_FRAME)
    {
            int we_locked_rtnl = FALSE;
            if (!rtnl_is_locked())
            {
                rtnl_lock();
                we_locked_rtnl = TRUE;
            }

            if (speed == 1000) /* When jumbo frame support is enabled - the jumbo MTU is applicable only for 1000M interfaces */
                dev_set_mtu(pNetDev, ENET_MAX_MTU_PAYLOAD_SIZE);
            else
                dev_set_mtu(pNetDev, (ENET_NON_JUMBO_MAX_MTU_PAYLOAD_SIZE));
            if (we_locked_rtnl == TRUE) {
                rtnl_unlock();
            }
    }
#endif
#endif
/* End TCH Modification */

            mib->ulIfLastChange  = (jiffies * 100) / HZ;
            mib->ulIfDuplex = (unsigned long)duplex;
            priv->linkState |= mask;
            bcmeapi_set_mac_speed(port, speed);
            printk((KERN_CRIT "%s (%s switch port: %d) (Logical Port: %d) Link UP %d mbps %s duplex\n"),
                    pNetDev->name, (LOGICAL_PORT_TO_UNIT_NUMBER(port)?"Ext":"Int"),LOGICAL_PORT_TO_PHYSICAL_PORT(port), port, speed, duplex?"full":"half");

            /* notify linux after we have finished setting our internal state */
            if (netif_carrier_ok(pNetDev) == 0)
                netif_carrier_on(pNetDev);
        } else {
#if defined(CONFIG_BCM_ETH_PWRSAVE)
            /* Link is down, so isolate the Phy. To prevent switch rx lockup
               because of packets entering switch with DLL/Clock disabled */
            if (IsExtPhyId(phyId)) {
                ethsw_isolate_phy(phyId, 1);
            }
#endif

#if defined(CONFIG_BCM_SWITCH_PORT_TRUNK_SUPPORT)
            {
                int grp_no=0;
                for (; grp_no < BCM_SW_MAX_TRUNK_GROUPS; grp_no++)
                {
                    if (trunk_grp[grp_no] & mask) {
                        /* This port is part of LAG Group */
                        if ((priv->linkState & ~mask)  & (trunk_grp[grp_no] & TRUNK_GRP_PORT_MASK)) {
                            printk((KERN_CRIT "%s (%s switch port: %d) (Logical Port: %d) Link DOWN.\n"),
                                    pNetDev->name, (LOGICAL_PORT_TO_UNIT_NUMBER(port)?"Ext":"Int"),LOGICAL_PORT_TO_PHYSICAL_PORT(port), port);
                            printk((KERN_CRIT "Skipping Link DN - one of other LAG member is still UP <0x%04x>\n"),(unsigned int)(priv->linkState & ~mask & trunk_grp[grp_no] & TRUNK_GRP_PORT_MASK));
                            priv->linkState &= ~mask;
                            up(&bcm_link_handler_config);
                            return;
                        }
                    }
                }
            }
#endif /* CONFIG_BCM_SWITCH_PORT_TRUNK_SUPPORT */

            if (IsExternalSwitchPort(sw_port))
            {
                extsw_fast_age_port(LOGICAL_PORT_TO_PHYSICAL_PORT(sw_port), 0);
            }
            else
            {
                fast_age_port(LOGICAL_PORT_TO_PHYSICAL_PORT(sw_port), 0);
            }

            mib->ulIfLastChange  = 0;
            mib->ulIfSpeed       = 0;
            mib->ulIfDuplex      = 0;
            priv->linkState &= ~mask;

            bcmeapi_EthSetPhyRate(port, 0, mib->ulIfSpeed, pNetDev->priv_flags & IFF_WANDEV);

            /* Clear any pending request to enable eee and disable it */
            priv->eee_enable_request_flag[0] &= ~(1<<sw_port);
            priv->eee_enable_request_flag[1] &= ~(1<<sw_port);
            BCM_ENET_DEBUG("%s: port %d  disabling EEE\n", __FUNCTION__, sw_port);
#if defined(CONFIG_BCM_GMAC)
            if (IsGmacPort( sw_port ) )
            {
                volatile GmacEEE_t *gmacEEEp = GMAC_EEE;
                gmacEEEp->eeeCtrl.linkUp = 0;
                gmacEEEp->eeeCtrl.enable = 0;
            }
#endif
            ethsw_eee_port_enable(sw_port, 0, 0);

            printk((KERN_CRIT "%s (%s switch port: %d) (Logical Port: %d) Link DOWN.\n"),
                    pNetDev->name, (LOGICAL_PORT_TO_UNIT_NUMBER(port)?"Ext":"Int"),LOGICAL_PORT_TO_PHYSICAL_PORT(port), port);

            /* notify linux after we have finished setting our internal state */
            if (netif_carrier_ok(pNetDev) != 0)
                netif_carrier_off(pNetDev);
        }
    }

    up(&bcm_link_handler_config);
}
/* Is the logical port configured as WAN? */
inline int IsLogPortWan( int log_port )
{
    BcmEnet_devctrl *pDevCtrl;
    pDevCtrl = netdev_priv(vnet_dev[0]);
    ASSERT(pDevCtrl != NULL);

    return ((pDevCtrl->wanPort >> log_port) & 0x1);
}
/*
 * Wrapper function for other Kernel modules to check
 * if a given logical port is WAN or NOT.
 */
static int bcmenet_is_wan_port(void *ctxt)
{
    int logical_port = *((int*)ctxt);
    return IsLogPortWan(logical_port);
}

static int bcmenet_link_might_changed(void)
{
    return force_link_check | bcmeapi_link_might_changed();
}

#if defined(SUPPORT_SWMDK)
static int link_change_handler_wrapper(void *ctxt)
{
    LinkChangeArgs *args = ctxt;

    BCM_ASSERT(args);
    if (args->activeELink) {
        bcmeapi_aelink_handler(args->linkstatus);
    }
    link_change_handler(args->port,
                        BP_CROSSBAR_NOT_DEFINED,
            args->linkstatus,
            args->speed,
            args->duplex);

#if defined(CONFIG_BCM_GMAC)
    if ( IsGmacPort(args->port) && IsLogPortWan(args->port) ) {
        gmac_link_status_changed(GMAC_PORT_ID, args->linkstatus, args->speed,
                args->duplex);
    }
#endif
    return 0;
}

static void bcm63xx_enet_poll_timer(unsigned long arg)
{
    struct net_device *dev = vnet_dev[0];
    BcmEnet_devctrl *priv = (BcmEnet_devctrl *)netdev_priv(dev);
    int i;

    /* */
    daemonize(ETHERNET_ROOT_DEVICE_NAME);
    bcmeapiPhyIntEnable(1);

    /* Enable the Phy interrupts of internal Phys */
    for (i = 0; i < TOTAL_SWITCH_PORTS - 1; i++) {
        if ((priv->EnetInfo[0].sw.port_map) & (1<<i)) {
            if (!IsExtPhyId(priv->EnetInfo[0].sw.phy_id[i])) {
                ethsw_phy_intr_ctrl(i, 1);
            } else {
                global.extPhyMask |= (1 << i);
            }
        }
    }
#if defined(CONFIG_BCM_ENET)
    set_current_state(TASK_INTERRUPTIBLE);
    /* Sleep for 1 tick) */
    schedule_timeout(HZ/100);
#endif
    /* Start with virtual interfaces as down */
    for (i = 1; i <= vport_cnt; i++) {
        if ( vnet_dev[i] != NULL )
        {
            if (netif_carrier_ok(vnet_dev[i]) != 0)
                netif_carrier_off(vnet_dev[i]);
        }
    }


    /* */
    while (atomic_read(&poll_lock) > 0)
    {
        bcmeapi_enet_poll_timer();
        /* reclaim tx descriptors and buffers */
        /*   */
        bcmeapi_update_link_status();
        set_current_state(TASK_INTERRUPTIBLE);

        /* Sleep for HZ jiffies (1sec) */
        schedule_timeout(HZ);
    }

    complete_and_exit(&poll_done, 0);
    printk("bcm63xx_enet_poll_timer: thread exits!\n");
}

#else
/*
 * bcm63xx_enet_poll_timer: reclaim transmit frames which have been sent out
 */
static void bcm63xx_enet_poll_timer(unsigned long arg)
{
    IOCTL_MIB_INFO *mib;
    PHY_STAT phys;
    int linkChanged = 0, oldstat, tmp, mask, i, log_port;
    int phyId, speed, unit, phy_port;
    struct net_device *dev = vnet_dev[0];
    BcmEnet_devctrl *priv = (BcmEnet_devctrl *)netdev_priv(dev);
    int ephy_sleep_delay = 0;

    /* */
    daemonize(ETHERNET_ROOT_DEVICE_NAME);

    /* */
    bcmeapiPhyIntEnable(1);

    /* Enable the Phy interrupts of internal Phys */
    for (i = 0; i < EPHY_PORTS; i++) {
        if ((priv->EnetInfo[0].sw.port_map) & (1<<i)) {
            if (!IsExtPhyId(priv->EnetInfo[0].sw.phy_id[i])) {
                ethsw_phy_intr_ctrl(i, 1);
            }
        }
    }

#if defined(CONFIG_BCM_ENET)
    set_current_state(TASK_INTERRUPTIBLE);
    /* Sleep for 1 tick) */
    schedule_timeout(HZ/100);
#endif
    /* Start with virtual interfaces as down */
    for (i = 1; i <= vport_cnt; i++) {
        if ( vnet_dev[i] != NULL )
        {
            if (netif_carrier_ok(vnet_dev[i]) != 0) {
                netif_carrier_off(vnet_dev[i]);
            }
        }
    }

    /* */
    while (atomic_read(&poll_lock) > 0)
    {
        bcmeapi_enet_poll_timer();

        /* Start with New link status of all vports as 0*/
        oldstat = priv->linkState;

#if defined(CONFIG_BCM_ETH_PWRSAVE)
        ephy_sleep_delay = ethsw_ephy_auto_power_down_wakeup();
#endif

        if((linkChanged = bcmenet_link_might_changed()))
        {
            for (i = 1; i <= vport_cnt; i++)
            {
            	if ( vnet_dev[i] != NULL )
            	{
            		log_port = VPORT_TO_LOGICAL_PORT(i);
            		unit = LOGICAL_PORT_TO_UNIT_NUMBER(log_port);
            		phy_port = LOGICAL_PORT_TO_PHYSICAL_PORT(log_port);
            		phyId = priv->EnetInfo[unit].sw.phy_id[LOGICAL_PORT_TO_PHYSICAL_PORT(log_port)];

#if defined(CONFIG_BCMGPON)
            		/* Skip GPON interface */
            		if(LOGICAL_PORT_TO_PHYSICAL_PORT(log_port) == GPON_PORT_ID) {
            			continue;
            		}
#endif
#if defined(ENET_EPON_CONFIG)
            		/* Skip EPON interface */
            		if(LOGICAL_PORT_TO_PHYSICAL_PORT(log_port) == EPON_PORT_ID) {
            			continue;
            		}
#endif

            		if (!linkChanged)
            			continue;

            		/* Get the MIB of this vport */
            		mib = &((BcmEnet_devctrl *)netdev_priv(vnet_dev[i]))->MibInfo;

            		/* Get the status of Phy connected to switch port i */
            		phys = ethsw_phy_stat(LOGICAL_PORT_TO_UNIT_NUMBER(log_port), LOGICAL_PORT_TO_PHYSICAL_PORT(log_port), BP_CROSSBAR_NOT_DEFINED);

            		/* Mask for this port */
            		mask = (1 << log_port);

            		/* If link is up, set tmp with the mask of this port */
            		tmp = (phys.lnk != 0) ? mask : 0;

            		/* Update the new link status */

            		/* If link status has changed for this switch port i, update
                   the interface status */
            		if ((linkChanged & ETHSW_LINK_MIGHT_CHANGED) ||
            				(priv->linkState & mask) != tmp)
            		{

            			priv->linkState &= ~mask;
            			priv->linkState |= tmp;
            			/* Set the MAC with link/speed/duplex status from Phy */
            			/* Note: The parameter i should be the vport id. The
                       ethsw_set_mac maps it to physical port id */
            			ethsw_set_mac(log_port, phys);

            			/* If Link has changed from down to up, indicate upper layers
                       and print the link status */
            			if (phys.lnk)
            			{
#if defined(CONFIG_BCM_ETH_PWRSAVE)
            				/* Link is up, so de-isolate the Phy  */
            				if (IsExtPhyId(phyId)) {
            					ethsw_isolate_phy(phyId, 0);
            				}
#endif

            				/* Just set a flag for EEE because a 1 second delay is required */
            				priv->eee_enable_request_flag[0] |= mask;

            				if (phys.spd100)
            					mib->ulIfSpeed = SPEED_100MBIT;
#if defined(CONFIG_TECHNICOLOR_GPON_PATCH)
                            else if (phys.spd1000)
                                mib->ulIfSpeed = SPEED_1000MBIT;
		                    else
#else
            				else if (!phys.spd1000)
#endif
            					mib->ulIfSpeed = SPEED_10MBIT;

#if defined(CONFIG_TECHNICOLOR_GPON_PATCH)
                            mib->ulIfDuplex = phys.fdx;
#endif

            				mib->ulIfLastChange  = (jiffies * 100) / HZ;

            				if (phys.spd1000)
            					speed=1000;
            				else if (phys.spd100)
            					speed=100;
            				else
            					speed=10;

            				if (netif_carrier_ok(vnet_dev[i]) == 0)
            				{
            					printk((KERN_CRIT "%s Link UP %d mbps %s duplex\n"),
            							vnet_dev[i]->name, speed, phys.fdx?"full":"half");
            					netif_carrier_on(vnet_dev[i]);
            				}
            			}
            			else
            			{
#if defined(CONFIG_BCM_ETH_PWRSAVE)
            				/* Link is down, so isolate the Phy. To prevent switch rx lockup
                           because of packets entering switch with DLL/Clock disabled */
            				if (IsExtPhyId(phyId)) {
            					ethsw_isolate_phy(phyId, 1);
            				}
#endif
            				/* Clear any pending request to enable eee and disable it */
            				priv->eee_enable_request_flag[0] &= ~mask;
            				priv->eee_enable_request_flag[1] &= ~mask;
            				ethsw_eee_port_enable(VPORT_TO_LOGICAL_PORT(i), 0, 0);

            				mib->ulIfLastChange  = 0;
            				mib->ulIfSpeed       = 0;

            				/* If link has changed from up to down, indicate upper
                           layers and print the 'Link Down' message */
            				if (netif_carrier_ok(vnet_dev[i]) != 0)
            				{
            					printk((KERN_CRIT "%s Link DOWN.\n"), vnet_dev[i]->name);
            					netif_carrier_off(vnet_dev[i]);
            				}

            			}
            		}
            	}
            }
        }

#if defined(CONFIG_BCM_ETH_PWRSAVE)
        ephy_sleep_delay += ethsw_ephy_auto_power_down_sleep();
#endif

        /* Check for delayed request to enable EEE */
        ethsw_eee_process_delayed_enable_requests();

#if (CONFIG_BCM_EXT_SWITCH_TYPE == 53115)
        extsw_apd_set_compatibility_mode();
#endif

        /*   */
        set_current_state(TASK_INTERRUPTIBLE);

        /* Sleep for HZ jiffies (1sec), minus the time that was already */
        /* spent waiting for EPHY PLL  */
        schedule_timeout(HZ - ephy_sleep_delay);
    }

    complete_and_exit(&poll_done, 0);
    printk("bcm63xx_enet_poll_timer: thread exits!\n");
}
#endif
#if defined(CONFIG_BCM_SWITCH_PORT_TRUNK_SUPPORT)

/* --------------------------------------------------------------------------
Name: bcmenet_get_num_active_trunk_grp_ports
Purpose: Returns the current active/linked-up portmap and number
-------------------------------------------------------------------------- */
static inline int bcmenet_get_num_active_trunk_grp_ports(unsigned int *p_log_pmap)
{
    BcmEnet_devctrl *priv = (BcmEnet_devctrl *)netdev_priv(vnet_dev[0]);
    unsigned int pmap = *p_log_pmap, bits_count=0;
    pmap &= priv->linkState;
    *p_log_pmap = pmap;
    while (pmap)
    {
        pmap &= pmap-1; /* reset the least significant bit */
        bits_count++;
    }
    return bits_count;
}
/* --------------------------------------------------------------------------
Name: bcmenet_get_hash
Purpose: Simple Random hash based on MAC DA and SA
-------------------------------------------------------------------------- */
static inline unsigned int bcmenet_get_hash(unsigned char *pDa, unsigned char *pSa)
{
#if 0
    unsigned int hash_val = (unsigned int)(pDa[0]) + (unsigned short)(pDa[4]) +
                            (unsigned int)(pSa[0]) + (unsigned short)(pSa[4]);
    hash_val ^= ( hash_val >> 16 );
    hash_val ^= ( hash_val >>  8 );
    hash_val ^= ( hash_val >>  3 );

    return ( hash_val );
#else
    return pDa[5]+pSa[5];
#endif
}
/* --------------------------------------------------------------------------
Name: bcmenet_load_balance
Purpose: Given a logical port returns the port number from one of the trunk group
         member to send the traffic based on load balancing
-------------------------------------------------------------------------- */
static inline uint16 bcmenet_load_balance(uint16 port_id, void* pHdr)
{
    int grp_no=-1; /* Init with -1 to get incremented to zero to start */
    unsigned int log_pmap = (1<< port_id) & TRUNK_GRP_PORT_MASK;

    if ( (log_pmap & trunk_grp[++grp_no]) || /* Part of first trunk group */
         (log_pmap & trunk_grp[++grp_no]) )  /* Part of second trunk group */
    {
        unsigned char *pDa = ((BcmEnet_hdr *)pHdr)->da;
        unsigned char *pSa = ((BcmEnet_hdr *)pHdr)->sa;
        unsigned int hash_value=0, port_to_use=0;
        int num_active_ports = 0;
        log_pmap = trunk_grp[grp_no] & TRUNK_GRP_PORT_MASK; /* Take all ports of trunk into account */
        num_active_ports = bcmenet_get_num_active_trunk_grp_ports(&log_pmap);
        hash_value = bcmenet_get_hash(pDa, pSa);
        port_to_use = hash_value%num_active_ports;
        BCM_ENET_TX_INFO("Port=%d Num active ports = %d ; Active pmap 0x%08X Hash 0x%08X port_to_use=%d\n",
                         port_id, num_active_ports,log_pmap,hash_value,port_to_use);
        while( (port_id = ffs(log_pmap)) != 0 ) {
            port_id--; /* ffs returns 1-based */
            if (port_to_use == 0)
            {
                break;
            }
            log_pmap &= ~(1<<port_id); /* reset port */
            port_to_use--;
        }
    }
    return port_id;
}
#else
#define bcmenet_load_balance(port_id, pHdr) port_id
#endif /* CONFIG_BCM_SWITCH_PORT_TRUNK_SUPPORT */

static struct sk_buff *bcm63xx_skb_put_tag(struct sk_buff *skb,
        struct net_device *dev, unsigned int port_map)
{
    BcmEnet_hdr *pHdr = (BcmEnet_hdr *)skb->data;
    int i, headroom;
    int tailroom;

    if (ntohs(pHdr->brcm_type) == BRCM_TYPE2)
    {
        headroom = 0;
        tailroom = ETH_ZLEN + BRCM_TAG_TYPE2_LEN - skb->len;
    }
    else
    {
        headroom = BRCM_TAG_TYPE2_LEN;
        tailroom = ETH_ZLEN - skb->len;
    }

    if (tailroom < 0)
    {
        tailroom = 0;
    }

#if defined(CONFIG_BCM_USBNET_ACCELERATION)
    if ((skb_writable_headroom(skb) < headroom) || (skb_tailroom(skb) < tailroom))
#else
    if ((skb_headroom(skb) < headroom) || (skb_tailroom(skb) < tailroom))
#endif
    {
        struct sk_buff *oskb = skb;
        skb = skb_copy_expand(oskb, headroom, tailroom, GFP_ATOMIC);
        kfree_skb(oskb);
        if (!skb)
        {
            return NULL;
        }
    }
#if defined(CONFIG_BCM_USBNET_ACCELERATION)
    else if ((headroom != 0) && (skb->clone_wr_head == NULL))
#else
    else if ((headroom != 0) && !(skb_clone_writable(skb, headroom)))
#endif
    {
        skb = skb_unshare(skb, GFP_ATOMIC);
        if (!skb)
        {
            return NULL;
        }
    }

    if (tailroom > 0)
    {
        if (skb_is_nonlinear(skb))
        {
            /* Non linear skb whose skb->len is < minimum Ethernet Packet Length
               (ETHZLEN or ETH_ZLEN + BroadcomMgmtTag Length) */
            if (skb_linearize(skb))
            {
                return NULL;
            }
        }
        memset(skb->data + skb->len, 0, tailroom);  /* padding to 0 */
        skb_put(skb, tailroom);
    }

    if (headroom != 0)
    {
        uint16 *to, *from;
        BcmEnet_hdr2 *pHdr2 = (BcmEnet_hdr2 *)skb_push(skb, headroom);
        to = (uint16*)pHdr2;
        from = (uint16*)(skb->data + headroom);
        for ( i=0; i<ETH_ALEN; *to++ = *from++, i++ ); /* memmove 2 * ETH_ALEN */
        /* set ingress brcm tag and TC bit */
        pHdr2->brcm_type = htons(BRCM_TAG2_EGRESS | (SKBMARK_GET_Q_PRIO(skb->mark) << 10));
        pHdr2->brcm_tag  = htons(port_map);
    }
    return skb;
}

static inline void bcm63xx_fkb_put_tag(FkBuff_t * fkb_p,
        struct net_device * dev, unsigned int port_map)
{
    int i;
    int tailroom, crc_len = 0;
    uint16 *from = (uint16*)fkb_p->data;
    BcmEnet_hdr2 *pHdr = (BcmEnet_hdr2 *)from;

    if (ntohs(pHdr->brcm_type) != BRCM_TYPE2)
    {
        uint16 * to = (uint16*)fkb_push(fkb_p, BRCM_TAG_TYPE2_LEN);
        pHdr = (BcmEnet_hdr2 *)to;
        for ( i=0; i<ETH_ALEN; *to++ = *from++, i++ ); /* memmove 2 * ETH_ALEN */
        /* set port of ingress brcm tag */
        pHdr->brcm_tag = htons(port_map);

    }
    /* set ingress brcm tag and TC bit */
    pHdr->brcm_type = htons(BRCM_TAG2_EGRESS | (SKBMARK_GET_Q_PRIO(fkb_p->mark) << 10));
    tailroom = ETH_ZLEN + BRCM_TAG_TYPE2_LEN - fkb_p->len;
    tailroom = (tailroom < 0) ? crc_len : crc_len + tailroom;
    fkb_put(fkb_p, tailroom);
}

#if defined(CONFIG_BCM_6802_MoCA)
/* Will be removed once bmoca-6802.c remove the calling */
void bcmenet_register_moca_fc_bits_cb(void (*cb)(void *, unsigned long *), int isWan, void * arg)
{
    return;
}
EXPORT_SYMBOL(bcmenet_register_moca_fc_bits_cb);
#endif


#if defined(ENET_GPON_CONFIG)

#define get_first_gemid_to_ifIdx_mapping(gemId) gem_to_gponifid[gemId][0]

#if defined(DEFINE_ME_TO_USE)
static void print_gemid_to_ifIdx_map(const char *pMark, int gemId)
{
    int ifIdx;
    unsigned int map1 = 0;
    unsigned int map2 = 0;

    for (ifIdx = 0; ifIdx < MAX_GPON_IFS_PER_GEM; ++ifIdx)
    {
        if (gem_to_gponifid[gemId][ifIdx] != UNASSIGED_IFIDX_VALUE)
        { /* Occupied */
            if (ifIdx > 31)
            {
                map2 |= (1<< (ifIdx-32));
            }
            else
            {
                map1 |= (1<<ifIdx);
            }
        }
    }
    if (map1 || map2)
    {
        printk("%s : GEM = %02d Map1 = 0x%08X Map2 = 0x%08X\n",pMark,gemId,map1,map2);
    }
}

static int get_next_gemid_to_ifIdx_mapping(int gemId, int ifId)
{
    int ifIdx;
    if (ifId == UNASSIGED_IFIDX_VALUE)
    { /* get first */
        return get_first_gemid_to_ifIdx_mapping(gemId);
    }
    for (ifIdx = 0; ifIdx < MAX_GPON_IFS_PER_GEM; ++ifIdx)
    {
        if (gem_to_gponifid[gemId][ifIdx] == UNASSIGED_IFIDX_VALUE)
        {
            break;
        }
        if ((ifIdx+1 < MAX_GPON_IFS_PER_GEM) &&
                (gem_to_gponifid[gemId][ifIdx] == ifId))
        {
            return gem_to_gponifid[gemId][ifIdx+1];
        }
    }
    return UNASSIGED_IFIDX_VALUE;
}
#endif /* DEFINE_ME_TO_USE */

static void initialize_gemid_to_ifIdx_mapping(void)
{
    int gemId,ifIdx;
    for (gemId = 0; gemId < MAX_GEM_IDS; ++gemId)
    {
        for (ifIdx = 0; ifIdx < MAX_GPON_IFS_PER_GEM; ++ifIdx)
        {
            gem_to_gponifid[gemId][ifIdx] = UNASSIGED_IFIDX_VALUE;
        }
    }
}
static int add_gemid_to_gponif_mapping(int gemId, int ifId)
{
    int ifIdx;
    for (ifIdx = 0; ifIdx < MAX_GPON_IFS_PER_GEM; ++ifIdx)
    {
        if (gem_to_gponifid[gemId][ifIdx] == UNASSIGED_IFIDX_VALUE)
        { /* Empty */
            gem_to_gponifid[gemId][ifIdx] = ifId;
            return 0;
        }
    }
    printk("Out of resources !! No more ifs available for gem<%d>\n",gemId);
    return -1;
}
static void remove_gemid_to_gponif_mapping(int gemId, int ifId)
{
    int idx;
    int usedIdx;
    int moveIdx;
    for (idx = 0; idx < MAX_GPON_IFS_PER_GEM; ++idx)
    {
        if (gem_to_gponifid[gemId][idx] != UNASSIGED_IFIDX_VALUE)
        { /* Occupied */
            if (gem_to_gponifid[gemId][idx] == ifId)
            {
                /* Remove the mapping */
                gem_to_gponifid[gemId][idx] = UNASSIGED_IFIDX_VALUE;
                moveIdx = idx;
                for (usedIdx = idx+1; usedIdx < MAX_GPON_IFS_PER_GEM; ++usedIdx)
                {
                    if (gem_to_gponifid[gemId][usedIdx] != UNASSIGED_IFIDX_VALUE)
                    {
                        gem_to_gponifid[gemId][moveIdx] = gem_to_gponifid[gemId][usedIdx];
                        gem_to_gponifid[gemId][usedIdx] = UNASSIGED_IFIDX_VALUE;
                        moveIdx = usedIdx;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
        else
        {
            break;
        }
    }
}
static BOOL is_gemid_mapped_to_gponif(int gemId, int ifId)
{
    int ifIdx;
    for (ifIdx = 0; ifIdx < MAX_GPON_IFS_PER_GEM; ++ifIdx)
    {
        if (gem_to_gponifid[gemId][ifIdx] == UNASSIGED_IFIDX_VALUE)
        {
            break;
        }
        if (gem_to_gponifid[gemId][ifIdx] == ifId)
        {
            return TRUE;
        }
    }
    return FALSE;
}
#endif /* defined(ENET_GPON_CONFIG) */

#if defined(ENET_GPON_CONFIG) && defined(SUPPORT_HELLO)
static int dropped_xmit_port = 0;
/* --------------------------------------------------------------------------
Name: bcm63xx_enet_xmit_port
Purpose: Send ethernet traffic given a port number
-------------------------------------------------------------------------- */
static int bcm63xx_enet_xmit_port(struct sk_buff * skb, int xPort)
{
    struct net_device * dev;

    BCM_ENET_TX_DEBUG("Send<0x%08x> len=%d xport<%d> "
            "mark<0x%08x> destQ<%d> gemId<%d> channel<%d>\n",
            (int)skb, skb->len, xPort, skb->mark,
            SKBMARK_GET_Q_PRIO(skb->mark), SKBMARK_GET_PORT(skb->mark),
            SKBMARK_GET_Q_CHANNEL(skb->mark));

    //DUMP_PKT(skb->data, skb->len);

    if (xPort == GPON_PORT_ID) {
        int gemid, gponifid;
        gemid = SKBMARK_GET_PORT(skb->mark);
        gponifid = get_first_gemid_to_ifIdx_mapping(gemid);
        BCM_ENET_INFO("gponifid<%d> \n ", gponifid);
        if ( gponifid < 0 )
        {
            BCM_ENET_INFO("-ve gponifid, dropping pkt \n");
            dropped_xmit_port++;
            return -1;
        }
        dev = gponifid_to_dev[gponifid];
        if ( dev == (struct net_device*)NULL )
        {
            dropped_xmit_port++;
            return -1;
        }
        BCM_ENET_INFO("dev<0x%08x:%s>\n", (int)dev, dev->name);
    }
    else
    {
        dev = vnet_dev[ LOGICAL_PORT_TO_VPORT(xPort) ];
    }

    return bcm63xx_enet_xmit( SKBUFF_2_PNBUFF(skb), dev );
}
#endif  /* defined(SUPPORT_HELLO) */

/* --------------------------------------------------------------------------
Name: bcm63xx_enet_xmit
Purpose: Send ethernet traffic
-------------------------------------------------------------------------- */
static int bcm63xx_enet_xmit(pNBuff_t pNBuff, struct net_device *dev)
{
    bool is_chained = FALSE;
#if defined(PKTC)
    pNBuff_t pNBuff_next = NULL;
#endif
    EnetXmitParams param = { {0}, 0};
    int ret;

#if defined(PKTC)
    /* for PKTC, pNBuff is chained skb */
    if (IS_SKBUFF_PTR(pNBuff))
    {
        is_chained = PKTISCHAINED(pNBuff);
    }
#endif

    param.pDevPriv = netdev_priv(dev);
    param.vstats   = &param.pDevPriv->stats;
    param.port_id  = port_id_from_dev(dev);
    param.is_chained = is_chained;
    param.vstats   = &param.pDevPriv->stats;
    param.pDevPriv->estats.tx_enet_xmit++;
    do {
        param.pNBuff = pNBuff;
        BCM_ENET_TX_DEBUG("The physical port_id is %d\n", param.port_id);

        if (nbuff_get_params_ext(pNBuff, &param.data, &param.len,
                    &param.mark, &param.priority, &param.r_flags) == NULL)
        {
            global.pVnetDev0_g->stats.tx_dropped++;
            param.vstats->tx_dropped++;
            param.pDevPriv->estats.tx_dropped_bad_nbuff++;
            return 0;
        }

        if (global.dump_enable)
            DUMP_PKT(param.data, param.len);

#if defined(CONFIG_BCM96328) || defined(CONFIG_BCM96362) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM96828) || defined(CONFIG_BCM96318) || defined(CONFIG_BCM63381) || defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
        if (!is_chained && IS_SKBUFF_PTR(pNBuff))
        {
            struct sk_buff *skb = PNBUFF_2_SKBUFF(pNBuff);        
            int hiPrioFlag=0;
            check_arp_lcp_pkt(skb->data, hiPrioFlag);
            if (hiPrioFlag)
            {
                /* Give the highest possible priority to ARP and LCP packets */
                skb->mark = param.mark = SKBMARK_SET_Q_PRIO(param.mark, MAX_PRIORITY_VALUE);
            }
        }   
#endif

#ifdef USE_DEFAULT_EGRESS_QUEUE
        if (param.pDevPriv->use_default_txq)
        {
            BCM_ENET_TX_DEBUG("Using default egress queue %d \n", param.egress_queue);
            param.egress_queue = SKBMARK_GET_Q_PRIO(param.mark);
            bcmeapi_select_tx_def_queue(&param);
        }
        else
#endif
        {
            BCM_ENET_TX_DEBUG("Using mark for channel and queue \n");
            param.egress_queue = SKBMARK_GET_Q_PRIO(param.mark);
            bcmeapi_select_tx_nodef_queue(&param);
        }

        BCM_ENET_TX_DEBUG("The egress queue is %d \n", param.egress_queue);

#if defined(PKTC)
        if (is_chained)
            pNBuff_next = PKTCLINK(pNBuff);
#endif

        ret = bcm63xx_enet_xmit2(dev, &param);

#if defined(PKTC)
        if (is_chained)
        {
            pNBuff = pNBuff_next;
	        param.pDevPriv->estats.tx_enet_xmit_extra_chained_nbuff++;
        }
#endif

    } while (is_chained && pNBuff && IS_SKBUFF_PTR(pNBuff));

    return ret;
}

static inline int bcm63xx_enet_xmit2(struct net_device *dev, EnetXmitParams *pParam)
{
    unsigned int port_map ; 
    /* Below we change the Egress port based on load balancing for turnk/LAG ports.
     * Assumption is the port_id parameter going forward is just used to direct the 
     * traffic to appropriate egress port and is not used to reference anything else.
     * Flow-cache and HW accelerators would also work based on this (new/changed) port_id. */

    /* When trunking/LAG is not used --  compiler will turn the below in NULL statement*/
     pParam->port_id = bcmenet_load_balance(pParam->port_id, pParam->data); 
    port_map = (1 << pParam->port_id);
    pParam->gemid = -1;
    /* tx request should never be on the bcmsw interface */
    BCM_ASSERT_R((dev != vnet_dev[0]), 0);

    if(IS_FKBUFF_PTR(pParam->pNBuff))
    {
        pParam->pFkb = PNBUFF_2_FKBUFF(pParam->pNBuff);
    }
    else
    {
        pParam->skb = PNBUFF_2_SKBUFF(pParam->pNBuff);
    }
    bcmeapi_get_tx_queue(pParam);

#if defined(ENET_GPON_CONFIG)
    /* If GPON port, get the gemid from mark and put it in the BD */
    if (LOGICAL_PORT_TO_PHYSICAL_PORT(pParam->port_id) == GPON_PORT_ID) {  /* port_id is logical */
        BcmEnet_devctrl *pDevCtrl = pParam->pDevPriv;
        int gemid = -1;

        switch (pDevCtrl->gem_count) {
            case 0:
                BCM_ENET_TX_DEBUG("No gem_ids, so dropping Tx \n");
                pParam->pDevPriv->estats.tx_dropped_no_gem_ids++;
                goto drop_exit;

            case 1:
                BCM_ENET_TX_DEBUG("Using the only gem_id\n");
                gemid = default_gemid[pDevCtrl->gponifid];
                break;

            default:
                BCM_ENET_TX_DEBUG("2 or more gem_ids, get gem_id from mark\n");
                gemid = SKBMARK_GET_PORT(pParam->mark);
                if (get_first_gemid_to_ifIdx_mapping(gemid) != pDevCtrl->gponifid) {
                    BCM_ENET_TX_DEBUG("The given gem_id is not associated with"
                                     " the given interface\n");
                    pParam->pDevPriv->estats.tx_dropped_bad_gem_id++;
                    goto drop_exit;
                }
                break;
        }
        BCM_ENET_TX_DEBUG("port_id<%d> gem_count<%d> gemid<%d>\n", pParam->port_id,
                pDevCtrl->gem_count, gemid );
        /* gemid is set later in bcmPktDma implementation of enet driver */
        pParam->blog_chnl = gemid;
        pParam->blog_phy  = BLOG_GPONPHY;
        pParam->gemid = gemid;
    }
    else    /* ! GPON_PORT_ID */
#endif
#if defined(ENET_EPON_CONFIG)
        if (LOGICAL_PORT_TO_PHYSICAL_PORT(pParam->port_id) == EPON_PORT_ID)  /* use always wan_flow=1 for epon upstream data traffic */
        {
            int gemid = SKBMARK_GET_PORT(pParam->mark);
            pParam->blog_chnl = gemid;
            pParam->blog_phy = BLOG_EPONPHY;
        }
        else
#endif
#ifdef RDPA_VPORTS
        if (pParam->port_id == SID_PORT_ID)
        {
            pParam->channel = pParam->blog_chnl = netdev_path_get_hw_port(dev);
            pParam->blog_phy = BLOG_SIDPHY;
        }
        else
#endif
        {
            pParam->blog_chnl = pParam->port_id;
            pParam->blog_phy  = BLOG_ENETPHY;
        }

#ifdef CONFIG_BLOG
    /*
     * Pass to blog->fcache, so it can construct the customized
     * fcache based execution stack.
     */
    if (pParam->is_chained == FALSE)
    {
        blog_emit( pParam->pNBuff, dev, TYPE_ETH, pParam->blog_chnl, pParam->blog_phy ); /* CONFIG_BLOG */
    }
#endif

    bcmeapi_buf_reclaim(pParam);

    if(bcmeapi_queue_select(pParam) == BCMEAPI_CTRL_BREAK)
    {
        pParam->pDevPriv->estats.tx_dropped_no_lowlvl_resource++;
        goto unlock_drop_exit;
    }

    bcmeapi_config_tx_queue(pParam);

    if (IsExternalSwitchPort(pParam->port_id))
    {
        if ( pParam->pFkb ) {
            FkBuff_t * pFkbOrig = pParam->pFkb;
            pParam->pFkb = fkb_unshare(pFkbOrig);

            if (pParam->pFkb == FKB_NULL)
            {
                fkb_free(pFkbOrig);
                global.pVnetDev0_g->stats.tx_dropped++;
                pParam->vstats->tx_dropped++;
                pParam->pDevPriv->estats.tx_dropped_no_fkb++;
                goto unlock_exit;
            }
            bcm63xx_fkb_put_tag(pParam->pFkb, dev, GET_PORTMAP_FROM_LOGICAL_PORTMAP(port_map, 1)); /* Portmap for external switch */
            pParam->data = pParam->pFkb->data;
            pParam->len  = pParam->pFkb->len;
            pParam->pNBuff = PBUF_2_PNBUFF((void*)pParam->pFkb,FKBUFF_PTR);
        } else {
#if !defined(CONFIG_BCM96838) && !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM963148)  /* FIXME: Temporary work around */
            ENET_TX_UNLOCK();
#endif
            pParam->skb = bcm63xx_skb_put_tag(pParam->skb, dev, GET_PORTMAP_FROM_LOGICAL_PORTMAP(port_map,1));    /* Portmap for external switch and also pads to 0 */
#if !defined(CONFIG_BCM96838) && !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM963148)  /* FIXME: Temporary work around */
            ENET_TX_LOCK();
#endif
            if (pParam->skb == NULL) {
                global.pVnetDev0_g->stats.tx_dropped++;
                pParam->vstats->tx_dropped++;
                pParam->pDevPriv->estats.tx_dropped_no_skb++;
                goto unlock_exit;
            }
            pParam->data = pParam->skb->data;   /* Re-encode pNBuff for adjusted data and len */
            pParam->len  = pParam->skb->len;
            pParam->pNBuff = PBUF_2_PNBUFF((void*)pParam->skb,SKBUFF_PTR);
        }

    }

    if ( pParam->len < ETH_ZLEN )
    {
        pParam->len = ETH_ZLEN;
    }


#if defined(CONFIG_BCM96368)
    if(kerSysGetSdramWidth() == MEMC_16BIT_BUS)
    {
        pParam->pNBuff = nbuff_align_data(pParam->pNBuff, &pParam->data, pParam->len, NBUFF_ALIGN_MASK_8);
        if(pParam->pNBuff == NULL)
        {
            global.pVnetDev0_g->stats.tx_dropped++;
            pParam->vstats->tx_dropped++;
            pParam->pDevPriv->estats.tx_dropped_misaligned_nbuff++;
            goto unlock_exit;
        }
    }
#endif

    switch(bcmeapi_pkt_xmt_dispatch(pParam))
    {
#if defined(ENET_GPON_CONFIG)
        case BCMEAPI_CTRL_BREAK:
#if !defined(CONFIG_BCM96838) && !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM963148)  /* FIXME: Temporary work around */
            ENET_TX_UNLOCK();
#endif
            goto drop_exit;
#endif
        case BCMEAPI_CTRL_SKIP:
            goto unlock_drop_exit;
        default:
            break;
    }

#ifdef DYING_GASP_API
    /* If in dying gasp, abort housekeeping since we're about to power down */
    if(dg_in_context)
        return 0;
#endif

    /* update stats */
    pParam->vstats->tx_bytes += pParam->len + ETH_CRC_LEN;
    pParam->vstats->tx_packets++;
    pParam->pDevPriv->estats.tx_packets_queue[pParam->egress_queue]++;

    global.pVnetDev0_g->stats.tx_bytes += pParam->len + ETH_CRC_LEN;
    global.pVnetDev0_g->stats.tx_packets++;
    global.pVnetDev0_g->dev->trans_start = jiffies;

unlock_exit:
    bcmeapi_xmit_unlock_exit_post(pParam);
    return 0;

unlock_drop_exit:
    global.pVnetDev0_g->stats.tx_dropped++;
    pParam->vstats->tx_dropped++;
    bcmeapi_xmit_unlock_drop_exit_post(pParam);
    return 0;

#if defined(ENET_GPON_CONFIG)
drop_exit:
    global.pVnetDev0_g->stats.tx_dropped++;
    pParam->vstats->tx_dropped++;
    nbuff_flushfree(pParam->pNBuff);
    return 0;
#endif
}



#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 1)
/* Driver for kernel 3.4 uses a dedicated thread for rx processing */
static int bcm63xx_enet_rx_thread(void *arg)
{
    struct BcmEnet_devctrl *pDevCtrl=(struct BcmEnet_devctrl *) arg;
    uint32 work_done;
    uint32 ret_done;
    int budget = NETDEV_WEIGHT;


    while (1)
    {
        wait_event_interruptible(pDevCtrl->rx_thread_wqh,
                pDevCtrl->rx_work_avail);

        if (kthread_should_stop())
        {
            printk(KERN_INFO "kthread_should_stop detected on bcmsw-rx\n");
            break;
        }

        local_bh_disable();
        /* got some work to do */
        bcmeapi_update_rx_queue(pDevCtrl);

        work_done = bcm63xx_rx(pDevCtrl, budget);
        ret_done = work_done & ENET_POLL_DONE;
        work_done &= ~ENET_POLL_DONE;
        local_bh_enable();

        BCM_ENET_RX_DEBUG("Work Done: %d \n", (int)work_done);

        if (work_done >= budget || ret_done != ENET_POLL_DONE)
        {
            bcmeapi_napi_leave(pDevCtrl);

            /* We have either exhausted our budget or there are
               more packets on the DMA (or both).  Yield CPU to allow
               others to have a chance, then continue to top of loop for more
               work.  */
            if (current->policy == SCHED_FIFO || current->policy == SCHED_RR)
                yield();
        }
        else
        {
            /*
             * No more packets.  Indicate we are done (rx_work_avail=0) and
             * re-enable interrupts (bcmeapi_napi_post) and go to top of
             * loop to wait for more work.
             */
            pDevCtrl->rx_work_avail = 0;
            bcmeapi_napi_post(pDevCtrl);
        }
    }

    return 0;
}
#else
/* Driver for kernel 2.6.30 uses NAPI. */
static int bcm63xx_enet_poll_napi(struct napi_struct *napi, int budget)
{
    struct BcmEnet_devctrl *pDevCtrl = container_of(napi, struct BcmEnet_devctrl, napi);

    uint32 work_done;
    uint32 ret_done;

    bcmeapi_update_rx_queue(pDevCtrl);

    work_done = bcm63xx_rx(pDevCtrl, budget);
    ret_done = work_done & ENET_POLL_DONE;
    work_done &= ~ENET_POLL_DONE;

    BCM_ENET_RX_DEBUG("Work Done: %d \n", (int)work_done);

    if (work_done == budget || ret_done != ENET_POLL_DONE)
    {
        bcmeapi_napi_leave(pDevCtrl);
        /* We have either exhausted our budget or there are
           more packets on the DMA (or both).  Simply
           return, and the framework will reschedule
           this function automatically */
        return work_done;
    }

    /* we are done processing packets */

    napi_complete(napi);
    bcmeapi_napi_post(pDevCtrl);
    return work_done;
}
#endif  /* LINUX_VERSION_CODE */

static inline struct net_device *phyPortId_to_netdev(int logical_port, int gemid)
{
    struct net_device *dev = NULL;
#if defined(ENET_GPON_CONFIG)
    /* If packet is from GPON port, get the gemid and find the gpon virtual
       interface with which that gemid is associated */
    if ( logical_port == GPON_PORT_ID)
    {
        int gponifid;

        gponifid = get_first_gemid_to_ifIdx_mapping(gemid);
        if (gponifid >= 0)
        {
            dev = gponifid_to_dev[gponifid];
        }

    } else
#endif
#ifdef RDPA_VPORTS
        if (logical_port == SID_PORT_ID)
            dev = rdpa_vport_to_dev[gemid]; /* gemid representing rx rdpa_if */
        else
#endif
#if defined(ENET_EPON_CONFIG)
        /* ************************************************************************
         * If packet is from EPON port, get the link index and find the epon virtual
         * interface with which corresponds to the link
         **************************************************************************/
        if ( logical_port == EPON_PORT_ID)
        {
            dev = eponifid_to_dev[0];
        } else
#endif
        {
            int vport;
            /* LAN or ETH-WAN ports */
            vport = LOGICAL_PORT_TO_VPORT(logical_port);
#if defined(BCM_ENET_CB_WAN_PORT_LNX_INTF_SUPPORT)
            {
                int cb_port = enet_get_current_cb_port(logical_port);
                if (cb_port != BP_CROSSBAR_NOT_DEFINED && cb_port >= 0 && cb_port < BCMENET_CROSSBAR_MAX_EXT_PORTS) {
                    vport = cbport_to_vportIdx[cb_port];
                }
            }
#endif
            BCM_ENET_RX_DEBUG("logical_port=%d vport=%d\n", (int)logical_port, vport);

            if ((vport > 0) && (vport <= vport_cnt))
            {
                dev = vnet_dev[vport];
            }

        }
        return dev;
}

/*
 *  bcm63xx_rx: Process all received packets.
 */
static uint32 bcm63xx_rx(void *ptr, uint32 budget)
{
    BcmEnet_devctrl *pDevCtrl = ptr;
    struct net_device *dev = NULL;
    unsigned char *pBuf = NULL;
    struct sk_buff *skb = NULL;
    int len=0, phy_port_id = -1, no_stat = 0, ret;
    uint32 rxpktgood = 0, rxpktprocessed = 0;
    uint32 rxpktmax = budget + (budget / 2);
    struct net_device_stats *vstats;
    struct enet_device_stats *estats;
    FkBuff_t * pFkb = NULL;
    uint32_t blog_chnl, blog_phy; /* used if CONFIG_BLOG enabled */
    uint32 cpuid=0;  /* initialize to silence compiler.  It is correctly set at runtime */
    uint32 rxContext1 = 0; /* Dummy variable used to hold the value returned from called function -
                              MUST not be changed from caller */
    int is_wifi_port = 0;
    int rxQueue;
    int gemid = 0;
#if defined(CONFIG_BLOG)
    BlogAction_t blogAction;
#endif
    /* bulk blog locking optimization only used in SMP builds */
    int got_blog_lock=0;

    // TBD -- this can be looked into but is not being done for now
    /* When the Kernel is upgraded to 2.6.24 or above, the napi call will
       tell you the received queue to be serviced. So, loop across queues
       can be removed. */
    /* RR loop across channels until either no more packets in any channel or
       we have serviced budget number of packets. The logic is to keep the
       channels to be serviced in next_channel array with channels_tbd
       tracking the number of channels that still need to be serviced. */
    for(bcmeapi_prepare_rx(); --budget > 0 && (rxpktgood & ENET_POLL_DONE) == 0; dev = NULL, pBuf = NULL, skb = NULL)
    {

        /* as optimization on SMP, hold blog lock across multiple pkts */
        /* must grab blog_lock before enet_rx_lock */
        if (!got_blog_lock)
        {
            blog_lock();
            got_blog_lock=1;
            /*
             * Get the processor id AFTER we acquire the blog_lock.
             * Holding a lock disables preemption and migration, so we know
             * the processor id will not change as long as we hold the lock.
             */
            cpuid = smp_processor_id();
        }

        /* as optimization on SMP, hold rx lock across multiple pkts */
        if (0 == BULK_RX_LOCK_ACTIVE())
        {
            ENET_RX_LOCK();
            RECORD_BULK_RX_LOCK();
        }

        ret = bcmeapi_rx_pkt(pDevCtrl, &pBuf, &pFkb, &len, &gemid, &phy_port_id, &is_wifi_port, &dev, &rxpktgood, &rxContext1, &rxQueue);

#if !(defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148))
        if(ret & BCMEAPI_CTRL_FLAG_TRUE)
        {
            no_stat = 1;
            ret &= ~BCMEAPI_CTRL_FLAG_MASK;
        }
        else
        {
            no_stat = 0;
        }
#endif

        if(ret != BCMEAPI_CTRL_TRUE)
        {
            switch(ret)
            {
                case BCMEAPI_CTRL_BREAK:
                    goto after_rx;
                case BCMEAPI_CTRL_CONTINUE:
                    goto next_rx;
                case BCMEAPI_CTRL_SKIP:
                    continue;
                default:
                    break;
            }
        }

        BCM_ENET_RX_DEBUG("Processing Rx packet");
        rxpktprocessed++;


        if (!is_wifi_port)/*TODO remove is_wifi_port from enet driver*/
        {
            dev = phyPortId_to_netdev(phy_port_id, gemid);
        }

        if(dev == NULL)
        {
            /* possibility of corrupted source port in dmaFlag */
            if (!no_stat)
                pDevCtrl->stats.rx_dropped++;

            pDevCtrl->estats.rx_dropped_no_rxdev++;
            RECORD_BULK_RX_UNLOCK();
            ENET_RX_UNLOCK();
            bcmeapi_kfree_buf_irq(pDevCtrl, pFkb, pBuf);
            BCM_ENET_INFO("ETH Rcv: Pkt with invalid phy_port_id/vport(0x%x/0x%x) or gemid = 0x%x\n",
                    phy_port_id, LOGICAL_PORT_TO_VPORT(phy_port_id), gemid);
            goto next_rx;
        }

        vstats = &(((BcmEnet_devctrl *) netdev_priv(dev))->stats);
        estats = &(((BcmEnet_devctrl *) netdev_priv(dev))->estats);
        if (!no_stat)
        {
            /* Store packet & byte count in our portion of the device structure */
            vstats->rx_packets ++;
            vstats->rx_bytes += len;
            estats->rx_packets_queue[rxQueue]++;

            /* Store packet & byte count in switch structure */
            pDevCtrl->stats.rx_packets++;
            pDevCtrl->stats.rx_bytes += len;
        }


#if defined(ENET_GPON_CONFIG)
        if (phy_port_id == GPON_PORT_ID) {
            blog_chnl = gemid;      /* blog rx channel is gemid */
            blog_phy = BLOG_GPONPHY;/* blog rx phy type is GPON */
        }
        else
#endif
#if defined(ENET_EPON_CONFIG)
            if (phy_port_id == EPON_PORT_ID) {
                blog_chnl = gemid;
                blog_phy = BLOG_EPONPHY;/* blog rx phy type is EPON */
            }
            else
#endif
#ifdef RDPA_VPORTS
            if (phy_port_id == SID_PORT_ID)
            {
                blog_chnl = netdev_path_get_hw_port(dev);
                blog_phy = BLOG_SIDPHY;
            }
            else
#endif
            {
#if 1 /*TODO remove is_wifi_port */
                if (is_wifi_port)
                {
                    blog_chnl = 0;
                    blog_phy = BLOG_WLANPHY;/* blog rx phy type is WLAN */
                }
                else
#endif
                {
                    blog_chnl = phy_port_id;/* blog rx channel is switch port */
                    blog_phy = BLOG_ENETPHY;/* blog rx phy type is ethernet */
                }
            }

        /* FkBuff_t<data,len> in-placed leaving headroom */
#if defined(CONFIG_BCM96838) || defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
        /*fkb is already created by rdpa, just use it */
#else
        pFkb = fkb_init(pBuf, BCM_PKT_HEADROOM,
                pBuf, len - ETH_CRC_LEN );
#endif
        bcmeapi_set_fkb_recycle_hook(pFkb);


#ifdef CONFIG_BLOG
        /* SMP: bulk rx, bulk blog optimization */
        blogAction = blog_finit_locked( pFkb, dev, TYPE_ETH, blog_chnl, blog_phy );

        if ( blogAction == PKT_DROP )
        {
            bcmeapi_blog_drop(pDevCtrl, pFkb, pBuf);

            /* Store dropped packet count in our portion of the device structure */
            vstats->rx_dropped++;
            estats->rx_dropped_blog_drop++;

			/* Store dropped packet count in switch structure */
            pDevCtrl->stats.rx_dropped++;
            goto next_rx;
        }
        else
        {
            bcmeapi_buf_alloc(pDevCtrl);
        }

        /* packet consumed, proceed to next packet*/
        if ( blogAction == PKT_DONE )
        {
            estats->rx_packets_blog_done++;
            goto next_rx;
        }

#endif /* CONFIG_BLOG */

        /*allocate skb & initialize it using fkb */

        if (bcmeapi_alloc_skb(pDevCtrl, &skb) == BCMEAPI_CTRL_FALSE) {
            RECORD_BULK_RX_UNLOCK();
            ENET_RX_UNLOCK();
            fkb_release(pFkb);
            pDevCtrl->stats.rx_dropped++;
            estats->rx_dropped_no_skb++;
            bcmeapi_kfree_buf_irq(pDevCtrl, pFkb, pBuf);
            if ( rxpktprocessed < rxpktmax )
                continue;
            break;
        }

        /*
         * We are outside of the fast path and not touching any
         * critical variables, so release all locks.
         */
        RECORD_BULK_RX_UNLOCK();
        ENET_RX_UNLOCK();

        got_blog_lock=0;
        blog_unlock();

        if(bcmeapi_skb_headerinit(len, pDevCtrl, skb, pFkb, pBuf) == BCMEAPI_CTRL_CONTINUE)
        {
            estats->rx_dropped_skb_headinit++;
            bcmeapi_kfree_buf_irq(pDevCtrl, pFkb, pBuf);
            goto next_rx;
        }

#if defined(ENET_GPON_CONFIG) || defined(ENET_EPON_CONFIG)
#if defined(CONFIG_BCM_RDPA) || defined(CONFIG_BCM_RDPA_MODULE)
        if (init_cfg.wan_type == rdpa_wan_gpon || init_cfg.wan_type == rdpa_wan_epon)
#endif
            skb->mark = SKBMARK_SET_PORT(skb->mark, gemid);
#endif
        skb->protocol = bcm_type_trans(skb, dev);
        skb->dev = dev;
        if (global.dump_enable) {
            DUMP_PKT(skb->data, skb->len);
        }
        estats->rx_packets_netif_receive_skb++;
        netif_receive_skb(skb);

next_rx:
        if (bcmeapi_prepare_next_rx(&rxpktgood) != BCMEAPI_CTRL_CONTINUE)
        {
            break;
        }
    } /* end while (budget > 0) */

after_rx:
    pDevCtrl->dev->last_rx = jiffies;

    if (got_blog_lock)
    {
        /*
         * Only need to check for BULK_RX_LOCK_ACTIVE if we have the blog_lock.
         * And we have the blog_lock, then cpuid was correctly set at runtime.
         */
        if (BULK_RX_LOCK_ACTIVE())
        {
            RECORD_BULK_RX_UNLOCK();
            ENET_RX_UNLOCK();
        }

        got_blog_lock=0;
        blog_unlock();
    }


    bcmeapi_rx_post(&rxpktgood);

    BCM_ASSERT_C(0 == got_blog_lock);
    BCM_ASSERT_NOT_HAS_SPINLOCK_C(&global.pVnetDev0_g->ethlock_rx);


    return rxpktgood;
}


static int bcm_set_soft_switching(int swPort, int type)
{
    int vport;

    BcmEnet_devctrl *pDevCtrl = netdev_priv(vnet_dev[0]);
    ASSERT(pDevCtrl != NULL);

    vport = LOGICAL_PORT_TO_VPORT(swPort);
    if (type == TYPE_ENABLE) {
        pDevCtrl->softSwitchingMap  |= (1 << swPort);
        vnet_dev[vport]->priv_flags &= ~IFF_HW_SWITCH;
    }
    else if (type == TYPE_DISABLE) {
        pDevCtrl->softSwitchingMap &= ~(1 << swPort);
        vnet_dev[vport]->priv_flags |= IFF_HW_SWITCH;
    }
    else {
        return -EINVAL;
    }

    if (IsExternalSwitchPort(swPort))
    {
        extsw_set_wanoe_portmap(GET_PORTMAP_FROM_LOGICAL_PORTMAP(pDevCtrl->wanPort,1));
    }
    else
    {
        ethsw_set_wanoe_portmap(GET_PORTMAP_FROM_LOGICAL_PORTMAP(pDevCtrl->wanPort,0));
    }

    /* Update the PBVLAN map in the switch if required */
    bcmenet_update_pbvlan_all_bridge();

    return 0;
}


/*
 * Set the hardware MAC address.
 */
static int bcm_set_mac_addr(struct net_device *dev, void *p)
{
    struct sockaddr *addr = p;

    if(netif_running(dev))
        return -EBUSY;

    memcpy(dev->dev_addr, addr->sa_data, dev->addr_len);
    return 0;
}

/*
 * bcm63xx_init_dev: initialize Ethernet MACs,
 * allocate Tx/Rx buffer descriptors pool, Tx header pool.
 * Note that freeing memory upon failure is handled by calling
 * bcm63xx_uninit_dev, so no need of explicit freeing.
 */
static int bcm63xx_init_dev(BcmEnet_devctrl *pDevCtrl)
{
    int rc = 0;

    TRACE(("bcm63xxenet: bcm63xx_init_dev\n"));

    bcmenet_in_init_dev = 1;

    /* Handle pkt rate limiting independently in the FAP. No need for global array */

    if ((rc = bcmeapi_init_queue(pDevCtrl)) < 0)
    {
        return rc;
    }

    pDevCtrl->use_default_txq = 0;

    bcmeapiPhyIntEnable(0);

    bcmenet_in_init_dev = 0;
    /* if we reach this point, we've init'ed successfully */
    return 0;
}

/* Uninitialize tx/rx buffer descriptor pools */
static int bcm63xx_uninit_dev(BcmEnet_devctrl *pDevCtrl)
{
    if (pDevCtrl) {

        bcmeapiPhyIntEnable(0);
        bcmeapi_free_queue(pDevCtrl);
        bcmeapi_free_irq(pDevCtrl);

        /* Deleate the proc files */
        ethsw_del_proc_files();

        /* unregister and free the net device */
        if (pDevCtrl->dev) {
            if (pDevCtrl->dev->reg_state != NETREG_UNINITIALIZED) {
                kerSysReleaseMacAddress(pDevCtrl->dev->dev_addr);
                unregister_netdev(pDevCtrl->dev);
            }
            free_netdev(pDevCtrl->dev);
        }
    }

    return 0;
}

void bcm_init_port_attr(void)
{
    int unit, port;
    BcmEnet_devctrl *pDevCtrl = netdev_priv(vnet_dev[0]);
    u32 port_flags, 
        port_maps,
        wan_only_def_map,
        lan_only_def_map,
        wan_pref_def_map;

    for(unit = 0; unit <= pDevCtrl->unit; unit++)
    {
        port_maps = enet_get_portmap(unit);
        wan_only_def_map = 0;
        lan_only_def_map = 0;
        wan_pref_def_map = 0;

        for(port = 0; port < MAX_SWITCH_PORTS; port++)
        {
            if((port_maps & (1<<port)) == 0 || IsInterSwitchPort(unit, port))
            {
                continue;
            }

            port_flags = enet_get_port_flags(unit, port) & PORT_FLAG_LANWAN_M;

            switch(port_flags)
            {
                case PORT_FLAG_WAN_ONLY:
                    wan_only_def_map |= (1<<port);
                    break;
                case PORT_FLAG_LAN_ONLY:
                    lan_only_def_map |= (1 << port);
                    break;
                case PORT_FLAG_WAN_PREFERRED:
                    wan_pref_def_map |= (1<<port);
                    break;
            }

#if defined(CONFIG_BCM_GMAC)
            /* GMAC port is only on the internal switch  */
            if ( gmac_is_gmac_supported() && !IsExternalSwitchUnit(unit) && gmac_is_gmac_port(port))
            {
                chip_arch_wan_pref_portmap[unit] |= (1<<port);
            }
#endif
        }

        if(chip_arch_wan_only_portmap[unit] & lan_only_def_map)
        {
            printk("ERROR: Conflict LAN Only Port Attributes on Switch %d:\n", unit);
            printk("     Chip WAN Only: %08x, Defined LAN Only Port: 0x%08x\n",
                chip_arch_wan_only_portmap[unit], lan_only_def_map);
            lan_only_def_map &= ~chip_arch_wan_only_portmap[unit];
            printk("     Forced Defined LAN Only Port Attribute to: 0x%08x\n", lan_only_def_map);
        }

        if(chip_arch_lan_only_portmap[unit] & wan_only_def_map)
        {
            printk("ERROR: Conflict WAN Only Port Attribute on Switch %d:\n", unit);
            printk("     Chip LAN Only: %08x, Defined WAN Only Port: 0x%08x\n", 
                chip_arch_lan_only_portmap[unit], wan_only_def_map);
            wan_only_def_map &= ~chip_arch_lan_only_portmap[unit];
            printk("     Forced Defined WAN Only Port Attribute to: 0x%08x\n", wan_only_def_map);
        }

        pDevCtrl->wanOnlyPortChip |= PHYSICAL_PORTMAP_TO_LOGICAL_PORTMAP(chip_arch_wan_only_portmap[unit] & port_maps, unit);
        pDevCtrl->wanPrefPortChip |= PHYSICAL_PORTMAP_TO_LOGICAL_PORTMAP(chip_arch_wan_pref_portmap[unit] & port_maps, unit);
        pDevCtrl->lanOnlyPortChip |= PHYSICAL_PORTMAP_TO_LOGICAL_PORTMAP(chip_arch_lan_only_portmap[unit] & port_maps, unit);

        pDevCtrl->wanOnlyPortDef |= PHYSICAL_PORTMAP_TO_LOGICAL_PORTMAP(wan_only_def_map, unit);
        pDevCtrl->wanPrefPortDef |= PHYSICAL_PORTMAP_TO_LOGICAL_PORTMAP(wan_pref_def_map, unit);
        pDevCtrl->lanOnlyPortDef |= PHYSICAL_PORTMAP_TO_LOGICAL_PORTMAP(lan_only_def_map, unit);
    }

    pDevCtrl->wanOnlyPorts = pDevCtrl->wanOnlyPortChip | pDevCtrl->wanOnlyPortDef;
    pDevCtrl->wanPrefPorts = pDevCtrl->wanPrefPortChip | pDevCtrl->wanPrefPortDef;
    pDevCtrl->lanOnlyPorts = pDevCtrl->lanOnlyPortChip | pDevCtrl->lanOnlyPortDef;

    printk("  Chip WAN Only Port %08x, Defined WAN Only Port %08x, WAN Only Port Result: 0x%08x\n",
        pDevCtrl->wanOnlyPortChip, pDevCtrl->wanOnlyPortDef, pDevCtrl->wanOnlyPorts);
    printk("  Chip WAN Preffered Port %08x, Defined WAN Preffered Port %08x, WAN Preffered Port Result: 0x%08x\n",
        pDevCtrl->wanPrefPortChip, pDevCtrl->wanPrefPortDef, pDevCtrl->wanPrefPorts);
    printk("  Chip LAN Only Port %08x, Defined LAN Only Port %08x, LAN Only Port Result: 0x%08x\n",
        pDevCtrl->lanOnlyPortChip, pDevCtrl->lanOnlyPortDef, pDevCtrl->lanOnlyPorts);
}

/* Technicolor: START: allow for crossbar ports to be selectively used as WAN or LAN
**   For crossbar ports that are associated with both LAN(switch) and WAN(runner) operation,
**   disable the crossbar input for the network that it is not enabled for.
*/
void enet_assign_crossbar_ports(PETHERNET_MAC_INFO pEnetInfo, unsigned char lan_mask)
{
    PETHERNET_MAC_INFO pRunner = &pEnetInfo[0];
    PETHERNET_MAC_INFO pSwitch = &pEnetInfo[1];
    unsigned char cb_mask = 1;
    int cb_port;
    unsigned short wanLinkLed = 0;
#if defined(_BCM963138_) || defined(CONFIG_BCM963138) || defined(_BCM963148_) || defined(CONFIG_BCM963148)
    unsigned short pusFunction;
    unsigned long pulMuxInfo;
#endif

    /* get the bp_usLinkLed for bp_ucPhyType0 and bp_ulPhyId0 */
    BpGetusLinkLed_tch(&wanLinkLed, 0, 0);

    for ( cb_port = 0; cb_port < BP_MAX_CROSSBAR_EXT_PORTS; cb_mask <<= 1, cb_port++ )
    {
        if (BP_IS_CROSSBAR_PORT_DEFINED(pRunner->sw, cb_port) &&
            BP_IS_CROSSBAR_PORT_DEFINED(pSwitch->sw, cb_port))
        {
            if (lan_mask & cb_mask)
            {
                printk("crossbar port %d defined for both LAN and WAN, assigning to LAN\n", cb_port);
                pRunner->sw.crossbar[cb_port].switch_port = BP_CROSSBAR_NOT_DEFINED;
#if defined(_BCM963138_) || defined(CONFIG_BCM963138) || defined(_BCM963148_) || defined(CONFIG_BCM963148)
                /* set pinmux only when WAN port Link/ACT is connected to GPIO 14.
                 * pinmux 4 means GPIO 14 works as GPHY4 Link/ACT LED */
                if((wanLinkLed & BP_GPIO_NUM_MASK) == (BP_GPIO_14_AL & BP_GPIO_NUM_MASK)) {
                    if(BpGrepPinmuxByPortandGpio_tch(bp_usLinkLed, pSwitch->sw.crossbar[cb_port].switch_port,
                                                    wanLinkLed & BP_GPIO_NUM_MASK, &pusFunction, &pulMuxInfo) == BP_SUCCESS) {
                        printk("set gpio 14 pinmux to %d\n",
                                (unsigned int)((pulMuxInfo & BP_PINMUX_VAL_MASK) >> BP_PINMUX_VAL_SHIFT));
                        bcm_set_pinmux(wanLinkLed, (pulMuxInfo & BP_PINMUX_VAL_MASK) >> BP_PINMUX_VAL_SHIFT);
                    }else
                        printk("pinmux is not found \n");
                }
#endif
            }
            else
            {
                printk("crossbar port %d defined for both LAN and WAN, assigning to WAN\n", cb_port);
                pSwitch->sw.crossbar[cb_port].switch_port = BP_CROSSBAR_NOT_DEFINED;
#if defined(_BCM963138_) || defined(CONFIG_BCM963138) || defined(_BCM963148_) || defined (CONFIG_BCM963148)
                /* set pinmux only when WAN port Link/ACT is connected to GPIO 14.
                 * pinmux 6 means GPIO 14 is the overlay of GPIO 22,it works as WAN port Link/Act LED */
                if(BpGrepPinmuxByPortandGpio_tch(bp_usLinkLed, pRunner->sw.crossbar[cb_port].switch_port,
                                                wanLinkLed & BP_GPIO_NUM_MASK, &pusFunction, &pulMuxInfo) == BP_SUCCESS) {
                    if((wanLinkLed & BP_GPIO_NUM_MASK) == (BP_GPIO_14_AL & BP_GPIO_NUM_MASK)) {
                            printk("set gpio 14 pinmux to %d\n",
                                    (unsigned int)((pulMuxInfo & BP_PINMUX_VAL_MASK) >> BP_PINMUX_VAL_SHIFT));
                            bcm_set_pinmux(wanLinkLed, (pulMuxInfo & BP_PINMUX_VAL_MASK) >> BP_PINMUX_VAL_SHIFT);
                        }else
                            printk("pinmux is not found \n");
                }
#endif
            }
        }
    }
}
/* Technicolor: END: allow for crossbar ports to be selectively used as WAN or LAN */

/*
 *      bcm63xx_enet_probe: - Probe Ethernet switch and allocate device
 */
int __init bcm63xx_enet_probe(void)
{
    static int probed = 0, j;
    struct net_device *dev = NULL;
    BcmEnet_devctrl *pDevCtrl = NULL;
    unsigned int chipid;
    unsigned int chiprev;
    unsigned char macAddr[ETH_ALEN];
    ETHERNET_MAC_INFO *EnetInfo;
    int status = 0, unit = 0, ret;
    BcmEnet_devctrl *pVnetDev0;
#if defined(CONFIG_BCM_IEEE1905)
    uint8_t ieee1905_multicast_mac[6] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x13};
#endif

    TRACE(("bcm63xxenet: bcm63xx_enet_probe\n"));

    if (probed)
    {
        /* device has already been initialized */
        return -ENXIO;
    }
    probed++;

    bcmeapi_get_chip_idrev(&chipid, &chiprev);
    dev = alloc_etherdev(sizeof(*pDevCtrl));
    if (dev == NULL)
    {
        printk(KERN_ERR CARDNAME": Unable to allocate net_device!\n");
        return -ENOMEM;
    }

    vnet_dev[0] = dev;
    pDevCtrl = netdev_priv(dev);
    memset(pDevCtrl, 0, sizeof(BcmEnet_devctrl));
    EnetInfo = pDevCtrl->EnetInfo;

    if(BpGetEthernetMacInfo(EnetInfo, BP_MAX_ENET_MACS) != BP_SUCCESS)
    {
        printk(KERN_DEBUG CARDNAME" board id not set\n");
        return -ENODEV;
    }

    enet_assign_crossbar_ports(EnetInfo, LanWanMux); /* Technicolor: allow for crossbar ports to be selectively used as WAN or LAN */

    if ((EnetInfo[1].ucPhyType == BP_ENET_EXTERNAL_SWITCH) ||
            (EnetInfo[1].ucPhyType == BP_ENET_SWITCH_VIA_INTERNAL_PHY)) {
        unit = 1;
    }

    pDevCtrl->dev = dev;
    pDevCtrl->unit = unit;
    pDevCtrl->chipId  = chipid;
    pDevCtrl->chipRev = chiprev;

    bcmeapi_ethsw_init();
    bcmeapi_ethsw_init_ports();

    // Create a port map with only end ports. A port connected to external switch is ignored.
    consolidated_portmap = PHYSICAL_PORTMAP_TO_LOGICAL_PORTMAP(EnetInfo[0].sw.port_map, 0); /* Internal Switch portmap */
    if (unit == 1)
    {
        consolidated_portmap |= PHYSICAL_PORTMAP_TO_LOGICAL_PORTMAP(EnetInfo[1].sw.port_map, 1); /* External Switch portmap */
        extSwInfo.connected_to_internalPort = BpGetPortConnectedToExtSwitch();
        consolidated_portmap &= ~SET_PORT_IN_LOGICAL_PORTMAP(extSwInfo.connected_to_internalPort, 0);
    }

    /* Remove ports connected to FTTdp DSP from unit 0 of consolidated_portmap */
    for (j = 0; j < BP_MAX_SWITCH_PORTS; j++)
    {
        if (EnetInfo[0].sw.port_flags[j] & PORT_FLAG_ATTACHED)
        {
        	consolidated_portmap &= ~SET_PORT_IN_LOGICAL_PORTMAP(j, 0);
#ifdef BRCM_FTTDP
        	bcmeapi_fttdp_init_cfg(j);
#endif
        }
    }

    global.pVnetDev0_g = pDevCtrl;
    global.pVnetDev0_g->extSwitch = &extSwInfo;
#if defined(CONFIG_BCM_PKTDMA_TX_SPLITTING)
    global.pVnetDev0_g->enetTxChannel = PKTDMA_ETH_TX_HOST_IUDMA;   /* default for enet tx on HOST */
#endif
    if (unit == 1) {
        int bus_type, spi_id;
        /* get external switch access details */
        get_ext_switch_access_info(EnetInfo[1].usConfigType, &bus_type, &spi_id);
        extSwInfo.accessType = bus_type;
        extSwInfo.bus_num = (bus_type == MBUS_SPI)?LEG_SPI_BUS_NUM:HS_SPI_BUS_NUM;
        extSwInfo.spi_ss = spi_id;
        extSwInfo.spi_cid = 0;
        extSwInfo.brcm_tag_type = BRCM_TYPE2;
        extSwInfo.present = 1;
        status = bcmeapi_init_ext_sw_if(&extSwInfo);
#if defined(CONFIG_BCM_PORTS_ON_INT_EXT_SW)
        pDevCtrl->wanPort |= 1 << (PHYSICAL_PORT_TO_LOGICAL_PORT(extSwInfo.connected_to_internalPort, 0));
#endif
    }

    spin_lock_init(&pDevCtrl->ethlock_tx);
    spin_lock_init(&pDevCtrl->ethlock_rx);
    spin_lock_init(&bcm_ethlock_phy_access);
    spin_lock_init(&bcm_extsw_access);

    {
        char buf[BRCM_MAX_CHIP_NAME_LEN];
        printk("Broadcom BCM%s Ethernet Network Device ", kerSysGetChipName(buf, BRCM_MAX_CHIP_NAME_LEN));
        printk(VER_STR);
        printk("\n");
    }

#if defined(CONFIG_BCM963268)
    // Now select ROBO at Phy3
    BCM_ENET_DEBUG( "Select ROBO at Mux (bit18=0x40000)" );
    GPIO->RoboswGphyCtrl |= GPHY_MUX_SEL_GMAC;
    GPIO->RoboswGphyCtrl &= ~GPHY_MUX_SEL_GMAC;

    BCM_ENET_DEBUG( "\tGPIORoboswGphyCtrl<0x%p>=0x%x",
            &GPIO->RoboswGphyCtrl, (uint32_t) GPIO->RoboswGphyCtrl );
#endif

#if defined(CONFIG_BCM_GMAC)
    gmac_init();
#endif

    if ((status = bcm63xx_init_dev(pDevCtrl)))
    {
        printk((KERN_ERR CARDNAME ": device initialization error!\n"));
        bcm63xx_uninit_dev(pDevCtrl);
        return -ENXIO;
    }

    dev_alloc_name(dev, dev->name);
    SET_MODULE_OWNER(dev);
    sprintf(dev->name, ETHERNET_ROOT_DEVICE_NAME);

    dev->base_addr = -1;    /* Set the default invalid address to identify bcmsw device */
    bcmeapi_add_proc_files(dev, pDevCtrl);
    ethsw_add_proc_files(dev);

    dev->netdev_ops = &bcm96xx_netdev_ops;
#if defined(SUPPORT_ETHTOOL)
    dev->ethtool_ops = &bcm63xx_enet_ethtool_ops;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 1)
    /*
     * In Linux 3.4, we do not use softirq or NAPI.  We create a thread to
     * do the rx processing work.
     */
    pDevCtrl->rx_work_avail = 0;
    init_waitqueue_head(&pDevCtrl->rx_thread_wqh);
    pDevCtrl->rx_thread = kthread_create(bcm63xx_enet_rx_thread, pDevCtrl, "bcmsw_rx");
    wake_up_process(pDevCtrl->rx_thread);
#else
    netif_napi_add(dev, &pDevCtrl->napi, bcm63xx_enet_poll_napi, NETDEV_WEIGHT);
#endif

    netdev_path_set_hw_port(dev, 0, BLOG_ENETPHY);

    dev->watchdog_timeo     = 2 * HZ;
#if !defined(CONFIG_BCM96838)
    /* setting this flag will cause the Linux bridge code to not forward
       broadcast packets back to other hardware ports */
    dev->priv_flags         = IFF_HW_SWITCH;

/* TCH Modification : Keep the default MTU (1500) set in ether_setup even when Jumbo frames are enabled */
#if 0
    dev->mtu = ENET_MAX_MTU_PAYLOAD_SIZE; /* bcmsw dev : Explicitly assign the MTU size based on buffer size allocated */
#endif
#endif
/* End TCH Modification */

#if defined(_CONFIG_BCM_FAP) && defined(CONFIG_BCM_FAP_GSO)
    dev->features           = NETIF_F_SG | NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_TSO | NETIF_F_TSO6 | NETIF_F_UFO;
#elif defined(CONFIG_BCM_PKTRUNNER_GSO)
    dev->features           = NETIF_F_SG | NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_TSO | NETIF_F_TSO6;
#endif

    /* Indicate we're supporting extended statistics */
    dev->features           |= NETIF_F_EXTSTATS;

    bitcount(vport_cnt, consolidated_portmap);
#if defined(BCM_ENET_CB_WAN_PORT_LNX_INTF_SUPPORT)
    /* Go through the crossbar configuration to see if we will be creating any additional virtual port */
    {
        int port, unit, cb_port;
        for (unit=0; unit < BP_MAX_ENET_MACS; unit++)
        {
            for (port = 0; (port < BP_MAX_SWITCH_PORTS); port++) /* go through all the ports including crossbar */
            {
                cb_port = enet_get_first_crossbar_port(PHYSICAL_PORT_TO_LOGICAL_PORT(port,unit));
                /* Skipping the first CB port because corresponding vport is already accounted for earlier in bitcount() */
                while ((cb_port = enet_get_next_crossbar_port(PHYSICAL_PORT_TO_LOGICAL_PORT(port,unit),cb_port)) != BP_CROSSBAR_NOT_DEFINED) 
                {
                    vport_cnt++;
                    if (vport_cnt > MAX_NUM_OF_VPORTS)
                    {
                        printk("\n\nERROR - MAX_NUM_OF_VPORTS out of range\n\n");
                        return -ENOMEM;
                    }
                }
            }
        }
    }
#endif /* BCM_ENET_CB_WAN_PORT_LNX_INTF_SUPPORT */
    ethsw_init_table(pDevCtrl);
    ethsw_reset_ports(dev);

    dev->vlan_features |= dev->features & (NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM);
    status = register_netdev(dev);

    if (status != 0)
    {
        bcm63xx_uninit_dev(pDevCtrl);
        printk(KERN_ERR CARDNAME "bcm63xx_enet_probe failed, returns %d\n", status);
        return status;
    }

#ifdef DYING_GASP_API
    /* Set up dying gasp handler */
    kerSysRegisterDyingGaspHandler(pDevCtrl->dev->name, &ethsw_switch_power_off, dev);
#endif

#if defined(CONFIG_BCM96368) && defined(_CONFIG_BCM_PKTCMF)
    pktCmfSarPortEnable  = ethsw_enable_sar_port;
    pktCmfSarPortDisable = ethsw_disable_sar_port;
    pktCmfSwcConfig();
#endif


    macAddr[0] = 0xff;
    kerSysGetMacAddress(macAddr, dev->ifindex);

    if((macAddr[0] & ETH_MULTICAST_BIT) == ETH_MULTICAST_BIT)
    {
        memcpy(macAddr, "\x00\x10\x18\x63\x00\x00", ETH_ALEN);
        printk((KERN_CRIT "%s: MAC address has not been initialized in NVRAM.\n"), dev->name);
    }

    memmove(dev->dev_addr, macAddr, ETH_ALEN);

    status = create_vport();

    if (status != 0)
        return status;

#ifdef RDPA_VPORTS
#ifdef BRCM_FTTDP
/* XXX: Move to system init / add ioctl */
#define MGMT_SID 24
#define MGMT_SID_LINK 4
    status = create_rdpa_vport(MGMT_SID, MGMT_SID_LINK, "dsp0");
    if (status)
        printk("%s %s Failed to create rdpa hmi port rc(%d)\n", __FILE__, __FUNCTION__, status);
    rdpa_vport_to_dev[rdpa_if_lan0 + MGMT_SID + 1] = rdpa_vport_to_dev[rdpa_if_lan0 + MGMT_SID];
    rdpa_vport_to_dev[rdpa_if_lan0 + MGMT_SID + 2] = rdpa_vport_to_dev[rdpa_if_lan0 + MGMT_SID];
    rdpa_vport_to_dev[rdpa_if_lan0 + MGMT_SID + 3] = rdpa_vport_to_dev[rdpa_if_lan0 + MGMT_SID];
    rdpa_vport_to_dev[rdpa_if_lan0 + MGMT_SID + 4] = rdpa_vport_to_dev[rdpa_if_lan0 + MGMT_SID];
#endif
    create_rdpa_vports();
#endif

#ifdef ENET_EPON_CONFIG
    {
#if defined(CONFIG_BCM_RDPA) || defined(CONFIG_BCM_RDPA_MODULE)
        if (init_cfg.wan_type == rdpa_wan_epon)
#else
        unsigned long optical_wan_type = 0;
        BpGetOpticalWan(&optical_wan_type);
        printk("optical_wan_type = %ld, WAN_GPON=%d, WAN_EPON=%d\n", optical_wan_type, BP_OPTICAL_WAN_GPON, BP_OPTICAL_WAN_EPON);
        // TBD need think about gpon_epon?
        if (optical_wan_type & BP_OPTICAL_WAN_EPON)
#endif
        {
            /* create epon0 linux interface if needed */
            create_epon_vport(eponif_name);
        }
    }
#endif

    pVnetDev0 = (BcmEnet_devctrl *) netdev_priv(vnet_dev[0]);

    ethsw_init_config(pDevCtrl->unit, EnetInfo[0].sw.port_map,  pDevCtrl->wanPort);
    ethsw_phy_config();

    /* Ethernet Switch init is complete - flush the dynamic ARL entries to start clean */
    fast_age_all(0);
    if (bcm63xx_enet_isExtSwPresent()) {
        fast_age_all_ext(0);
    }

    enet_arl_write((uint8_t*)dev->dev_addr, 0, 
        ARL_DATA_ENTRY_VALID|ARL_DATA_ENTRY_STATIC|IMP_PORT_ID);

    if (extSwInfo.present == 1)
    {
        enet_arl_write_ext((uint8_t*)dev->dev_addr, 0, 
                ARL_DATA_ENTRY_VALID|ARL_DATA_ENTRY_STATIC|IMP_PORT_ID);
    }
#if defined(CONFIG_BCM_IEEE1905)
    bcmeapi_set_multiport_address(ieee1905_multicast_mac);
    bcmsw_set_multiport_address_ext(ieee1905_multicast_mac);
#endif

    if( (ret = bcmeapi_init_dev(dev)) < 0)
        return ret;

    bcm_init_port_attr();

#if !defined(SUPPORT_SWMDK)
    ethsw_eee_init();
#endif
    phy_int_mapped = bcmeapi_map_interrupt(pDevCtrl);
    if((bcmenet_use_ext_phy && !(phy_int_mapped & BCMEAPI_INT_MAPPED_EXTPHY)) ||
        !(phy_int_mapped & BCMEAPI_INT_MAPPED_INTPHY))
    {
        force_link_check = ETHSW_LINK_FORCE_CHECK;
    }

    poll_pid = kernel_thread((int(*)(void *))bcm63xx_enet_poll_timer, 0, CLONE_KERNEL);
    set_bit(__LINK_STATE_START, &dev->state);
    dev->netdev_ops->ndo_open(dev);
    dev->flags |= IFF_UP;

    return ((poll_pid < 0)? -ENOMEM: 0);
}

int bcm63xx_enet_isExtSwPresent(void)
{
    return extSwInfo.present;
}

int bcm63xx_enet_intSwPortToExtSw(void)
{
    return extSwInfo.connected_to_internalPort;
}

unsigned int bcm63xx_enet_extSwId(void)
{
    return extSwInfo.switch_id;
}

static int bridge_notifier(struct notifier_block *nb, unsigned long event, void *brName);
static void bridge_update_pbvlan(char *brName);
struct notifier_block br_notifier = {
    .notifier_call = bridge_notifier,
};

static int bridge_stp_handler(struct notifier_block *nb, unsigned long event, void *portInfo);
static struct notifier_block br_stp_handler = {
    .notifier_call = bridge_stp_handler,
};

static void __exit bcmenet_module_cleanup(void)
{
    BcmEnet_devctrl *pDevCtrl;
    TRACE(("bcm63xxenet: bcmenet_module_cleanup\n"));

    /*first disable the poll timer*/
    if (poll_pid >= 0)
    {
      atomic_dec(&poll_lock);
      wait_for_completion(&poll_done);
    }

    bcmeapi_enet_module_cleanup();

#ifdef CONFIG_BLOG
   blog_eth_get_tx_mark_fn = (blog_eth_get_tx_mark_t)NULL;
#endif

    delete_vport();
#if defined(ENET_GPON_CONFIG)
    delete_all_gpon_vports();
#endif
#if defined(ENET_EPON_CONFIG)
    delete_all_epon_vports();
#endif
#ifdef RDPA_VPORTS
#ifdef BRCM_FTTDP
    rdpa_vport_to_dev[rdpa_if_lan0 + 17] = NULL;
    rdpa_vport_to_dev[rdpa_if_lan0 + 18] = NULL;
    rdpa_vport_to_dev[rdpa_if_lan0 + 19] = NULL;
    rdpa_vport_to_dev[rdpa_if_lan0 + 20] = NULL;
#endif
    delete_all_rdpa_vports();
#endif


    pDevCtrl = (BcmEnet_devctrl *)netdev_priv(vnet_dev[0]);

    if (pDevCtrl)
    {
#ifdef DYING_GASP_API
        if(pDevCtrl->EnetInfo[0].ucPhyType == BP_ENET_EXTERNAL_SWITCH)
            kerSysDeregisterDyingGaspHandler(pDevCtrl->dev->name);
#endif
        bcm63xx_uninit_dev(pDevCtrl);
    }

    bcmFun_dereg(BCM_FUN_ID_ENET_LINK_CHG);
    bcmFun_dereg(BCM_FUN_ID_RESET_SWITCH);
    bcmFun_dereg(BCM_FUN_ID_ENET_CHECK_SWITCH_LOCKUP);
    bcmFun_dereg(BCM_FUN_ID_ENET_GET_PORT_BUF_USAGE);
    bcmFun_dereg(BCM_FUN_IN_ENET_CLEAR_ARL_ENTRY);

    /* Unregister bridge notifier hooks */
    unregister_bridge_notifier(&br_notifier);
    unregister_bridge_stp_notifier(&br_stp_handler);
#ifdef CONFIG_BCM_PTP_1588
    ptp_1588_uninit();
#endif
}

static int bcmeapi_ioctl_use_default_txq_config(BcmEnet_devctrl *pDevCtrl,
        struct ethswctl_data *e)
{
    if (e->type == TYPE_GET) {
        if (copy_to_user((void*)(&e->ret_val),
                    (void*)&pDevCtrl->use_default_txq, sizeof(int))) {
            return -EFAULT;
        }
        BCM_ENET_DEBUG("e->ret_val: 0x%02x \n ", e->ret_val);
    } else {
        BCM_ENET_DEBUG("Given use_default_txq: 0x%02x \n ", e->val);
        pDevCtrl->use_default_txq = e->val;
    }

    return 0;
}

static int bcmeapi_ioctl_default_txq_config(BcmEnet_devctrl *pDevCtrl,
        struct ethswctl_data *e)
{
    if (e->type == TYPE_GET) {
        if (copy_to_user((void*)(&e->queue),
                    (void*)&pDevCtrl->default_txq, sizeof(int))) {
            return -EFAULT;
        }
        BCM_ENET_DEBUG("e->queue: 0x%02x \n ", e->queue);
    } else {
        BCM_ENET_DEBUG("Given queue: 0x%02x \n ", e->queue);
        if ((e->queue >= NUM_EGRESS_QUEUES) || (e->queue < 0)) {
            printk("Invalid queue \n");
            return BCM_E_ERROR;
        }
        pDevCtrl->default_txq = e->queue;
    }

    return 0;
}



#ifdef BCM_ENET_DEBUG_BUILD
static int bcmeapi_ioctl_getrxcounters(void)
{
    int a = 0, b = 0, c = 0, d = 0, f = 0, cnt = 0;

    for (cnt = 0; cnt < ENET_RX_CHANNELS_MAX; cnt++)
    {
        printk("Rx counters: %d No Rx Pkts counters: %d No Rx BDs counters: %d\n", ch_pkts[cnt],
                ch_no_pkts[cnt], ch_no_bds[cnt]);
    }

    printk("Channels: ");
    for (cnt = 0; cnt < NUM_ELEMS; cnt++) {
        if (ch_serviced[cnt] == WRR_RELOAD) {
            printk("\nCh0 = %d, Ch1 = %d, Ch2 = %d, Ch3 = %d \n", a,b,c,d);
            a = b = c =d = 0;
            printk("\nReloaded WRR weights \n");
        } else if (ch_serviced[cnt] == ISR_START) {
            printk("ISR START (Weights followed by channels serviced) \n");
            printk("x- indicates pkt received \n");
        } else {
            if (ch_serviced[cnt] & (1<<31)) {
                printk("x-");
                f = ch_serviced[cnt] & 0xF;
                if (f == 0) {
                    a++;
                } else if (f == 1) {
                    b++;
                } else if (f == 2) {
                    c++;
                } else if (f == 3) {
                    d++;
                }
            }
            printk("%d ", ch_serviced[cnt] & (~(1<<31)));
        }
    }
    printk("\n");
    return 0;
}

static int bcmeapi_ioctl_setrxcounters(void)
{
    int cnt = 0;

    for (cnt = 0; cnt < ENET_RX_CHANNELS_MAX; cnt++)
    {
        ch_pkts[cnt] = ch_no_pkts[cnt] = ch_no_bds[cnt] = 0;
    }
    for (cnt=0; cnt<4000; cnt++) {
        ch_serviced[cnt] = 0;
    }
    dbg_index = 0;

    return 0;
}
#endif


#define ESTATS_FMT "  %-40s: %10lu;   %-40s: %10lu"
#define ESTATS_VAL(v1, v2) #v1, estats->v1, #v2, estats->v2
#define ESTATS_FMT1 "  %-40s: %10lu"
#define ESTATS_FMT1b "  %-40s: %10s"
#define ESTATS_VAL1b(v1) #v1, v1? "True": "False"
#define ESTATS_VAL1(v1) #v1, v1
#define ESTATS_VAL2(v1, v2) #v1, v1, #v2, v2
typedef struct counterSum
{
    unsigned long rxIn, rx2Kernel, rx2Blog, rxDrops,
                  txIn, txOut, txDrops, txExtraChain;
} counterSum_t;

static void display_enet_dev_stats(BcmEnet_devctrl *pDevCtl, counterSum_t *cts)
{
    struct net_device *dev = pDevCtl->dev;
    enet_device_stats *estats = &pDevCtl->estats;
    counterSum_t portCnt;
    int i;

    memset(&portCnt, 0, sizeof(portCnt));

    printk("Device %s:\n", dev->name);
    printk(ESTATS_FMT "\n", ESTATS_VAL(rx_packets_queue[0], rx_packets_queue[1]));
    printk(ESTATS_FMT "\n", ESTATS_VAL(rx_packets_queue[2], rx_packets_queue[3]));
    printk(ESTATS_FMT "\n", ESTATS_VAL(rx_packets_queue[4], rx_packets_queue[5]));
    printk(ESTATS_FMT "\n", ESTATS_VAL(rx_packets_queue[6], rx_packets_queue[7]));
    printk(ESTATS_FMT "\n", ESTATS_VAL(tx_packets_queue[0], tx_packets_queue[1]));
    printk(ESTATS_FMT "\n", ESTATS_VAL(tx_packets_queue[2], tx_packets_queue[3]));
    printk(ESTATS_FMT "\n", ESTATS_VAL(tx_packets_queue[4], tx_packets_queue[5]));
    printk(ESTATS_FMT "\n", ESTATS_VAL(tx_packets_queue[6], tx_packets_queue[7]));
    printk(ESTATS_FMT "\n", ESTATS_VAL(rx_dropped_no_rxdev, rx_dropped_blog_drop));
    printk(ESTATS_FMT "\n", ESTATS_VAL(rx_dropped_no_skb, rx_packets_blog_done));
    printk(ESTATS_FMT "\n", ESTATS_VAL(rx_dropped_skb_headinit, rx_packets_netif_receive_skb));
    printk(ESTATS_FMT "\n", ESTATS_VAL(rx_errors_indicated_by_low_level, rx_dropped_undersize));
    printk(ESTATS_FMT "\n", ESTATS_VAL(rx_dropped_overrate, tx_enet_xmit));
    printk(ESTATS_FMT "\n", ESTATS_VAL(tx_dropped_bad_nbuff, tx_enet_xmit_extra_chained_nbuff));
    printk(ESTATS_FMT "\n", ESTATS_VAL(tx_dropped_no_lowlvl_resource, tx_dropped_no_fkb));
    printk(ESTATS_FMT "\n", ESTATS_VAL(tx_dropped_no_skb, tx_dropped_no_gem_ids));
    printk(ESTATS_FMT "\n", ESTATS_VAL(tx_dropped_bad_gem_id, tx_dropped_misaligned_nbuff));
    printk(ESTATS_FMT "\n", ESTATS_VAL(tx_drops_no_valid_gem_fun, tx_drops_skb_linearize_error));
    printk(ESTATS_FMT "\n", ESTATS_VAL(tx_dropped_runner_lan_fail, tx_dropped_runner_wan_fail));
    printk(ESTATS_FMT "\n", ESTATS_VAL(tx_dropped_no_gso_dsc, tx_dropped_sid_tx_fail));
    printk(ESTATS_FMT "\n", ESTATS_VAL(tx_dropped_no_rdpa_port_mapped, tx_dropped_no_gem_tcount));
    printk(ESTATS_FMT "\n", ESTATS_VAL(tx_dropped_no_epon_tx_fun, tx_dropped_no_epon_oam_fun));
    printk(ESTATS_FMT "\n", ESTATS_VAL(tx_dropped_gpon_tx_fail, tx_dropped_epon_tx_fail));
    printk(ESTATS_FMT "\n", ESTATS_VAL(tx_dropped_epon_oam_fail, tx_dropped_xpon_lan_fail));

    for(i = 0; i<8; i++)
    {
        portCnt.rxIn += estats->rx_packets_queue[i]; 
        portCnt.txOut += estats->tx_packets_queue[i]; 
    }

    portCnt.rx2Kernel = estats->rx_packets_netif_receive_skb;
    portCnt.rx2Blog = estats->rx_packets_blog_done;
    portCnt.rxDrops =   estats->rx_dropped_no_rxdev +
        estats->rx_dropped_blog_drop +
        estats->rx_dropped_no_skb +
        estats->rx_dropped_skb_headinit +
        estats->rx_errors_indicated_by_low_level +
        estats->rx_dropped_undersize +
        estats->rx_dropped_overrate;

    portCnt.txIn = estats->tx_enet_xmit;
    portCnt.txDrops = estats->tx_dropped_bad_nbuff +
        estats->tx_dropped_no_lowlvl_resource +
        estats->tx_dropped_no_fkb +
        estats->tx_dropped_no_skb +
        estats->tx_dropped_no_gem_ids +
        estats->tx_dropped_bad_gem_id +
        estats->tx_dropped_misaligned_nbuff +
        estats->tx_drops_no_valid_gem_fun +
        estats->tx_drops_skb_linearize_error +
        estats->tx_dropped_runner_lan_fail +
        estats->tx_dropped_runner_wan_fail +
        estats->tx_dropped_no_gso_dsc +
        estats->tx_dropped_sid_tx_fail +
        estats->tx_dropped_no_rdpa_port_mapped +
        estats->tx_dropped_no_gem_tcount +
        estats->tx_dropped_no_epon_tx_fun +
        estats->tx_dropped_no_epon_oam_fun +
        estats->tx_dropped_gpon_tx_fail +
        estats->tx_dropped_epon_tx_fail +
        estats->tx_dropped_epon_oam_fail +
        estats->tx_dropped_xpon_lan_fail;
    portCnt.txExtraChain = estats->tx_enet_xmit_extra_chained_nbuff;

    printk(ESTATS_FMT "\n", ESTATS_VAL2(portCnt.rxIn, portCnt.rx2Kernel));
    printk(ESTATS_FMT "\n", ESTATS_VAL2(portCnt.rx2Blog, portCnt.rx2Kernel + portCnt.rx2Blog));
    printk(ESTATS_FMT1 "\n", ESTATS_VAL1(portCnt.rxDrops));
    printk(ESTATS_FMT1 "\n", ESTATS_VAL1((portCnt.rx2Kernel + portCnt.rx2Blog + portCnt.rxDrops)));
    printk(ESTATS_FMT1b "\n", ESTATS_VAL1b(portCnt.rxIn == (portCnt.rx2Kernel + portCnt.rx2Blog + portCnt.rxDrops)));
    printk(ESTATS_FMT "\n", ESTATS_VAL2(portCnt.txIn, portCnt.txOut));
    printk(ESTATS_FMT "\n", ESTATS_VAL2(portCnt.txDrops, portCnt.txExtraChain));
    printk(ESTATS_FMT1 "\n", ESTATS_VAL1((portCnt.txOut + portCnt.txDrops - portCnt.txExtraChain)));
    printk(ESTATS_FMT1b "\n", ESTATS_VAL1b(portCnt.txIn == (portCnt.txOut + portCnt.txDrops - portCnt.txExtraChain)));
    printk("\n");

    if(cts)
    {
        cts->rxIn += portCnt.rxIn;
        cts->rx2Kernel += portCnt.rx2Kernel;
        cts->rx2Blog += portCnt.rx2Blog;
        cts->rxDrops += portCnt.rxDrops;
        cts->txIn += portCnt.txIn;
        cts->txOut += portCnt.txOut;
        cts->txDrops += portCnt.txDrops;
        cts->txExtraChain += portCnt.txExtraChain;
    }
    return;
}

static void display_enet_stats(BcmEnet_devctrl *_pDevCtl)
{
    struct net_device *dev = _pDevCtl->dev;
    BcmEnet_devctrl * pDevCtl;
    counterSum_t total;
    int vport;

    memset(&total, 0, sizeof(total));

    if(strcmp(dev->name, ETHERNET_ROOT_DEVICE_NAME) != 0)
        return display_enet_dev_stats(_pDevCtl, NULL);

    for (vport = 0; vport <= vport_cnt; vport++) 
    {
        dev = vnet_dev[vport];
        if (dev == NULL)
            continue;

        pDevCtl = (BcmEnet_devctrl *)netdev_priv(dev);
        display_enet_dev_stats(pDevCtl, &total);
    }

    printk("\n");
    printk(ESTATS_FMT "\n", ESTATS_VAL2(total.rxIn, total.rx2Kernel));
    printk(ESTATS_FMT "\n", ESTATS_VAL2(total.rx2Blog, total.rx2Kernel + total.rx2Blog));
    printk(ESTATS_FMT "\n", ESTATS_VAL2(total.rxDrops, (total.rx2Kernel + total.rx2Blog + total.rxDrops)));
    printk(ESTATS_FMT1b "\n", ESTATS_VAL1b(total.rxIn == (total.rx2Kernel + total.rx2Blog + total.rxDrops)));
    printk(ESTATS_FMT "\n", ESTATS_VAL2(total.txIn, total.txOut));
    printk(ESTATS_FMT "\n", ESTATS_VAL2(total.txDrops, total.txExtraChain));
    printk(ESTATS_FMT1 "\n", ESTATS_VAL1((total.txOut + total.txDrops - total.txExtraChain)));
    printk(ESTATS_FMT1b "\n", ESTATS_VAL1b(total.txIn == (total.txOut + total.txDrops - total.txExtraChain)));
}

static void display_software_stats(BcmEnet_devctrl * pDevCtrl)
{

    printk("\n");
    printk("TxPkts:       %10lu \n", pDevCtrl->stats.tx_packets);
    printk("TxOctets:     %10lu \n", pDevCtrl->stats.tx_bytes);
    printk("TxDropPkts:   %10lu \n", pDevCtrl->stats.tx_dropped);
    printk("\n");
    printk("RxPkts:       %10lu \n", pDevCtrl->stats.rx_packets);
    printk("RxOctets:     %10lu \n", pDevCtrl->stats.rx_bytes);
    printk("RxDropPkts:   %10lu \n", pDevCtrl->stats.rx_dropped);
    printk("\n");

    display_enet_stats(pDevCtrl);
}

#define BIT_15 0x8000
#define MAX_NUM_WAN_IFACES 8
#define MAX_WAN_IFNAMES_LEN ((MAX_NUM_WAN_IFACES * (IFNAMSIZ + 1)) + 2)

#if defined(_CONFIG_BCM_FAP) && \
    (defined(CONFIG_BCM_PKTDMA_RX_SPLITTING) || defined(CONFIG_BCM963268) ||  defined(CONFIG_BCM96828))
static void enet_conf_cos_for_wan(int swPort, int *data)
{
    struct ethswctl_data e2;
    int i, j;

    if(data)
    {
        if ( !IsExternalSwitchPort(swPort))
        {
            /* The equivalent of "ethswctl -c cosq -p {i} -q {j} -v 1" */
            /* where i = all eth ports (0..5) except the WAN port (swPort) */
            /* This routes packets of all priorities on the WAN eth port to egress queue 0 */
            /* This routes packets of all priorities on all other eth ports to egress queue 1 */
            for(i = 0; i < BP_MAX_SWITCH_PORTS; i++)
            {
                for(j = 0; j <= MAX_PRIORITY_VALUE; j++)
                {
                    e2.type = TYPE_SET;
                    e2.port = i;
                    e2.priority = j;

                    if ((LOGICAL_PORT_TO_PHYSICAL_PORT(swPort) == i)
                       )
                    {
                        e2.queue = PKTDMA_ETH_DS_IUDMA;  /* WAN port mapped to DS FAP */
                    }
                    else
                    {
                        e2.queue = PKTDMA_ETH_US_IUDMA;  /* other ports to US FAP */
                    }

                    bcmeapi_ioctl_ethsw_cosq_port_mapping(&e2);
                }
            }
        }
    }
    else
    {
        /* Return all ethernet ports to be processed on the FAP - Nov 2010 (Jira 7811) */
        /* The equivalent of "ethswctl -c cosq -p {i} -q {j} -v 0" */
        /* where i = all eth ports (0..5) including the WAN port (swPort) */
        for(i = 0; i < BP_MAX_SWITCH_PORTS ; i++)
        {
            for(j = 0; j <= MAX_PRIORITY_VALUE; j++)
            {
                e2.type = TYPE_SET;
                e2.port = i;
                e2.priority = j;
                /* All ports mapped to default iuDMA - Mar 2011 */
                /* US iuDMA for 63268/6828 and DS iuDMA (ie FAP owned) for 6362 */
#if defined(CONFIG_BCM96828) && !defined(CONFIG_EPON_HGU)
                /* Revert to initial config when a WAN port is deleted */
                e2.queue = restoreEthPortToRxIudmaConfig(e2.port);
#else
                e2.queue = PKTDMA_DEFAULT_IUDMA;
#endif
                bcmeapi_ioctl_ethsw_cosq_port_mapping(&e2);
            }
        }
    }
}
#else
static void enet_conf_cos_for_wan(int swPort, int *data)
{
}
#endif

int enet_ioctl_kernel_poll(struct ethswctl_data *e)
{
    int link_change;

    bcmeapi_ioctl_kernel_poll(e);
    link_change = bcmenet_link_might_changed();
    copy_to_user((void*)(&e->mdk_kernel_poll.link_change), (void*)&link_change, sizeof(link_change));
    return 0;
}

static int str_append(char *buf, char *name, int *cum_len, int bufLen)
{
    int len = strlen(name);

    if ((*cum_len + (*cum_len>0) + len + 1) > bufLen)
        return 0;

    if(*cum_len) {
        strcat(buf, ",");
        cum_len++;
    }

    strcat(buf, name);
    *cum_len += len;

    return len;
}

static int bcm63xx_enet_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
    BcmEnet_devctrl *pDevCtrl;
    char *buf, *ubuf = (char *)rq->ifr_data;
    int *data=(int*)rq->ifr_data;
#if defined(ENET_GPON_CONFIG)
    struct gponif_data g;
#endif // #if defined(ENET_GPON_CONFIG)


    struct ethswctl_data *e=(struct ethswctl_data*)rq->ifr_data;
    struct ethctl_data *ethctl=(struct ethctl_data*)rq->ifr_data;
    struct interface_data *enetif_data=(struct interface_data*)rq->ifr_data;
    struct mii_ioctl_data *mii;
    int val = 0, mask = 0, bufLen = 0, cum_len = 0, portMap = 0;
    int i, vport, phy_id;
    struct net_device_stats *vstats;
#ifdef SEPARATE_MAC_FOR_WAN_INTERFACES
    struct sockaddr sockaddr;
#endif
    int swPort;
    int phyId;

    pDevCtrl = netdev_priv(vnet_dev[0]);
    ASSERT(pDevCtrl != NULL);

#if defined(CONFIG_BCM_ETH_PWRSAVE)
    ethsw_phy_pll_up(0);
#endif

    switch (cmd)
    {
        case SIOCGMIIPHY:       /* Get address of MII PHY in use. */
            mii = (struct mii_ioctl_data *)&rq->ifr_data;
            swPort = port_id_from_dev(dev);
            if (swPort >= MAX_TOTAL_SWITCH_PORTS )
            {
                printk("SIOCGMIIPHY : Invalid swPort: %d \n", swPort);
                return -EINVAL;
            }
            val = 0;
            phy_id = enet_logport_to_phyid(swPort);
            mii->phy_id =  (u16)phy_id;
            /* Let us also return phy flags needed for accessing the phy */
            mii->val_out =  phy_id & CONNECTED_TO_EXTERN_SW? ETHCTL_FLAG_ACCESS_EXTSW_PHY: 0;
            mii->val_out |= IsExtPhyId(phy_id)? ETHCTL_FLAG_ACCESS_EXT_PHY: 0;
            BCM_ENET_DEBUG("%s: swPort/logport %d phy_id: 0x%x flag 0x%x \n", __FUNCTION__,
                    swPort, mii->phy_id, mii->val_out);
            break;

        case SIOCGMIIREG:       /* Read MII PHY register. */
            {
                int flags;
                mii = (struct mii_ioctl_data *)&rq->ifr_data;
                flags = mii->val_out;
                down(&bcm_ethlock_switch_config);
                ethsw_phyport_rreg2(mii->phy_id, mii->reg_num & 0x1f,
                        (uint16 *)&mii->val_out, mii->val_out);
                BCM_ENET_DEBUG("phy_id: %d; reg_num = %d  val = 0x%x\n",
                        mii->phy_id, mii->reg_num, flags);
                up(&bcm_ethlock_switch_config);
                break;
            }

        case SIOCGSWITCHPORT:       /* Get Switch Port. */
            val = -1;
            for (vport = 1; vport <= vport_cnt; vport++) {
                if ((vnet_dev[vport]) &&
                        (strcmp(enetif_data->ifname, vnet_dev[vport]->name) == 0)) {
                    val = ((BcmEnet_devctrl *)netdev_priv(vnet_dev[vport]))->sw_port_id;
                    break;
                }
            }
            if (copy_to_user((void*)&enetif_data->switch_port_id, (void*)&val, sizeof(int)))
                return -EFAULT;
            break;

        case SIOCSMIIREG:       /* Write MII PHY register. */
            {
                mii = (struct mii_ioctl_data *)&rq->ifr_data;
                down(&bcm_ethlock_switch_config);
                BCM_ENET_DEBUG("phy_id: %d; reg_num = %d; val = 0x%x \n", mii->phy_id,
                        mii->reg_num, mii->val_in);
                ethsw_phyport_wreg2(mii->phy_id, mii->reg_num & 0x1f, (uint16 *)&mii->val_in, mii->val_out);
                up(&bcm_ethlock_switch_config);
                break;
            }

        case SIOCGLINKSTATE:
            if (dev == vnet_dev[0])
            {
                mask = 0xffffffff;
            }
            else
            {
                swPort = port_id_from_dev(dev);
                if (swPort >= MAX_TOTAL_SWITCH_PORTS )
                {
                    printk("SIOCGLINKSTATE : Invalid swPort: %d \n", swPort);
                    return -EINVAL;
                }
                mask = 0x00000001 << swPort;
#if defined(BCM_ENET_CB_WAN_PORT_LNX_INTF_SUPPORT)
                {
                    BcmEnet_devctrl *cb_priv = netdev_priv(dev);
                    if (cb_priv->cb_ext_port != BP_CROSSBAR_NOT_DEFINED) /* Crossbar port */
                    {
                        mask = cb_priv->linkState << swPort;
            }
                }
#endif /* BCM_ENET_CB_WAN_PORT_LNX_INTF_SUPPORT */
            }

            val = (pDevCtrl->linkState & mask)? 1: 0;

            if (copy_to_user((void*)data, (void*)&val, sizeof(int)))
                return -EFAULT;

            val = 0;
            break;

        case SIOCSCLEARMIBCNTR:
            ASSERT(pDevCtrl != NULL);

            bcm63xx_enet_query(dev);

            memset(&pDevCtrl->stats, 0, sizeof(struct net_device_stats));
            /* port 0 is bcmsw */
            for (vport = 1; vport <= vport_cnt; vport++)
            {
                if (vnet_dev[vport])
                {
                    bcmeapi_reset_mib_cnt(port_id_from_dev(vnet_dev[vport]));
                    vstats = &(((BcmEnet_devctrl *)netdev_priv(vnet_dev[vport]))->stats);
                    memset(vstats, 0, sizeof(struct net_device_stats));
                }
            }
            bcmeapi_reset_mib();
            bcmeapi_reset_mib_ext();
            val = 0;
            break;

        case SIOCMIBINFO:
            // Setup correct port indexes.
            swPort = port_id_from_dev(dev);
            if (swPort >= MAX_TOTAL_SWITCH_PORTS )
            {
                printk("SIOCMIBINFO : Invalid swPort: %d \n", swPort);
                return -EINVAL;
            }
            vport = LOGICAL_PORT_TO_VPORT(swPort);

            if (vnet_dev[vport])
            {
                IOCTL_MIB_INFO *mib;

                // Create MIB address.
                mib = &((BcmEnet_devctrl *)netdev_priv(vnet_dev[vport]))->MibInfo;

                // Copy MIB to caller.
                if (copy_to_user((void*)data, (void*)mib, sizeof(IOCTL_MIB_INFO)))
                    return -EFAULT;
            }
            else
            {
                return -EFAULT;
            }

            val = 0;
            break;

        case SIOCGQUERYNUMPORTS:
            val = 1;
            if (copy_to_user((void*)data, (void*)&val, sizeof(int))) {
                return -EFAULT;
            }
            val = 0;
            break;

        case SIOCSWANPORT:
            if (dev == vnet_dev[0])
                return -EFAULT;

            swPort = port_id_from_dev(dev);
            if (swPort >= MAX_TOTAL_SWITCH_PORTS)
            {
                printk("SIOCSWANPORT : Invalid swPort: %d \n", swPort);
                return -EINVAL;
            }

            phyId  = enet_logport_to_phyid(swPort);

            if (phyId >= 0) {
                if(IsWanPort(phyId)) {
                    if ((int)data) {
#if defined(CONFIG_BCM_GMAC)
                        if ( IsGmacPort(swPort) )
                        {
                            gmac_set_wan_port( 1 );
                        }
#endif
                        return 0;
                    }
                    else
                    {
                        BCM_ENET_DEBUG("This port cannot be removed "
                                "from WAN port map");
                        return -EFAULT;
                    }
                }
            }

#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
            /* For 63138 and 63148 only Runner Port is allowed as ETH_WAN */
            if (((int)data) &&  /* Enabling WAN port */
                (LOGICAL_PORT_TO_UNIT_NUMBER(swPort) != 0 || LOGICAL_PORT_TO_PHYSICAL_PORT(swPort) != 0))
            {
                printk("SIOCSWANPORT : %s not allowed as WAN <%s switch port: %d> <Logical Port: %d>\n",
                       dev->name,
                       (LOGICAL_PORT_TO_UNIT_NUMBER(swPort)?"Ext":"Runner"),LOGICAL_PORT_TO_PHYSICAL_PORT(swPort), swPort);
                return -EINVAL;
            }
#endif /* CONFIG_BCM963138 || CONFIG_BCM963148 */

            if ( (int)data ) {
                BcmEnet_devctrl *priv = (BcmEnet_devctrl *)netdev_priv(vnet_dev[0]);
                pDevCtrl->wanPort |= (1 << swPort);
                dev->priv_flags |= IFF_WANDEV;
#if !defined(CONFIG_BCM96838)
                dev->priv_flags &= ~IFF_HW_SWITCH;
#endif
#if defined(CONFIG_BCM_GMAC)
                if ( IsGmacPort(swPort) )
                {
                    int vport = LOGICAL_PORT_TO_VPORT(swPort);

                    if (vnet_dev[vport])
                    {
                        IOCTL_MIB_INFO *mib;

                        mib = &((BcmEnet_devctrl *)netdev_priv(vnet_dev[vport]))->MibInfo;

                        ethsw_eee_port_enable(swPort, 0, 0);
                        gmac_set_wan_port( 1 );
                        ethsw_eee_port_enable(swPort, 1, 0);
                    }
                }
#endif
                if (priv->linkState & (1<<swPort))
                {
                    bcmeapi_EthSetPhyRate(swPort, 1, ((BcmEnet_devctrl *)netdev_priv(dev))->MibInfo.ulIfSpeed, 1);
                }
#ifdef SEPARATE_MAC_FOR_WAN_INTERFACES
                val = kerSysGetMacAddress(dev->dev_addr, dev->ifindex);
                if (val == 0) {
                    memmove(sockaddr.sa_data, dev->dev_addr, ETH_ALEN);
                    bcmenet_set_dev_mac_addr(dev, &sockaddr);
                }
#endif
            } else {
                BcmEnet_devctrl *priv = (BcmEnet_devctrl *)netdev_priv(vnet_dev[0]);
#if defined(CONFIG_BCM_GMAC)
                if (IsGmacPort( swPort ) && IsLogPortWan(swPort) )
                {
                    int vport = LOGICAL_PORT_TO_VPORT(swPort);

                    if (vnet_dev[vport])
                    {
                        IOCTL_MIB_INFO *mib;

                        mib = &((BcmEnet_devctrl *)netdev_priv(vnet_dev[vport]))->MibInfo;

                        gmac_set_wan_port( 0 );
                    }
                }
#endif
                if (priv->linkState & (1<<swPort))
                {
                    bcmeapi_EthSetPhyRate(swPort, 1, ((BcmEnet_devctrl *)netdev_priv(dev))->MibInfo.ulIfSpeed, 0);
                }


#ifdef SEPARATE_MAC_FOR_WAN_INTERFACES
                /* only release MAC if the port was configured as a WAN port  previously */
                if ( pDevCtrl->wanPort & (1 << swPort)) {
                    kerSysReleaseMacAddress(dev->dev_addr);
                    memmove(sockaddr.sa_data, vnet_dev[0]->dev_addr, ETH_ALEN);
                    bcmenet_set_dev_mac_addr(dev, &sockaddr);
                }
#endif

                pDevCtrl->wanPort &= ~(1 << swPort);
                dev->priv_flags &= (~IFF_WANDEV);
#if !defined(CONFIG_BCM96838)
                dev->priv_flags |= IFF_HW_SWITCH;
#endif
            }

            enet_conf_cos_for_wan(swPort, data);

            if (IsExternalSwitchPort(swPort))
            {
                extsw_set_wanoe_portmap(GET_PORTMAP_FROM_LOGICAL_PORTMAP(pDevCtrl->wanPort,1));
            }
            else
            {
                ethsw_set_wanoe_portmap(GET_PORTMAP_FROM_LOGICAL_PORTMAP(pDevCtrl->wanPort,0));
            }

            TRACE(("Set %s wan port %d", dev->name, (int)data));
            BCM_ENET_DEBUG("bcmenet:SIOCSWANPORT Set %s wan port %s, wan map 0x%x dev flags 0x%x \n",
                    dev->name, data?"ENB": "DISB",
                    (unsigned int)pDevCtrl->wanPort, (unsigned int)dev->priv_flags);

            /* Update the PBVLAN map in the switch if required */
            bcmenet_update_pbvlan_all_bridge();

            val = 0;
            break;

        case SIOCGGMACPORT:
            printk("*** WARNING: SIOGGMACPORT is an Obsolete to be remove soon, please use SIOCGPORTWANPREFERRED\n");
#if defined(CONFIG_BCM_GMAC)
            portMap = pDevCtrl->gmacPort;
#endif
            ubuf = (char *)rq->ifr_data;
            bufLen = MAX_WAN_IFNAMES_LEN;
            goto PORTMAPIOCTL;
        case SIOCGWANPORT:
            portMap = pDevCtrl->wanPort;
            ubuf = e->up_len.uptr;
            bufLen = e->up_len.len;
            goto PORTMAPIOCTL;
        case SIOCGPORTWANONLY:
            portMap = pDevCtrl->wanOnlyPorts;
            ubuf = e->up_len.uptr;
            bufLen = e->up_len.len;
            goto PORTMAPIOCTL;
        case SIOCGPORTWANPREFERRED:
            portMap = pDevCtrl->wanPrefPorts;
            ubuf = e->up_len.uptr;
            bufLen = e->up_len.len;
            goto PORTMAPIOCTL;
        case SIOCGPORTLANONLY:
            portMap = pDevCtrl->lanOnlyPorts;
            ubuf = e->up_len.uptr;
            bufLen = e->up_len.len;
        PORTMAPIOCTL:
            val = 0;
            if (ubuf == NULL) {
                val = -EFAULT;
                break;
            }

            buf = kmalloc(bufLen, GFP_KERNEL);
            if( buf == NULL )
            {
                printk(KERN_ERR "bcmenet:SIOCGWANPORT: kmalloc of %d bytes failed\n", bufLen);
                return -ENOMEM;
            }
            buf[0] = 0;

            for (i = 0; i < MAX_TOTAL_SWITCH_PORTS; i++)
            {
                int vport_idx = LOGICAL_PORT_TO_VPORT(i);
                struct net_device *dev;

                if ((portMap & (1<<i)) == 0 || vport_idx <= 0)
                    continue;

                dev = vnet_dev[vport_idx];
                if (str_append(buf, dev->name, &cum_len, bufLen) == 0)
                    break;

#if defined(BCM_ENET_CB_WAN_PORT_LNX_INTF_SUPPORT)
                {
                    int cb_port;
                    for (cb_port = enet_get_first_crossbar_port(i);
                            cb_port != BP_CROSSBAR_NOT_DEFINED;
                            cb_port = enet_get_next_crossbar_port(i, cb_port))
                    {
                        vport_idx = CBPORT_TO_VPPORT(cb_port);
                        dev = vnet_dev[vport_idx];
                        if (str_append(buf, dev->name, &cum_len, bufLen) == 0)
                            break;
                    }
                }
#endif
            }

#if defined(ENET_GPON_CONFIG)
            if(cmd == SIOCGWANPORT)
            {
                for (i = 0; i < MAX_GPON_IFS; i++) {
                    dev = gponifid_to_dev[i];
                    if (dev == NULL)
                        continue;

                    if (str_append(buf, dev->name, &cum_len, bufLen) == 0)
                        break;
                }
            }
#endif

            if (copy_to_user((void*)ubuf, (void*)buf, cum_len + 1))
            {
                val = -EFAULT;
            }

            kfree(buf);

            break;

#if defined(ENET_GPON_CONFIG)
        case SIOCGPONIF:
            if (copy_from_user((void*)&g, rq->ifr_data, sizeof(struct gponif_data)))
            {
                val = -EFAULT;
                break;
            }
            BCM_ENET_DEBUG("The op is %d (KULI!!!) \n", g.op);
            dumpGemIdxMap(g.gem_map_arr); BCM_ENET_DEBUG("The ifnum is %d \n", g.ifnumber);
            BCM_ENET_DEBUG("The ifname is %s \n", g.ifname);
            switch (g.op) {
                /* Add, Remove, and Show gem_ids */
                case GETFREEGEMIDMAP:
                case SETGEMIDMAP:
                case GETGEMIDMAP:
                    val = set_get_gem_map(g.op, g.ifname, g.ifnumber,
                            g.gem_map_arr);

                    if (val == 0)
                    {
                        if (copy_to_user(rq->ifr_data, (void*)&g, sizeof(struct gponif_data)))
                            val = -EFAULT;
                    }
                    break;

                    /* Create a gpon virtual interface */
                case CREATEGPONVPORT:
                    val = create_gpon_vport(g.ifname);
                    break;

                    /* Delete the given gpon virtual interface */
                case DELETEGPONVPORT:
                    val = delete_gpon_vport(g.ifname);
                    break;

                    /* Delete all gpon virtual interfaces */
                case DELETEALLGPONVPORTS:
                    val = delete_all_gpon_vports();
                    break;

                    /* Set multicast gem index */
                case SETMCASTGEMID:
                    val = set_mcast_gem_id(g.gem_map_arr);
                    break;

                default:
                    val = -EOPNOTSUPP;
                    break;
            }
            break;
#endif

        case SIOCETHSWCTLOPS:

            switch(e->op) {
                case ETHSWDUMPPAGE:
                    BCM_ENET_DEBUG("ethswctl ETHSWDUMPPAGE ioctl");
                    if (e->unit) {
                        bcmsw_dump_page_ext(e->page);
                    } else {
                        bcmeapi_ethsw_dump_page(e->page);
                    }
                    val = 0;
                    break;

                    /* Print out enet iuDMA info - Aug 2010 */
                case ETHSWDUMPIUDMA:
                    bcmeapi_dump_queue(e, pDevCtrl);
                    break;

                    /* Get/Set the iuDMA rx channel for a specific eth port - Jan 2011 */
                case ETHSWIUDMASPLIT:
                    bcmeapi_config_queue(e);
                    break;

                case ETHSWDUMPMIB:
                    BCM_ENET_DEBUG("ethswctl ETHSWDUMPMIB ioctl");
                    if (e->unit) {
                        val = bcmsw_dump_mib_ext(e->port, e->type);
                    }
                    else
                    {
                        val = bcmeapi_ethsw_dump_mib(e->port, e->type, e->queue);
                    }
                    break;

                case ETHSWSWITCHING:
                    BCM_ENET_DEBUG("ethswctl ETHSWSWITCHING ioctl");
                    if (e->type == TYPE_ENABLE) {
                        val = ethsw_set_hw_switching(HW_SWITCHING_ENABLED);
                    } else if (e->type == TYPE_DISABLE) {
                        val = ethsw_set_hw_switching(HW_SWITCHING_DISABLED);
                    } else {
                        val = ethsw_get_hw_switching_state();
                        if (copy_to_user((void*)(&e->status), (void*)&val,
                                    sizeof(int))) {
                            return -EFAULT;
                        }
                        val = 0;
                    }
                    break;

                case ETHSWRXSCHEDULING:
                    BCM_ENET_DEBUG("ethswctl ETHSWRXSCHEDULING ioctl");
                    return bcmeapi_ioctl_ethsw_rxscheduling(e);
                    break;

                case ETHSWWRRPARAM:
                    BCM_ENET_DEBUG("ethswctl ETHSWWRRPARAM ioctl");
                    return bcmeapi_ioctl_ethsw_wrrparam(e);
                    break;

                case ETHSWUSEDEFTXQ:
                    BCM_ENET_DEBUG("ethswctl ETHSWUSEDEFTXQ ioctl");
                    pDevCtrl = (BcmEnet_devctrl *)netdev_priv(dev);
                    return bcmeapi_ioctl_use_default_txq_config(pDevCtrl,e);
                    break;

                case ETHSWDEFTXQ:
                    BCM_ENET_DEBUG("ethswctl ETHSWDEFTXQ ioctl");
                    pDevCtrl = (BcmEnet_devctrl *)netdev_priv(dev);
                    return bcmeapi_ioctl_default_txq_config(pDevCtrl, e);
                    break;

#if defined(RXCHANNEL_BYTE_RATE_LIMIT)
                case ETHSWRXRATECFG:
                    BCM_ENET_DEBUG("ethswctl ETHSWRXRATECFG ioctl");
                    return bcmeapi_ioctl_rx_rate_config(e);
                    break;

                case ETHSWRXRATELIMITCFG:
                    BCM_ENET_DEBUG("ethswctl ETHSWRXRATELIMITCFG ioctl");
                    return bcmeapi_ioctl_rx_rate_limit_config(e);
                    break;
#endif /* defined(RXCHANNEL_BYTE_RATE_LIMIT) */

#if defined(RXCHANNEL_PKT_RATE_LIMIT)
                case ETHSWRXPKTRATECFG:
                    BCM_ENET_DEBUG("ethswctl ETHSWRXRATECFG ioctl");
                    return bcmeapi_ioctl_rx_pkt_rate_config(e);
                    break;

                case ETHSWRXPKTRATELIMITCFG:
                    BCM_ENET_DEBUG("ethswctl ETHSWRXRATELIMITCFG ioctl");
                    return bcmeapi_ioctl_rx_pkt_rate_limit_config(e);
                    break;
#endif

                case ETHSWTEST1:
                    BCM_ENET_DEBUG("ethswctl ETHSWTEST1 ioctl");
                    return bcmeapi_ioctl_test_config(e);
                    break;


                case ETHSWPORTPAUSECAPABILITY:
                    BCM_ENET_DEBUG("ethswctl ETHSWPORTPAUSECAPABILITY ioctl");
                    return bcmeapi_ioctl_ethsw_port_pause_capability(e);
                    break;

                case ETHSWACBCONTROL:
                    return bcmeapi_ioctl_extsw_config_acb(e);
                    break;
                case ETHSWCONTROL:
                    BCM_ENET_DEBUG("ethswctl ETHSWCONTROL ioctl");
                    if(bcm63xx_enet_isExtSwPresent() && e->unit == 1) {
                        return bcmeapi_ioctl_extsw_control(e);
                    } else {
                        return bcmeapi_ioctl_ethsw_control(e);
                    }
                    break;

                case ETHSWPRIOCONTROL:
                    BCM_ENET_DEBUG("ethswctl ETHSWPRIOCONTROL ioctl");
                    if(bcm63xx_enet_isExtSwPresent() && e->unit == 1) {
                        return bcmeapi_ioctl_extsw_prio_control(e);
                    } else {
                        return bcmeapi_ioctl_ethsw_prio_control(e);
                    }
                    break;

                case ETHSWVLAN:
                    BCM_ENET_DEBUG("ethswctl ETHSWVLAN ioctl");
#ifdef CONFIG_BCM96838
                    if (e->type == TYPE_SET) {
                        return bcmeapi_ioctl_ethsw_port_vlanlist_set(e);
                    }
#else
                    return bcmeapi_ioctl_ethsw_vlan(e);
#endif
                    break;
#ifdef BCM_ENET_DEBUG_BUILD
                case ETHSWGETRXCOUNTERS:
                    BCM_ENET_DEBUG("ethswctl ETHSWGETRXCOUNTERS ioctl");
                    return bcmeapi_ioctl_getrxcounters();
                    break;

                case ETHSWRESETRXCOUNTERS:
                    BCM_ENET_DEBUG("ethswctl ETHSWRESETRXCOUNTERS ioctl");
                    return bcmeapi_ioctl_setrxcounters();
                    break;
#endif

                case ETHSWPBVLAN:
                    BCM_ENET_DEBUG("ethswctl ETHSWPBVLAN ioctl");
                    if(bcm63xx_enet_isExtSwPresent() && e->unit == 1) {
                        return bcmeapi_ioctl_extsw_pbvlan(e);
                    } else if (e->unit == 0){
                        return bcmeapi_ioctl_ethsw_pbvlan(e);
                    }
                    return BCM_E_ERROR; /* Invalid switch unit */
                    break;

                case ETHSWCOSCONF:
                    BCM_ENET_DEBUG("ethswctl ETHSWCOSCONF ioctl");
                    return bcmeapi_ioctl_ethsw_cosq_config(e);
                    break;

                case ETHSWCOSSCHED:
                    BCM_ENET_DEBUG("ethswctl ETHSWCOSSCHED ioctl");
                    if(bcm63xx_enet_isExtSwPresent() && e->unit == 1) {
                        return bcmeapi_ioctl_extsw_cosq_sched(e);
                    } else {
                        return bcmeapi_ioctl_ethsw_cosq_sched(e);
                    }
                    break;

                case ETHSWCOSPORTMAP:
                    BCM_ENET_DEBUG("ethswctl ETHSWCOSMAP ioctl");
                    if(bcm63xx_enet_isExtSwPresent() && e->unit == 1) {
                        val = bcmeapi_ioctl_extsw_cosq_port_mapping(e);
                    } else {
                        val = bcmeapi_ioctl_ethsw_cosq_port_mapping(e);
                    }
                    if(val < 0)
                    {
                        if(-BCM_E_ERROR == val)
                            return BCM_E_ERROR;
                        return(val);
                    }

                    if(e->type == TYPE_GET)
                    {
                        /* queue returned from function. Return value to user */
                        if (copy_to_user((void*)(&e->queue), (void*)&val, sizeof(int)))
                        {
                            printk(" _cosq_port_mapping() -- copy_to_user() ret -EFAULT  \n");
                            return -EFAULT;
                        }
                        val = 0;
                    }
                    break;

#if !defined(CONFIG_BCM96368)
                case ETHSWCOSRXCHMAP:
                    BCM_ENET_DEBUG("ethswctl ETHSWRXCOSCHMAP ioctl");
                    return bcmeapi_ioctl_ethsw_cosq_rxchannel_mapping(e);
                    break;

                case ETHSWCOSTXCHMAP:
                    BCM_ENET_DEBUG("ethswctl ETHSWCOSTXCHMAP ioctl");
                    return bcmeapi_ioctl_ethsw_cosq_txchannel_mapping(e);
                    break;
#endif

                case ETHSWCOSTXQSEL:
                    BCM_ENET_DEBUG("ethswctl ETHSWCOSTXQSEL ioctl");
                    return bcmeapi_ioctl_ethsw_cosq_txq_sel(e);
                    break;

                case ETHSWSTATCLR:
                    BCM_ENET_DEBUG("ethswctl ETHSWSTATINIT ioctl");
                    return bcmeapi_ioctl_ethsw_clear_stats
                        ((uint32_t)pDevCtrl->EnetInfo[0].sw.port_map);
                    break;

                case ETHSWSTATPORTCLR:
                    BCM_ENET_DEBUG("ethswctl ETHSWSTATCLEAR ioctl");
                    return bcmeapi_ioctl_ethsw_clear_port_stats(e);
                    break;

                case ETHSWSTATSYNC:
                    BCM_ENET_DEBUG("ethswctl ETHSWSTATSYNC ioctl");
                    return ethsw_counter_collect
                        ((uint32_t)pDevCtrl->EnetInfo[0].sw.port_map, 0);
                    break;

                case ETHSWSTATGET:
                    BCM_ENET_DEBUG("ethswctl ETHSWSTATGET ioctl");
                    return bcmeapi_ioctl_ethsw_counter_get(e);
                    break;

                case ETHSWEMACGET:
                    BCM_ENET_DEBUG("ethswctl ETHSWEMACGET ioctl");
                    return bcmeapi_ioctl_ethsw_get_port_emac(e);
                    break;

                case ETHSWRDPAPORTGET:
                    BCM_ENET_DEBUG("ethswctl ETHSWRDPAPORTGET ioctl");
                    return bcmeapi_ioctl_ethsw_phys_port_to_rdpa_port(e);
                    break;

                case ETHSWEMACCLEAR:
                    BCM_ENET_DEBUG("ethswctl ETHSWEMACCLEAR ioctl");
                    return bcmeapi_ioctl_ethsw_clear_port_emac(e);
                    break;

                case ETHSWPORTRXRATE:
                    BCM_ENET_DEBUG("ethswctl ETHSWPORTRXRATE ioctl");
                    if (e->type == TYPE_GET) {
                        return bcmeapi_ioctl_ethsw_port_irc_get(e);
                    } else {
                        return bcmeapi_ioctl_ethsw_port_irc_set(e);
                    }
                    break;

                case ETHSWPORTTXRATE:
                    BCM_ENET_DEBUG("ethswctl ETHSWPORTTXRATE ioctl");
                    if(bcm63xx_enet_isExtSwPresent() && e->unit == 1) {
                        return bcmeapi_ioctl_extsw_port_erc_config(e);
                    } else {
                    if (e->type == TYPE_GET) {
                        return bcmeapi_ioctl_ethsw_port_erc_get(e);
                    } else {
                        return bcmeapi_ioctl_ethsw_port_erc_set(e);
                    }
                    }
                    break;
                case ETHSWPORTSHAPERCFG:
                    BCM_ENET_DEBUG("ethswctl ETHSWPORTSHAPERCFG ioctl");
                    if(bcm63xx_enet_isExtSwPresent() && e->unit == 1) {
                        return bcmeapi_ioctl_extsw_port_shaper_config(e);
                    }
                    break;
#if defined(CONFIG_BCM96838)
                case ETHSWPORTSALDAL:
                    BCM_ENET_DEBUG("ethswctl ETHSWPORTSALDAL ioctl");
                    if (e->type == TYPE_SET) {
                        return bcmeapi_ioctl_ethsw_sal_dal_set(e);
                    }
                    break;
                    
                case ETHSWPORTMTU:
                    BCM_ENET_DEBUG("ethswctl ETHSWPORTMTU ioctl");
                    if (e->type == TYPE_SET) {
                        return bcmeapi_ioctl_ethsw_mtu_set(e);
                    }
                    break;
                    
                case ETHSWSTATPORTGET:
                    BCM_ENET_DEBUG("ethswctl ETHSWSTATPORTGET ioctl");
                    return bcmeapi_ioctl_ethsw_get_port_stats(e);
                    break;
                    
#endif
                case ETHSWJUMBO:
                    BCM_ENET_DEBUG("ethswctl ETHSWJUMBO ioctl");
                    if (e->unit == 0) {
                        return bcmeapi_ioctl_ethsw_port_jumbo_control(e);
                    } else {
                        return bcmeapi_ioctl_extsw_port_jumbo_control(e);
                    }
                    break;

                case ETHSWPORTTRAFFICCTRL:
                    BCM_ENET_DEBUG("ethswctl ETHSWPORTTRAFFICCTRL ioctl");
#ifdef CONFIG_BCM96838
                    phy_id = pDevCtrl->EnetInfo[0].sw.phy_id[e->port];
                    return bcmeapi_ioctl_ethsw_port_traffic_control(e, phy_id);
#else
                    return bcmeapi_ioctl_ethsw_port_traffic_control(e);
#endif
                    break;

                case ETHSWPORTLOOPBACK:
                    BCM_ENET_DEBUG("ethswctl ETHSWPORTLOOPBACK ioctl");
                    phy_id = pDevCtrl->EnetInfo[0].sw.phy_id[e->port];
                    return bcmeapi_ioctl_ethsw_port_loopback(e, phy_id);
                    break;

                case ETHSWARLACCESS:
                    BCM_ENET_DEBUG("ethswctl ETHSWARLACCESS ioctl");
                    return bcmeapi_ioctl_ethsw_arl_access(e);
                    break;

                case ETHSWPORTDEFTAG:
                    BCM_ENET_DEBUG("ethswctl ETHSWPORTDEFTAG ioctl");
                    return bcmeapi_ioctl_ethsw_port_default_tag_config(e);
                    break;

                case ETHSWCOSPRIORITYMETHOD:
                    BCM_ENET_DEBUG("ethswctl ETHSWCOSPRIORITYMETHOD ioctl");
                    if (bcm63xx_enet_isExtSwPresent() && e->unit == 1 && 
                                 enet_is_mmapped_external_switch(e->unit)) {
                        // call for StarFighter2
                        return bcmeapi_ioctl_extsw_cos_priority_method_config(e);
                    } else {
                        // call for Legcy external & Robo switch
                        return bcmeapi_ioctl_ethsw_cos_priority_method_config(e);
                    }
                    break;

                case ETHSWCOSDSCPPRIOMAP:
                    BCM_ENET_DEBUG("ethswctl ETHSWCOSDSCPPRIOMAP ioctl");
                    if(bcm63xx_enet_isExtSwPresent() && e->unit == 1) {
                        return bcmeapi_ioctl_extsw_dscp_to_priority_mapping(e);
                    } else {
                        return bcmeapi_ioctl_ethsw_dscp_to_priority_mapping(e);
                    }
                    break;

                case ETHSWCOSPCPPRIOMAP:
                    BCM_ENET_DEBUG("ethswctl ETHSWCOSPCPPRIOMAP ioctl");
                    if(bcm63xx_enet_isExtSwPresent() && e->unit == 1) {
                        return bcmeapi_ioctl_extsw_pcp_to_priority_mapping(e);
                    } else {
                        return bcmeapi_ioctl_ethsw_pcp_to_priority_mapping(e);
                    }
                    break;

                case ETHSWCOSPIDPRIOMAP:
                    BCM_ENET_DEBUG("ethswctl ETHSWCOSPCPPRIOMAP ioctl");
                    if((bcm63xx_enet_isExtSwPresent() && e->unit == 1) &&
                                 enet_is_mmapped_external_switch(e->unit)) {
                        // call for StarFighter2
                        return bcmeapi_ioctl_extsw_pid_to_priority_mapping(e); //
                    } else {
                        return bcmeapi_ioctl_ethsw_pid_to_priority_mapping(e);
                    }
                    break;

                case ETHSWREGACCESS:
                    val = enet_ioctl_ethsw_regaccess(e);
                    break;

                case ETHSWSPIACCESS:
                    //BCM_ENET_DEBUG("ethswctl ETHSWSPIACCESS ioctl");
                    val = bcmeapi_ioctl_ethsw_spiaccess(global.pVnetDev0_g->extSwitch->bus_num,
                            global.pVnetDev0_g->extSwitch->spi_ss, global.pVnetDev0_g->extSwitch->spi_cid, e);
                    break;

                case ETHSWPSEUDOMDIOACCESS:
                    //BCM_ENET_DEBUG("ethswctl ETHSWPSEUDOMDIOACCESS ioctl");
                    val = enet_ioctl_ethsw_pmdioaccess(dev, e);
                    break;

                case ETHSWINFO:
                    BCM_ENET_DEBUG("ethswctl ETHSWINFO ioctl");
                    val = enet_ioctl_ethsw_info(dev, e);
                    break;

                case ETHSWLINKSTATUS:
                    BCM_ENET_DEBUG("ethswctl ETHSWLINKSTATUS ioctl");
#ifdef CONFIG_BCM96838
                    phy_id = pDevCtrl->EnetInfo[0].sw.phy_id[e->port];
                    if (e->type == TYPE_GET)
                        val = bcmeapi_ioctl_ethsw_link_status(e, phy_id);
                    else
                    {
#endif
                        swPort = PHYSICAL_PORT_TO_LOGICAL_PORT(e->port, e->unit);

#if defined(CONFIG_BCM_GMAC)
                        if (IsGmacPort( swPort ) && IsLogPortWan(swPort) )
                        {
                            gmac_link_status_changed(GMAC_PORT_ID, e->status,
                                    e->speed, e->duplex);
                        }
#endif
                    link_change_handler(swPort, BP_CROSSBAR_NOT_DEFINED, e->status, e->speed, e->duplex);
                        val = 0;
#ifdef CONFIG_BCM96838
                    }
#endif
                    break;

                case ETHSWEXTPHYLINKSTATUS:
                    BCM_ENET_DEBUG("ethswctl ETHSWEXTPHYLINKSTATUS ioctl %d, %d, val %d, status %d", e->unit, e->port, e->val, e->status);
                    if (e->type == TYPE_SET)
                    {
                        int cbPort;
                        int cbIndex = 0;
                        PHY_STAT phys;

                        for(cbPort = 0; cbPort < BCMENET_CROSSBAR_MAX_EXT_PORTS; cbPort++)
                        {
                            if ( (pDevCtrl->EnetInfo[e->unit].sw.crossbar[cbPort].switch_port == e->port) &&
                                 (pDevCtrl->EnetInfo[e->unit].sw.crossbar[cbPort].phyconn == e->val) )
                            {
                                break;
                            }
                        }

                        if ( cbPort == BCMENET_CROSSBAR_MAX_EXT_PORTS ) {
                           cbPort = BP_CROSSBAR_NOT_DEFINED;
                           cbIndex = 0;
                        }
                        else
                        {
                           cbIndex = cbPort;
                        }

                        memset(&phys, 0, sizeof(PHY_STAT));
                        /* use speed and duplex from ioctl if provided */
                        if ( e->speed != 0 )
                        {
                            if ( e->speed == 1000 )
                            {
                                phys.spd1000 = 1;
                            }
                            else if ( e->speed == 100 )
                            {
                                phys.spd100 = 1;
                            }
                            else
                            {
                                phys.spd10 = 1;
                            }
                            phys.fdx = e->duplex ? 1 : 0;
                        }
                        else
                        {
                            phys = ethsw_phy_stat(e->unit, e->port, cbPort);
                        }
                        phys.lnk = e->status ? 1 : 0;
                        ext_phy_stat[e->unit][e->port][cbIndex] = phys;
                    }
                    else
                    {
                        BCM_ENET_DEBUG("ethswctl ETHSWEXTPHYLINKSTATUS operation not supported (%d)", e->type);
                    }
                    break;

#if defined(SUPPORT_SWMDK)
                case ETHSWKERNELPOLL:
                    val = enet_ioctl_kernel_poll(e);
                    break;
#endif
                case ETHSWPHYCFG:
                    BCM_ENET_DEBUG("ethswctl ETHSWPHYCFG ioctl");
                    val = bcmeapi_ioctl_phy_cfg_get(dev, e);
                    break;

                case ETHSWPHYMODE:
                    BCM_ENET_DEBUG("ethswctl ETHSWPHYMODE ioctl");
                    phy_id = pDevCtrl->EnetInfo[e->unit].sw.phy_id[e->port];
                    val = bcmeapi_ioctl_ethsw_phy_mode(e, phy_id);
                    return val;
                    break;

                case ETHSWGETIFNAME:
                    {
                        int logical_port = PHYSICAL_PORT_TO_LOGICAL_PORT(e->port, e->unit);
                        BCM_ENET_DEBUG("ethswctl ETHSWGETIFNAME ioctl");
                        if ((LOGICAL_PORT_TO_VPORT(logical_port) != -1) &&
                                (vnet_dev[LOGICAL_PORT_TO_VPORT(logical_port)] != NULL)) {
                            char *ifname = vnet_dev[LOGICAL_PORT_TO_VPORT(logical_port)]->name;
                            unsigned int len = sizeof(vnet_dev[LOGICAL_PORT_TO_VPORT(logical_port)]->name);
                            if (copy_to_user((void*)&e->ifname, (void*)ifname, len)) {
                                return -EFAULT;
                            }
                        } else {
                            /* Return error as there is no interface for the given port */
                            return -EFAULT;
                        }
                    }
                    return 0;
                    break;

                case ETHSWMULTIPORT:
                    BCM_ENET_DEBUG("ethswctl ETHSWMULTIPORT ioctl");
                    val = bcmeapi_ioctl_set_multiport_address(e);
                    return val;
                    break;

                case ETHSWDOSCTRL:
                    BCM_ENET_DEBUG("ethswctl ETHSWDOSCTRL ioctl");
                    val = enet_ioctl_ethsw_dos_ctrl(e);
                    return val;
                    break;

                case ETHSWDEBUG:
                    bcmeapi_ioctl_debug_conf(e);
                    break;

                case ETHSWUNITPORT:
                    if (bcm63xx_enet_getPortmapFromName(e->ifname, &e->unit, &e->port_map)){
                        val = -EFAULT;
                    }
                    return val;
                    break;

                case ETHSWSOFTSWITCHING:
                    swPort = PHYSICAL_PORT_TO_LOGICAL_PORT(e->port, e->unit);
                    if (swPort >= MAX_TOTAL_SWITCH_PORTS) {
                        return -EINVAL;
                    }

                    if ( e->type == TYPE_GET ) {
                        if (copy_to_user((void*)(&e->status), (void*)&pDevCtrl->softSwitchingMap, sizeof(int))) {
                            return -EFAULT;
                        }
                    }
                    else {
                        val = bcm_set_soft_switching(swPort, e->type);
                    }
                    break;

                case ETHSWHWSTP:
                    swPort = PHYSICAL_PORT_TO_LOGICAL_PORT(e->port, e->unit);
                    if (swPort >= MAX_TOTAL_SWITCH_PORTS) {
                        return -EINVAL;
                    }

                    if ( e->type == TYPE_GET ) {
                        if (copy_to_user((void*)(&e->status), (void*)&pDevCtrl->stpDisabledPortMap, sizeof(int))) {
                            return -EFAULT;
                        }
                    }
                    else {
                        vport = LOGICAL_PORT_TO_VPORT(swPort);
                        if (e->type == TYPE_ENABLE) {
                            pDevCtrl->stpDisabledPortMap &= ~(1 << swPort);
                            if ( vnet_dev[vport]->flags & IFF_UP) {
                                bcmeapi_ethsw_set_stp_mode(e->unit, e->port, REG_PORT_STP_STATE_BLOCKING);
                                dev_change_flags(vnet_dev[vport], (vnet_dev[vport]->flags & ~IFF_UP));
                                dev_change_flags(vnet_dev[vport], (vnet_dev[vport]->flags | IFF_UP));
                            }
                            else {
                                bcmeapi_ethsw_set_stp_mode(e->unit, e->port, REG_PORT_STP_STATE_DISABLED);
                            }
                        }
                        else if (e->type == TYPE_DISABLE) {
                            pDevCtrl->stpDisabledPortMap |= (1 << swPort);
                            bcmeapi_ethsw_set_stp_mode(e->unit, e->port, REG_PORT_NO_SPANNING_TREE);
                        }
                        else {
                            return -EINVAL;
                        }
                    }
                    break;

#ifdef CONFIG_BCM96838
                case ETHSWPHYAUTONEG:
                    BCM_ENET_DEBUG("ethswctl ETHSWPHYAUTONEG ioctl");
                    phy_id = pDevCtrl->EnetInfo[e->unit].sw.phy_id[e->port];
                    val = bcmeapi_ioctl_ethsw_phy_autoneg_info(e, phy_id);
                    return val;
                    break;
                case ETHSWPORTTRANSPARENT:
                    BCM_ENET_DEBUG("ethswctl ETHSWPORTTRANSPARENT ioctl");
                    if (e->type == TYPE_SET) {
                        return bcmeapi_ioctl_ethsw_port_transparent_set(e);
                    }
                    break;
                case ETHSWPORTVLANISOLATION:
                    BCM_ENET_DEBUG("ethswctl ETHSWPORTVLANISOLATION ioctl");
                    if (e->type == TYPE_SET) {
                        return bcmeapi_ioctl_ethsw_port_vlan_isolation_set(e);
                    }
                    break;
                case ETHSWCPUMETER:
                    BCM_ENET_DEBUG("ethswctl ETHSWCPUMETER ioctl");
                    return bcmeapi_ioctl_ethsw_cpu_meter_set(e);
                    break;
                case ETHSWPHYAUTONEGCAPADV:
                    BCM_ENET_DEBUG("ethswctl ETHSWPHYAUTONEGCAPADV ioctl");
                    phy_id = pDevCtrl->EnetInfo[e->unit].sw.phy_id[e->port];
                    return bcmeapi_ioctl_ethsw_phy_autoneg_cap_adv_set(e, phy_id);
                    break;
#endif
/* Technicolor: START: Allow selection of a crossbar port for LAN/WAN operation
**   Inputs: e->port: the external crossbar port to be controlled
**           e->val:  0 = set to WAN, !0 = set to LAN
**           e->type: TYPE_SET = set the LanWanMux value, OTHER: just return the current setting
**   Outputs: e->val: 0 = port was set to WAN, 1 = port was set to LAN
*/
                case ETHSWLANWANMUX:
                    if (e->port < 0 || e->port >= BCMENET_CROSSBAR_MAX_EXT_PORTS )
                        return -EINVAL;
#if !defined(CONFIG_5x3_CROSSBAR_SUPPORT) /* 5x3 crossbar */
                        return -EINVAL;
#endif
                    if ( e->type == TYPE_SET ) {
                        /* Get a fresh set of EnetInfo settings */
                        BpGetEthernetMacInfo(pDevCtrl->EnetInfo, BP_MAX_ENET_MACS);

                        if (!(BP_IS_CROSSBAR_PORT_DEFINED(pDevCtrl->EnetInfo[0].sw, e->port) &&
                            BP_IS_CROSSBAR_PORT_DEFINED(pDevCtrl->EnetInfo[1].sw, e->port)))
                            return -EINVAL;

                        if ( e->val )
                            LanWanMux |= (1 << e->port);
                        else
                            LanWanMux &= ~(1 << e->port);

                        /* limit each cross bar to a single port */
                        enet_assign_crossbar_ports(pDevCtrl->EnetInfo, LanWanMux);

                        /* reconfigure the crossbar to match the (possibly) new settings; */
                        bcmeapi_ethsw_reinit();
                    }else {
                        ETHERNET_MAC_INFO plocEnetInfo[BP_MAX_ENET_MACS];

                        /* NG-97506:for get operation, shouldn't read EnetInfo again to pDevCtrl->EnetInfo.
                         * it will lead to pDevCtrl->EnetInfo be updated to orginal configuration unexpectly.
                         * it will lead to strange behavior, such as eth3 is forced to up, lan/wan port is switched to WAN port.
                         * read to a local structure insteadly */
                        BpGetEthernetMacInfo(plocEnetInfo, BP_MAX_ENET_MACS);
                        if (!(BP_IS_CROSSBAR_PORT_DEFINED(plocEnetInfo[0].sw, e->port) &&
                            BP_IS_CROSSBAR_PORT_DEFINED(plocEnetInfo[1].sw, e->port)))
                            return -EINVAL;
                    }

                    /* compute return value */
                    e->val = (LanWanMux >> e->port) & 1;
                    break;
/* Technicolor: END: Allow selection of a crossbar port for LAN/WAN operation */
                case ETHSWMIRROR:
                    if ( e->type == TYPE_GET ) {
                        bcmeapi_ioctl_ethsw_port_mirror_get(e);
                    }
                    else {
                        bcmeapi_ioctl_ethsw_port_mirror_set(e);
                    }
                    break;

#if defined(CONFIG_BCM_SWITCH_PORT_TRUNK_SUPPORT)
                case ETHSWPORTTRUNK:
                    if ( e->type == TYPE_GET ) {
                        bcmeapi_ioctl_ethsw_port_trunk_get(e);
                    }
                    else {
                        bcmeapi_ioctl_ethsw_port_trunk_set(e);
                    }
                    break;
#endif /* CONFIG_BCM_SWITCH_PORT_TRUNK_SUPPORT */

#if defined(CONFIG_BCM_ETH_PWRSAVE)
                case ETHSWPHYAPD:
                    if ( e->type == TYPE_GET )
                        e->val = BcmPwrMngtGetEthAutoPwrDwn();
                    else
                        BcmPwrMngtSetEthAutoPwrDwn(e->val);
                    val = 0;
                    break;

                case ETHSWPHYEEE:
                    if ( e->type == TYPE_GET )
                        e->val = BcmPwrMngtGetEnergyEfficientEthernetEn();
                    else
                        BcmPwrMngtSetEnergyEfficientEthernetEn(e->val);
                    val = 0;
                    break;

                case ETHSWPHYAUTOGREEEN:
                    if ( e->type == TYPE_GET )
                        e->val = BcmPwrMngtGetAutoGreeEn();
                    else
                        BcmPwrMngtSetAutoGreeEn(e->val);
                    val = 0;
                    break;
#endif
                default:
                    BCM_ENET_DEBUG("ethswctl unsupported ioctl");
                    val = -EOPNOTSUPP;
                    break;
            }
            break;

        case SIOCETHCTLOPS:
            switch(ethctl->op) {
                case ETHGETNUMTXDMACHANNELS:
                    val = bcmeapi_get_num_txques(ethctl);
                    break;

                case ETHSETNUMTXDMACHANNELS:
                    val = bcmeapi_set_num_txques(ethctl);
                    break;

                case ETHGETNUMRXDMACHANNELS:
                    val = bcmeapi_get_num_rxques(ethctl);
                    break;

                case ETHSETNUMRXDMACHANNELS:
                    val = bcmeapi_set_num_rxques(ethctl);
                    break;

                case ETHGETSOFTWARESTATS:
                    pDevCtrl = (BcmEnet_devctrl *)netdev_priv(dev);
                    display_software_stats(pDevCtrl);
                    val = 0;
                    break;

                case ETHSETSPOWERUP:
                    swPort = port_id_from_dev(dev);
                    if (swPort >= MAX_TOTAL_SWITCH_PORTS)
                    {
                        printk("ETHSETSPOWERUP : Invalid swPort: %d \n", swPort);
                        return -EINVAL;
                    }

                    ethsw_switch_manage_port_power_mode(swPort, 1);
                    val = 0;
                    break;

                case ETHSETSPOWERDOWN:
                    swPort = port_id_from_dev(dev);
                    if (swPort >= MAX_TOTAL_SWITCH_PORTS)
                    {
                        printk("ETHSETSPOWERDOWN : Invalid swPort: %d \n", swPort);
                        return -EINVAL;
                    }

                    ethsw_switch_manage_port_power_mode(swPort, 0);
                    val = 0;
                    break;

                case ETHGETMIIREG:       /* Read MII PHY register. */
                    BCM_ENET_DEBUG("phy_id: %d; reg_num = %d \n", ethctl->phy_addr, ethctl->phy_reg);
                    {
                        uint16 val;
                        down(&bcm_ethlock_switch_config);
                        ethsw_phyport_rreg2(ethctl->phy_addr,
                                       ethctl->phy_reg, &val, ethctl->flags);
                        BCM_ENET_DEBUG("phy_id: %d;   reg_num = %d  val = 0x%x\n",
                                ethctl->phy_addr, ethctl->phy_reg, val);
                        up(&bcm_ethlock_switch_config);
                        ethctl->ret_val = val;
                    }
                    break;

                case ETHSETMIIREG:       /* Write MII PHY register. */
                    BCM_ENET_DEBUG("phy_id: %d; reg_num = %d; val = 0x%x \n", ethctl->phy_addr,
                            ethctl->phy_reg, ethctl->val);
                    {
                        uint16 val = ethctl->val;
                        down(&bcm_ethlock_switch_config);
                        ethsw_phyport_wreg2(ethctl->phy_addr,
                               ethctl->phy_reg, &val, ethctl->flags);
                        BCM_ENET_DEBUG("phy_id: %d; reg_num = %d  val = 0x%x\n",
                                ethctl->phy_addr, ethctl->phy_reg, val);
                        up(&bcm_ethlock_switch_config);
                    }
                    break;

                default:
                    val = -EOPNOTSUPP;
                    break;
            }
            break;

        default:
            val = -EOPNOTSUPP;
            break;
    }

    return val;
}

#ifdef RDPA_VPORTS
static int delete_all_rdpa_vports(void)
{
    int i;
    struct net_device *dev;

    synchronize_net();

    for (i = rdpa_if_lan0; i < rdpa_if_lan_max; i++)
    {
        if (!(dev = rdpa_vport_to_dev[i]))
            continue;

        rdpa_vport_to_dev[i] = NULL;
        if (memcmp(vnet_dev[0]->dev_addr, dev->dev_addr, ETH_ALEN))
            kerSysReleaseMacAddress(dev->dev_addr);

        /* No need to call free_netdev after this as dev->destructor
           is set to free_netdev */
        unregister_netdev(dev);
    }

    return 0;
}

static const char* sidif_name = "sid%d";

int create_rdpa_vport(int sid, int physical_port, char *name)
{
    struct net_device *dev;
    struct sockaddr sockaddr;
    BcmEnet_devctrl *pDevCtrl;
    int rc;
    rdpa_if rdpa_port = rdpa_if_lan0 + sid;

    if (rdpa_port < rdpa_if_lan0 || rdpa_port >= G9991_DEBUG_RDPA_PORT)
    {
        printk("SID %d out of range (%d)\n", sid, rdpa_if_lan_max -
            rdpa_if_lan0);
        return -ENOMEM;
    }

    if ((dev = rdpa_vport_to_dev[rdpa_port]))
    {
        printk("SID %d already registered to dev %s\n", sid, dev->name);
        return -EPERM;
    }

    if (!(dev = alloc_etherdev(sizeof(BcmEnet_devctrl))))
    {
        rc = -ENOMEM;
        goto Exit;
    }

    memset(netdev_priv(dev), 0, sizeof(BcmEnet_devctrl));
    SET_MODULE_OWNER(dev);
    dev->netdev_ops = vnet_dev[0]->netdev_ops;
    dev->priv_flags = vnet_dev[0]->priv_flags;

    if ((rc = dev_alloc_name(dev, name ? : sidif_name)) < 0)
        goto Exit;

    pDevCtrl = netdev_priv(dev);
    dev->base_addr = pDevCtrl->sw_port_id = SID_PORT_ID;
    pDevCtrl->physical_inx = physical_port;
    netdev_path_set_hw_port(dev, sid, BLOG_SIDPHY);
    
    if ((rc = register_netdev(dev)))
        goto Exit;
    
    rdpa_vport_to_dev[rdpa_port] = dev;
    rc = bcmeapi_create_vport(dev);
    if (rc)
        goto Exit;

    memmove(sockaddr.sa_data, vnet_dev[0]->dev_addr, ETH_ALEN);
    bcmenet_set_dev_mac_addr(dev, &sockaddr);
    printk("%s: sid %d on phys_port %d MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n", dev->name,
        sid, physical_port, dev->dev_addr[0], dev->dev_addr[1], dev->dev_addr[2],
        dev->dev_addr[3], dev->dev_addr[4], dev->dev_addr[5]);

Exit:
    if (rc)
    {
        printk("Failed (%d) creating sid %d/port %d\n", rc, sid, physical_port);
        if (dev)
            unregister_netdev(dev);
        free_netdev(dev);
        rdpa_vport_to_dev[rdpa_port] = NULL;
    }

    return rc;
}

static int create_attached_ports(int bp_port_idx)
{
    int port = 0, rc;
    BP_ATTACHED_INFO bp_attached_info;

    if (BpGetAttachedInfo(bp_port_idx, &bp_attached_info))
    {
        printk("Failed to get attached ports\n");
        return -EPERM;
    }

    for (; port < BP_MAX_ATTACHED_PORTS; ++port)
    {
        if (!(bp_attached_info.port_map & 1 << port))
            continue;

        if ((rc = create_rdpa_vport(bp_attached_info.ports[port], bp_port_idx,
                bp_attached_info.devnames[port])))
            return rc;
    }

    return 0;
}

static int create_rdpa_vports(void)
{
    int bp_port_idx = 0, rc = 0;
    ETHERNET_MAC_INFO *enetinfo =
        &((BcmEnet_devctrl *)netdev_priv(vnet_dev[0]))->EnetInfo[0];

    for (; bp_port_idx < BP_MAX_SWITCH_PORTS; ++bp_port_idx)
    {
        if (!(enetinfo->sw.port_map & 1 << bp_port_idx))
            continue;

        if (enetinfo->sw.port_flags[bp_port_idx] & PORT_FLAG_ATTACHED)
        {
            if ((rc = create_attached_ports(bp_port_idx)))
                return rc;
        }
    }

    return 0;
}
#endif

#if defined(ENET_GPON_CONFIG)

static const char* gponif_name = "gpon%d";

/****************************************************************************/
/* Create a new GPON Virtual Interface                                      */
/* Inputs: name = the name for the gpon i/f. If not specified, a name of    */
/*          gponXX will be assigned where XX is the next available number   */
/* Returns: 0 on success; a negative value on failure                       */
/* Notes: 1. The max num gpon virtual interfaces is limited to MAX_GPON_IFS.*/
/****************************************************************************/
static int create_gpon_vport(char *name)
{
    struct net_device *dev;
    struct sockaddr sockaddr;
    BcmEnet_devctrl *pDevCtrl = NULL;
    int status, ifid = 0;
    PHY_STAT phys;

    phys.lnk = 0;
    phys.fdx = 0;
    phys.spd1000 = 0;
    phys.spd100 = 0;

    /* Verify that given name is valid */
    if (name[0] != 0) {
        if (!dev_valid_name(name)) {
            printk("The given interface name is invalid \n");
            return -EINVAL;
        }

        /* Verify that no interface exists with this name */
        dev = dev_get_by_name(&init_net, name);
        if (dev != NULL) {
            dev_put(dev);
            printk("The given interface already exists \n");
            return -EINVAL;
        }
    }

    /* Find a free id for the gponif */
    for (ifid = 0; ifid < MAX_GPON_IFS; ifid++) {
        if (gponifid_to_dev[ifid] == NULL)
            break;
    }
    /* No free id is found. We can't create a new gpon virtual i/f */
    if (ifid == MAX_GPON_IFS) {
        printk("Create Failed as the number of gpon interfaces is "
                "limited to %d\n", MAX_GPON_IFS);
        return -EPERM;
    }

    /* Allocate the dev */
    if ((dev = alloc_etherdev(sizeof(BcmEnet_devctrl))) == NULL) {
        return -ENOMEM;
    }
    /* Set the private are to 0s */
    memset(netdev_priv(dev), 0, sizeof(BcmEnet_devctrl));

    /* Set the pDevCtrl->dev to dev */
    pDevCtrl = netdev_priv(dev);
    pDevCtrl->dev = dev;

    /* Assign name to the i/f */
    if (name[0] == 0) {
        /* Allocate and assign a name to the i/f */
        dev_alloc_name(dev, gponif_name);
    } else {
        /* Assign the given name to the i/f */
        strcpy(dev->name, name);
    }

    SET_MODULE_OWNER(dev);

    dev->netdev_ops       = vnet_dev[0]->netdev_ops;
    dev->priv_flags       = vnet_dev[0]->priv_flags;

    /* Set this flag to block forwarding of traffic between
       GPON virtual interfaces */
    dev->priv_flags       |= IFF_WANDEV;

    dev->base_addr        = GPON_PORT_ID;
    bcmeapi_create_vport(dev);
    /* For now, let us use this base_addr field to identify GPON port */
    /* TBD: Change this to a private field in pDevCtrl for all eth and gpon
       virtual ports */
    pDevCtrl->sw_port_id  = GPON_PORT_ID;

    netdev_path_set_hw_port(dev, GPON_PORT_ID, BLOG_GPONPHY);
    dev->path.hw_subport_mcast_idx = NETDEV_PATH_HW_SUBPORTS_MAX;

    /* The unregister_netdevice will call the destructor
       through netdev_run_todo */
    dev->destructor        = free_netdev;

    /* Note: Calling from ioctl, so don't use register_netdev
       which takes rtnl_lock */
    status = register_netdevice(dev);

    if (status != 0) {
        unregister_netdevice(dev);
        return status;
    }

    /* Indicate that ifid is used */
    freeid_map[ifid] = 1;

    /* Store the dev pointer at the index given by ifid */
    gponifid_to_dev[ifid] = dev;

    /* Store the ifid in dev private area */
    pDevCtrl->gponifid = ifid;

#ifdef SEPARATE_MAC_FOR_WAN_INTERFACES
    status = kerSysGetMacAddress(dev->dev_addr, dev->ifindex);
    if (status < 0)
#endif
    {
        memmove(dev->dev_addr, vnet_dev[0]->dev_addr, ETH_ALEN);
    }
    memmove(sockaddr.sa_data, dev->dev_addr, ETH_ALEN);
    dev->netdev_ops->ndo_set_mac_address(dev, &sockaddr);

    printk("%s: MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
            dev->name,
            dev->dev_addr[0],
            dev->dev_addr[1],
            dev->dev_addr[2],
            dev->dev_addr[3],
            dev->dev_addr[4],
            dev->dev_addr[5]
          );

    return 0;
}

/****************************************************************************/
/* Delete GPON Virtual Interface                                            */
/* Inputs: ifname = the GPON virtual interface name                         */
/****************************************************************************/
static int delete_gpon_vport(char *ifname)
{
    int i, ifid = 0;
    struct net_device *dev = NULL;
    BcmEnet_devctrl *pDevCtrl = NULL;

    /* Get the device structure from ifname */
    for (ifid = 0; ifid < MAX_GPON_IFS; ifid++) {
        dev = gponifid_to_dev[ifid];
        if (dev != NULL) {
            if (strcmp(dev->name, ifname) == 0) {
                break;
            }
        }
    }

    if (ifid >= MAX_GPON_IFS) {
        printk("delete_gpon_vport() : No such device \n");
        return -ENXIO;
    }

    /* Get the ifid of this interface */
    pDevCtrl = (BcmEnet_devctrl *)netdev_priv(dev);
    ifid =  pDevCtrl->gponifid;

    /* */
    synchronize_net();

    /* Remove the gem_ids supported by this interface */
    for (i = 0; i < MAX_GEM_IDS; i++) {
        remove_gemid_to_gponif_mapping(i, ifid);
    }

    if(memcmp(vnet_dev[0]->dev_addr, dev->dev_addr, ETH_ALEN)) {
        kerSysReleaseMacAddress(dev->dev_addr);
    }

    /* Delete the given gopn virtual interfaces. No need to call free_netdev
       after this as dev->destructor is set to free_netdev */
    unregister_netdevice(dev);
    gponifid_to_dev[ifid] = NULL;

    /* Free the ifid */
    freeid_map[ifid] = 0;

    /* Set the default gemid for this ifid as 0 */
    default_gemid[ifid] = 0;

    return 0;
}

/****************************************************************************/
/* Delete GPON Virtual Interface                                            */
/* Inputs: port = the GPON virtual interface number                         */
/****************************************************************************/
static int delete_all_gpon_vports(void)
{
    int i;
    struct net_device *dev = NULL;

    /* */
    synchronize_net();

    /* Make no gemid is assigned to an interface */
    initialize_gemid_to_ifIdx_mapping();

    /* Delete all gpon virtual interfaces */
    for (i = 0; i < MAX_GPON_IFS; i++) {
        if (gponifid_to_dev[i] == NULL) {
            continue;
        }
        dev = gponifid_to_dev[i];

        if(memcmp(vnet_dev[0]->dev_addr, dev->dev_addr, ETH_ALEN)) {
            kerSysReleaseMacAddress(dev->dev_addr);
        }

        /* No need to call free_netdev after this as dev->destructor
           is set to free_netdev */
        unregister_netdevice(dev);
        gponifid_to_dev[i] = NULL;
    }

    /* Free the ifids */
    for (i = 0; i < MAX_GPON_IFS; i++) {
        freeid_map[i] = 0;
    }

    /* Make default gem_ids for all ifids as 0 */
    for (i = 0; i < MAX_GPON_IFS; i++) {
        default_gemid[i] = 0;
    }

    return 0;
}

/****************************************************************************/
/* Set the multicast gem ID in GPON virtual interface                       */
/* Inputs: multicast gem port index                                         */
/****************************************************************************/
static int set_mcast_gem_id(uint8 *pgem_map_arr)
{
    int i;
    int mcast_gemid;
    int ifIdx;
    int ifid;
    struct net_device *dev = NULL;
    bool found = false;

    for (i = 0; i < MAX_GEM_IDS; i++) {
        if (pgem_map_arr[i]) {
            mcast_gemid = i;
            found = true;
            break;
        }
    }
    if (!found)
    {
        printk("Error - set_mcast_gem_id() : No gemIdx in gem_map\n");
        return -1;
    }
    for (ifIdx = 0; ifIdx < MAX_GPON_IFS_PER_GEM; ++ifIdx)
    {
        if (gem_to_gponifid[mcast_gemid][ifIdx] == UNASSIGED_IFIDX_VALUE)
        {
            break;
        }
        ifid = gem_to_gponifid[mcast_gemid][ifIdx];
        if (ifid >= 0 && ifid < MAX_GPON_IFS) {
            if (gponifid_to_dev[ifid] != NULL) {
                dev = gponifid_to_dev[ifid];
                netdev_path_set_hw_subport_mcast_idx(dev, mcast_gemid);
                BCM_ENET_INFO("mcast_gem <%d> added to if <%s>\n",mcast_gemid,dev->name);
            }
        }
    }
    return 0;
}
/****************************************************************************/
/* Set the GEM Mask or Get the GEM Mask or Get the Free GEM Mask            */
/* Inputs: ifname = the interface name                                      */
/*         pgemmask = ptr to GEM mask if op is Set                          */
/* Outputs: pgem_map = ptr to GEM mask of an interface for Get op           */
/*                     ptr to Free GEM mask for GetFree op                  */
/* Returns: 0 on success; non-zero on failure                               */
/****************************************************************************/
static int set_get_gem_map(int op, char *ifname, int ifnum, uint8 *pgem_map_arr)
{
    int i, ifid = 0, count = 0, def_gem = 0;
    struct net_device *dev = NULL;
    BcmEnet_devctrl *pDevCtrl = NULL;

    /* Check whether ifname is all */
    if (ifname[0] != 0) {
        /* The ifname is not all */

        /* Get the device structure from ifname */
        for (ifid = 0; ifid < MAX_GPON_IFS; ifid++) {
            if (gponifid_to_dev[ifid] != NULL) {
                if (strcmp(gponifid_to_dev[ifid]->name, ifname) == 0) {
                    dev = gponifid_to_dev[ifid];
                    break;
                }
            }
        }

        if (dev == NULL) {
            printk("set_get_gem_map() : No such device \n");
            return -ENXIO;
        }

        /* Get the pointer to DevCtrl private structure */
        pDevCtrl = (BcmEnet_devctrl *)netdev_priv(dev);

        if (op == GETGEMIDMAP) {
            initGemIdxMap(pgem_map_arr);
            /* Get the gem ids of given interface */
            for (i = 0; i < MAX_GEM_IDS; i++) {
                if (is_gemid_mapped_to_gponif(i, ifid) == TRUE) {
                    pgem_map_arr[i] = 1;
                }
            }
        } else if (op == GETFREEGEMIDMAP) {
            initGemIdxMap(pgem_map_arr);
            /* Get the free gem ids */
            for (i = 0; i < MAX_GEM_IDS; i++) {
                if (get_first_gemid_to_ifIdx_mapping(i) == UNASSIGED_IFIDX_VALUE) {
                    pgem_map_arr[i] = 1;
                }
            }
        } else if (op == SETGEMIDMAP) {
            //printk("SETGEMIDMAP: Given gemmap is ");
            dumpGemIdxMap(pgem_map_arr);
            /* Set the gem ids for given interface */
            for (i = 0; i < MAX_GEM_IDS; i++) {
                /* Check if gem_id(=i) is already a member */
                if (is_gemid_mapped_to_gponif(i, ifid) == TRUE) {
                    /* gem_id is already a member */
                    /* Check whether to remove it or not */
                    if (!(pgem_map_arr[i])) {
                        /* It is not a member in the new
                           gem_map_arr, so remove it */
                        remove_gemid_to_gponif_mapping(i, ifid);
                    } else {
                        count++;
                        if (count == 1)
                            def_gem = i;
                    }
                }
                else if (pgem_map_arr[i]) {
                    /* gem_id(=i) is not a member and is in the new
                       gem_map, so add it */
                    if (add_gemid_to_gponif_mapping(i, ifid) < 0)
                    {
                        printk("Error while adding gem<%d> to if<%s>\n",i,ifname);
                        return -ENXIO;
                    }
                    count++;
                    if (count == 1)
                        def_gem = i;
                }
            }
            pDevCtrl->gem_count = count;

            /* Set the default_gemid[ifid] when count is 1 */
            if (count == 1) {
                default_gemid[ifid] = def_gem;
            }
        }
    } else {
        /* ifname is all */
        if (op == GETGEMIDMAP) {
            /* Give the details if there is an interface at given ifnumber */
            initGemIdxMap(pgem_map_arr);
            if (gponifid_to_dev[ifnum] != NULL) {
                dev = gponifid_to_dev[ifnum];
                /* Get the gem ids of given interface */
                for (i = 0; i < MAX_GEM_IDS; i++) {
                    if (is_gemid_mapped_to_gponif(i, ifnum) == TRUE) {
                        pgem_map_arr[i] = 1;
                    }
                }
                /* Get the interface name */
                strcpy(ifname, dev->name);
            }
        }
        else {
            printk("valid ifname is required \n");
            return -ENXIO;
        }
    }

    return 0;
}

/****************************************************************************/
/* Dump the Gem Index Map                                                   */
/* Inputs: Gem Index Map Array                                              */
/* Outputs: None                                                            */
/* Returns: None                                                            */
/****************************************************************************/
static void dumpGemIdxMap(uint8 *pgem_map_arr)
{
    int gemDumpIdx = 0;
    bool gemIdxDumped = false;

    BCM_ENET_DEBUG("GemIdx Map: ");
    for (gemDumpIdx = 0; gemDumpIdx < MAX_GEM_IDS; gemDumpIdx++)
    {
        if (pgem_map_arr[gemDumpIdx])
        {
            BCM_ENET_DEBUG("%d ", gemDumpIdx);
            gemIdxDumped = true;
        }
    }
    if (!gemIdxDumped)
    {
        BCM_ENET_DEBUG("No gem idx set");
    }
}

/****************************************************************************/
/* Initialize the Gem Index Map                                                   */
/* Inputs: Gem Index Map Array                                              */
/* Outputs: None                                                            */
/* Returns: None                                                            */
/****************************************************************************/
static void initGemIdxMap(uint8 *pgem_map_arr)
{
    int i=0;
    for (i = 0; i < MAX_GEM_IDS; i++)
    {
        pgem_map_arr[i] = 0;
    }
}
#endif

#if defined(ENET_EPON_CONFIG)
static int create_epon_vport(const char *name)

{
    struct net_device *dev;
    struct sockaddr sockaddr;
    BcmEnet_devctrl *pDevCtrl = NULL;
    int status, ifid = 0;
    PHY_STAT phys;

    phys.lnk = 0;
    phys.fdx = 0;
    phys.spd1000 = 0;
    phys.spd100 = 0;
    /* Verify that given name is valid */
    if (name[0] != 0) {
        if (!dev_valid_name(name)) {
            printk("The given interface name is invalid \n");
            return -EINVAL;
        }

        /* Verify that no interface exists with this name */
        dev = dev_get_by_name(&init_net, name);
        if (dev != NULL) {
            dev_put(dev);
            printk("The given interface already exists \n");
            return -EINVAL;
        }
    }

    /* Find a free id for the eponif */
    for (ifid = 0; ifid < MAX_EPON_IFS; ifid++) {
        if (eponifid_to_dev[ifid] == NULL)
            break;
    }


    /* No free id is found. We can't create a new epon virtual i/f */
    if (ifid == MAX_EPON_IFS) {
        printk("Create Failed as the number of epon interfaces is "
                "limited to %d\n", MAX_EPON_IFS);
        return -EPERM;
    }

    /* Allocate the dev */
    if ((dev = alloc_etherdev(sizeof(BcmEnet_devctrl))) == NULL) {
        return -ENOMEM;
    }
    /* Set the private are to 0s */
    memset(netdev_priv(dev), 0, sizeof(BcmEnet_devctrl));

    /* Set the pDevCtrl->dev to dev */
    pDevCtrl = netdev_priv(dev);
    pDevCtrl->dev = dev;
    pDevCtrl->default_txq = 0;
    pDevCtrl->use_default_txq = 0;

    /* Assign name to the i/f */
    if (name[0] == 0)
    {
        /* Allocate and assign a name to the i/f */
        dev_alloc_name(dev, eponif_name);
    }
    else
    {
        /* Assign the given name to the i/f */
        strcpy(dev->name, name);
        dev_alloc_name(dev, dev->name);
    }

    SET_MODULE_OWNER(dev);

    dev->netdev_ops  = vnet_dev[0]->netdev_ops;
    dev->priv_flags  = vnet_dev[0]->priv_flags;
    dev->features    = vnet_dev[0]->features;
    dev->vlan_features = vnet_dev[0]->vlan_features;

    /* Set this flag to block forwarding of traffic between
       EPON virtual interfaces */
    dev->priv_flags       |= IFF_WANDEV;

    dev->base_addr        = EPON_PORT_ID;
    bcmeapi_create_vport(dev);
    /* For now, let us use this base_addr field to identify EPON port */
    /* TBD: Change this to a private field in pDevCtrl for all eth and gpon
       virtual ports */
    pDevCtrl->sw_port_id  = EPON_PORT_ID;

    dev->path.hw_subport_mcast_idx = NETDEV_PATH_HW_SUBPORTS_MAX;

    /* The unregister_netdevice will call the destructor
       through netdev_run_todo */
    dev->destructor        = free_netdev;


    /* Note: Calling from ioctl, so don't use register_netdev
       which takes rtnl_lock */
    status = register_netdev(dev);

    if (status != 0) {
        unregister_netdev(dev);
        return status;
    }

    /* Store the dev pointer at the index given by ifid */
    eponifid_to_dev[ifid] = dev;

#ifdef SEPARATE_MAC_FOR_WAN_INTERFACES
    status = kerSysGetMacAddress(dev->dev_addr, dev->ifindex);
    if (status < 0)
#endif
    {
        memmove(dev->dev_addr, vnet_dev[0]->dev_addr, ETH_ALEN);
    }

    memmove(sockaddr.sa_data, dev->dev_addr, ETH_ALEN);


    dev->netdev_ops->ndo_set_mac_address(dev, &sockaddr);
    netdev_path_set_hw_port(dev, EPON_PORT_ID, BLOG_EPONPHY);

    printk("%s: MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
            dev->name,
            dev->dev_addr[0],
            dev->dev_addr[1],
            dev->dev_addr[2],
            dev->dev_addr[3],
            dev->dev_addr[4],
            dev->dev_addr[5]
          );

    return 0;
}

/****************************************************************************/
/* Delete All EPON Virtual Interface                                            */
/****************************************************************************/
static int delete_all_epon_vports(void)
{
    int i;
    struct net_device *dev = NULL;

    /* */
    synchronize_net();

    /* Delete all epon virtual interfaces */
    for (i = 0; i < MAX_EPON_IFS; i++) {
        if (eponifid_to_dev[i] == NULL) {
            continue;
        }
        dev = eponifid_to_dev[i];

        if(memcmp(vnet_dev[0]->dev_addr, dev->dev_addr, ETH_ALEN)) {
            kerSysReleaseMacAddress(dev->dev_addr);
        }

        /* No need to call free_netdev after this as dev->destructor
           is set to free_netdev */
        unregister_netdev(dev);
        eponifid_to_dev[i] = NULL;
    }

    return 0;
}

int create_oam_vport(char *name)
{
    struct net_device *dev;
    struct sockaddr sockaddr;
    BcmEnet_devctrl *pDevCtrl = NULL;
    int status = 0;
    PHY_STAT phys;

    phys.lnk = 0;
    phys.fdx = 0;
    phys.spd1000 = 0;
    phys.spd100 = 0;
    /* Verify that given name is valid */
    if (name[0] != 0) {
        if (!dev_valid_name(name)) {
            printk("The given interface name is invalid \n");
            return -EINVAL;
        }

        /* Verify that no interface exists with this name */
        dev = dev_get_by_name(&init_net, name);
        if (dev != NULL) {
            dev_put(dev);
            printk("The given interface already exists \n");
            return -EINVAL;
        }
    }

    /* No free id is found. We can't create a new epon virtual i/f */
    if (oam_dev != NULL) {
        printk("Create oam network interface failed: device already exists!\n");
        return -EPERM;
    }

    /* Allocate the dev */
    if ((dev = alloc_etherdev(sizeof(BcmEnet_devctrl))) == NULL) {
        return -ENOMEM;
    }

    /* Set the private are to 0s */
    memset(netdev_priv(dev), 0, sizeof(BcmEnet_devctrl));

    /* Set the pDevCtrl->dev to dev */
    pDevCtrl = netdev_priv(dev);
    pDevCtrl->dev = dev;
    pDevCtrl->default_txq = 0;
    pDevCtrl->use_default_txq = 0;

    /* Assign name to the i/f */
    if (name[0] == 0) {
        /* Allocate and assign a name to the i/f */
        dev_alloc_name(dev, oamif_name);
    } else {
        /* Assign the given name to the i/f */
        strcpy(dev->name, name);
        dev_alloc_name(dev, dev->name);
    }

    SET_MODULE_OWNER(dev);

    dev->netdev_ops       = vnet_dev[0]->netdev_ops;
    dev->priv_flags       = vnet_dev[0]->priv_flags;
    dev->features               = vnet_dev[0]->features;
    dev->vlan_features          = vnet_dev[0]->vlan_features;

    /* Set this flag to block forwarding of traffic between
       EPON virtual interfaces */
    dev->priv_flags       |= IFF_WANDEV;

    dev->base_addr        = EPON_PORT_ID;
    bcmeapi_create_vport(dev);
    /* For now, let us use this base_addr field to identify EPON port */
    /* TBD: Change this to a private field in pDevCtrl for all eth and gpon
       virtual ports */
    pDevCtrl->sw_port_id  = EPON_PORT_ID;

    dev->path.hw_subport_mcast_idx = NETDEV_PATH_HW_SUBPORTS_MAX;

    /* The unregister_netdevice will call the destructor
       through netdev_run_todo */
    dev->destructor        = free_netdev;

    /* Note: Calling from ioctl, so don't use register_netdev
       which takes rtnl_lock */
    status = register_netdev(dev);

    if (status != 0) {
        unregister_netdev(dev);
        return status;
    }

    oam_dev = dev;

    /* oam network interface mac address is to be configured by userspace */
    memmove(dev->dev_addr, vnet_dev[0]->dev_addr, ETH_ALEN);

    memmove(sockaddr.sa_data, dev->dev_addr, ETH_ALEN);
    dev->netdev_ops->ndo_set_mac_address(dev, &sockaddr);

    printk("%s: MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
            dev->name,
            dev->dev_addr[0],
            dev->dev_addr[1],
            dev->dev_addr[2],
            dev->dev_addr[3],
            dev->dev_addr[4],
            dev->dev_addr[5]
          );

    return 0;
}

int delete_oam_vport(char *ifname)
{
    struct net_device *dev = NULL;
    BcmEnet_devctrl *pDevCtrl = NULL;

    if ((oam_dev == NULL) ||
            (strcmp(oam_dev->name, ifname) != 0))
    {
        printk("delete_oam_vport() : No such device \n");
        return -ENXIO;
    }

    dev = oam_dev;

    /* Get the ifid of this interface */
    pDevCtrl = (BcmEnet_devctrl *)netdev_priv(dev);

    /* */
    synchronize_net();
    if(memcmp(vnet_dev[0]->dev_addr, dev->dev_addr, ETH_ALEN)) {
        kerSysReleaseMacAddress(dev->dev_addr);
    }

    /* Delete the given epon virtual interfaces. No need to call free_netdev
       after this as dev->destructor is set to free_netdev */
    unregister_netdev(dev);
    oam_dev = NULL;
    return 0;
}

EXPORT_SYMBOL(create_oam_vport);
EXPORT_SYMBOL(delete_oam_vport);
#endif

#ifdef CONFIG_BLOG
/*---------------------------------------------------------------------------
 * unsigned long bcmenet_get_xmit_mark(void *dev, int priority, unsigned long uMark)
 * Description:
 *    If port is WAN, bump to priority specified in input parameter.
 *    This function is for blog to raise TCP ACK TX priority. 
 *---------------------------------------------------------------------------
 */
static unsigned long bcmenet_get_xmit_mark(void *dev, int priority, unsigned long uMark)
{
    struct net_device *net_dev = (struct net_device *)dev;
    int log_port = port_id_from_dev(net_dev);
    
    if (IsLogPortWan(log_port))
    {
        uMark = SKBMARK_SET_Q_PRIO(uMark, priority);
    }
    return uMark;
}  /* bcmenet_get_xmit_mark() */
#endif 

static int __init bcmenet_module_init(void)
{
    int status;

    TRACE(("bcm63xxenet: bcmenet_module_init\n"));

    bcmeapi_module_init();

    sema_init(&bcm_ethlock_switch_config, 1);
    sema_init(&bcm_link_handler_config, 1);

    if ( BCM_SKB_ALIGNED_SIZE != skb_aligned_size() )
    {
        printk("skb_aligned_size mismatch. Need to recompile enet module\n");
        return -ENOMEM;
    }
#ifdef CONFIG_BCM_PTP_1588
    status = ptp_1588_init();
    if (status)
        return status;
#endif
#if defined(_CONFIG_BCM_ARL)
    bcm_arl_process_hook_g = enet_hook_for_arl_access;
#endif

#if defined(ENET_GPON_CONFIG)
    initialize_gemid_to_ifIdx_mapping();
#endif

    /* create a slab cache for device descriptors */
    enetSkbCache = kmem_cache_create("bcm_EnetSkbCache",
            BCM_SKB_ALIGNED_SIZE,
            0, /* align */
            SLAB_HWCACHE_ALIGN, /* flags */
            NULL); /* ctor */
    if(enetSkbCache == NULL)
    {
        printk(KERN_NOTICE "Eth: Unable to create skb cache\n");

        return -ENOMEM;
    }

    status = bcm63xx_enet_probe();

#if defined(SUPPORT_SWMDK)
    bcmFun_reg(BCM_FUN_ID_ENET_LINK_CHG, link_change_handler_wrapper);
#endif /*SUPPORT_SWMDK*/
    bcmFun_reg(BCM_FUN_ID_ENET_IS_WAN_PORT, bcmenet_is_wan_port);

    bcmeapi_module_init2();
    register_bridge_notifier(&br_notifier);
    register_bridge_stp_notifier(&br_stp_handler);

#ifdef CONFIG_BLOG
   blog_eth_get_tx_mark_fn = bcmenet_get_xmit_mark;
#endif

    return status;
}
int bcm63xx_enet_getPortmapFromDev(struct net_device *dev, int *pUnit, unsigned int *pPortmap)
{
    int port;
    int i;

    for (i = 1; i <= vport_cnt; i++) {
        if (dev == vnet_dev[i]) {
            break;
        }
    }

    if ( i > vport_cnt ) {
        return -1;
    }

    port = port_id_from_dev(dev);
    if ( port >= MAX_TOTAL_SWITCH_PORTS )
    {
        return -1;
    }
    *pUnit = LOGICAL_PORT_TO_UNIT_NUMBER(port);
    *pPortmap = ( 1 << LOGICAL_PORT_TO_PHYSICAL_PORT(port) );

#if defined(CONFIG_BCM_SWITCH_PORT_TRUNK_SUPPORT)
    {
        int grp_no=-1; /* Init with -1 to get incremented to zero to start */
        unsigned int log_pmap = (1<< port) & TRUNK_GRP_PORT_MASK;

        if ( (log_pmap & trunk_grp[++grp_no]) || /* Part of first trunk group */
             (log_pmap & trunk_grp[++grp_no]) )  /* Part of second trunk group */
        {
            *pPortmap = LOGICAL_PORTMAP_TO_PHYSICAL_PORTMAP( (trunk_grp[grp_no] & TRUNK_GRP_PORT_MASK) );
        }
    }
#endif
    return 0;

}

int bcm63xx_enet_getPortFromDev(struct net_device *dev, int *pUnit, int *pPort)
{
    unsigned int portmap;
    int err;
    err = bcm63xx_enet_getPortmapFromDev(dev,pUnit,&portmap);
    if (err) return err;

    *pPort = 0;
    while(portmap)
    {
        if (portmap & (1<<(*pPort)))
        {
            break;
        }
        *pPort += 1;
    }
    return 0;

}
int bcm63xx_enet_getPortmapFromName(char *pIfName, int *pUnit, unsigned int *pPortmap)
{
    struct net_device *dev;

    dev = dev_get_by_name(&init_net, pIfName);
    if (NULL == dev)
    {
        return -1;
    }

    /* find root device */
    while( 1 )
    {
        if(netdev_path_is_root(dev))
        {
            break;
        }
        dev_put(dev);
        dev = netdev_path_next_dev(dev);
        dev_hold(dev);
    }

    if ( bcm63xx_enet_getPortmapFromDev(dev, pUnit, pPortmap) < 0 )
    {
        dev_put(dev);
        return -1;
    }
    dev_put(dev);
    return 0;
}

int bcm63xx_enet_getPortFromName(char *pIfName, int *pUnit, int *pPort)
{
    int err;
    unsigned int portmap;

    err = bcm63xx_enet_getPortmapFromName(pIfName,pUnit,&portmap);
    if (err) return err;

    *pPort=0;
    while(portmap)
    {
        if (portmap & (1<<(*pPort)))
        {
            break;
        }
        *pPort +=1;
    }
    return 0;
}

/* return logical portmap for all the Ethernet Driver managed ports in this bridge */
static uint32_t bcmenet_bridge_get_logical_pmap(char *brName)
{
    unsigned int brPort = 0xFFFFFFFF;
    struct net_device *dev;
    uint32_t portMap = 0, physclPortMap = 0;
    int unit;

    for(;;)
    {
        dev = bridge_get_next_port(brName, &brPort);
        if (dev == NULL)
            break;
        /* One linux device may have multiple physical ports in case of port-trunking/LAG */
        if ( bcm63xx_enet_getPortmapFromDev(dev, &unit, &physclPortMap) < 0 )
        {
            continue;
        }

        portMap |= SET_PORTMAP_IN_LOGICAL_PORTMAP(physclPortMap, unit);
    }

    return portMap;
}
void bcmenet_update_pbvlan_all_bridge(void)
{
    char br_list[64];
    char *brName, *tracker;
    rcu_read_lock();
    bridge_get_br_list(br_list, sizeof(br_list));
    tracker = br_list;
    while (tracker)
    {
        brName = tracker;
        tracker = strchr(tracker,',');
        if(tracker)
        {
            *tracker = '\0';
            tracker++;
        }
        bridge_update_pbvlan(brName);
    }
    rcu_read_unlock();
}


static int bridge_notifier(struct notifier_block *nb, unsigned long event, void *brName)
{
    switch (event)
    {
        case BREVT_IF_CHANGED:
            bridge_update_pbvlan(brName);
            break;
    }
    return NOTIFY_DONE;
}

static int bridge_stp_handler(struct notifier_block *nb, unsigned long event, void *portInfo)
{
    struct stpPortInfo *pInfo = (struct stpPortInfo *)portInfo;
    BcmEnet_devctrl *pDevCtrl = netdev_priv(vnet_dev[0]);

    switch (event)
    {
        case BREVT_STP_STATE_CHANGED:
            {
                unsigned char stpVal;
                int port;
                int unit;

                if ( bcm63xx_enet_getPortFromName(&pInfo->portName[0], &unit, &port ) < 0 )
                {
                    break;
                }

                switch ( pInfo->stpState )
                {
                    case BR_STATE_BLOCKING:
                        stpVal = REG_PORT_STP_STATE_BLOCKING;
                        break;

                    case BR_STATE_FORWARDING:
                        stpVal = REG_PORT_STP_STATE_FORWARDING;
                        break;

                    case BR_STATE_LEARNING:
                        stpVal = REG_PORT_STP_STATE_LEARNING;
                        break;

                    case BR_STATE_LISTENING:
                        stpVal = REG_PORT_STP_STATE_LISTENING;
                        break;

                    case BR_STATE_DISABLED:
                        stpVal = REG_PORT_STP_STATE_DISABLED;
                        break;

                    default:
                        stpVal = REG_PORT_NO_SPANNING_TREE;
                        break;
                }

                if ( 0 == (pDevCtrl->stpDisabledPortMap & (1 << PHYSICAL_PORT_TO_LOGICAL_PORT(port, unit))) ) {
                    bcmeapi_ethsw_set_stp_mode(unit, port, stpVal);
                }
                break;
            }
    }
    return NOTIFY_DONE;
}


static void bridge_update_pbvlan(char *brName)
{
    uint32_t portMapLoop, logicalPortMap, fwdPortMap, port, unit;

    /* Get the logical portmap for all the Ethernet Driver managed ports connected to this bridge */
    logicalPortMap = bcmenet_bridge_get_logical_pmap(brName); 
    /* Remove the bits from portmap for wanPort and softSwitchingMap --- These ports are always isolated */
    logicalPortMap &= ~(global.pVnetDev0_g->wanPort | global.pVnetDev0_g->softSwitchingMap);
    if (logicalPortMap == 0) /* No ports in this bridge are from Ethernet Driver - bail out */
    {
        return;
    }

    for(unit=0; unit < BP_MAX_ENET_MACS; unit++)
    {
        fwdPortMap = portMapLoop = GET_PORTMAP_FROM_LOGICAL_PORTMAP(logicalPortMap,unit);
        fwdPortMap |= (1<<IMP_PORT_ID); /* Set IMP port as well in the forwarding portmap */
        port = 0; /* Start from first switch port */
        while(portMapLoop)
        {
            if (portMapLoop & (1<<port))
            {
                /* Set PBVLANMAP for this port */
                if (unit == 0) /* Internal switch */
                {
                    ethsw_set_pbvlan(port, fwdPortMap);
                }
                else
                {
                    extsw_set_pbvlan(port, fwdPortMap);
                }
                portMapLoop &= ~(1<<port); /* reset port bit in portMap */
            }
            port++;
        }
    }
}

/*
 * We need this function in non-gmac build as well.
 * It searches both the internal and external switch ports.
 */
int enet_logport_to_phyid(int log_port)
{

    ASSERT(log_port < (MAX_TOTAL_SWITCH_PORTS));
    return enet_sw_port_to_phyid(LOGICAL_PORT_TO_UNIT_NUMBER(log_port), LOGICAL_PORT_TO_PHYSICAL_PORT(log_port));
}

int enet_sw_port_to_phyid(int unit, int phy_port)
{
    ETHERNET_MAC_INFO *info = EnetGetEthernetMacInfo();
    if (unit >= BP_MAX_ENET_MACS || phy_port < 0 || phy_port >= BP_MAX_SWITCH_PORTS) {
        /* Error */
        return 0;
    }
    return info[unit].sw.phy_id[phy_port];
}

int enet_cb_port_to_phyid(int unit, int cb_port)
{
    ETHERNET_MAC_INFO *info = EnetGetEthernetMacInfo();
    if ( (cb_port < 0) || (cb_port >= BCMENET_CROSSBAR_MAX_EXT_PORTS))
    {
        return 0;
    }
    return info[unit].sw.crossbar[cb_port].phy_id;
}

unsigned long enet_get_portmap(unsigned char unit)
{
    struct net_device *dev = vnet_dev[0];
    BcmEnet_devctrl *pDevCtrl = (BcmEnet_devctrl *)netdev_priv(dev);
    if (unit >= BP_MAX_ENET_MACS) {
        /* Error */
        return 0;
    }
    return pDevCtrl->EnetInfo[unit].sw.port_map;
}

unsigned long enet_get_port_flags(unsigned char unit, int port)
{
    struct net_device *dev = vnet_dev[0];
    BcmEnet_devctrl *pDevCtrl = (BcmEnet_devctrl *)netdev_priv(dev);
    if (unit >= BP_MAX_ENET_MACS || port < 0 || port >= BP_MAX_SWITCH_PORTS) {
        /* Error */
        return 0;
    }
    return pDevCtrl->EnetInfo[unit].sw.port_flags[port];
}

unsigned int enet_get_consolidated_portmap(void)
{
    return consolidated_portmap;
}

#ifdef DYING_GASP_API
int enet_send_dying_gasp_pkt(void)
{
    struct net_device *dev = NULL;
    int i;

    /* Indicate we are in a dying gasp context and can skip
       housekeeping since we're about to power down */
    dg_in_context = 1;

    if (dg_skbp == NULL) {
        BCM_ENET_DEBUG("%s No DG skb to send \n", __FUNCTION__);
        return -1;
    }
    for (i = 0; i < MAX_TOTAL_SWITCH_PORTS - 1; i++)
    {
        int iVport = LOGICAL_PORT_TO_VPORT(i);

        /* Is this a port for a valid ethernet device? */
        if(iVport <= 0)
        {
            /* Nope - skip to next port */
            continue;
        }

        /* Get dev pointer for this port */
        dev = vnet_dev[iVport];

        /* Is this a WAN port? */
        if (dev && dev->priv_flags & IFF_WANDEV) {
            int iRetVal;

            /* Copy src MAC from dev into dying gasp packet */
            memcpy(dg_skbp->data + ETH_ALEN, dev->dev_addr, ETH_ALEN);

            /* Transmit dying gasp packet */
            iRetVal = bcm63xx_enet_xmit(SKBUFF_2_PNBUFF(dg_skbp), dev);
            printk("\n%s DG sent out on wan port %s (ret=%d)\n", __FUNCTION__, dev->name, iRetVal);
            break;
        }
    } // for
    return 0;
}
#endif

/* Physical port to logical port mapping */
struct net_device *enet_phyport_to_vport_dev(int port)
{
    ASSERT(port < TOTAL_SWITCH_PORTS);
    return (vnet_dev[LOGICAL_PORT_TO_VPORT(PHYSICAL_PORT_TO_LOGICAL_PORT(port, extSwInfo.present))]);
}

uint32 ConfigureJumboPort(uint32 regVal, int portVal, unsigned int configVal) // bill
{
    UINT32 controlBit;

    // Test for valid port specifier.
    if ((portVal >= ETHSWCTL_JUMBO_PORT_GPHY_0) && (portVal <= ETHSWCTL_JUMBO_PORT_ALL))
    {
        // Switch on port ID.
        switch (portVal)
        {
            case ETHSWCTL_JUMBO_PORT_MIPS:
                controlBit = ETHSWCTL_JUMBO_PORT_MIPS_MASK;
                break;
            case ETHSWCTL_JUMBO_PORT_GPON:
                controlBit = ETHSWCTL_JUMBO_PORT_GPON_MASK;
                break;
            case ETHSWCTL_JUMBO_PORT_USB:
                controlBit = ETHSWCTL_JUMBO_PORT_USB_MASK;
                break;
            case ETHSWCTL_JUMBO_PORT_MOCA:
                controlBit = ETHSWCTL_JUMBO_PORT_MOCA_MASK;
                break;
            case ETHSWCTL_JUMBO_PORT_GPON_SERDES:
                controlBit = ETHSWCTL_JUMBO_PORT_GPON_SERDES_MASK;
                break;
            case ETHSWCTL_JUMBO_PORT_GMII_2:
                controlBit = ETHSWCTL_JUMBO_PORT_GMII_2_MASK;
                break;
            case ETHSWCTL_JUMBO_PORT_GMII_1:
                controlBit = ETHSWCTL_JUMBO_PORT_GMII_1_MASK;
                break;
            case ETHSWCTL_JUMBO_PORT_GPHY_1:
                controlBit = ETHSWCTL_JUMBO_PORT_GPHY_1_MASK;
                break;
            case ETHSWCTL_JUMBO_PORT_GPHY_0:
                controlBit = ETHSWCTL_JUMBO_PORT_GPHY_0_MASK;
                break;
            default: // ETHSWCTL_JUMBO_PORT_ALL:
                controlBit = ETHSWCTL_JUMBO_PORT_MASK_VAL;  // ALL bits
                break;
        }

        // Test for accept JUMBO frames.
        if (configVal != 0)
        {
            // Setup register value to accept JUMBO frames.
            regVal |= controlBit;
        }
        else
        {
            // Setup register value to reject JUMBO frames.
            regVal &= ~controlBit;
        }
    }

    // Return new JUMBO configuration control register value.
    return regVal;
}

int enet_get_first_crossbar_port(int logPort)
{
    int phsclPort = LOGICAL_PORT_TO_PHYSICAL_PORT(logPort);
    int sw_unit = LOGICAL_PORT_TO_UNIT_NUMBER(logPort);
    BcmEnet_devctrl *pDevCtrl = (BcmEnet_devctrl *)netdev_priv(vnet_dev[0]);
    ETHERNET_MAC_INFO *info = &pDevCtrl->EnetInfo[sw_unit];
    int muxExtPort = 0;

    for(/* muxExtPort init above*/; muxExtPort < BCMENET_CROSSBAR_MAX_EXT_PORTS; muxExtPort++)
    {
        /* Skip no member crossbar ports */
        if (info->sw.crossbar[muxExtPort].switch_port != phsclPort) continue;
        return muxExtPort;
    }
    return BP_CROSSBAR_NOT_DEFINED;
}
int enet_get_next_crossbar_port(int logPort, int cb_port)
{
    int phsclPort = LOGICAL_PORT_TO_PHYSICAL_PORT(logPort);
    int sw_unit = LOGICAL_PORT_TO_UNIT_NUMBER(logPort);
    BcmEnet_devctrl *pDevCtrl = (BcmEnet_devctrl *)netdev_priv(vnet_dev[0]);
    ETHERNET_MAC_INFO *info = &pDevCtrl->EnetInfo[sw_unit];
    int muxExtPort = (cb_port >= 0 ? cb_port+1 : 0);

    for(/* muxExtPort init above*/; muxExtPort < BCMENET_CROSSBAR_MAX_EXT_PORTS; muxExtPort++)
    {
        /* Skip no member crossbar ports */
        if (info->sw.crossbar[muxExtPort].switch_port != phsclPort) continue;
        return muxExtPort;
    }
    return BP_CROSSBAR_NOT_DEFINED;
}

PHY_STAT enet_get_ext_phy_stat(int unit, int phy_port, int cb_port)
{
    PHY_STAT phys;

    memset(&phys, 0, sizeof(PHY_STAT));
    if ((unit >= BP_MAX_ENET_MACS) || (phy_port < 0) || (phy_port >= BP_MAX_SWITCH_PORTS)) {
        return phys;
    }
    else
    {
        if ( cb_port == BP_CROSSBAR_NOT_DEFINED )
        {
            cb_port = 0;
        }
        if ( (cb_port < 0) || (cb_port >= BCMENET_CROSSBAR_MAX_EXT_PORTS) )
        {
            return phys;
        }
        return ext_phy_stat[unit][phy_port][cb_port];
    }
}

module_init(bcmenet_module_init);
module_exit(bcmenet_module_cleanup);
MODULE_LICENSE("GPL");

