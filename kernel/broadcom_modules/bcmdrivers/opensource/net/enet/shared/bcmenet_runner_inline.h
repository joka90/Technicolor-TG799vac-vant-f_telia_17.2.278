
/*
 Copyright 2002-2010 Broadcom Corp. All Rights Reserved.

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
#ifndef _BCMENET_RUNNER_INLINE_H_
#define _BCMENET_RUNNER_INLINE_H_

#include "rdpa_api.h"
#include "rdpa_epon.h"
#include "autogen/rdpa_ag_epon.h"
#ifdef CONFIG_BCM_PTP_1588
#include "bcmenet_ptp_1588.h"
#endif
#ifdef BCMENET_RUNNER_CPU_RING
#include "rdpa_cpu_helper.h"
extern ENET_RING_S enet_ring[2];
#define ENET_RING_MAX_BUFF_IN_CACHE 32
#endif
#define WL_NUM_OF_SSID_PER_UNIT 8 

extern int wan_port_id; 
extern rdpa_system_init_cfg_t init_cfg;


static inline int bcmeapi_alloc_skb(BcmEnet_devctrl *pDevCtrl, struct sk_buff **skb)
{

#if 1
    if (pDevCtrl->freeSkbList) {
        *skb = pDevCtrl->freeSkbList;
        pDevCtrl->freeSkbList = pDevCtrl->freeSkbList->next_free;
    }
    else
#endif
    {
        *skb = kmem_cache_alloc(enetSkbCache, GFP_ATOMIC);

        if (!(*skb)) {
            return BCMEAPI_CTRL_FALSE;
        }
    }
	return BCMEAPI_CTRL_TRUE;
}



static inline int bcmeapi_free_skb(BcmEnet_devctrl *pDevCtrl, 
        struct sk_buff *skb, int free_flag)
{
    unsigned int is_bulk_rx_lock_active;
    uint32 cpuid;

    if( !(free_flag & SKB_RECYCLE ))
    {
        return BCMEAPI_CTRL_FALSE;
    }

    /*
     * Disable preemption so that my cpuid will not change in this func.
     * Not possible for the state of bulk_rx_lock_active to change
     * underneath this function on the same cpu.
     */
    preempt_disable();
    cpuid =  smp_processor_id();
    is_bulk_rx_lock_active = pDevCtrl->bulk_rx_lock_active[cpuid];

    if (0 == is_bulk_rx_lock_active)
        ENET_RX_LOCK();

    if ((unsigned char *)skb < pDevCtrl->skbs_p || (unsigned char *)skb >= pDevCtrl->end_skbs_p)
    {
        kmem_cache_free(enetSkbCache, skb);
    }
    else
    {
        skb->next_free = pDevCtrl->freeSkbList;
        pDevCtrl->freeSkbList = skb;      
    }

    if (0 == is_bulk_rx_lock_active)
        ENET_RX_UNLOCK();

    preempt_enable();
    return BCMEAPI_CTRL_TRUE;
}

extern void bdmf_sysb_databuf_recycle(void *pBuf, unsigned context);

/* Callback: fkb and data recycling */
static inline void __bcm63xx_enet_recycle_fkb(struct fkbuff * pFkb,
                                              uint32 context)
{
    /* No cache flush */
    bdmf_sysb_databuf_recycle(pFkb, context);
}

static inline void bcmeapi_kfree_buf_irq(BcmEnet_devctrl *pDevCtrl, struct fkbuff * pFkb, unsigned char *pBuf) 
{
    nbuff_flush(pFkb, pFkb->data, pFkb->len);/*TODO change this to invalidate */
    __bcm63xx_enet_recycle_fkb(pFkb, pFkb->recycle_context);
}
static inline void bcmeapi_blog_drop(BcmEnet_devctrl *pDevCtrl, struct fkbuff  *pFkb, unsigned char *pBuf)
{
    bcmeapi_kfree_buf_irq(pDevCtrl, pFkb, pBuf);
}

