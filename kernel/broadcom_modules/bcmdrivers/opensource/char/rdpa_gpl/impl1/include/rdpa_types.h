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


#ifndef _RDPA_TYPES_H_
#define _RDPA_TYPES_H_

/** \defgroup types Commonly used types and constants
 * @{
 */

#include <bdmf_data_types.h>

/** Traffic direction */
typedef enum
{
    rdpa_dir_ds,        /**< Downstream */
    rdpa_dir_us         /**< Upstream */
} rdpa_traffic_dir;

/** PPPoE header */
typedef struct
{
    uint32_t session;   /**< PPPoE session ID */
} rdpa_pppoe_t;

/** PBIT */
typedef uint8_t rdpa_pbit;

/** DSCP */
typedef uint8_t rdpa_dscp;

/** Priority */
typedef uint8_t rdpa_prty;

/** TCONT index */
typedef uint8_t rdpa_tcont;

/** IP subnet */
typedef enum
{
    rdpa_subnet_none,   /**< Not set */
    rdpa_subnet_wan0,   /**< WAN0 */
    rdpa_subnet_wan1,   /**< WAN1 */
    rdpa_subnet_wan2,   /**< WAN2 */
    rdpa_subnet_wan3,   /**< WAN3 */
    rdpa_subnet_wan4,   /**< WAN4 */
    rdpa_subnet_wan5,   /**< WAN5 */
    rdpa_subnet_wan6,   /**< WAN6 */
    rdpa_subnet_wan7,   /**< WAN7 */

    rdpa_subnet_bridge, /**< L2 bridge */
    rdpa_subnet_iptv,   /**< IPTV */

    rdpa_subnet_lan0,   /**< LAN4 */
    rdpa_subnet_lan1,   /**< LAN1 */
    rdpa_subnet_lan2,   /**< LAN2 */
    rdpa_subnet_lan3,   /**< LAN3 */

    rdpa_subnet__num_of,    /**< Number of constants */
} rdpa_subnet;

/** Dual Stack lite tunnel ID */
typedef enum
{
    rdpa_ds_lite_tunnel_0, 
    rdpa_ds_lite_tunnel_1, 
    rdpa_ds_lite_tunnel_2,
    rdpa_ds_lite_tunnel_3,
    rdpa_ds_lite_max_tunnel_id = rdpa_ds_lite_tunnel_3
} rdpa_ds_lite_tunnel_id;

/** GEM flow ID (internal index) */
typedef uint16_t rdpa_gem;

/** Ethernet/GEM flow ID */
typedef uint16_t rdpa_flow;

/** UNASSIGNED value */
#define RDPA_VALUE_UNASSIGNED ((unsigned)-1)
#define RDPA_VALUE_UNMATCHED RDPA_VALUE_UNASSIGNED

/** ANY value */
#define RDPA_VALUE_ANY ((unsigned)-2)

/** VID/PBIT */
#define RDPA_VID_UNTAGGED ((unsigned)-3)
#define RDPA_MAX_VID 4095
#define RDPA_MAX_PBIT 7

/** Layer-4 PROTOCOL definitions */
#define RDPA_INVALID_PROTOCOL (0xFF)

/** Forwarding action */
typedef enum
{
    rdpa_forward_action_none    = 0,    /** ACL */
    rdpa_forward_action_forward = 1,    /**< Forward */
    rdpa_forward_action_host    = 2,    /**< Trap to the host */
    rdpa_forward_action_drop    = 4,    /**< Discard */
    rdpa_forward_action_flood   = 8     /**< Flood, for DA lookup only */
} rdpa_forward_action;

/** Filtering action */
typedef enum {
    rdpa_filter_action_allow,   /**< Allow through */
    rdpa_filter_action_deny     /**< Block packet */
} rdpa_filter_action;

/** QoS mapping method */
typedef enum
{
    rdpa_qos_method_flow,        /**< Flow-based QoS mapping */
    rdpa_qos_method_pbit       /**< Pbit-based QoS mapping */
} rdpa_qos_method;

