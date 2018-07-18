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

#include "bcm_OS_Deps.h"
#include <linux/bcm_log.h>
#include <rdpa_api.h>
#include <linux/blog_rule.h>
#include <linux/vlanctl_bind.h>
#include "rdpa_mw_blog_parse.h"
#include "rdpa_mw_vlan.h"

int rdpa_egress_tm_queue_id_by_wan_flow_index(int *wan_flow, int index, uint32_t *queue_id);
int rdpa_egress_tm_queue_id_by_lan_port_queue(rdpa_if port, int queue, uint32_t *queue_id);

/* taken from bcm_vlan.h: */
#define BCM_VLAN_MAX_TPID_VALUES 4

/* taken from bcm_vlan_local.h: */
#define BCM_VLAN_PBITS_MASK 0xE000
#define BCM_VLAN_PBITS_SHIFT 13
#define BCM_VLAN_VID_MASK 0x0FFF
#define BCM_VLAN_CFI_MASK 0x1000
#define BCM_VLAN_FILTER_FLAGS_IS_UNICAST   0x0001
#define BCM_VLAN_FILTER_FLAGS_IS_MULTICAST 0x0002
#define BCM_VLAN_FILTER_FLAGS_IS_BROADCAST 0x0004


#define protoLogDebug bcmLog_logIsEnabled(BCM_LOG_ID_RDPA, BCM_LOG_LEVEL_DEBUG)
#define protoDebug(fmt, arg...) BCM_LOG_DEBUG(BCM_LOG_ID_RDPA, fmt, ##arg)
#define protoInfo(fmt, arg...) BCM_LOG_INFO(BCM_LOG_ID_RDPA, fmt, ##arg)

#define BLOG_KEY_FROM_DIR_IC_FLOW(dir, ingress_class_idx, flow_idx, egress_port) \
    (dir << 31 | egress_port << 24 | ingress_class_idx << 16 | flow_idx)
#define BLOG_HASH_FROM_KEY(blog_key) (blog_key & ~BLOG_KEY_FROM_DIR_IC_FLOW(0, 0, 0, 0x3F))
#define DIR_FROM_BLOG_KEY(blog_key) (blog_key >> 31)
#define EGRESS_PORT_FROM_BLOG_KEY(blog_key) ((blog_key >> 24) & 0x3F)
#define IC_IDX_FROM_BLOG_KEY(blog_key) ((blog_key >> 16) & 0xFF)
#define FLOW_IDX_FROM_BLOG_KEY(blog_key) (blog_key & 0xFF)

int rdpa_mw_packet_based_forward = 1; /* Packet based forwarding is always in use */
int rdpa_init_system(void);

struct blog_flow_entry
{
    DLIST_ENTRY(blog_flow_entry) list;
    uint32_t hash; /* blog_key without the egress_port set */
    uint8_t ref_cnt;
};

typedef struct blog_flow_entry blog_flow_entry_t;
DLIST_HEAD(blog_flow_list_t, blog_flow_entry);
static struct blog_flow_list_t blog_flow_list;

/* Compare ALL ic flow fields */
static int compare_ic_flows(rdpa_ic_info_t *a, rdpa_ic_info_t *b)
{
#define COMPARE(field) do \
    { \
        if (a->field != b->field) \
        { \
            protoDebug("mismatch at %s %ld %ld offset 0x%x", #field, (long)a->field, (long)b->field, \
                offsetof(rdpa_ic_info_t, field)); \
            return -1; \
        } \
    } while (0)

    if (bdmf_ip_cmp(&a->key.src_ip, &b->key.src_ip))
        return -1;
    if (bdmf_ip_cmp(&a->key.dst_ip, &b->key.dst_ip))
        return -1;
    COMPARE(key.src_port);
    COMPARE(key.dst_port);
    COMPARE(key.protocol);
    COMPARE(key.outer_vid);
    COMPARE(key.inner_vid);
    if (memcmp(&a->key.dst_mac, &b->key.dst_mac, sizeof(a->key.dst_mac)))
        return -1;
    if (memcmp(&a->key.src_mac, &b->key.src_mac, sizeof(a->key.src_mac)))
        return -1;
    COMPARE(key.etype);
    COMPARE(key.dscp);
    COMPARE(key.ssid);
    COMPARE(key.ingress_port);
    COMPARE(key.outer_pbits);
    COMPARE(key.inner_pbits);
    COMPARE(key.number_of_vlans);
    COMPARE(key.ipv6_flow_label);
    COMPARE(key.outer_tpid);
    COMPARE(key.inner_tpid);
    COMPARE(key.l3_protocol);
    COMPARE(result.qos_method);
    COMPARE(result.wan_flow);
    COMPARE(result.action);
    COMPARE(result.policer);
    COMPARE(result.forw_mode);
    COMPARE(result.egress_port);
    COMPARE(result.queue_id);
    COMPARE(result.vlan_action);
    COMPARE(result.opbit_remark);
    COMPARE(result.opbit_val);
    COMPARE(result.ipbit_remark);
    COMPARE(result.ipbit_val);
    COMPARE(result.dscp_remark);
    COMPARE(result.dscp_val);
    COMPARE(result.ecn_val);
    COMPARE(result.pbit_to_gem_table);

    return 0;
}

