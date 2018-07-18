/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/** \file
 * Multimedia connection generic netlink interface.
 *
 * \version v1.0
 *
 *************************************************************************/
#ifndef __MMCONN_NETLINK
#define __MMCONN_NETLINK

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
#define MMCONN_GENL_FAMILY            "MMCONN"        /**< Generic netlink family used for MMCONN subsystem. */
#define MMCONN_GENL_MCGRP_EV_NAME     "MMCONN_EVENT"  /**< Generic netlink multicast group for eventing MMCONN changes */
#define MMCONN_GENL_FAMILY_VERSION    1               /**< Version of Generic netlink family. */
/*########################################################################
#                                                                       #
#  TYPES                                                                #
#                                                                       #
########################################################################*/

/**
 * Generic Netlink commands
 */
enum {
    MMCONN_CMD_UNSPEC = 0,
    MMCONN_CMD_NOTIFY,      /**< Notification */
    __MMCONN_CMD_MAX,
};
#define MMCONN_CMD_MAX    (__MMCONN_CMD_MAX - 1) /**< Maximum number of commands */

/**
 * Generic Netlink command attributes
 */
enum {
    MMCONN_ATTR_UNSPEC = 0,
    MMCONN_ATTR_REF_MMCONN,           /**< Kernel space reference of MmConn */
    MMCONN_ATTR_EVENT,                /**< MmConnEvent */
    MMCONN_ATTR_EVENT_PARM,           /**< Optional parameter */
    __MMCONN_ATTR_MAX,
};
#define MMCONN_ATTR_MAX    (__MMCONN_ATTR_MAX - 1) /**< Maximum number of attributes */
#endif /*__MMCONN_NETLINK */