/** Allow frame types */
typedef enum
{
    rdpa_port_allow_any,        /**< Allow tagged and untagged frames */
    rdpa_port_allow_tagged,     /**< Allow tagged frames only */
    rdpa_port_allow_untagged,   /**< Allow untagged frames only */
} rdpa_port_frame_allow;

/** Forwarding mode */
typedef enum
{
    rdpa_forwarding_mode_pkt,   /**< Packet-based forwarding */
    rdpa_forwarding_mode_flow,  /**< Flow-based forwarding */
} rdpa_forwarding_mode;

/** DS Ethernet flow classification mode */
typedef enum
{
    rdpa_classify_mode_pkt,     /**< Packet-based classification */
    rdpa_classify_mode_flow,    /**< Flow-based classification */
} rdpa_classify_mode;

/** Discard priority */
typedef enum
{
    rdpa_discard_prty_low,      /**< Low priority */
    rdpa_discard_prty_high      /**< High priority */
} rdpa_discard_prty;

/** Flow destination */
typedef enum
{
    rdpa_flow_dest_none,        /**< Not set */

    rdpa_flow_dest_iptv,        /**< IPTV */
    rdpa_flow_dest_eth,         /**< Flow */
    rdpa_flow_dest_omci,        /**< OMCI */

    rdpa_flow_dest__num_of,     /**< Number of values in rdpa_flow_destination enum */
} rdpa_flow_destination;

/** WAN technology */
typedef enum
{
    rdpa_wan_none,              /**< Not configured */

    rdpa_wan_gpon,              /**< GPON */
    rdpa_wan_epon,              /**< EPON */
    rdpa_wan_gbe,               /**< GbE */
    rdpa_wan_dsl,               /**< xDSL */

    rdpa_wan_type__num_of
} rdpa_wan_type;

/** Simple statistics */
typedef struct
{
    uint32_t packets;           /**< Packets */
    uint32_t bytes;             /**< Bytes */
} rdpa_stat_t;

/** Generic 1-way statistics */
typedef struct
{
    rdpa_stat_t passed;         /**< Passed statistics */
    rdpa_stat_t discarded;      /**< Discarded statistics */
} rdpa_stat_1way_t;

/** Tx+Rx statistics */
typedef struct
{
    rdpa_stat_1way_t tx;        /**< Transmit statistics */
    rdpa_stat_1way_t rx;        /**< Receive statistics */
} rdpa_stat_tx_rx_t;

/** RDPA interface (port).
 * The enum includes physical and virtual ports that
 * can appear in bridging / routing rules as ingress or egress interface.
 * The port list does not include VLAN-based virtual interfaces and tunnels.
 * Aggregation ports, such as PCI and SWITCH, can be used in 2-level scheduling.
 */