static inline void bcm63xx_enet_recycle_skb_or_data(struct sk_buff *skb,
                                             uint32 context, uint32 free_flag)
{
    BcmEnet_devctrl *pDevCtrl = (BcmEnet_devctrl *)netdev_priv(vnet_dev[0]);

    if (bcmeapi_free_skb(pDevCtrl, skb, free_flag) != BCMEAPI_CTRL_TRUE)
    { // free data
        uint8 *pData = skb->head + BCM_PKT_HEADROOM;
        uint8 *pEnd;
#if defined(ENET_CACHE_SMARTFLUSH)
        uint8 *dirty_p = skb_shinfo(skb)->dirty_p;
        uint8 *shinfoBegin = (uint8 *)skb_shinfo(skb);
        uint8 *shinfoEnd;
        if (skb_shinfo(skb)->nr_frags == 0) {
            // no frags was used on this skb, so can shorten amount of data
            // flushed on the skb_shared_info structure
            shinfoEnd = shinfoBegin + offsetof(struct skb_shared_info, frags);
        }
        else {
            shinfoEnd = shinfoBegin + sizeof(struct skb_shared_info);
        }
        //cache_flush_region(shinfoBegin, shinfoEnd);
        cache_invalidate_region(shinfoBegin, shinfoEnd);

        // If driver returned this buffer to us with a valid dirty_p,
        // then we can shorten the flush length.
        if (dirty_p) {
            if ((dirty_p < skb->head) || (dirty_p > shinfoBegin)) {
                printk("invalid dirty_p detected: %p valid=[%p %p]\n",
                        dirty_p, skb->head, shinfoBegin);
                pEnd = shinfoBegin;
            } else {
                pEnd = (dirty_p < pData) ? pData : dirty_p;
            }
        } else {
            pEnd = shinfoBegin;
        }
#else
        pEnd = pData + BCM_MAX_PKT_LEN;
#endif
        cache_invalidate_region(pData, pEnd);
        bdmf_sysb_databuf_recycle(PDATA_TO_PFKBUFF(pData, BCM_PKT_HEADROOM), context);
    }
}

/* Common recycle callback for fkb, skb or data */
inline void bcm63xx_enet_recycle(pNBuff_t pNBuff, uint32 context, uint32 flags)
{
    if ( IS_FKBUFF_PTR(pNBuff) ) {
        __bcm63xx_enet_recycle_fkb(PNBUFF_2_FKBUFF(pNBuff), context);
    } else { /* IS_SKBUFF_PTR(pNBuff) */
        bcm63xx_enet_recycle_skb_or_data(PNBUFF_2_SKBUFF(pNBuff),context,flags);
    }
}

#if 1
static inline void bcmeapi_set_fkb_recycle_hook(FkBuff_t * pFkb)
{

        pFkb->recycle_hook = (RecycleFuncP)bcm63xx_enet_recycle;
    //    pFkb->recycle_context = 0;
}
#endif

static inline int bcmeapi_skb_headerinit(int len, BcmEnet_devctrl *pDevCtrl, struct sk_buff *skb, 
						   FkBuff_t * pFkb, unsigned char *pBuf)
{

    skb_headerinit(BCM_PKT_HEADROOM,
#if defined(ENET_CACHE_SMARTFLUSH)
            SKB_DATA_ALIGN(len+BCM_SKB_TAILROOM),
#else
            BCM_MAX_PKT_LEN,
#endif
            skb, pBuf, (RecycleFuncP)bcm63xx_enet_recycle_skb_or_data,
            pFkb->recycle_context, pFkb->blog_p);

    skb_trim(skb, len);
    return BCMEAPI_CTRL_TRUE;
}

