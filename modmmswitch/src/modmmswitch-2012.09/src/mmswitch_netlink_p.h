/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/** \file
 * Multimedia switch generic netlink private API.
 *
 * \version v1.0
 *
 *************************************************************************/
#ifndef __MMSWITCH_NETLINK_P
#define __MMSWITCH_NETLINK_P

/*########################################################################
#                                                                       #
#  HEADER (INCLUDE) SECTION                                             #
#                                                                       #
########################################################################*/
#include "mmswitch_netlink.h"
#include "mmcommon.h"
#include "mmconntypes.h"
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
MmPbxError mmSwitchGeNlSetTraceLevel(MmPbxTraceLevel level);
MmPbxError mmSwitchGeNlInit(void);
MmPbxError mmSwitchGeNlDeinit(void);
MmPbxError mmSwitchMmConnUsrSendConstructReply(MmConnUsrHndl  mmConnUsr,
                                               u32            geNlSeq,
                                               u32            geNlPid);
MmPbxError mmSwitchMmConnUsrSendDestoyGenlFamReply(MmConnUsrHndl  mmConnUsr,
                                                   u32            geNlSeq,
                                                   u32            geNlPid);
struct workqueue_struct *mmSwitchGetWq(void);
#endif /* __MMSWITCH_NETLINK_P */