typedef enum
{
    rdpa_if_first,          /*< First interface */

    /** WAN ports */
    rdpa_if_wan0 = rdpa_if_first, /**< WAN0 port */
    rdpa_if_wan1,           /*< WAN1 port */

    /** LAN ports */
    rdpa_if_lan0,           /**< LAN0 port */
    rdpa_if_lan1,           /**< LAN1 port */
    rdpa_if_lan2,           /**< LAN2 port */
    rdpa_if_lan3,           /**< LAN3 port */
    rdpa_if_lan4,           /**< LAN4 port */
    rdpa_if_lan5,           /**< LAN5 port */
    rdpa_if_lan6,           /**< LAN6 port */
    rdpa_if_lan7,           /**< LAN7 port */
    rdpa_if_lan8,           /**< LAN8 port */
    rdpa_if_lan9,           /**< LAN9 port */
    rdpa_if_lan10,           /**< LAN10 port */
    rdpa_if_lan11,           /**< LAN11 port */
    rdpa_if_lan12,           /**< LAN12 port */
    rdpa_if_lan13,           /**< LAN13 port */
    rdpa_if_lan14,           /**< LAN14 port */
    rdpa_if_lan15,           /**< LAN15 port */
    rdpa_if_lan16,           /**< LAN16 port */
    rdpa_if_lan17,           /**< LAN17 port */
    rdpa_if_lan18,           /**< LAN18 port */
    rdpa_if_lan19,           /**< LAN19 port */
    rdpa_if_lan20,           /**< LAN20 port */
    rdpa_if_lan21,           /**< LAN21 port */
#ifdef G9991
    rdpa_if_lan22,           /**< LAN22 port */
    rdpa_if_lan23,           /**< LAN23 port */
    rdpa_if_lan24,           /**< LAN24 port */
    rdpa_if_lan25,           /**< LAN25 port */
    rdpa_if_lan26,           /**< LAN26 port */
    rdpa_if_lan27,           /**< LAN27 port */
    rdpa_if_lan28,           /**< LAN28 port */
    rdpa_if_lan29,           /**< LAN29 port */
    rdpa_if_lan_max = rdpa_if_lan29,
#else
    rdpa_if_lan_max = rdpa_if_lan21,
#endif

    /** Special ports */
    rdpa_if_lag0,           /**< Physical emac0 port */
    rdpa_if_lag1,           /**< Physical emac1 port */
    rdpa_if_lag2,           /**< Physical emac2 port */
    rdpa_if_lag3,           /**< Physical emac3 port */
    rdpa_if_lag4,           /**< Physical emac4 port */
    rdpa_if_lag_max = rdpa_if_lag4,

    /** Switch aggregate port */
    rdpa_if_switch,         /**< LAN switch port */

    /** Wi-Fi physical ports */
    rdpa_if_wlan0,          /**< WLAN0 port */
    rdpa_if_wlan1,          /**< WLAN1 port */

    /** CPU (local termination) */
    rdpa_if_cpu,                /**< CPU port (local termination) */

    rdpa_if_max_mcast_port = rdpa_if_cpu, /* only above ports could be part of mcast egress port mask */

    /** Wi-Fi logical ports (SSIDs) */
    rdpa_if_ssid0,          /**< Wi-Fi: SSID0 */
    rdpa_if_ssid1,          /**< Wi-Fi: SSID1 */
    rdpa_if_ssid2,          /**< Wi-Fi: SSID2 */
    rdpa_if_ssid3,          /**< Wi-Fi: SSID3 */
    rdpa_if_ssid4,          /**< Wi-Fi: SSID4 */
    rdpa_if_ssid5,          /**< Wi-Fi: SSID5 */
    rdpa_if_ssid6,          /**< Wi-Fi: SSID6 */
    rdpa_if_ssid7,          /**< Wi-Fi: SSID7 */
    rdpa_if_ssid8,          /**< Wi-Fi: SSID8 */
    rdpa_if_ssid9,          /**< Wi-Fi: SSID9 */
    rdpa_if_ssid10,         /**< Wi-Fi: SSID10 */
    rdpa_if_ssid11,         /**< Wi-Fi: SSID11 */
    rdpa_if_ssid12,         /**< Wi-Fi: SSID12 */
    rdpa_if_ssid13,         /**< Wi-Fi: SSID13 */
    rdpa_if_ssid14,         /**< Wi-Fi: SSID14 */
    rdpa_if_ssid15,         /**< Wi-Fi: SSID15 */

    rdpa_if__number_of,

    rdpa_if_none            /**< No port */

} rdpa_if;

/** EMAC id */
typedef enum
{
    rdpa_emac0,             /**< EMAC0 */
    rdpa_emac1,             /**< EMAC1 */
    rdpa_emac2,             /**< EMAC2 */
    rdpa_emac3,             /**< EMAC3 */
    rdpa_emac4,             /**< EMAC4 */
    rdpa_emac5,             /**< EMAC5 */
#ifdef FUTURE_USE
    rdpa_emac6,             /**< EMAC6 */
    rdpa_emac7,             /**< EMAC7 */
#endif /* FUTURE_USE */
    rdpa_emac__num_of,      /**< Max number of EMACs */
    rdpa_emac_none,      /**< None */
} rdpa_emac;