#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
static inline int bcmeapi_rx_pkt(BcmEnet_devctrl *pDevCtrl, unsigned char **pBuf, FkBuff_t **pFkb, 
                                 int *len, int *gemid, int *phy_port_id, int *is_wifi_port, struct net_device **dev,
                                 uint32 *rxpktgood, uint32 *context_p, int *rxQueue)
{
    rdpa_if src_port;
    int rc = BDMF_ERR_NO_MORE; /* Important to have this value match the second if condition */

    (*rxpktgood)++;

    if (*context_p || (rc = rdpa_cpu_host_packet_get_enet(NETDEV_CPU_HI_RX_QUEUE_ID, (bdmf_sysb *)pFkb, &src_port)) != 0) 
    {
        uint32 cpuid = smp_processor_id();

        if (rc == BDMF_ERR_NO_MORE) /* rc MUST be initialized with this value */
        {
            *context_p = 1; /* Mark that no more Hi Priority packets to look for in this NAPI budget */
            /* CPU Rx queue is empty. Get from low priority queue. */
            rc = rdpa_cpu_host_packet_get_enet(NETDEV_CPU_RX_QUEUE_ID, (bdmf_sysb *)pFkb, &src_port);
            if (rc)
            {
                RECORD_BULK_RX_UNLOCK();
                ENET_RX_UNLOCK();
                if (rc == BDMF_ERR_NO_MORE)
                {
                    /* CPU Rx queue is empty. */
                    *rxpktgood |= ENET_POLL_DONE;
                    return BCMEAPI_CTRL_BREAK;
                }
                else
                {
//                    printk(KERN_NOTICE "Error in rdpa_cpu_packet_get() (%d)\n", rc);
                    pDevCtrl->stats.rx_dropped++;
                    pDevCtrl->estats.rx_errors_indicated_by_low_level++;
                    return BCMEAPI_CTRL_CONTINUE;
                }
            }
            else
            {
                *rxQueue = NETDEV_CPU_RX_QUEUE_ID;
            }
        }
        else
        {
            RECORD_BULK_RX_UNLOCK();
            ENET_RX_UNLOCK();
//            printk(KERN_NOTICE "Error in rdpa_cpu_packet_get() (%d)\n", rc);
            pDevCtrl->stats.rx_dropped++;
            pDevCtrl->estats.rx_errors_indicated_by_low_level++;
            return BCMEAPI_CTRL_CONTINUE;
        }
    }
    else
    {
        *rxQueue = NETDEV_CPU_HI_RX_QUEUE_ID;
    }
        
    
    *pBuf = (*pFkb)->data;
    *len = (*pFkb)->len;

    if(src_port == rdpa_if_wan0)
    {
        *phy_port_id = wan_port_id;
    }
    else
    {
#if defined(CONFIG_BCM_EXT_SWITCH)
        /* In 63138/63148 we must retrieve the physical port from the
           Broadcom tag because the connected HW switch ports are not
           contiguous, while rdpa_if provides a contiguous enumeration */
        uint16_t brcm_tag = ((BcmEnet_hdr2*)(*pBuf))->brcm_tag;
        *phy_port_id = BCM_PORT_FROM_TYPE2_TAG_LE(brcm_tag);
        *phy_port_id = PHYSICAL_PORT_TO_LOGICAL_PORT(*phy_port_id, 1); /* Logical port for External switch port */
        ((BcmEnet_hdr2*)(*pBuf))->brcm_type = BRCM_TYPE2_LE;
#else
        *phy_port_id = src_port - rdpa_if_lan0;
#endif
    }

//    printk("phy_port_id %u\n", *phy_port_id);

    return BCMEAPI_CTRL_TRUE;
}
#else

inline void * _databuf_alloc(ENET_RING_S *p_ring)
{
    if (likely(p_ring->buff_cache_cnt))
    {
        return (void *) (p_ring->buff_cache[--p_ring->buff_cache_cnt]);
    }
    else
    {
        uint32_t alloc_cnt;
        /* refill the local cache from global pool */
#if (defined(CONFIG_BCM_BPM) || defined(CONFIG_BCM_BPM_MODULE))
        int i;
        if(gbpm_alloc_mult_buf(ENET_RING_MAX_BUFF_IN_CACHE, p_ring->buff_cache) == GBPM_ERROR)
        {
            /* BPM returns either all the buffers requested or none */
            alloc_cnt = 0;
            goto test_alloc;
        }

        /* no cache invalidation of buffers is needed for buffers coming from BPM */

        /*reserve space for headroom & FKB */
        for(i=0; i < ENET_RING_MAX_BUFF_IN_CACHE; i++ )
        {
            p_ring->buff_cache[i]= (uint32_t)PFKBUFF_TO_PDATA((void *)(p_ring->buff_cache[i]),BCM_PKT_HEADROOM);
        }

        alloc_cnt = ENET_RING_MAX_BUFF_IN_CACHE;
#else
        uint32_t *datap;
        /* allocate from kernel directly */
        datap = kmalloc(BCM_PKTBUF_SIZE, GFP_ATOMIC);

        if (!datap)
        {
            alloc_cnt = 0;
        }
        /* do a cache invalidate of the buffer */
        INV_RANGE((unsigned long)datap, BCM_PKTBUF_SIZE );

        /*reserve space for headroom & FKB */
        p_ring->buff_cache[0] = (uint32_t) PFKBUFF_TO_PDATA((void *) (datap), BCM_PKT_HEADROOM);

        /* always return only one buffer when BPM is not enabled */
        alloc_cnt = 1;
        goto test_alloc;
#endif
test_alloc:
        if (alloc_cnt)
        {
            p_ring->buff_cache_cnt = alloc_cnt;
            return (void *) (p_ring->buff_cache[--p_ring->buff_cache_cnt]);
        }
    }
    return NULL;
}

