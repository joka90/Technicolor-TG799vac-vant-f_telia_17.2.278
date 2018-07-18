/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/** \file
 * Multimedia switch generic netlink interface.
 *
 * \version v1.0
 *
 *************************************************************************/
#ifndef __MMSWITCH_NETLINK
#define __MMSWITCH_NETLINK

/*########################################################################
#                                                                       #
#  HEADER (INCLUDE) SECTION                                             #
#                                                                       #
########################################################################*/

/*########################################################################
#                                                                       #
#  MACROS/DEFINES                                                       #
#                                                                       #
########################################################################*/
#define MMSWITCH_GENL_FAMILY            "MMSWITCH"  /**< Generic netlink family used for configuration/management. */
#define MMSWITCH_GENL_FAMILY_VERSION    1           /**< Generic netlink family version. */

/*########################################################################
#                                                                       #
#  TYPES                                                                #
#                                                                       #
########################################################################*/

/**
 * Generic Netlink commands
 */
enum {
    MMSWITCH_CMD_UNSPEC = 0,                        /**< Unspecified. */
    MMSWITCH_CMD_MMCONN_XCONN,                      /**< Cross-connect. */
    MMSWITCH_CMD_MMCONN_DISC,                       /**< Disconnect. */
    MMSWITCH_CMD_MMCONN_DESTRUCT,                   /**< Destruct. */
    MMSWITCH_CMD_MMCONN_SET_TRACE_LEVEL,            /**< Set mmConn tracelevel */
    MMSWITCH_CMD_MMCONNUSR_CONSTRUCT,               /** Construct user connection */
    MMSWITCH_CMD_MMCONNUSR_SYNC,                    /** Synchronise */
    MMSWITCH_CMD_MMCONNUSR_DESTROY_GENL_FAM,        /**< Destroy dynamicalily created family. */
    MMSWITCH_CMD_MMCONNUSR_SET_TRACE_LEVEL,         /**< Set mmConnUsr tracelevel */
    MMSWITCH_CMD_MMCONNKRNL_CONSTRUCT,              /**< Construct kernel connection. */
    MMSWITCH_CMD_MMCONNKRNL_SET_TRACE_LEVEL,        /**< Set mmConnKrnl tracelevel. */
    MMSWITCH_CMD_MMCONNMULTICAST_CONSTRUCT,         /**< Construct multicast connection. */
    MMSWITCH_CMD_MMCONNMULTICAST_SET_TRACE_LEVEL,   /**< Set mmConnMulticast tracelevel. */
    MMSWITCH_CMD_MMCONNMULTICAST_ADD_SINK,          /**< Add mmConnMulticast sink. */
    MMSWITCH_CMD_MMCONNMULTICAST_REMOVE_SINK,       /**< Remove mmConnMulticast sink. */
    MMSWITCH_CMD_MMCONNTONE_CONSTRUCT,              /**< mmConnTone construct. */
    MMSWITCH_CMD_MMCONNTONE_SET_TRACE_LEVEL,        /**< Set mmConnTone tracelevel. */
    MMSWITCH_CMD_MMCONNTONE_SEND_PATTERN,           /**< Send tone pattern. */
    MMSWITCH_CMD_MMCONNRELAY_CONSTRUCT,             /**< mmConnRelay construct. */
    MMSWITCH_CMD_MMCONNRELAY_SET_TRACE_LEVEL,       /**< Set mmConnrelay tracelevel. */
    MMSWITCH_CMD_MMCONNRTCP_CONSTRUCT,              /**< mmConnRtcp construct */
    MMSWITCH_CMD_MMCONNRTCP_SET_TRACE_LEVEL,        /**< Set mmConnRtcp tracelevel. */
    MMSWITCH_CMD_MMCONNRTCP_MOD_PACKET_TYPE,        /**< Modify mmConnRtcp packet type */
    MMSWITCH_CMD_MMCONNRTCP_MOD_GEN_RTCP,           /**< mmConnRtcp: Enable/disable RTCP generation. */
    MMSWITCH_CMD_MMCONNRTCP_MOD_REMOTE_MEDIA_ADDR,  /**< Modify mmConnRtcp remote media address. */
    MMSWITCH_CMD_MMCONNRTCP_MOD_REMOTE_RTCP_ADDR,   /**< Modify mmConnRtcp remote RTCP address. */
    MMSWITCH_CMD_MMCONNRTCP_MOD_RTCPBANDWIDTH,      /**< Modify mmConnrtcp RTCP bandwidth */
    MMSWITCH_CMD_MMCONNRTCP_GET_STATS,              /**< Get mmConnRtcp statistics */
    MMSWITCH_CMD_MMCONNRTCP_RESET_STATS,            /**< Get mmConnRtcp statistics */
    __MMSWITCH_CMD_MAX,
};
#define MMSWITCH_CMD_MAX    (__MMSWITCH_CMD_MAX - 1) /**< Maximum number of mmswitch commands */