/** EMAC mode */
typedef enum
{
    rdpa_emac_mode_sgmii,   /**< SGMII */
    rdpa_emac_mode_hisgmii, /**< HISGMII */
    rdpa_emac_mode_qsgmii,  /**< QSGMII */
    rdpa_emac_mode_ss_smii, /**< SS SMII */
    rdpa_emac_mode_rgmii,   /**< RGMII */
    rdpa_emac_mode_mii,     /**< MII */
    rdpa_emac_mode_tmii,    /**< TMII */

    rdpa_emac_mode__num_of, /**< Number of EMAC modes */
} rdpa_emac_mode;


/** EMAC rates */
typedef enum
{
    rdpa_emac_rate_10m,     /**< 10 Mbps */
    rdpa_emac_rate_100m,    /**< 100 Mbps */
    rdpa_emac_rate_1g,      /**< 1 Gbps */

    rdpa_emac_rate__num_of, /**< Number of rates */
} rdpa_emac_rate;

/** EMAC configuration */
typedef struct
{
    char loopback;      /**< 1 = line loopback */
    rdpa_emac_rate rate;       /**< EMAC rate */
    char generate_crc;  /**< 1 = generate CRC */
    char full_duplex;   /**< 1 = full duplex */
    char pad_short;     /**< 1 = pad short frames */
    char allow_too_long;/**< 1 = allow long frames */
    char check_length;  /**< 1 = check frame length */
    uint32_t preamble_length;   /**< Preamble length */
    uint32_t back2back_gap;     /**< Back2Back inter-packet gap */
    uint32_t non_back2back_gap; /**< Non Back2Back inter-packet gap */
    uint32_t min_interframe_gap; /**< Min inter-frame gap */
    char rx_flow_control;/**< 1 = enable RX flow control */
    char tx_flow_control;/**< 1 = enable TX flow control */
} rdpa_emac_cfg_t;

/** RX RMON counters.
 * Underlying type for emac_rx_stat aggregate type.
 */
typedef struct
{
    uint32_t byte;              /**< Receive Byte Counter */
    uint32_t packet;            /**< Receive Packet Counter */
    uint32_t frame_64;          /**< Receive 64 Byte Frame Counter */
    uint32_t frame_65_127;      /**< Receive 65 to 127 Byte Frame Counter */
    uint32_t frame_128_255;     /**< Receive 128 to 255 Byte Frame Counter */
    uint32_t frame_256_511;     /**< Receive 256 to 511 Byte Frame Counter */
    uint32_t frame_512_1023;    /**< Receive 512 to 1023 Byte Frame Counter */
    uint32_t frame_1024_1518;   /**< Receive 1024 to 1518 Byte Frame Counter */
    uint32_t frame_1519_mtu;    /**< Receive 1519 to MTU Frame Counter */
    uint32_t multicast_packet;  /**< Receive Multicast Packet */
    uint32_t broadcast_packet;  /**< Receive Broadcast Packet */
    uint32_t unicast_packet;    /**< Receive Unicast Packet */
    uint32_t alignment_error;   /**< Receive Alignment error */
    uint32_t frame_length_error;/**< Receive Frame Length Error Counter */
    uint32_t code_error;        /**< Receive Code Error Counter */
    uint32_t carrier_sense_error;/**< Receive Carrier sense error */
    uint32_t fcs_error;         /**< Receive FCS Error Counter */
    uint32_t control_frame;     /**< Receive Control Frame Counter */
    uint32_t pause_control_frame;/**< Receive Pause Control Frame */
    uint32_t unknown_opcode;    /**< Receive Unknown opcode */
    uint32_t undersize_packet;  /**< Receive Undersize Packet */
    uint32_t oversize_packet;   /**< Receive Oversize Packet */
    uint32_t fragments;         /**< Receive Fragments */
    uint32_t jabber;            /**< Receive Jabber counter */
    uint32_t overflow;          /**< Receive Overflow counter */
} rdpa_emac_rx_stat_t;

