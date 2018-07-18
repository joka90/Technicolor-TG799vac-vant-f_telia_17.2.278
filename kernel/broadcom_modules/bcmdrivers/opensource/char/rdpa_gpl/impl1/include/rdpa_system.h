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


#ifndef _RDPA_SYSTEM_H_
#define _RDPA_SYSTEM_H_

#include "bdmf_dev.h"
#include "rdpa_types.h"

/** \defgroup system System-level Configuration
 * System object is a root object of RDPA object hierarchy.
 * Therefore, system object must be created first, before any other
 * RDPA object.
 *
 * Once created, system object performs initialization and initial configuration
 * based on configuration profile and other object attributes.
 * @{
 */
/** RDPA sw version struct */
typedef struct
{
    uint8_t rdpa_version_major; /**< Major */
    uint8_t rdpa_version_middle; /**< Middle */
    uint8_t rdpa_version_service_pack; /**< Service Pack */
    uint8_t rdpa_version_minor; /**< Minor */
    uint8_t rdpa_version_patch; /**< Patch */
    uint32_t rdpa_version_rdd; /**< RDD */
    uint32_t rdpa_version_firmware; /**< Firmware */
} rdpa_sw_version_t;

/** RDPA system profile type.
 * \n IT: the following list is just an illustration.
 */
typedef enum
{
    rdpa_profile_none, /**< Not set */

    rdpa_profile_rgw, /**< RGW - NAT router with LAN bridge */
    rdpa_profile_sfu, /**< SFU bridge */
    rdpa_profile_mdu, /**< MDU bridge */

    rdpa_profile__num_of /**< Number of profiles in rdpa_profile list */
} rdpa_profile;

/** VLAN switching methods. */
typedef enum
{
    rdpa_vlan_aware_switching,      /**< VLAN aware switching*/
    rdpa_mac_based_switching,       /**< MAC based switching*/
    rdpa_switching_none,            /**< MAC based switching*/
} rdpa_vlan_switching;

/** External switch type configuration. */
typedef enum
{
    rdpa_brcm_hdr_opcode_0, /**< 4 bytes long */
    rdpa_brcm_hdr_opcode_1, /**< 8 bytes long */
    rdpa_brcm_fttdp,        /**< FTTdp */
    rdpa_brcm_none
} rdpa_ext_sw_type;

/** External switch configuration. */
typedef struct
{
    bdmf_boolean enabled;       /**< Toggle external switch */
    rdpa_emac emac_id;          /**< External switch EMAC ID */
    rdpa_ext_sw_type type;      /**< External switch port identification type */
} rdpa_runner_ext_sw_cfg_t;

#define RDPA_DP_MAX_TABLES        2  /*< One drop precedence table per direction.*/

/** Drop eligibility configuration parameters, combination of PBIT and DEI used to define packet drop eligibility. */
typedef struct
{
    rdpa_traffic_dir dir;   /**< Configure the traffic direction */
    rdpa_pbit pbit;         /**< PBIT value */
    uint8_t dei;            /**< Drop Eligible Indicator value */
} rdpa_dp_key_t;

/** RDPA initial system configuration.
 * This is underlying structure of system aggregate.
 */
typedef struct
{
    rdpa_wan_type wan_type; /**< WAN technology */
    /** Profile-specific configuration */
    uint32_t enabled_emac; /**< backward mode - enabled EMAC bitmask*/
    rdpa_emac gbe_wan_emac; /**< EMAC4/EMAC5 */
    rdpa_vlan_switching switching_mode; /**< VLAN switching working mode */
    rdpa_ip_class_method ip_class_method;  /**< Operational mode of the IP class object */
    rdpa_runner_ext_sw_cfg_t runner_ext_sw_cfg; /**<Runner configuration when external switch is connected */
    bdmf_boolean us_ddr_queue_enable; /**<WAN TX queue DDR offload enable */
} rdpa_system_init_cfg_t;

/** RDPA system configuration that can be changed in runtime.
 * This is the underlying structure of system aggregate.
 */
typedef struct
{
    bdmf_boolean car_mode; /**< Is CAR mode enabled/disabled */
    int headroom_size; /**< Min skb headroom size */
    int mtu_size; /**< MTU size */
    uint16_t inner_tpid; /**< Inner TPID (For single-tag VLAN action commands) */
    uint16_t outer_tpid; /**< Outer TPID (For double-tag VLAN action commands) */
    uint16_t add_always_tpid; /**< 'Add Always' TPID (For 'Add Always' VLAN action commands) */    
    bdmf_boolean ic_dbg_stats; /**< Enable Ingress class debug statistics */
    uint32_t options;	       /**< global reserved flag */
} rdpa_system_cfg_t;

