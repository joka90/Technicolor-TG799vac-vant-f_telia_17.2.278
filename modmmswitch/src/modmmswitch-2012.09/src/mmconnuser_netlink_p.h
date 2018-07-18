/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/** \file
 * Multimedia user connection generic netlink private API.
 *
 * \version v1.0
 *
 *************************************************************************/
#ifndef __MMCONNUSR_NETLINK_P
#define __MMCONNUSR_NETLINK_P

/*########################################################################
#                                                                       #
#  HEADER (INCLUDE) SECTION                                             #
#                                                                       #
########################################################################*/
#include <net/genetlink.h>
#include "mmconnuser_netlink.h"
#include "mmconn_p.h"
#include "mmcommon.h"
/*########################################################################
#                                                                       #
#  MACROS/DEFINES                                                       #
#                                                                       #
########################################################################*/


/*########################################################################
#                                                                       #
#  TYPES                                                                #
#                                                                       #
########################################################################*/

/*########################################################################
#                                                                       #
#  FUNCTION PROTOTYPES                                                  #
#                                                                       #
########################################################################*/
MmPbxError mmConnUsrGeNlSetTraceLevel(MmPbxTraceLevel level);
MmPbxError mmConnUsrGeNlInit(void);
MmPbxError mmConnUsrGeNlDeinit(void);
MmPbxError mmConnUsrGeNlCreateFam(MmConnUsrHndl     mmConnUsr,
                                  struct genl_info  *info);
MmPbxError mmConnUsrGeNlDestroyFam(MmConnUsrHndl    mmConnUsr,
                                   struct genl_info *info);
MmPbxError mmConnUsrGeNlWrite(MmConnUsrHndl       mmConnUsr,
                              MmConnPacketHeader  *header,
                              uint8_t             *buff,
                              unsigned int        bytes);
#endif /* __MMCONNUSR_NETLINK_P */