static int blog_rule_mapping_get(blogRule_t *blog_rule, blogRuleCommand_t req_cmd, int *out)
{
    int i;

    for (i = 0; i < blog_rule->actionCount; i++)
    {
        blogRuleCommand_t cmd;

        cmd = blog_rule->action[i].cmd;
        if (cmd != req_cmd)
            continue;

        switch (cmd)
        {
            case BLOG_RULE_CMD_SET_SKB_MARK_PORT:
                *out = blog_rule->action[i].skbMarkPort;
                return 0;
            case BLOG_RULE_CMD_SET_SKB_MARK_QUEUE:
                *out = blog_rule->action[i].skbMarkQueue;
                return 0;
            case BLOG_RULE_CMD_SET_DSCP:
                *out = blog_rule->action[i].dscp;
                return 0;
            case BLOG_RULE_CMD_DROP:
                return 0;
            case BLOG_RULE_CMD_SET_DEI:
                *out = blog_rule->action[i].dei == 1 ? RDPA_IC_DEI_SET : RDPA_IC_DEI_CLEAR;
                return 0;
            default:
                break;
        }
    }

    return -1;
}

/* lookup if we have an existing flow with the same key/result besides vlan_action. Assume vlan_action in flow is not
   set */
static void match_ic_flow(bdmf_object_handle ingress_class_obj, rdpa_ic_info_t *flow, bdmf_index *flow_idx)
{
    rdpa_ic_info_t flow_tmp;

    *flow_idx = BDMF_INDEX_UNASSIGNED;
    while (rdpa_ingress_class_flow_get_next(ingress_class_obj, flow_idx) != BDMF_ERR_NO_MORE)
    {
        rdpa_ingress_class_flow_get(ingress_class_obj, *flow_idx, &flow_tmp);
        flow_tmp.result.vlan_action = NULL;
        if (!compare_ic_flows(&flow_tmp, flow))
        {
            protoDebug("IC flow match found idx(%d)", (int)*flow_idx);
            return;
        }
    }

    *flow_idx = BDMF_INDEX_UNASSIGNED;
    protoDebug("IC flow no match found");
}

static int ds_dal_enabled_get(int *enabled)
{
    bdmf_object_handle port_obj = NULL;
    rdpa_port_dp_cfg_t cfg;
    int rc;

    rc = rdpa_port_get(rdpa_if_wan0, &port_obj);
    if (rc)
        BDMF_TRACE_RET(rc, "Failed to get wan interface object\n");

    rdpa_port_cfg_get(port_obj, &cfg);
    *enabled = cfg.dal_enable;
    bdmf_put(port_obj);

    return 0;
}

static int configure_action_per_port(bdmf_object_handle ingress_class_obj, uint32_t flow_idx, rdpa_if port,
    bdmf_object_handle vlan_action_obj, int drop)
{
    rdpa_port_action_key_t pa_key = { .flow = flow_idx, .port = port };
    rdpa_port_action_t pa = { .vlan_action = vlan_action_obj, .drop = drop };
    int rc;

    protoDebug("per-port action config: flow_idx %d, port %d, drop %d vlan %p", (int)flow_idx, (int)port, drop,
        vlan_action_obj);
    rc = rdpa_ingress_class_port_action_set(ingress_class_obj, &pa_key, &pa);
    if (rc < 0)
        BDMF_TRACE_RET(rc, "Failed to update port action flow attribute\n");

    return 0;
}