/** US system-level drop statistics. */
typedef struct
{
    uint16_t eth_flow_action;                   /**< Flow action = drop */
    uint16_t sa_lookup_failure;                 /**< SA lookup failure */
    uint16_t da_lookup_failure;                 /**< DA lookup failure */
    uint16_t sa_action;                         /**< SA action == drop */
    uint16_t da_action;                         /**< DA action == drop */
    uint16_t forwarding_matrix_disabled;        /**< Disabled in forwarding matrix */
    uint16_t connection_action;                 /**< Connection action == drop */
    uint16_t parsing_exception;                 /**< Parsing exception */
    uint16_t parsing_error;                     /**< Parsing error */
    uint16_t local_switching_congestion;        /**< Local switching congestion */
    uint16_t vlan_switching;                    /**< VLAN switching */
    uint16_t tpid_detect;                       /**< Invalid TPID */
    uint16_t invalid_subnet_ip;                 /**< Invalid subnet */
    uint16_t acl_oui;                           /**< Dropped by OUI ACL */
    uint16_t acl;                               /**< Dropped by ACL */
} rdpa_system_us_stat_t;

/** DS system-level drop statistics */
typedef struct
{
    uint16_t eth_flow_action;                   /**< Flow action = drop */
    uint16_t sa_lookup_failure;                 /**< SA lookup failure */
    uint16_t da_lookup_failure;                 /**< DA lookup failure */
    uint16_t sa_action;                         /**< SA action == drop */
    uint16_t da_action;                         /**< DA action == drop */
    uint16_t forwarding_matrix_disabled;        /**< Disabled in forwarding matrix */
    uint16_t connection_action;                 /**< Connection action == drop */
    uint16_t policer;                           /**< Dropped by policer */
    uint16_t parsing_exception;                 /**< Parsing exception */
    uint16_t parsing_error;                     /**< Parsing error */
    uint16_t iptv_layer3;                       /**< IPTV L3 drop */
    uint16_t vlan_switching;                    /**< VLAN switching */
    uint16_t tpid_detect;                       /**< Invalid TPID */
    uint16_t dual_stack_lite_congestion;        /**< DSLite congestion */
    uint16_t invalid_subnet_ip;                 /**< Invalid subnet */
    uint16_t invalid_layer2_protocol;           /**< Invalid L2 protocol */
    uint16_t firewall;                          /**< Dropped by firewall */
    uint16_t dst_mac_non_router;                /**< DST MAC is not equal to the router's MAC */
} rdpa_system_ds_stat_t;

/** System-level drop statistics. */
typedef struct
{
    rdpa_system_us_stat_t us;                   /**< Upstream drop statistics */
    rdpa_system_ds_stat_t ds;                   /**< Downstream drop statistics */
} rdpa_system_stat_t;

/** Time Of Day. */
typedef struct {
    uint16_t sec_ms;    /**< Seconds, MS bits   */
    uint32_t sec_ls;    /**< Seconds, LS bits   */

    uint32_t nsec;      /**< Nanoseconds        */
} rdpa_system_tod_t;

/** TPID Detect.
 *  A set of TPID values, both pre- and user-defined,
 *  used to apply tagging classification rules on incoming
 *  traffic. Accordingly, every packet is classified as Single
 *  or Double tagged. */
typedef enum
{
    rdpa_tpid_detect_0x8100, /**< Pre-Defined, 0x8100 */
    rdpa_tpid_detect_0x88A8, /**< Pre-Defined, 0x88A8 */
    rdpa_tpid_detect_0x9100, /**< Pre-Defined, 0x9100 */
    rdpa_tpid_detect_0x9200, /**< Pre-Defined, 0x9200 */

    rdpa_tpid_detect_udef_1, /**< User-Defined, #1    */
    rdpa_tpid_detect_udef_2, /**< User-Defined, #2    */

    rdpa_tpid_detect__num_of
} rdpa_tpid_detect_t;

/** TPID Detect: Configuration. */
typedef struct
{
    uint16_t val_udef; /**< TPID Value, User-Defined */

    bdmf_boolean otag_en; /**< Outer tag, Enabled Detection flag */
    bdmf_boolean itag_en; /**< Inner tag, Enabled Detection flag */
    bdmf_boolean triple_en; /**< Triple tag (most inner tag), Enabled Detection flag */
} rdpa_tpid_detect_cfg_t;

/** @} end of system doxygen group */

/***************************************************************************
 * system object type
 **************************************************************************/

/* system object private data */
typedef struct
{
    rdpa_sw_version_t sw_version;
    rdpa_system_init_cfg_t init_cfg;
    rdpa_system_cfg_t cfg;
    uint16_t dp_bitmask[2];
    rdpa_tpid_detect_cfg_t tpids_detect[rdpa_tpid_detect__num_of];
} system_drv_priv_t;

extern struct bdmf_object *system_object;
static inline const rdpa_system_init_cfg_t *_rdpa_system_init_cfg_get(void) 
{
    system_drv_priv_t *system = (system_drv_priv_t *)bdmf_obj_data(system_object);

    return &system->init_cfg;
}

static inline const rdpa_system_cfg_t *_rdpa_system_cfg_get(void) 
{
    system_drv_priv_t *system = (system_drv_priv_t *)bdmf_obj_data(system_object);

    return &system->cfg;
}

#endif /* _RDPA_SYSTEM_H_ */
