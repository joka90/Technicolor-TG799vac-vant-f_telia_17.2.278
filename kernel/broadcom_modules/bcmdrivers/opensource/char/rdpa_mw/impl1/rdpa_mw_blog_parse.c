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
#include "rdpa_mw_blog_parse.h"
#include "bcmenet_common.h"
#include "rdpa_mw_vlan.h"

rdpa_if rdpa_mw_root_dev2rdpa_if(struct net_device *root_dev, rdpa_if *wifi_ssid)
{
    uint32_t hw_port, hw_port_type, physical_hw_port;

    hw_port = netdev_path_get_hw_port(root_dev);
    /* In case of external switch netdev_path_get_hw_port return logical port and not HW port.  
       It is assumed that hw port cvalues are 0-7 and logical port values are 8-15*/
    physical_hw_port = LOGICAL_PORT_TO_PHYSICAL_PORT(hw_port);

    hw_port_type = netdev_path_get_hw_port_type(root_dev);

    switch (hw_port_type)
    {
    case BLOG_SIDPHY:
        return rdpa_if_lan0 + hw_port;
    case BLOG_ENETPHY:
        return rdpa_port_map_from_hw_port(physical_hw_port, 1);
    case BLOG_WLANPHY:
        if (wifi_ssid)
            *wifi_ssid = rdpa_if_ssid0 + hw_port;
        return rdpa_if_wlan0;
    case BLOG_GPONPHY:
    case BLOG_EPONPHY:        
        return rdpa_if_wan0;
    default:
        BCM_LOG_ERROR(BCM_LOG_ID_RDPA, "Unknown HW port type %u\n", hw_port_type);
        break;
    }
    return rdpa_if_none;
}
EXPORT_SYMBOL(rdpa_mw_root_dev2rdpa_if);

static int blog_commands_parse_qos(Blog_t *blog, rdpa_ic_result_t *mcast_result)
{
    /* TODO: Implement me. */
    mcast_result->qos_method = rdpa_qos_method_flow;
    mcast_result->queue_id = 0;
    return 0; /* Temporary for testing */
}

static inline rdpa_if _blog_parse_port_get(struct net_device *dev)
{
    rdpa_if wifi_ssid = 0, port;

    port = rdpa_mw_root_dev2rdpa_if(dev, &wifi_ssid);

    return wifi_ssid ? : port;
}

static rdpa_forward_action blog_command_parse_macst_action(rdpa_if port)
{
    bdmf_object_handle cpu;

    /* If WFD supported – action forward, else trap. */
    if (port >= rdpa_if_ssid0 && port <= rdpa_if_ssid15)
    {
        /* If WLAN accelerator module is loaded, port rdpa_cpu_wlan0 should exist. If it is unloaded, rdpa_cpu_wlan0
         * should not exist and wifi multicast traffic should be trapped to CPU. */
        if (rdpa_cpu_get(rdpa_cpu_wlan0, &cpu))
            return rdpa_forward_action_host;
        bdmf_put(cpu);
    }
    return rdpa_forward_action_forward;
}

rdpa_if blog_parse_ingress_port_get(Blog_t *blog)
{
    return _blog_parse_port_get(netdev_path_get_root((struct net_device *)blog->rx.dev_p));
}
EXPORT_SYMBOL(blog_parse_ingress_port_get);

rdpa_if blog_parse_egress_port_get(Blog_t *blog)
{
    return _blog_parse_port_get(netdev_path_get_root((struct net_device *)blog->tx.dev_p));
}
EXPORT_SYMBOL(blog_parse_egress_port_get);

int blog_parse_mcast_result_get(Blog_t *blog, rdpa_ic_result_t *mcast_result)
{
    memset(mcast_result, 0, sizeof(rdpa_ic_result_t));

    BCM_LOG_INFO(BCM_LOG_ID_RDPA, "Parsing Multicast blog commands\n");

    mcast_result->egress_port = blog_parse_egress_port_get(blog);
    if (blog_rule_to_vlan_action(blog->blogRule_p, rdpa_dir_ds, &mcast_result->vlan_action))
        return -1;
    mcast_result->action = blog_command_parse_macst_action(mcast_result->egress_port);

    /* Parse DSCP, Opbit/Ipbit, and other QoS parameters */
    return blog_commands_parse_qos(blog, mcast_result); 
}
EXPORT_SYMBOL(blog_parse_mcast_result_get);

void blog_parse_mcast_result_put(rdpa_ic_result_t *mcast_result)
{
    if (mcast_result->policer)
        bdmf_put(mcast_result->policer);
    if (mcast_result->vlan_action)
        bdmf_put(mcast_result->vlan_action);
}
EXPORT_SYMBOL(blog_parse_mcast_result_put);