static int add_modify_ic_flow(uint32_t field_mask, Blog_t *blog, bdmf_object_handle ingress_class_obj, rdpa_traffic_dir dir,
    uint32_t *flow_idx_p, rdpa_if *egress_port)
{
    int rc, is_drop_rule, blog_out;
    bdmf_index flow_idx = BDMF_INDEX_UNASSIGNED;
    rdpa_ic_info_t flow = {};
    blogRule_t *blog_rule = blog->blogRule_p;
    blogRuleFilter_t *filter = &blog_rule->filter;
    bdmf_object_handle vlan_action_obj = NULL;
    uint32_t queue_id=0, blog_queue_id=0;

    is_drop_rule = !blog_rule_mapping_get(blog_rule, BLOG_RULE_CMD_DROP, NULL);

    if (field_mask & RDPA_IC_MASK_DST_MAC)
        memcpy(&flow.key.dst_mac, filter->eth.value.h_dest, sizeof(flow.key.dst_mac));

    if (field_mask & RDPA_IC_MASK_SRC_MAC)
        memcpy(&flow.key.src_mac, filter->eth.value.h_source, sizeof(flow.key.src_mac));

    if (field_mask & RDPA_IC_MASK_ETHER_TYPE)
    {
        if (filter->nbrOfVlanTags == 2)
            flow.key.etype = filter->vlan[1].value.h_vlan_encapsulated_proto;
        else if (filter->nbrOfVlanTags == 1)
            flow.key.etype = filter->vlan[0].value.h_vlan_encapsulated_proto;
        else
            flow.key.etype = filter->eth.value.h_proto;
    }

    if (field_mask & RDPA_IC_MASK_IP_PROTOCOL)
        flow.key.protocol = filter->ipv4.value.ip_proto;

    if (field_mask & RDPA_IC_MASK_DSCP)
        flow.key.dscp = blog_rule_filterInUse(filter->ipv6.mask.tclass) ? filter->ipv6.value.tclass : filter->ipv4.value.tos;

    if (field_mask & RDPA_IC_MASK_OUTER_PBIT)
        flow.key.outer_pbits = (filter->vlan[0].value.h_vlan_TCI & BCM_VLAN_PBITS_MASK) >> BCM_VLAN_PBITS_SHIFT;

    if (field_mask & RDPA_IC_MASK_INNER_PBIT)
        flow.key.inner_pbits = (filter->vlan[1].value.h_vlan_TCI & BCM_VLAN_PBITS_MASK) >> BCM_VLAN_PBITS_SHIFT;

    if (field_mask & RDPA_IC_MASK_OUTER_VID)
        flow.key.outer_vid = filter->vlan[0].value.h_vlan_TCI & BCM_VLAN_VID_MASK;

    if (field_mask & RDPA_IC_MASK_INNER_VID)
        flow.key.inner_vid = filter->vlan[1].value.h_vlan_TCI & BCM_VLAN_VID_MASK;

    if (field_mask & RDPA_IC_MASK_OUTER_TPID)
        flow.key.outer_tpid = filter->eth.value.h_proto;
    
    if (field_mask & RDPA_IC_MASK_INNER_TPID)
        flow.key.inner_tpid = filter->vlan[0].value.h_vlan_encapsulated_proto;

    flow.key.number_of_vlans = filter->nbrOfVlanTags;
    flow.result.policer = NULL;
    flow.result.qos_method = rdpa_qos_method_flow;
    flow.result.queue_id = 0; /* XXX: insert qos queue mapping function here */
    if (!blog_rule_mapping_get(blog_rule, BLOG_RULE_CMD_SET_DEI, &blog_out))
        flow.result.dei_command = blog_out;
    else
        flow.result.dei_command = RDPA_IC_DEI_COPY;

    if (!is_drop_rule && (blog->tx.info.phyHdrType == BLOG_GPONPHY || blog->tx.info.phyHdrType == BLOG_EPONPHY))
    {
        if (blog_rule_mapping_get(blog_rule, BLOG_RULE_CMD_SET_SKB_MARK_PORT, &blog_out))
            BDMF_TRACE_RET(-1, "skb port not set for xPON, skipping blog (0x%p) rule\n", blog);
        
        flow.result.wan_flow = blog_out;
    }

    /* DS will use per-port vlan_action drop instead of forward_action_drop */
    flow.result.action = (dir == rdpa_dir_ds || !is_drop_rule) ? rdpa_forward_action_forward : rdpa_forward_action_drop;

    if (!blog_rule_mapping_get(blog_rule, BLOG_RULE_CMD_SET_SKB_MARK_QUEUE, &blog_out))
        blog_queue_id = blog_out; /* Defaults to queue 0 if not specified in blog */

    if (dir == rdpa_dir_us)
    {
        int wan_flow;

        flow.key.ingress_port = blog_parse_ingress_port_get(blog);
        if (rdpa_if_is_wifi(flow.key.ingress_port))
        {
            flow.key.ssid = flow.key.ingress_port;
            flow.key.ingress_port = rdpa_if_wlan0;
        }

        *egress_port = 0; /* egress_port is passed on for the blog_key mask */
        if (blog->tx.info.phyHdrType == BLOG_GPONPHY)
        {
            /*for us, we should get the queue id as given from the wan flow->us cfg->tcont->egress_tm->priority X subsidary->queue-0*/
           /*rc id should be retrieved from wan_flow->tcont->egress_tm*/
           wan_flow = flow.result.wan_flow;
           if(rdpa_egress_tm_queue_id_by_wan_flow_index(&wan_flow, blog_queue_id, &queue_id)==0)
           {
                 flow.result.queue_id  = queue_id;
            }
            else
            {
                 flow.result.queue_id  = 0;
            }
        }
    }
    
    else /* dir = rdpa_dir_ds */
    {
        if (field_mask & RDPA_IC_MASK_INGRESS_PORT && blog->rx.info.phyHdrType == BLOG_GPONPHY) /* XXX: check EPON LLID */
        {
            /* port mark filter value is offset by 1 because
             * 0 is reserved to indicate filter not in use.
             * Therefore use 16-bit to cover the supported
             * port range [0 to 255].
             */ 
            flow.key.ingress_port = filter->skb.markPort - 1;
        }

        *egress_port = blog_parse_egress_port_get(blog);

        // for downstream , it's stricted priority queue scheduler. There're GPON_PHY_DS_PQ_MAX (4)
        // downstream priority queue that have indexes from 0 to 3
        // the rdpa downstream priority queue is reversed  with cms priority number
        // for ex: if priority number is 0 then priority queue idx should be 3
        // if priority number is 1 then priority queue idx should be 2
       if (blog->rx.info.phyHdrType == BLOG_GPONPHY)
       {
            if((RDPA_IC_PHY_DS_PQ_MAX - 1) >= flow.result.queue_id)
            {
                if (rdpa_egress_tm_queue_id_by_lan_port_queue(*egress_port, (int)((RDPA_IC_PHY_DS_PQ_MAX - 1) -blog_queue_id), &queue_id)==0)
                {
                     flow.result.queue_id  = queue_id;
                }
                else
                {
                     flow.result.queue_id  = 0;
                }
            }
            else
            {
                   flow.result.queue_id  = 0;
            }
       }

        /* XXX: ds_dal is not supposed to change during runtime, should be moved to init after rdpa script is eliminated */
        if (ds_dal_enabled_get(&rdpa_mw_packet_based_forward))
            return -1;
        if (rdpa_mw_packet_based_forward)
        {
            flow.result.forw_mode = rdpa_forwarding_mode_pkt;
            /* XXX: rdpa_ingress_class should ignore egress_port when rdpa_forwarding_mode_pkt */
            flow.result.egress_port = rdpa_if_lan0;

            /* This should always be the last line after flow configuration, check if the same flow exists besides the
               vlan_action */
            match_ic_flow(ingress_class_obj, &flow, &flow_idx);
        }
        else
        {
            flow.result.forw_mode = rdpa_forwarding_mode_flow;
            flow.result.egress_port = *egress_port;
        }
    }

    if (!is_drop_rule)
    {
        rc = blog_rule_to_vlan_action(blog_rule, dir, &vlan_action_obj);
        if (rc < 0)
            BDMF_TRACE_RET(rc, "Failed to assign a valid vlan_action\n");
    }

    /* US flows and new DS flows have vlan_action set, otherwise set port_action */
    if (flow_idx != BDMF_INDEX_UNASSIGNED)
    {
        if (dir == rdpa_dir_ds)
        {
            if (!rdpa_mw_packet_based_forward)
            {
                BDMF_TRACE_RET(BDMF_ERR_INTERNAL, "Flow based forwarding cannot assign multiple ports to same flow"
                    "(%d)\n", (int)flow_idx);
            }

            protoDebug("Reusing flow_idx %d:", (int)flow_idx);
            rc = configure_action_per_port(ingress_class_obj, flow_idx, *egress_port, vlan_action_obj, is_drop_rule);
            if (rc)
                return rc;
        }
        else
            BDMF_TRACE_RET(BDMF_ERR_INTERNAL, "US flow already assigned to flow %d\n", (int)flow_idx);
    }
    else
    {
        /* Packet based forwarding sets vlan_action per-port only */
        if (!rdpa_mw_packet_based_forward || dir == rdpa_dir_us)
            flow.result.vlan_action = vlan_action_obj;

        rc = rdpa_ingress_class_flow_add(ingress_class_obj, &flow_idx, &flow);
        if (rc < 0)
            BDMF_TRACE_RET(rc, "Failed to create ingress_class flow attribute\n");

        if (rdpa_mw_packet_based_forward && dir == rdpa_dir_ds)
        {
            rdpa_if lan_port;

            /* Since this is a new flow, initialize all non-configured lan egress ports to drop */
            for (lan_port = rdpa_if_lan0; lan_port <= rdpa_if_ssid0; ++lan_port)
            {
                int this_rule_port = (lan_port == *egress_port);

                /* Per-port vlan action can set rdpa_if_0..4 + any ssid port (only one action for all wifi ports) */
                if (!rdpa_if_is_lan_or_wifi(lan_port) || (lan_port >= rdpa_if_lan5 && lan_port <= rdpa_if_lan7) ||
                    lan_port == rdpa_if_wlan0 || lan_port == rdpa_if_wlan1)
                {
                    continue;
                }

                protoDebug("this_rule_port %d: ", this_rule_port);
                rc = configure_action_per_port(ingress_class_obj, flow_idx, lan_port,
                    this_rule_port ? vlan_action_obj : NULL, is_drop_rule);
                if (rc)
                    return rc;
            }
        }
    }

    *flow_idx_p = flow_idx;
    return 0;
}