/**
 * Generic Netlink command attributes.
 */
enum {
    MMSWITCH_ATTR_UNSPEC = 0,                         /**< Unspecified attribute. */
    MMSWITCH_ATTR_MMCONN,                             /**< Shared info between user/kernel space MmConn. */
    MMSWITCH_ATTR_MMCONNUSR,                          /**< mmConnUsr attribute. */
    MMSWITCH_ATTR_MMCONNRTCP,                         /**< mmConnRtcp attribute. */
    MMSWITCH_ATTR_MMCONNKRNL_ENDPOINT_ID,             /**< Unique identifier of endpoint in kernel space. */
    MMSWITCH_ATTR_MMCONNTONE_ENDPOINT_ID,             /**< Unique identifier of endpoint in kernel space. */
    MMSWITCH_ATTR_MMCONNTONE_TYPE,                    /**< Tone type attribute. */
    MMSWITCH_ATTR_MMCONNTONE_PATTERNTABLE_SIZE,       /**< Pattern table size attribute */
    MMSWITCH_ATTR_MMCONNTONE_PATTERN,                 /**< Pattern attribute. */
    MMSWITCH_ATTR_MMPBX_TRACELEVEL,                   /**< Tracelevel attribute. */
    MMSWITCH_ATTR_ENCODINGNAME,                       /**< Encoding name as described in RFC3551 and http://www.iana.org/assignments/rtp-parameters. */
    MMSWITCH_ATTR_PACKETPERIOD,                       /**< Packet period. */
    MMSWITCH_ATTR_REF_MMCONN,                         /**< Kernel space reference of MmConn. */
    MMSWITCH_ATTR_LOCAL_SOCKFD,                       /**< Local socket file descriptor. */
    MMSWITCH_ATTR_LOCAL_RTCP_SOCKFD,                  /**< Local socket file descriptor. */
    MMSWITCH_ATTR_REMOTE_ADDR,                        /**< Remote socket address. */
    MMSWITCH_ATTR_REMOTE_RTCP_ADDR,                   /**< Remote socket address. */
    MMSWITCH_ATTR_PACKET_TYPE,                        /**< Packet type. */
    MMSWITCH_ATTR_TIMEOUT,                            /**< Media Timeout. */
    MMSWITCH_ATTR_MUTE_TIME,                          /**< Media Mute Time. */
    MMSWITCH_ATTR_RTCP_BANDWIDTH,                     /**< RTCP bandwidth (bytes/second). */
    MMSWITCH_ATTR_GEN_RTCP,                           /**< Do we need to generate RTCP ? (0== FALSE, 1== TRUE). */
    __MMSWITCH_ATTR_MAX,
};
#define MMSWITCH_ATTR_MAX    (__MMSWITCH_ATTR_MAX - 1) /**< Maximum number of mmswitch attributes. */

/**
 * Generic Netlink MMCONN nested attributes.
 */
enum {
    MMSWITCH_ATTR_MMCONN_UNSPEC = 0,
    MMSWITCH_ATTR_MMCONN_REF, /**< Kernel space identifier of mmConn attribute. */
    __MMSWITCH_ATTR_MMCONN_MAX,
};
#define MMSWITCH_ATTR_MMCONN_MAX    (__MMSWITCH_ATTR_MMCONN_MAX - 1)  /**< Maximum number of mmConn nested attributes. */


/**
 * Generic Netlink MMCONNUSR nested attributes
 */
