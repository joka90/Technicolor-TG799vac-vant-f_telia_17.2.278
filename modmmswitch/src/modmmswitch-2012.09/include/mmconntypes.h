/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/** \file
 * Multimedia switch connection types.
 *
 * \version v1.0
 *
 *************************************************************************/
#ifndef  MMCONN_TYPES_INC
#define  MMCONN_TYPES_INC

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


/*########################################################################
#                                                                       #
#  TYPES                                                                #
#                                                                       #
########################################################################*/

/**
 * Handle that represents a multimedia switch connection instance.
 *
 * All communication with a multimedia switch connection will happen using this handle.
 *
 * \since v1.0
 */
typedef struct MmConn           *MmConnHndl;

/**
 * Handle that represents a multimedia switch kernel connection instance.
 *
 * All communication with a multimedia switch kernel connection instance will happen using this handle.
 *
 * \since v1.0
 */
typedef struct MmConnKrnl       *MmConnKrnlHndl;

/**
 * Handle that represents a multicast connection instance.
 *
 * All communication with a multicast connection instance will happen using this handle.
 *
 * \since v1.0
 */
typedef struct MmConnMulticast  *MmConnMulticastHndl;

/**
 * Handle that represents a multimedia switch kernel connection instance.
 *
 * All communication with a multimedia switch kernel connection instance will happen using this handle.
 *
 * \since v1.0
 */
typedef struct MmConnTone       *MmConnToneHndl;

/**
 * Handle that represents a multimedia switch relay connection instance.
 *
 * All communication with a multimedia switch relay connection instance will happen using this handle.
 *
 * \since v1.0
 */
typedef struct MmConnRelay      *MmConnRelayHndl;

/**
 * Handle that represents a multimedia switch rtcp connection instance.
 *
 * All communication with a multimedia switch rtcp connection instance will happen using this handle.
 *
 * \since v1.0
 */
typedef struct MmConnRtcp       *MmConnRtcpHndl;

/**
 * Handle that represents a multimedia switch user connection instance.
 *
 * All communication with a multimedia switch user connection instance will happen using this handle.
 *
 * \since v1.0
 */
typedef struct MmConnUsr        *MmConnUsrHndl;
#endif   /* ----- #ifndef MMCONN_TYPES_INC  ----- */