static inline int get_pkt_from_ring(int qid,FkBuff_t **pFkb, rdpa_cpu_rx_info_t *info)
{
    uint32_t ret;
    FkBuff_t *fkb;
    ENET_RING_S *p_ring = &enet_ring[qid - NETDEV_CPU_RX_QUEUE_ID_BASE];
    CPU_RX_DESCRIPTOR *p_desc = p_ring->head;
    void *pNewBuf;


    ret = rdpa_cpu_rx_pd_get(p_desc, info);
    if (ret)
    {
        return  ret;
    }

    /* A valid packet is recieved try to allocate a new data buffer and
    * refill the ring before giving the packet to upper layers
    */
    pNewBuf  = _databuf_alloc(p_ring);

    /*validate allocation*/
    if (unlikely(!pNewBuf))
    {
        /*assign old data buffer back to ring*/
        printk("%s(%d):Failed to allocate new ring buffer!\n",__FUNCTION__,__LINE__);
        pNewBuf   = (void*)info->data;
        info->data = (uint32_t)NULL;
        return BCMEAPI_CTRL_SKIP;
    }

    rdpa_cpu_ring_rest_desc(p_desc, pNewBuf);

    /*move to next descriptor, wrap around if needed*/

    if(++p_ring->head == p_ring->end)
        p_ring->head = p_ring->base;

    /*create the fkb*/
    fkb = fkb_init((void*)info->data , BCM_PKT_HEADROOM, (void*)info->data, info->size);
    /*set the recyle hook */
    fkb->recycle_hook = bdmf_sysb_recycle;
    fkb->recycle_context = 0;
    *pFkb = fkb;

    return ret;
}

static inline int bcmeapi_rx_pkt(BcmEnet_devctrl *pDevCtrl, unsigned char **pBuf, FkBuff_t **pFkb, 
                   int *len, int *gemid, int *phy_port_id, int *is_wifi_port, struct net_device **dev, uint32 *rxpktgood,
                   uint32 *context_p, int *rxQueue)
{
   int rc = 0;
   uint32 cpuid = smp_processor_id();
   rdpa_cpu_rx_info_t info = {};
   int queue_id = *context_p ? NETDEV_CPU_RX_QUEUE_ID : NETDEV_CPU_HI_RX_QUEUE_ID;

    /*TODO:remove wifi code from enet driver move it to WFD
     * check with yoni/Ilya  
     */ 
#if 1
    *is_wifi_port = 0;
#endif
    
    *rxQueue = queue_id;
    (*rxpktgood)++;
    rc = get_pkt_from_ring(queue_id, pFkb, &info);
    if(rc)
    {
        RECORD_BULK_RX_UNLOCK();
        ENET_RX_UNLOCK();
        if (rc == BDMF_ERR_NO_MORE)
        {
            if (queue_id == NETDEV_CPU_HI_RX_QUEUE_ID)
            {
                *context_p = 1;
                return BCMEAPI_CTRL_SKIP;
            }
            else
            {
                *rxpktgood |= ENET_POLL_DONE;
                return BCMEAPI_CTRL_BREAK;
            }
        }

        /* Some error */
        printk(KERN_NOTICE "Error in rdpa_cpu_packet_get() (%d)\n", rc);
        /* Consider error packets (assuming RDD succeeded dequeuing
         * them) as read packets. */
        return BCMEAPI_CTRL_CONTINUE;
    }

    *pBuf = (*pFkb)->data;
    *len = (*pFkb)->len;
    *gemid = info.reason_data;
    
    if (unlikely(global.dump_enable))
        rdpa_cpu_rx_dump_packet("enet", rdpa_cpu_host, queue_id, &info, 0);
    
#ifdef CONFIG_BCM_PTP_1588
    if (info.reason == rdpa_cpu_rx_reason_etype_ptp_1588)
        ptp_1588_rx_pkt_store_timestamp(*pBuf, *len, info.ptp_index);
#endif
    switch (info.src_port)
    {
        case rdpa_if_wan0:
        case rdpa_if_wan1:
            *phy_port_id = wan_port_id;
            break;

#if 1
        case rdpa_if_ssid0 ... rdpa_if_ssid15:
            *is_wifi_port = 1;
            break; /* 'phy_port_id' should remain -1. */
#endif

        default:
#if defined(CONFIG_BCM_EXT_SWITCH)  /* Only external switch ports in-use */
           {
              /* Ext Switch Only */
               uint16_t brcm_tag = ((BcmEnet_hdr2*)(*pBuf))->brcm_tag;
              ((BcmEnet_hdr2*)(*pBuf))->brcm_type = htons(BRCM_TYPE2);
              *phy_port_id = BCM_PORT_FROM_TYPE2_TAG(ntohs(brcm_tag));
              *phy_port_id = PHYSICAL_PORT_TO_LOGICAL_PORT(*phy_port_id, 1); /* Logical port for External switch port */
           }
#else  /* Only Internal switch */
#ifndef BRCM_FTTDP
           *phy_port_id = info.src_port - rdpa_if_lan0;
#else
           if (info.src_port == G9991_DEBUG_RDPA_PORT)
               *phy_port_id = g9991_bp_debug_port;
           else
           {
               *gemid = info.src_port;
               *phy_port_id = SID_PORT_ID;
           }
#endif
#endif
           break;
   }

#if 1
    if (*is_wifi_port)
    {
        char devname[IFNAMSIZ];

        unsigned int unit;

        unit = info.src_port - rdpa_if_ssid0;
        if (unit < WL_NUM_OF_SSID_PER_UNIT)
        {
            if (unit == 0)
                sprintf(devname, "wl0");
            else
                sprintf(devname, "wl0.%u", unit);
        }
        else
        {
            if (unit == WL_NUM_OF_SSID_PER_UNIT)
                strcpy(devname, "wl1");
            else
                sprintf(devname, "wl1.%u", unit- WL_NUM_OF_SSID_PER_UNIT);
        }
        *dev = __dev_get_by_name(&init_net, devname);
        return BCMEAPI_CTRL_TRUE | BCMEAPI_CTRL_FLAG_TRUE;
    }
#endif
    return BCMEAPI_CTRL_TRUE;
}
#endif /* (CONFIG_BCM963138 || CONFIG_BCM963148) */