/** Tx RMON counters.
 * Underlying type for emac_tx_stat aggregate type.
 */
typedef struct
{
    uint32_t byte;              /**< Transmit Byte Counter */
    uint32_t packet;            /**< Transmit Packet Counter */
    uint32_t frame_64;          /**< Transmit 64 Byte Frame Counter */
    uint32_t frame_65_127;      /**< Transmit 65 to 127 Byte Frame Counter */
    uint32_t frame_128_255;     /**< Transmit 128 to 255 Byte Frame Counter */
    uint32_t frame_256_511;     /**< Transmit 256 to 511 Byte Frame Counter */
    uint32_t frame_512_1023;    /**< Transmit 512 to 1023 Byte Frame Counter */
    uint32_t frame_1024_1518;   /**< Transmit 1024 to 1518 Byte Frame Counter */
    uint32_t frame_1519_mtu;    /**< Transmit 1519 to MTU Frame Counter */
    uint32_t fcs_error;         /**< Transmit FCS Error */
    uint32_t multicast_packet;  /**< Transmit Multicast Packet */
    uint32_t broadcast_packet;  /**< Transmit Broadcast Packet */
    uint32_t unicast_packet;    /**< Transmit Unicast Packet */
    uint32_t excessive_collision; /**< Transmit Excessive collision counter */
    uint32_t late_collision;    /**< Transmit Late collision counter */
    uint32_t single_collision;  /**< Transmit Single collision frame counter */
    uint32_t multiple_collision;/**< Transmit Multiple collision frame counter */
    uint32_t total_collision;   /**< Transmit Total Collision Counter */
    uint32_t pause_control_frame; /**< Transmit PAUSE Control Frame */
    uint32_t deferral_packet;   /**< Transmit Deferral Packet */
    uint32_t excessive_deferral_packet; /**< Transmit Excessive Deferral Packet */
    uint32_t jabber_frame;      /**< Transmit Jabber Frame */
    uint32_t control_frame;     /**< Transmit Control Frame */
    uint32_t oversize_frame;    /**< Transmit Oversize Frame counter */
    uint32_t undersize_frame;   /**< Transmit Undersize Frame */
    uint32_t fragments_frame;   /**< Transmit Fragments Frame counter */
    uint32_t error;             /**< Transmission errors*/
    uint32_t underrun;          /**< Transmission underrun */
} rdpa_emac_tx_stat_t;

/** Emac statistics */
typedef struct
{
    rdpa_emac_rx_stat_t rx; /**< Emac Receive Statistics */
    rdpa_emac_tx_stat_t tx; /**< Emac Transmit Statistics */
} rdpa_emac_stat_t;

/** RDPA EMAC mask
 * A combination of \ref rdpa_emac_id constants.
 */
typedef unsigned int rdpa_emacs;

/** RDPA emac mask.
 * \param[in] __emac EMAC
 * \return EMAC - Mask representation
 */
static inline rdpa_emacs rdpa_emac_id(rdpa_emac __emac)
{
    return 1 << (__emac);
}

/** RDPA port mask
 * A combination of \ref rdpa_if_id constants.
 */
typedef unsigned long long rdpa_ports;

/** RDPA interface (port) mask.
 * Can be combined in \ref rdpa_ports mask to specify multiple ports in the same operation.
 * \param[in] __if Interface
 * \return Interface - Mask representation
 */
static inline rdpa_ports rdpa_if_id(rdpa_if __if)
{
    return 1LL << (__if);
}

/** All WAN ports */
#define RDPA_PORT_ALL_WAN   (rdpa_if_id(rdpa_if_wan0) | rdpa_if_id(rdpa_if_wan1))