static int add_ic_cfg(uint32_t field_mask, rdpa_traffic_dir dir, int prty, bdmf_object_handle *ingress_class_obj)
{
    rdpa_ic_cfg_t cfg = { .type = RDPA_IC_TYPE_FLOW, .field_mask = field_mask, .prty = prty };
    int rc;
    BDMF_MATTR(ingress_class_attrs, rdpa_ingress_class_drv());

    rdpa_ingress_class_dir_set(ingress_class_attrs, dir);
    rdpa_ingress_class_cfg_set(ingress_class_attrs, &cfg);
    rc = bdmf_new_and_set(rdpa_ingress_class_drv(), NULL, ingress_class_attrs, ingress_class_obj);
    if (rc < 0)
        BDMF_TRACE_RET(rc, "Failed to create ingress_class object\n");

    return 0;
}

static int filter_to_mask(blogRuleFilter_t *filter, rdpa_traffic_dir dir, int is_us_wifi, uint32_t *mask)
{
    *mask = RDPA_IC_MASK_NUM_OF_VLANS;

    if (filter->nbrOfVlanTags > 2 ||
        blog_rule_filterInUse(filter->skb.priority) ||
        blog_rule_filterInUse(filter->skb.markFlowId) || /* Used by linux tc - will not be in use by rdpa_mw */
        (filter->flags && filter->flags != (BCM_VLAN_FILTER_FLAGS_IS_UNICAST | BCM_VLAN_FILTER_FLAGS_IS_BROADCAST)) 
/* It appears sometimes blogs are send with CFI filter (always set to 0). We ignore them since we do not want to fail blog creation:
        || filter->vlan[0].mask.h_vlan_TCI & BCM_VLAN_CFI_MASK || filter->vlan[1].mask.h_vlan_TCI & BCM_VLAN_CFI_MASK ||
*/
        )
    {
        return -1; /* Unsupported filter */
    }
    if ((filter->nbrOfVlanTags == 2 && blog_rule_filterInUse(filter->vlan[1].mask.h_vlan_encapsulated_proto)) ||
        (filter->nbrOfVlanTags == 1 && blog_rule_filterInUse(filter->vlan[0].mask.h_vlan_encapsulated_proto)) ||
        (blog_rule_filterInUse(filter->eth.mask.h_proto) && !is_tpid(filter->eth.value.h_proto)))
    {
        *mask |= RDPA_IC_MASK_ETHER_TYPE;
    }

    *mask |= blog_rule_filterInUse(filter->eth.mask.h_dest) ? RDPA_IC_MASK_DST_MAC : 0;
    *mask |= blog_rule_filterInUse(filter->eth.mask.h_source) ? RDPA_IC_MASK_SRC_MAC : 0;
    *mask |= blog_rule_filterInUse(filter->ipv4.mask.ip_proto) ? RDPA_IC_MASK_IP_PROTOCOL : 0;
    *mask |= blog_rule_filterInUse(filter->ipv4.mask.tos) ? RDPA_IC_MASK_DSCP : 0;
    *mask |= blog_rule_filterInUse(filter->ipv6.mask.tclass) ? RDPA_IC_MASK_DSCP : 0;
    *mask |= dir == rdpa_dir_us || blog_rule_filterInUse(filter->skb.markPort) ? RDPA_IC_MASK_INGRESS_PORT : 0;
    *mask |= is_us_wifi ? RDPA_IC_MASK_SSID : 0;
    *mask |= filter->vlan[0].mask.h_vlan_TCI & BCM_VLAN_PBITS_MASK ? RDPA_IC_MASK_OUTER_PBIT : 0;
    *mask |= filter->vlan[0].mask.h_vlan_TCI & BCM_VLAN_VID_MASK ? RDPA_IC_MASK_OUTER_VID : 0;
    *mask |= filter->vlan[1].mask.h_vlan_TCI & BCM_VLAN_PBITS_MASK ? RDPA_IC_MASK_INNER_PBIT : 0;
    *mask |= filter->vlan[1].mask.h_vlan_TCI & BCM_VLAN_VID_MASK ? RDPA_IC_MASK_INNER_VID : 0;

    if (filter->nbrOfVlanTags >= 1 && blog_rule_filterInUse(filter->eth.mask.h_proto))
        *mask |= RDPA_IC_MASK_OUTER_TPID;
    
    if (filter->nbrOfVlanTags >= 2 && filter->vlan[0].mask.h_vlan_encapsulated_proto)
        *mask |= RDPA_IC_MASK_INNER_TPID;
    
    return 0;
}