enum {
    MMSWITCH_ATTR_MMCONNUSR_UNSPEC = 0,
    MMSWITCH_ATTR_MMCONNUSR_GENL_FAMID,   /**< Generic netlink family ID attribute. */
    MMSWITCH_ATTR_MMCONNUSR_GENL_FAMNAME, /**< Generic netlink family name. */
    __MMSWITCH_ATTR_MMCONNUSR_MAX,
};
#define MMSWITCH_ATTR_MMCONNUSR_MAX    (__MMSWITCH_ATTR_MMCONNUSR_MAX - 1) /**< Maximum number of mmConnUser nested attributes. */

/**
 * Generic Netlink MMCONNRTCP nested attributes.
 */
enum {
    MMSWITCH_ATTR_MMCONNRTCP_UNSPEC = 0,
    MMSWITCH_ATTR_MMCONNRTCP_CALLLENGTH,
    MMSWITCH_ATTR_MMCONNRTCP_TXRTPPACKETS,
    MMSWITCH_ATTR_MMCONNRTCP_TXRTPBYTES,
    MMSWITCH_ATTR_MMCONNRTCP_RXRTPPACKETS,
    MMSWITCH_ATTR_MMCONNRTCP_RXRTPBYTES,
    MMSWITCH_ATTR_MMCONNRTCP_RXTOTALPACKETSLOST,
    MMSWITCH_ATTR_MMCONNRTCP_RXTOTALPACKETSDISCARDED,
    MMSWITCH_ATTR_MMCONNRTCP_RXPACKETSLOSTRATE,
    MMSWITCH_ATTR_MMCONNRTCP_RXPACKETSDISCARDEDRATE,
    MMSWITCH_ATTR_MMCONNRTCP_SIGNALLEVEL,
    MMSWITCH_ATTR_MMCONNRTCP_NOISELEVEL,
    MMSWITCH_ATTR_MMCONNRTCP_RERL,
    MMSWITCH_ATTR_MMCONNRTCP_RFACTOR,
    MMSWITCH_ATTR_MMCONNRTCP_EXTERNALRFACTOR,
    MMSWITCH_ATTR_MMCONNRTCP_MOSLQ,
    MMSWITCH_ATTR_MMCONNRTCP_MOSCQ,
    MMSWITCH_ATTR_MMCONNRTCP_MEANE2EDELAY,
    MMSWITCH_ATTR_MMCONNRTCP_WORSTE2EDELAY,
    MMSWITCH_ATTR_MMCONNRTCP_CURRENTE2EDELAY,
    MMSWITCH_ATTR_MMCONNRTCP_RXJITTER,
    MMSWITCH_ATTR_MMCONNRTCP_RXMINJITTER,
    MMSWITCH_ATTR_MMCONNRTCP_RXMAXJITTER,
    MMSWITCH_ATTR_MMCONNRTCP_RXDEVJITTER,
    MMSWITCH_ATTR_MMCONNRTCP_RXMEANJITTER,
    MMSWITCH_ATTR_MMCONNRTCP_RXWORSTJITTER,
    MMSWITCH_ATTR_MMCONNRTCP_JITTER_BUFFER_OVERRUNS,
    MMSWITCH_ATTR_MMCONNRTCP_JITTER_BUFFER_UNDERRUNS,
    MMSWITCH_ATTR_MMCONNRTCP_REMOTE_TXRTPPACKETS,
    MMSWITCH_ATTR_MMCONNRTCP_REMOTE_TXRTPBYTES,
    MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RXTOTALPACKETSLOST,
    MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RXPACKETSLOSTRATE,
    MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RXPACKETSDISCARDEDRATE,
    MMSWITCH_ATTR_MMCONNRTCP_REMOTE_SIGNALLEVEL,
    MMSWITCH_ATTR_MMCONNRTCP_REMOTE_NOISELEVEL,
    MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RERL,
    MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RFACTOR,
    MMSWITCH_ATTR_MMCONNRTCP_REMOTE_EXTERNALRFACTOR,
    MMSWITCH_ATTR_MMCONNRTCP_REMOTE_MOSLQ,
    MMSWITCH_ATTR_MMCONNRTCP_REMOTE_MOSCQ,
    MMSWITCH_ATTR_MMCONNRTCP_REMOTE_MEANE2EDELAY,
    MMSWITCH_ATTR_MMCONNRTCP_REMOTE_WORSTE2EDELAY,
    MMSWITCH_ATTR_MMCONNRTCP_REMOTE_CURRENTE2EDELAY,
    MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RXJITTER,
    MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RXMINJITTER,
    MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RXMAXJITTER,
    MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RXDEVJITTER,
    MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RXMEANJITTER,
    MMSWITCH_ATTR_MMCONNRTCP_REMOTE_RXWORSTJITTER,
    /*AT&T extension*/
    MMSWITCH_ATTR_MMCONNRTCP_TXRTCPPACKETS,
    MMSWITCH_ATTR_MMCONNRTCP_RXRTCPPACKETS,
    MMSWITCH_ATTR_MMCONNRTCP_SUM_FRACTIONLOSS,
    MMSWITCH_ATTR_MMCONNRTCP_SUM_SQR_FRACTIONLOSS,
    MMSWITCH_ATTR_MMCONNRTCP_REMOTE_SUM_FRACTIONLOSS,
    MMSWITCH_ATTR_MMCONNRTCP_REMOTE_SUM_SQR_FRACTIONLOSS,
    MMSWITCH_ATTR_MMCONNRTCP_SUM_JITTER,
    MMSWITCH_ATTR_MMCONNRTCP_SUM_SQR_JITTER,
    MMSWITCH_ATTR_MMCONNRTCP_REMOTE_SUM_JITTER,
    MMSWITCH_ATTR_MMCONNRTCP_REMOTE_SUM_SQR_JITTER,
    MMSWITCH_ATTR_MMCONNRTCP_SUM_E2EDELAY,
    MMSWITCH_ATTR_MMCONNRTCP_SUM_SQR_E2EDELAY,
    MMSWITCH_ATTR_MMCONNRTCP_MAX_ONEWAYDELAY,
    MMSWITCH_ATTR_MMCONNRTCP_SUM_ONEWAYDELAY,
    MMSWITCH_ATTR_MMCONNRTCP_SUM_SQR_ONEWAYDELAY,
    MMSWITCH_ATTR_MMCONNRTCP_FIRST_RX_RTCP_LENGTH,
    MMSWITCH_ATTR_MMCONNRTCP_FIRST_RX_RTCP_BUFFER,
    MMSWITCH_ATTR_MMCONNRTCP_SECOND_RX_RTCP_LENGTH,
    MMSWITCH_ATTR_MMCONNRTCP_SECOND_RX_RTCP_BUFFER,
    MMSWITCH_ATTR_MMCONNRTCP_FIRST_TX_RTCP_LENGTH,
    MMSWITCH_ATTR_MMCONNRTCP_FIRST_TX_RTCP_BUFFER,
    MMSWITCH_ATTR_MMCONNRTCP_SECOND_TX_RTCP_LENGTH,
    MMSWITCH_ATTR_MMCONNRTCP_SECOND_TX_RTCP_BUFFER,
    __MMSWITCH_ATTR_MMCONNRTCP_MAX,
};
#define MMSWITCH_ATTR_MMCONNRTCP_MAX    (__MMSWITCH_ATTR_MMCONNRTCP_MAX - 1) /**< Maximum number of mmConnRtcp nested attributes. */

