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
#ifndef __MMCONNUSR_NETLINK
#define __MMCONNUSR_NETLINK

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

#define MMCONNUSR_GENL_FAMILY_VERSION    1 /**< Generic netlink family used for data transfer */

/*########################################################################
#                                                                       #
#  TYPES                                                                #
#                                                                       #
########################################################################*/

/**
 * Generic Netlink commands
 */
enum {
    MMCONNUSR_CMD_UNSPEC = 0, /**< Unspecified command. */
    MMCONNUSR_CMD_WRITE,      /**< Write command. */
    MMCONNUSR_CMD_SYNC,       /**< Synchronise command. */
    __MMCONNUSR_CMD_MAX,
};
#define MMCONNUSR_CMD_MAX    (__MMCONNUSR_CMD_MAX - 1) /**< Maximum number of commands */

/**
 * Generic Netlink command attributes
 */
enum {
    MMCONNUSR_ATTR_UNSPEC = 0,      /**< Unspecified attribute */
    MMCONNUSR_ATTR_PACKETHEADER,    /**< PacketHeader ,to identify packet */
    MMCONNUSR_ATTR_DATA,            /**< Data to write to user space / kernel space */
    MMCONNUSR_ATTR_REF_MMCONN,      /**< Kernel space reference of MmConn */
    __MMCONNUSR_ATTR_MAX,
};
#define MMCONNUSR_ATTR_MAX    (__MMCONNUSR_ATTR_MAX - 1) /**< Maximum number of attributes */

/* Generic Netlink PACKETHEADER attributes */
enum {
    MMCONNUSR_ATTR_PACKETHEADER_UNSPEC = 0,
    MMCONNUSR_ATTR_PACKETHEADER_TYPE,      /**< Packet type */
    __MMCONNUSR_ATTR_PACKETHEADER_MAX,
};
#define MMCONNUSR_ATTR_PACKETHEADER_MAX    (__MMCONNUSR_ATTR_PACKETHEADER_MAX - 1) /**< Maximum number of packetheader attributes */
#endif /*__MMCONNUSR_NETLINK */
