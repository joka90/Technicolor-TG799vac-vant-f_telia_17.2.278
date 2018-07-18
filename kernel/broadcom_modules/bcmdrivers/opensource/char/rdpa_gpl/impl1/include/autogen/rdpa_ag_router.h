// <:copyright-BRCM:2013:DUAL/GPL:standard
// 
//    Copyright (c) 2013 Broadcom Corporation
//    All Rights Reserved
// 
// Unless you and Broadcom execute a separate written software license
// agreement governing use of this software, this software is licensed
// to you under the terms of the GNU General Public License version 2
// (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
// with the following added to such license:
// 
//    As a special exception, the copyright holders of this software give
//    you permission to link this software with independent modules, and
//    to copy and distribute the resulting executable under terms of your
//    choice, provided that you also meet, for each linked independent
//    module, the terms and conditions of the license of that module.
//    An independent module is a module which is not derived from this
//    software.  The special exception does not apply to any modifications
//    of the software.
// 
// Not withstanding the above, under no circumstances may you combine
// this software in any way with any other Broadcom software provided
// under a license other than the GPL, without Broadcom's express prior
// written consent.
// 
// :>
/*
 * router object header file.
 * This header file is generated automatically. Do not edit!
 */
#ifndef _RDPA_AG_ROUTER_H_
#define _RDPA_AG_ROUTER_H_

/** \addtogroup router
 * @{
 */


/** Get router type handle.
 *
 * This handle should be passed to bdmf_new_and_set() function in
 * order to create a router object.
 * \return router type handle
 */
bdmf_type_handle rdpa_router_drv(void);

/* router: Attribute types */
typedef enum {
    rdpa_router_attr_firewall_enable = 0, /* firewall_enable : RW : bool/1 : Enable/Disable IP Firewall */
    rdpa_router_attr_arp = 1, /* arp : RWDF : ether_addr/6[4096(ip)] : ARP table */
} rdpa_router_attr_types;

extern int (*f_rdpa_router_get)(bdmf_object_handle *pmo);

/** Get router object.

 * This function returns router object instance.
 * \param[out] router_obj    Object handle
 * \return    0=OK or error <0
 */
int rdpa_router_get(bdmf_object_handle *router_obj);

/** Get router/firewall_enable attribute.
 *
 * Get Enable/Disable IP Firewall.
 * \param[in]   mo_ router object handle or mattr transaction handle
 * \param[out]  firewall_enable_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_router_firewall_enable_get(bdmf_object_handle mo_, bdmf_boolean *firewall_enable_)
{
    bdmf_number _nn_;
    int _rc_;
    _rc_ = bdmf_attr_get_as_num(mo_, rdpa_router_attr_firewall_enable, &_nn_);
    *firewall_enable_ = (bdmf_boolean)_nn_;
    return _rc_;
}


/** Set router/firewall_enable attribute.
 *
 * Set Enable/Disable IP Firewall.
 * \param[in]   mo_ router object handle or mattr transaction handle
 * \param[in]   firewall_enable_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_router_firewall_enable_set(bdmf_object_handle mo_, bdmf_boolean firewall_enable_)
{
    return bdmf_attr_set_as_num(mo_, rdpa_router_attr_firewall_enable, firewall_enable_);
}


/** Get router/arp attribute entry.
 *
 * Get ARP table.
 * \param[in]   mo_ router object handle or mattr transaction handle
 * \param[in]   ai_ Attribute array index
 * \param[out]  arp_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task and softirq contexts.
 */
static inline int rdpa_router_arp_get(bdmf_object_handle mo_, const bdmf_ip_t * ai_, bdmf_mac_t * arp_)
{
    return bdmf_attrelem_get_as_buf(mo_, rdpa_router_attr_arp, (bdmf_index)ai_, arp_, sizeof(*arp_));
}


/** Set router/arp attribute entry.
 *
 * Set ARP table.
 * \param[in]   mo_ router object handle or mattr transaction handle
 * \param[in]   ai_ Attribute array index
 * \param[in]   arp_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task and softirq contexts.
 */
static inline int rdpa_router_arp_set(bdmf_object_handle mo_, const bdmf_ip_t * ai_, const bdmf_mac_t * arp_)
{
    return bdmf_attrelem_set_as_buf(mo_, rdpa_router_attr_arp, (bdmf_index)ai_, arp_, sizeof(*arp_));
}


/** Delete router/arp attribute entry.
 *
 * Delete ARP table.
 * \param[in]   mo_ router object handle
 * \param[in]   ai_ Attribute array index
 * \return 0 or error code < 0
 * The function can be called in task and softirq contexts.
 */
static inline int rdpa_router_arp_delete(bdmf_object_handle mo_, const bdmf_ip_t * ai_)
{
    return bdmf_attrelem_delete(mo_, rdpa_router_attr_arp, (bdmf_index)ai_);
}