/** Check if interface is WAN interface
 * \param[in]   __if     Interface
 * \return 1 WAN, 0 otherwise
 */
static inline int rdpa_if_is_wan(rdpa_if __if)
{
    return (RDPA_PORT_ALL_WAN & rdpa_if_id(__if)) ? 1 : 0;
}

/** All LAN MACs */
#define RDPA_PORT_ALL_IH_LOOKUP_PORTS \
    (rdpa_if_id(rdpa_if_wan0) | rdpa_if_id(rdpa_if_lan0) | rdpa_if_id(rdpa_if_lan1) | rdpa_if_id(rdpa_if_lan2) | \
        rdpa_if_id(rdpa_if_lan3) | rdpa_if_id(rdpa_if_lan4))

/** All LAN MACs */
#ifdef G9991
#define RDPA_PORT_ALL_LAN_MACS \
    (rdpa_if_id(rdpa_if_lan0) | rdpa_if_id(rdpa_if_lan1) | rdpa_if_id(rdpa_if_lan2) | \
     rdpa_if_id(rdpa_if_lan3) | rdpa_if_id(rdpa_if_lan4) | rdpa_if_id(rdpa_if_lan5) | \
     rdpa_if_id(rdpa_if_lan6) | rdpa_if_id(rdpa_if_lan7) | rdpa_if_id(rdpa_if_lan8) | \
     rdpa_if_id(rdpa_if_lan9) | rdpa_if_id(rdpa_if_lan10) | rdpa_if_id(rdpa_if_lan11) | \
     rdpa_if_id(rdpa_if_lan12) | rdpa_if_id(rdpa_if_lan13) | rdpa_if_id(rdpa_if_lan14) | \
     rdpa_if_id(rdpa_if_lan15) | rdpa_if_id(rdpa_if_lan16) | rdpa_if_id(rdpa_if_lan17) | \
     rdpa_if_id(rdpa_if_lan18) | rdpa_if_id(rdpa_if_lan19) | rdpa_if_id(rdpa_if_lan20) | \
     rdpa_if_id(rdpa_if_lan21) | rdpa_if_id(rdpa_if_lan22) | rdpa_if_id(rdpa_if_lan23) | \
     rdpa_if_id(rdpa_if_lan24) | rdpa_if_id(rdpa_if_lan25) | rdpa_if_id(rdpa_if_lan26) | \
     rdpa_if_id(rdpa_if_lan27) | rdpa_if_id(rdpa_if_lan28) | rdpa_if_id(rdpa_if_lan29))
#else
#define RDPA_PORT_ALL_LAN_MACS \
    (rdpa_if_id(rdpa_if_lan0) | rdpa_if_id(rdpa_if_lan1) | rdpa_if_id(rdpa_if_lan2) | \
     rdpa_if_id(rdpa_if_lan3) | rdpa_if_id(rdpa_if_lan4) | rdpa_if_id(rdpa_if_lan5) | \
     rdpa_if_id(rdpa_if_lan6) | rdpa_if_id(rdpa_if_lan7) | rdpa_if_id(rdpa_if_lan8) | \
     rdpa_if_id(rdpa_if_lan9) | rdpa_if_id(rdpa_if_lan10) | rdpa_if_id(rdpa_if_lan11) | \
     rdpa_if_id(rdpa_if_lan12) | rdpa_if_id(rdpa_if_lan13) | rdpa_if_id(rdpa_if_lan14) | \
     rdpa_if_id(rdpa_if_lan15) | rdpa_if_id(rdpa_if_lan16) | rdpa_if_id(rdpa_if_lan17) | \
     rdpa_if_id(rdpa_if_lan18) | rdpa_if_id(rdpa_if_lan19) | rdpa_if_id(rdpa_if_lan20) | \
     rdpa_if_id(rdpa_if_lan21))
#endif