/**
 * Generic Netlink MMCONNTONE_PATTERN nested attributes.
 */
enum {
    MMSWITCH_ATTR_MMCONNTONE_UNSPEC = 0,
    MMSWITCH_ATTR_MMCONNTONE_PATTERN_ID,
    MMSWITCH_ATTR_MMCONNTONE_PATTERN_ON,
    MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ1,
    MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ2,
    MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ3,
    MMSWITCH_ATTR_MMCONNTONE_PATTERN_FREQ4,
    MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER1,
    MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER2,
    MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER3,
    MMSWITCH_ATTR_MMCONNTONE_PATTERN_POWER4,
    MMSWITCH_ATTR_MMCONNTONE_PATTERN_DURATION,
    MMSWITCH_ATTR_MMCONNTONE_PATTERN_NEXTID,
    MMSWITCH_ATTR_MMCONNTONE_PATTERN_MAXLOOPS,
    MMSWITCH_ATTR_MMCONNTONE_PATTERN_NEXTIDAFTERLOOPS,
    __MMSWITCH_ATTR_MMCONNTONE_PATTERN_MAX,
};
#define MMSWITCH_ATTR_MMCONNTONE_PATTERN_MAX    (__MMSWITCH_ATTR_MMCONNTONE_PATTERN_MAX - 1) /**< Maximum number of mmConnTone nested attributes. */
#endif /*__MMSWITCH_NETLINK */