static inline rdpa_traffic_dir blog_get_direction(Blog_t *blog)
{
    return ((struct net_device *)blog->rx.dev_p)->priv_flags & IFF_WANDEV ? rdpa_dir_ds : rdpa_dir_us;
}
            
static int ic_idx_exists_in_ref_list(bdmf_number ic_idx)
{
    blog_flow_entry_t *entry = NULL, *tmp_entry;

    DLIST_FOREACH_SAFE(entry, &blog_flow_list, list, tmp_entry)
    {
        if (IC_IDX_FROM_BLOG_KEY(entry->hash) == (int)ic_idx)
            return 1;
    }

    return 0;
}

static int find_lowest_free_prty(rdpa_traffic_dir dir, int from_prty)
{
    int i, used_prio[RDPA_IC_MAX_PRIORITY] = {};
    bdmf_object_handle obj = NULL;
    rdpa_traffic_dir dir_temp;
    rdpa_ic_cfg_t cfg;
    
    while ((obj = bdmf_get_next(rdpa_ingress_class_drv(), obj, NULL)))
    {
        rdpa_ingress_class_dir_get(obj, &dir_temp);
        if (dir != dir_temp)
            continue;

        rdpa_ingress_class_cfg_get(obj, &cfg);
        if (cfg.type != RDPA_IC_TYPE_FLOW)
            continue;

        used_prio[cfg.prty] = 1;
    }

    for (i = from_prty; i < RDPA_IC_MAX_PRIORITY; i++)
        if (!used_prio[i])
            return i;

    return -1;
}