/** All physical ports */
#define RDPA_PORT_LAG_AND_SWITCH_PORTS \
    (rdpa_if_id(rdpa_if_lag0) | rdpa_if_id(rdpa_if_lag1) | rdpa_if_id(rdpa_if_lag2) | \
        rdpa_if_id(rdpa_if_lag3) | rdpa_if_id(rdpa_if_lag4) | rdpa_if_id(rdpa_if_switch))

/** All EMACs */
#define RDPA_PORT_ALL_EMACS \
    (rdpa_emac_id(rdpa_emac0) | rdpa_emac_id(rdpa_emac1) | rdpa_emac_id(rdpa_emac2) | \
        rdpa_emac_id(rdpa_emac3) | rdpa_emac_id(rdpa_emac4) | rdpa_emac_id(rdpa_emac5))

/** All LAN ports */
#define RDPA_PORT_ALL_LAN (RDPA_PORT_ALL_LAN_MACS)
/** All LAN ports and physical */
#define RDPA_PORT_ALL_LAN_AND_LAG (RDPA_PORT_ALL_LAN_MACS | RDPA_PORT_LAG_AND_SWITCH_PORTS)

/** Check if interface is LAN interface (LAN EMAC port or LAN switch port, not including Wi-Fi ports)
 * \param[in]   __if     Interface
 * \return 1 LAN (port or switch), 0 otherwise
 */
static inline int rdpa_if_is_lan(rdpa_if __if)
{
    return (RDPA_PORT_ALL_LAN & rdpa_if_id(__if)) ? 1 : 0;
}

/** Check if interface is LAN interface (LAN EMAC port, not including WiFi ports)
 * \param[in]   __if     Interface
 * \return 1 LAN , 0 otherwise
 */
static inline int rdpa_if_is_lan_mac(rdpa_if __if)
{
    return (RDPA_PORT_ALL_LAN_MACS & rdpa_if_id(__if)) ? 1 : 0;
}

/** Check if interface is LAN interface (LAN EMAC port or physical port)
 * \param[in]   __if     Interface
 * \return 1 LAN (port or phisical), 0 otherwise
 */
static inline int rdpa_if_is_lan_lag_and_switch(rdpa_if __if)
{
    return (RDPA_PORT_ALL_LAN_AND_LAG & rdpa_if_id(__if)) ? 1 : 0;
}


/** Check if interface is LAG interface (physical port)
 * \param[in]   __if     Interface
 * \return 1 LAN (port or phisical), 0 otherwise
 */
static inline int rdpa_if_is_lag_and_switch(rdpa_if __if)
{
    return (RDPA_PORT_LAG_AND_SWITCH_PORTS & rdpa_if_id(__if)) ? 1 : 0;
}

/** All WiFi virtual interfaces */
#define RDPA_PORT_ALL_SSIDS \
    (rdpa_if_id(rdpa_if_ssid0) | rdpa_if_id(rdpa_if_ssid1) | rdpa_if_id(rdpa_if_ssid2) | rdpa_if_id(rdpa_if_ssid3) | \
        rdpa_if_id(rdpa_if_ssid4) | rdpa_if_id(rdpa_if_ssid5) | rdpa_if_id(rdpa_if_ssid6) | rdpa_if_id(rdpa_if_ssid7) | \
        rdpa_if_id(rdpa_if_ssid8) | rdpa_if_id(rdpa_if_ssid9) | rdpa_if_id(rdpa_if_ssid10) | rdpa_if_id(rdpa_if_ssid11) | \
        rdpa_if_id(rdpa_if_ssid12) | rdpa_if_id(rdpa_if_ssid13) | rdpa_if_id(rdpa_if_ssid14) | rdpa_if_id(rdpa_if_ssid15))

/** All WLAN ports */
#define RDPA_PORT_ALL_WLAN \
    (rdpa_if_id(rdpa_if_wlan0) | rdpa_if_id(rdpa_if_wlan1))

/** Check if interface is Wi-Fi SSID
 * \param[in]   __if     Interface
 * \return 1 Wi-Fi SSID, 0 otherwise
 */
