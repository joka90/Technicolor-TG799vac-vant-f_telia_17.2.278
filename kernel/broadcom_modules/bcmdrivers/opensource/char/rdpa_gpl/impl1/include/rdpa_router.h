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


#ifndef _RDPA_ROUTER_H_
#define _RDPA_ROUTER_H_


/** \defgroup router Router
 * APIs in this group are used for configuration of IPv4 / IPv6 router
 * - IP subnets
 * - IP protocol filters
 * - TCP/UDP Connections
 * - Firewall rules
 * @{
 */

/** \defgroup subnet IP Subnets
 * RDPA IP Sub-net roughly correspond to TR-98's "routing interface".
 * @{
 */

/** IP Subnet PPPoE configuration.
 * Structure underlying subnet_pppoe aggregate type
 */
typedef struct {
    bdmf_boolean enable;        /**< true=PPPoE tunnel */
    uint32_t session;           /**< PPPoE session id */
} rdpa_subnet_pppoe_cfg_t;

/** Subnet configuration.
 * This structure can be used for interface with RDD layer
 */
typedef struct {
    rdpa_subnet_pppoe_cfg_t pppoe;  /**< PPPoE configuration */
    bdmf_mac_t mac;             /**< subnet's MAC address. DA on ingress, SA on egress */
    bdmf_ipv4 ipv4;             /**< IPv4 address. subnet only lets IPv4 traffic true if ipv4 address is set */
    bdmf_ipv6_t ipv6;           /**< IPv6 address. subnet only lets IPv6 traffic true if ipv6 address is set */
} rdpa_subnet_cfg_t;

/** @} end of subnet Doxygen group */

/** \defgroup fw_rule IP Firewall Rules
 * Firewall rules identify packets that should be forwarded
 * to the host in case of connection lookup miss
 * @{
 */

#define RDPA_MAX_FW_RULE  256 /**< Max number of firewall rules ToDo: check */

#define RDPA_FW_RULE_ANY_IP_MATCH 0 /**< All source IPs are matched, i.e. source IP check is not needed */

#define RDPA_FW_RULE_SPECIFC_IP_MATCH 32 /**< Mask is disabled, i.e. all 32 bit of the source ip should be compared */



/** Firewall rule,
 * Underlying type for fw_rule aggregate type
 */
typedef struct {
    rdpa_subnet subnet;     /**< Ingress IP subnet */
    uint8_t protocol;       /**< IP protocol. Protocol value or 0-for all supported */
    uint16_t from_port;     /**< port range: from port */
    uint16_t to_port;       /**< port range: to port */
    bdmf_ip_t src_ip;       /**< optional: Source IP address */
    uint32_t src_ip_mask;   /**< optional: Source IP mask - number of 1s in mask from left , applicable for IPv4 only*/
    bdmf_ip_t dst_ip;       /**< optional: Destination IP address */
} rdpa_ip_fw_rule_t;

/** @} end of firewall Doxygen group */

/** @} end of router Doxygen group */

#endif /* _RDPA_ROUTER_H_ */