/* highest_prty returns priority of highest rdpa_mw IC */
static void find_prty_and_ic_by_field_mask(uint32_t field_mask, rdpa_traffic_dir dir, bdmf_object_handle *ingress_class_obj,
    int *highest_prty)
{
    bdmf_object_handle obj = NULL;
    rdpa_traffic_dir dir_temp;
    rdpa_ic_cfg_t cfg;
    bdmf_number idx;
    
    *highest_prty = 0;
    *ingress_class_obj = NULL;
    while ((obj = bdmf_get_next(rdpa_ingress_class_drv(), obj, NULL)))
    {
        rdpa_ingress_class_dir_get(obj, &dir_temp);
        if (dir != dir_temp)
            continue;

        rdpa_ingress_class_cfg_get(obj, &cfg);
        if (cfg.type != RDPA_IC_TYPE_FLOW)
            continue;

        if (ingress_class_obj && !*ingress_class_obj && cfg.field_mask == field_mask)
        {
            bdmf_number idx;

            rdpa_ingress_class_index_get(obj, &idx);
            /* Reuse only ic's which rdpa_mw created */
            if (ic_idx_exists_in_ref_list(idx))
                *ingress_class_obj = obj;
        }

        /* XXX: should pick highest number from all IC's, not only rdpa_mw's */
        rdpa_ingress_class_index_get(obj, &idx);
        if (ic_idx_exists_in_ref_list(idx) && cfg.prty > *highest_prty)
            *highest_prty = cfg.prty;
    }
}

static uint8_t *find_blog_flow_hash(uint32_t key, int add)
{
    blog_flow_entry_t *entry = NULL, *tmp_entry;

    DLIST_FOREACH_SAFE(entry, &blog_flow_list, list, tmp_entry)
    {
        if (entry->hash == key)
            return &entry->ref_cnt;
    }

    if (!add)
        return NULL;

    entry = bdmf_alloc(sizeof(blog_flow_entry_t));
    if (!entry)
        return NULL;

    entry->ref_cnt = 0;
    entry->hash = key;
    DLIST_INSERT_HEAD(&blog_flow_list, entry, list);

    return &entry->ref_cnt;
}

static void remove_blog_flow_hash_by_ref_cnt(uint8_t *ref_cnt)
{
    blog_flow_entry_t *entry = NULL, *tmp_entry;

    DLIST_FOREACH_SAFE(entry, &blog_flow_list, list, tmp_entry)
    {
        if (&entry->ref_cnt == ref_cnt)
        {
            DLIST_REMOVE(entry, list);
            bdmf_free(entry);
        }
    }
}

static void ic_clean(bdmf_object_handle ingress_class_obj)
{
    bdmf_number nflows;
    
    rdpa_ingress_class_nflow_get(ingress_class_obj, &nflows);
    if (!nflows)
        bdmf_destroy(ingress_class_obj);
}