static inline int rdpa_if_is_wifi(rdpa_if __if)
{
    return (RDPA_PORT_ALL_SSIDS & rdpa_if_id(__if)) ? 1 : 0;
}

/** Check if interface is WLAN (PCI port)
 * \param[in]   __if     Interface
 * \return 1 if WLAN port, 0 otherwise
 */
static inline int rdpa_if_is_wlan(rdpa_if __if)
{
    return (RDPA_PORT_ALL_WLAN & rdpa_if_id(__if)) ? 1 : 0;
}

/** Check if interface is either LAN interface (LAN EMAC port or LAN switch port) or Wi-Fi SSID
 * \param[in] __if Interface
 * \return 1 LAN interface or Wi-Fi SSID, 0 otherwise
 */
static inline int rdpa_if_is_lan_or_wifi(rdpa_if __if)
{
    return rdpa_if_is_lan(__if) || rdpa_if_is_wifi(__if);
}

/** Check if interface is either LAN interface (LAN EMAC port or LAN switch port) or WLAN (PCI port)
 * \param[in] __if Interface
 * \return 1 LAN interface or WLAN, 0 otherwise
 */
static inline int rdpa_if_is_lan_or_wlan(rdpa_if __if)
{
    return rdpa_if_is_lan(__if) || rdpa_if_is_wlan(__if);
}

/** All MACs */
#define RDPA_PORT_ALL_MACS (RDPA_PORT_ALL_LAN_MACS | RDPA_PORT_ALL_WAN)

/** Check if port mask contains single port
 * \param[in] ports Port Mask
 * \return 1 if mask contains a single port , 0 otherwise
 */
static inline int rdpa_port_is_single(rdpa_ports ports)
{
    return (ports & (ports - 1)) == 0;
}

/** Check if port mask contains wan0 port
 * \param[in] ports Port Mask
 * \return 1 if mask contains a wan0 port , 0 otherwise
 */
static inline int rdpa_ports_contains_wan0_if(rdpa_ports ports)
{
    return ports & rdpa_if_id(rdpa_if_wan0);
}

/** IP class object method */
typedef enum
{
    rdpa_method_none,         /**< flow cache is disabled */
    rdpa_method_fc,           /**< flow cache is used for all traffic */
    rdpa_method_mixed,        /**< flow cache usage is depends on system configuration */
} rdpa_ip_class_method;

/** IPTV entries lookup method */
typedef enum
{
    iptv_lookup_method_mac, /**< Perform IPTV entry lookup by MAC address (L2) */
    iptv_lookup_method_mac_vid, /**< Perform IPTV entry lookup by MAC address and VID (L2) */
    iptv_lookup_method_group_ip, /**< Perform IPTV entry lookup by Multicast Group IP address (IGMPv2/MLDv1) */
    iptv_lookup_method_group_ip_src_ip, /**< Perform IPTV entry lookup by Multicast Group IP and Source IP
                                             addresses (IGMPv3/MLDv2). Source IP address is optional. */
    iptv_lookup_method_group_ip_src_ip_vid /**< Perform IPTV entry lookup by Multicast Group IP and Source IP
                                                addresses and VID. Source IP address is optional. */
} rdpa_iptv_lookup_method;

/** EPON mode */
typedef enum
{
    rdpa_epon_none,            /**< not EPON mode */ 
    rdpa_epon_ctc,             /**< CTC OAM mode */
    rdpa_epon_dpoe,            /**< DPOE OAM mode */
    rdpa_epon_bcm              /**< BCM OAM mode */
} rdpa_epon_mode;

/** Packet offset type */
typedef enum
{
    RDPA_OFFSET_L2, /**< Offset of L2 header */
    RDPA_OFFSET_L3, /**< Offset of L3 header */
    RDPA_OFFSET_L4, /**< Offset of L4 header */
} rdpa_offset_t;

/** @} end of types Doxygen group */

#endif /* _RDPA_TYPES_H_ */