/** Get next router/arp attribute entry.
 *
 * Get next ARP table.
 * \param[in]   mo_ router object handle or mattr transaction handle
 * \param[in,out]   ai_ Attribute array index
 * \return 0 or error code < 0
 * The function can be called in task and softirq contexts.
 */
static inline int rdpa_router_arp_get_next(bdmf_object_handle mo_, bdmf_ip_t * ai_)
{
    return bdmf_attrelem_get_next(mo_, rdpa_router_attr_arp, (bdmf_index *)ai_);
}

/** @} end of router Doxygen group */


/** \addtogroup subnet
 * @{
 */


/** Get subnet type handle.
 *
 * This handle should be passed to bdmf_new_and_set() function in
 * order to create a subnet object.
 * \return subnet type handle
 */
bdmf_type_handle rdpa_subnet_drv(void);

/* subnet: Attribute types */
typedef enum {
    rdpa_subnet_attr_index = 0, /* index : MKRI : enum/4 : Subnet index */
    rdpa_subnet_attr_pppoe = 1, /* pppoe : RW : aggregate/8 subnet_pppoe(rdpa_subnet_pppoe_cfg_t) : PPPoE tunnel configuration */
    rdpa_subnet_attr_mac = 2, /* mac : RW : ether_addr/6 : MAC address */
    rdpa_subnet_attr_ipv4 = 3, /* ipv4 : RW : ipv4/4 : IPv4 address */
    rdpa_subnet_attr_ipv6 = 4, /* ipv6 : RW : ipv6/16 : IPv6 address */
} rdpa_subnet_attr_types;

extern int (*f_rdpa_subnet_get)(rdpa_subnet index_, bdmf_object_handle *pmo);

/** Get subnet object by key.

 * This function returns subnet object instance by key.
 * \param[in] index_    Object key
 * \param[out] subnet_obj    Object handle
 * \return    0=OK or error <0
 */
int rdpa_subnet_get(rdpa_subnet index_, bdmf_object_handle *subnet_obj);

/** Get subnet/index attribute.
 *
 * Get Subnet index.
 * \param[in]   mo_ subnet object handle or mattr transaction handle
 * \param[out]  index_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task and softirq contexts.
 */
static inline int rdpa_subnet_index_get(bdmf_object_handle mo_, rdpa_subnet *index_)
{
    bdmf_number _nn_;
    int _rc_;
    _rc_ = bdmf_attr_get_as_num(mo_, rdpa_subnet_attr_index, &_nn_);
    *index_ = (rdpa_subnet)_nn_;
    return _rc_;
}


/** Set subnet/index attribute.
 *
 * Set Subnet index.
 * \param[in]   mo_ subnet object handle or mattr transaction handle
 * \param[in]   index_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task and softirq contexts.
 */
static inline int rdpa_subnet_index_set(bdmf_object_handle mo_, rdpa_subnet index_)
{
    return bdmf_attr_set_as_num(mo_, rdpa_subnet_attr_index, index_);
}


/** Get subnet/pppoe attribute.
 *
 * Get PPPoE tunnel configuration.
 * \param[in]   mo_ subnet object handle or mattr transaction handle
 * \param[out]  pppoe_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_subnet_pppoe_get(bdmf_object_handle mo_, rdpa_subnet_pppoe_cfg_t * pppoe_)
{
    return bdmf_attr_get_as_buf(mo_, rdpa_subnet_attr_pppoe, pppoe_, sizeof(*pppoe_));
}


/** Set subnet/pppoe attribute.
 *
 * Set PPPoE tunnel configuration.
 * \param[in]   mo_ subnet object handle or mattr transaction handle
 * \param[in]   pppoe_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_subnet_pppoe_set(bdmf_object_handle mo_, const rdpa_subnet_pppoe_cfg_t * pppoe_)
{
    return bdmf_attr_set_as_buf(mo_, rdpa_subnet_attr_pppoe, pppoe_, sizeof(*pppoe_));
}


/** Get subnet/mac attribute.
 *
 * Get MAC address.
 * \param[in]   mo_ subnet object handle or mattr transaction handle
 * \param[out]  mac_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_subnet_mac_get(bdmf_object_handle mo_, bdmf_mac_t * mac_)
{
    return bdmf_attr_get_as_buf(mo_, rdpa_subnet_attr_mac, mac_, sizeof(*mac_));
}


/** Set subnet/mac attribute.
 *
 * Set MAC address.
 * \param[in]   mo_ subnet object handle or mattr transaction handle
 * \param[in]   mac_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_subnet_mac_set(bdmf_object_handle mo_, const bdmf_mac_t * mac_)
{
    return bdmf_attr_set_as_buf(mo_, rdpa_subnet_attr_mac, mac_, sizeof(*mac_));
}


/** Get subnet/ipv4 attribute.
 *
 * Get IPv4 address.
 * \param[in]   mo_ subnet object handle or mattr transaction handle
 * \param[out]  ipv4_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_subnet_ipv4_get(bdmf_object_handle mo_, bdmf_ipv4 *ipv4_)
{
    bdmf_number _nn_;
    int _rc_;
    _rc_ = bdmf_attr_get_as_num(mo_, rdpa_subnet_attr_ipv4, &_nn_);
    *ipv4_ = (bdmf_ipv4)_nn_;
    return _rc_;
}


/** Set subnet/ipv4 attribute.
 *
 * Set IPv4 address.
 * \param[in]   mo_ subnet object handle or mattr transaction handle
 * \param[in]   ipv4_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_subnet_ipv4_set(bdmf_object_handle mo_, bdmf_ipv4 ipv4_)
{
    return bdmf_attr_set_as_num(mo_, rdpa_subnet_attr_ipv4, ipv4_);
}


/** Get subnet/ipv6 attribute.
 *
 * Get IPv6 address.
 * \param[in]   mo_ subnet object handle or mattr transaction handle
 * \param[out]  ipv6_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_subnet_ipv6_get(bdmf_object_handle mo_, bdmf_ipv6_t * ipv6_)
{
    return bdmf_attr_get_as_buf(mo_, rdpa_subnet_attr_ipv6, ipv6_, sizeof(*ipv6_));
}


/** Set subnet/ipv6 attribute.
 *
 * Set IPv6 address.
 * \param[in]   mo_ subnet object handle or mattr transaction handle
 * \param[in]   ipv6_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_subnet_ipv6_set(bdmf_object_handle mo_, const bdmf_ipv6_t * ipv6_)
{
    return bdmf_attr_set_as_buf(mo_, rdpa_subnet_attr_ipv6, ipv6_, sizeof(*ipv6_));
}

/** @} end of subnet Doxygen group */