static uint32_t activate_blogRule(Blog_t *blog)
{
    uint32_t field_mask, flow_idx;
    bdmf_object_handle ingress_class_obj;
    rdpa_traffic_dir dir = blog_get_direction(blog);
    int prty, rc, reuse = 0;
    uint8_t *ref_cnt;
    bdmf_number ic_idx;
    rdpa_if egress_port;

    rc = filter_to_mask(&((blogRule_t*)blog->blogRule_p)->filter, dir,
        rdpa_if_is_wifi(blog_parse_ingress_port_get(blog)), &field_mask);
    if (rc < 0)
        BDMF_TRACE_RET(BLOG_KEY_INVALID, "blogRule filter not supported for blog %p\n", blog);

    find_prty_and_ic_by_field_mask(field_mask, dir, &ingress_class_obj, &prty);
    if (!ingress_class_obj)
    {
        /* Priority of classifier will be higher then the highest in use, since we assume classifiers are handled in the
           order blog rules are processed.
           XXX: if a blog rule/classfier in the "middle" will be removed/inserted, this would not be handled correctly
        */
        prty = find_lowest_free_prty(dir, prty);
        if (prty == -1)
            BDMF_TRACE_RET(BLOG_KEY_INVALID, "Cannot create IC: all priorities are in use\n");

        protoDebug("<prty=%d>", prty);
        rc = add_ic_cfg(field_mask, dir, prty, &ingress_class_obj);
        if (rc)
            return BLOG_KEY_INVALID;
    }
    else
        reuse = 1;

    rdpa_ingress_class_index_get(ingress_class_obj, &ic_idx);
    protoDebug("%s ingress_class/dir=%s,index=%d", reuse ? "Reusing" : "Created", dir == rdpa_dir_ds ? "ds" : "us",
        (int)ic_idx);

    rc = add_modify_ic_flow(field_mask, blog, ingress_class_obj, dir, &flow_idx, &egress_port);
    if (rc)
    {
        BDMF_TRACE_ERR("Failed to create ic flow. blogRule not supported, rc=%d\n", rc);
        goto Error;
    }

    protoDebug("Created flow dir %d, ic_idx %d, flow_idx %d, blog_key %x", dir, (int)ic_idx, (int)flow_idx,
        (uint32_t)BLOG_KEY_FROM_DIR_IC_FLOW(dir, ic_idx, flow_idx, egress_port));

    /* inc. ref_cnt of blog_flow_hash - matching flows with different egress_port share same entry */
    ref_cnt = find_blog_flow_hash(BLOG_KEY_FROM_DIR_IC_FLOW(dir, ic_idx, flow_idx, 0), 1);
    if (!ref_cnt)
    {
        rdpa_ingress_class_flow_delete(ingress_class_obj, flow_idx);
        BDMF_TRACE_ERR("malloc failed for find_blog_flow_hash\n");
        goto Error;
    }

    ++(*ref_cnt);
    return BLOG_KEY_FROM_DIR_IC_FLOW(dir, ic_idx, flow_idx, egress_port);

Error:
    if (!reuse)
        ic_clean(ingress_class_obj);

    return BLOG_KEY_INVALID;
}

static uint32_t rdpa_config_fp(Blog_t *blog_p, BlogTraffic_t traffic)
{
    blogRule_t *blog_rule = blog_p->blogRule_p;

    BCM_ASSERT(blog_p != BLOG_NULL && traffic < BlogTraffic_MAX);
    protoDebug("Activating Blog rule (%p), Blog (%p)", blog_rule, blog_p);
    if (protoLogDebug)
    {
        /* blog_dump(blog_p); */
        blog_rule_dump(blog_rule);
    }

    if (traffic != BlogTraffic_Layer2_Flow)
        BDMF_TRACE_RET(BLOG_KEY_INVALID, "rdpa_mw handles only L2 blog rule flows\n");

    return activate_blogRule(blog_p);
}

/* Always return BLOG_NULL, even if error */
static Blog_t *deactivate_blogRule(uint32_t blog_key)
{
    rdpa_ingress_class_key_t key = { .dir = DIR_FROM_BLOG_KEY(blog_key), .index = IC_IDX_FROM_BLOG_KEY(blog_key) };
    bdmf_object_handle ingress_class_obj = NULL;
    int rc;
    char *dir_str = key.dir == rdpa_dir_ds ? "ds" : "us";
    uint8_t *ref_cnt;
    
    /* dec. ref_cnt of blog_flow_hash entry */
    ref_cnt = find_blog_flow_hash(BLOG_HASH_FROM_KEY(blog_key), 0);
    if (!ref_cnt)
    {
        /* Matching blog_key entry not found */
        BUG();
        return BLOG_NULL;
    }
    
    rdpa_ingress_class_get(&key, &ingress_class_obj);
    if (!ingress_class_obj)
    {
        BDMF_TRACE_ERR("Cannot find ingress_class/dir=%s,index=%d\n", dir_str, (int)key.index);
        return BLOG_NULL;
    }
    bdmf_put(ingress_class_obj); /* above rdpa_ingress_class_get, obj still locked by 'blog-flow' so its safe to put */

    if (--(*ref_cnt))
    {
        protoDebug("IC flow still in use, setting port-action to drop, still in use by %d ports", *ref_cnt);
        configure_action_per_port(ingress_class_obj, FLOW_IDX_FROM_BLOG_KEY(blog_key),
            EGRESS_PORT_FROM_BLOG_KEY(blog_key), NULL, 1);
        return BLOG_NULL; /* Do nothing because there are still ports using the same flow */
    }

    rc = rdpa_ingress_class_flow_delete(ingress_class_obj, FLOW_IDX_FROM_BLOG_KEY(blog_key));
    if (rc)
    {
        BDMF_TRACE_ERR("Cannot delete ingress_class/dir=%s,index=%d flow[%d], rc=%d\n",
            dir_str, (int)key.index, FLOW_IDX_FROM_BLOG_KEY(blog_key), rc);
    }

    remove_blog_flow_hash_by_ref_cnt(ref_cnt);
    ic_clean(ingress_class_obj);

    return BLOG_NULL;
}