typedef unsigned char (*EponLinkTxFunc) (unsigned char *data,unsigned int size,
    rdpa_cpu_tx_info_t *info);
        
static inline int _rdpa_cpu_send_sysb(bdmf_sysb sysb, rdpa_cpu_tx_info_t *info)
{
    int rc;

#ifdef BRCM_FTTDP
    /* BRCM_FTTDP FW does not support sending from sysb, so we need to copy to bpm */
    rc = rdpa_cpu_send_raw(bdmf_sysb_data(sysb), bdmf_sysb_length(sysb), info);
    /* rdpa_cpu_send_raw copies to bpm but does not free buffer */
    nbuff_flushfree(sysb);
#else
    rc = rdpa_cpu_send_sysb(sysb, info);
#endif
    return rc;
}

#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
static inline int bcmeapi_pkt_xmt_dispatch (EnetXmitParams *pParam)
{
    if (pParam->port_id == wan_port_id)
    {
        int rc;

        rc = rdpa_cpu_tx_port_enet_wan((bdmf_sysb)pParam->pNBuff, pParam->egress_queue);
        if (rc < 0)
        {
            /* skb is already released by rdpa_cpu_tx_port_enet_wan() */
            pParam->pDevPriv->estats.tx_dropped_runner_wan_fail++;
            return BCMEAPI_CTRL_SKIP;
        }
    }
    else /* LAN */
    {
        int rc;
        uint32_t phys_port = LOGICAL_PORT_TO_PHYSICAL_PORT(pParam->port_id);

        rc = rdpa_cpu_tx_port_enet_lan((bdmf_sysb)pParam->pNBuff, pParam->egress_queue, phys_port);
        if (rc < 0)
        {
            /* skb is already released by rdpa_cpu_tx_port_enet_lan() */
            pParam->pDevPriv->estats.tx_dropped_runner_lan_fail++;
            return BCMEAPI_CTRL_SKIP;
        }
    }

    return BCMEAPI_CTRL_CONTINUE;
}
#else /* (CONFIG_BCM963138 || CONFIG_BCM963148) */
static inline int bcmeapi_pkt_xmt_dispatch (EnetXmitParams *pParam)
{
    int rc;
    rdpa_cpu_tx_info_t info = {0};

    
    if (pParam->port_id == GPON_PORT_ID || ((init_cfg.wan_type == rdpa_wan_gbe) && (init_cfg.gbe_wan_emac == pParam->port_id)))
    {
        info.method = rdpa_cpu_tx_port;
        info.port = rdpa_if_wan0;
        info.x.wan.queue_id = pParam->egress_queue;
#if defined(ENET_GPON_CONFIG)
        if (init_cfg.wan_type == rdpa_wan_gpon)
            info.x.wan.flow = pParam->gemid;
#endif
        rc = _rdpa_cpu_send_sysb((bdmf_sysb)pParam->pNBuff, &info);
        if (rc < 0)
        {
#if defined(ENET_GPON_CONFIG)
            if (init_cfg.wan_type == rdpa_wan_gpon)
            {
                rdpa_gem_flow_us_cfg_t us_cfg = {};
                bdmf_object_handle gem = NULL; 

                rdpa_gem_get(pParam->gemid, &gem);
                if (gem)
                { 
                    rdpa_gem_us_cfg_get(gem,&us_cfg); 
                    bdmf_put(gem);
                    if (!us_cfg.tcont)
                    { 
                        printk("can't send sysb - no Tcont US cfg for gem (%d) \n", pParam->gemid);
                        pParam->pDevPriv->estats.tx_dropped_no_gem_tcount++;
                        return BCMEAPI_CTRL_SKIP;
                    }
                }
            }
#endif
            printk(KERN_NOTICE "rdpa_cpu_send_sysb() for WAN port "
                "returned %d (wan_flow: %d queue_id: %u)\n", rc, info.x.wan.flow,
                info.x.wan.queue_id);
            /* skb is already released by rdpa_cpu_send_sysb() */

            pParam->pDevPriv->estats.tx_dropped_gpon_tx_fail++;
            return BCMEAPI_CTRL_SKIP;
        }
    }
#ifdef RDPA_VPORTS
    else if (pParam->port_id == SID_PORT_ID)
    {
        info.method = rdpa_cpu_tx_bridge;
        info.port = rdpa_if_wan0;
        /* XXX: Temporarily hardcoded: send to reserved gemflow */
        info.x.wan.flow = RDPA_MAX_GEM_FLOW + pParam->channel;
        info.x.wan.queue_id = 0; /* XXX: Temp. limitation default flow to queue 0 */
        rc = _rdpa_cpu_send_sysb((bdmf_sysb)pParam->pNBuff, &info);
        if (rc < 0)
        {
            printk(KERN_NOTICE "rdpa_cpu_send_raw() for rdpa vport %d rdpa_if %d "
                    "returned %d\n", pParam->port_id, info.port, rc);
            pParam->pDevPriv->estats.tx_dropped_sid_tx_fail++;
            return BCMEAPI_CTRL_SKIP;
        }
    }
#endif
#if defined(ENET_EPON_CONFIG)
    else if	(pParam->port_id == EPON_PORT_ID)
    {
        int rc = 0;
        bdmf_object_handle epon_obj = NULL;
        rdpa_epon_mode oam_mode =  rdpa_epon_ctc;

        /*TODO fix this , should not assume skb here and fkb does not have dev*/
        info.method = rdpa_cpu_tx_port;
        info.port = rdpa_if_wan0;
        info.x.wan.queue_id = pParam->egress_queue ;
        info.x.wan.flow = 0;

        if ((rc = rdpa_epon_get(&epon_obj)))
        {
            printk("rdpa_system_get failed, rc(%d) \n", rc);
        }
        else
        { 
            rdpa_epon_mode epon_mode;
            if ((rc = rdpa_epon_mode_get(epon_obj, &epon_mode)))
                printk("rdpa_epon_mode_get failed, rc(%d) \n", rc);
            else
                oam_mode = epon_mode;
            bdmf_put(epon_obj);
        }

        if (oam_mode == rdpa_epon_dpoe || oam_mode == rdpa_epon_bcm)
        {
            info.x.wan.queue_id = SKBMARK_GET_Q(pParam->mark);
            info.x.wan.flow = SKBMARK_GET_PORT (pParam->mark);
        }

        /*TODO fix this , should not assume skb here and fkb does not have dev*/
        if (IS_FKBUFF_PTR(pParam->pNBuff) || pParam->skb->dev != oam_dev)
        {
            /* epon data traffic */
            if (epon_data_tx_func != NULL)
            {            
                if (FALSE == (*(EponLinkTxFunc)epon_data_tx_func)(pParam->pNBuff,
                    bdmf_sysb_length((bdmf_sysb)pParam->pNBuff), &info))
                {
                    //printk(KERN_NOTICE "epon_data_tx_func() failed\n");
                    /* skb is already released by rdpa_cpu_send_sysb() */
                    pParam->pDevPriv->estats.tx_dropped_epon_tx_fail++;
                    return BCMEAPI_CTRL_SKIP;
                }
            }
            else
            {
                printk(KERN_NOTICE "epon_data_tx_func not defined\n");
                pParam->pDevPriv->estats.tx_dropped_no_epon_tx_fun++;
                return BCMEAPI_CTRL_BREAK;
            }
        } 
        else
        {
            /* epon oam traffic */
            if (oam_tx_func != NULL)
            {   
                if (FALSE == (*(EponLinkTxFunc)oam_tx_func)((unsigned char *)pParam->pNBuff,
                    bdmf_sysb_length((bdmf_sysb)pParam->pNBuff), &info))
                {
                    //printk(KERN_NOTICE "oam_tx_func() failed\n"); 
                    /* skb is already released by rdpa_cpu_send_sysb() */
                    pParam->pDevPriv->estats.tx_dropped_epon_oam_fail++;
                    return BCMEAPI_CTRL_SKIP;
                }
            }
            else
            {
                printk(KERN_NOTICE "oam_tx_func  not defined\n");
                pParam->pDevPriv->estats.tx_dropped_no_epon_oam_fun++;
                return BCMEAPI_CTRL_BREAK;
            }
        }    
    }
#endif	          		
    else /* LAN */
    {
#ifdef CONFIG_BCM_PTP_1588
        char *ptp_offset;
#endif
        info.method = rdpa_cpu_tx_port;
#if defined(CONFIG_BCM_EXT_SWITCH)
        info.port = rdpa_physical_port_to_rdpa_if(LOGICAL_PORT_TO_PHYSICAL_PORT(pParam->port_id));
#else
#ifndef BRCM_FTTDP
        info.port = LOGICAL_PORT_TO_PHYSICAL_PORT(pParam->port_id);
        info.port = (info.port == wan_port_id) ? rdpa_if_wan0 :  info.port + rdpa_if_lan0;

#else
        if (LOGICAL_PORT_TO_PHYSICAL_PORT(pParam->port_id) == g9991_bp_debug_port)
            info.port = G9991_DEBUG_RDPA_PORT;
        else
        {
            printk(KERN_NOTICE "Cannot map rdpa port for LAN port %d\n",
               pParam->port_id);
            nbuff_flushfree(pParam->pNBuff);
            pParam->pDevPriv->estats.tx_dropped_no_rdpa_port_mapped++;
            return BCMEAPI_CTRL_SKIP;
        }
#endif
#endif
        /* ToDo: should be correct queue mapping here */
#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
        info.x.lan.queue_id = pParam->egress_queue;
#else
        info.x.lan.queue_id = 0;
#endif
#ifdef CONFIG_BCM_PTP_1588
        if (is_pkt_ptp_1588((bdmf_sysb)pParam->pNBuff, &ptp_offset))
            rc = ptp_1588_cpu_send_sysb((bdmf_sysb)pParam->pNBuff, &info, ptp_offset);
        else
#endif
        rc = _rdpa_cpu_send_sysb((bdmf_sysb)pParam->pNBuff, &info);
        if (rc < 0)
        {
            printk(KERN_NOTICE "rdpa_cpu_send_sysb() for LAN port %d rdpa_if %d "
                "returned %d\n", pParam->port_id, info.port, rc);
            /* skb is already released by rdpa_cpu_send_sysb() */
            pParam->pDevPriv->estats.tx_dropped_xpon_lan_fail++;
            return BCMEAPI_CTRL_SKIP;
        }
    }
    return BCMEAPI_CTRL_CONTINUE;
}
#endif /* (CONFIG_BCM963138 || CONFIG_BCM963148) */

#endif /* _BCMENET_RUNNER_INLINE_H_ */