/** \addtogroup fw_rule
 * @{
 */


/** Get fw_rule type handle.
 *
 * This handle should be passed to bdmf_new_and_set() function in
 * order to create a fw_rule object.
 * \return fw_rule type handle
 */
bdmf_type_handle rdpa_fw_rule_drv(void);

/* fw_rule: Attribute types */
typedef enum {
    rdpa_fw_rule_attr_index = 0, /* index : KRI : number/4 : Rule index */
    rdpa_fw_rule_attr_rule = 1, /* rule : MRW : aggregate/56 fw_rule(rdpa_ip_fw_rule_t) : Rule configuration */
} rdpa_fw_rule_attr_types;

extern int (*f_rdpa_fw_rule_get)(bdmf_number index_, bdmf_object_handle *pmo);

/** Get fw_rule object by key.

 * This function returns fw_rule object instance by key.
 * \param[in] index_    Object key
 * \param[out] fw_rule_obj    Object handle
 * \return    0=OK or error <0
 */
int rdpa_fw_rule_get(bdmf_number index_, bdmf_object_handle *fw_rule_obj);

/** Get fw_rule/index attribute.
 *
 * Get Rule index.
 * \param[in]   mo_ fw_rule object handle or mattr transaction handle
 * \param[out]  index_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task and softirq contexts.
 */
static inline int rdpa_fw_rule_index_get(bdmf_object_handle mo_, bdmf_number *index_)
{
    bdmf_number _nn_;
    int _rc_;
    _rc_ = bdmf_attr_get_as_num(mo_, rdpa_fw_rule_attr_index, &_nn_);
    *index_ = (bdmf_number)_nn_;
    return _rc_;
}


/** Set fw_rule/index attribute.
 *
 * Set Rule index.
 * \param[in]   mo_ fw_rule object handle or mattr transaction handle
 * \param[in]   index_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task and softirq contexts.
 */
static inline int rdpa_fw_rule_index_set(bdmf_object_handle mo_, bdmf_number index_)
{
    return bdmf_attr_set_as_num(mo_, rdpa_fw_rule_attr_index, index_);
}


/** Get fw_rule/rule attribute.
 *
 * Get Rule configuration.
 * \param[in]   mo_ fw_rule object handle or mattr transaction handle
 * \param[out]  rule_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_fw_rule_rule_get(bdmf_object_handle mo_, rdpa_ip_fw_rule_t * rule_)
{
    return bdmf_attr_get_as_buf(mo_, rdpa_fw_rule_attr_rule, rule_, sizeof(*rule_));
}


/** Set fw_rule/rule attribute.
 *
 * Set Rule configuration.
 * \param[in]   mo_ fw_rule object handle or mattr transaction handle
 * \param[in]   rule_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_fw_rule_rule_set(bdmf_object_handle mo_, const rdpa_ip_fw_rule_t * rule_)
{
    return bdmf_attr_set_as_buf(mo_, rdpa_fw_rule_attr_rule, rule_, sizeof(*rule_));
}

/** @} end of fw_rule Doxygen group */




#endif /* _RDPA_AG_ROUTER_H_ */