/* Deactivate the FP configuration, recognized by 'key'. */
static Blog_t *rdpa_deconf_fp(uint32_t blog_key, BlogTraffic_t traffic)
{
    protoDebug("rdpa_deconf_fp, blog key %x", blog_key);
    if (traffic != BlogTraffic_Layer2_Flow)
        return BLOG_NULL;
    
    return deactivate_blogRule(blog_key);
}

static bdmf_object_handle rdpa_system = NULL;
static rdpa_ip_class_method method;

static void rdpa_disable_all_tpid(void)
{
    rdpa_tpid_detect_cfg_t entry = {};
    rdpa_tpid_detect_t tpid;

    for(tpid=0; tpid< rdpa_tpid_detect__num_of; ++tpid)
        rdpa_system_tpid_detect_set(rdpa_system, tpid, &entry);
}

static void rdpa_setTpidTable(unsigned int *tpidTable)
{
    rdpa_tpid_detect_cfg_t entry = {.val_udef=0, .otag_en=1, .itag_en=1};
    rdpa_tpid_detect_t tpid;
    int rc, i;

    rdpa_disable_all_tpid();

    for(i=0; i<BCM_VLAN_MAX_TPID_VALUES; ++i)
    {
        switch(tpidTable[i])
        {
        case 0x8100:
            tpid = rdpa_tpid_detect_0x8100;
            break;
        case 0x88A8:
            tpid = rdpa_tpid_detect_0x88A8;
            break;
        case 0x9100:
            tpid = rdpa_tpid_detect_0x9100;
            break;
        case 0x9200:
            tpid = rdpa_tpid_detect_0x9200;
            break;
        default:
            tpid = rdpa_tpid_detect_udef_1;
            entry.val_udef = tpidTable[i];
            break;
        }
		
        rc = rdpa_system_tpid_detect_set(rdpa_system, tpid, &entry);
        if (rc)
            protoDebug("Set tpid error\n");
    }
}

static void rdpa_notify_fp( vlanctl_bind_Notify_t event, void *ptr)
{
    unsigned int *tpidTable = (unsigned int *)ptr;

    switch(event)
    {
    case VLANCTL_BIND_TPID_NOTIFY:
        protoDebug("Activating blog tpid set");
        rdpa_setTpidTable(tpidTable);
        break;
    default:
        break;
    }
}

static void rdpa_bind_vlanctl(int enabled)
{
    vlanctl_bind_t hook_info;

    protoDebug("rdpa_bind_vlanctl %s", enabled ? "enable" : "disable");

    /* Register for all possible calls from vlanctl_bind */
    hook_info.hook_info = 0xFF;
    if (enabled)
    {
        if (method == rdpa_method_fc)
            vlanctl_bind_config((vlanctl_bind_ScHook_t)NULL, (vlanctl_bind_SdHook_t)NULL, 
                 (vlanctl_bind_SnHook_t)rdpa_notify_fp,  VLANCTL_BIND_CLIENT_RUNNER, hook_info);
        else 
            vlanctl_bind_config((vlanctl_bind_ScHook_t)rdpa_config_fp, (vlanctl_bind_SdHook_t)rdpa_deconf_fp, 
                 (vlanctl_bind_SnHook_t)rdpa_notify_fp,  VLANCTL_BIND_CLIENT_RUNNER, hook_info);
    }
    else 
        vlanctl_bind_config(NULL, NULL, NULL, VLANCTL_BIND_CLIENT_RUNNER, hook_info);

    protoInfo("vlanctl_bind rules reflection to Runner %s.", enabled ? "enabled" : "disabled");
}

int __init rdpa_mw_init(void)
{
    rdpa_system_init_cfg_t sys_cfg;

    bcmLog_setLogLevel(BCM_LOG_ID_RDPA, BCM_LOG_LEVEL_ERROR);
    rdpa_init_system();
    if (rdpa_system_get(&rdpa_system))
        return -1;

    rdpa_system_init_cfg_get(rdpa_system, &sys_cfg);
    method = sys_cfg.ip_class_method;
    BCM_LOG_INFO(BCM_LOG_ID_RDPA, "Loading rdpa_mw, RDPA ip class method = %d\n", method);
    
    DLIST_INIT(&blog_flow_list);

    rdpa_bind_vlanctl(1);

    return 0;
}

void __exit rdpa_mw_exit(void)
{
    blog_flow_entry_t *entry = NULL, *tmp_entry;

    BCM_LOG_INFO(BCM_LOG_ID_RDPA, "Exit\n");
    
    rdpa_bind_vlanctl(0);

    DLIST_FOREACH_SAFE(entry, &blog_flow_list, list, tmp_entry)
    {
        DLIST_REMOVE(entry, list);
        bdmf_free(entry);
    }
}

module_init(rdpa_mw_init);
module_exit(rdpa_mw_exit);

MODULE_DESCRIPTION("RDPA MW layer");
MODULE_VERSION("0.9");
MODULE_LICENSE("GPL");

