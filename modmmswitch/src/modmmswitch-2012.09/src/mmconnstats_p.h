/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2015-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/** \file
 *      Stats private data structure / function declarations
 *
 * \version v1.0
 *
 *************************************************************************/
#ifndef  MMCONNSTATS_P_H
#define  MMCONNSTATS_P_H

/*########################################################################
#                                                                       #
#  HEADER (INCLUDE) SECTION                                             #
#                                                                       #
########################################################################*/
#include "mmconnbrcmstats_p.h"

/*########################################################################
#                                                                       #
#  MACROS/DEFINES                                                       #
#                                                                       #
########################################################################*/
/*
 * Number of RTCP frames that need to be saved each connection and
 * per direction
 */
#define MMCONNSTATS_NUMBER_OF_SAVED_RTCP_FRAMES    2

/*########################################################################
#                                                                       #
#  TYPES                                                                #
#                                                                       #
########################################################################*/
typedef MmPbxError (*MmConnStatsUpdateRtcpReceivedCb)(void    *cookie,
                                                      uint8_t *data,
                                                      int     len);
typedef MmPbxError (*MmConnStatsUpdateRtcpSendCb)(void    *cookie,
                                                  uint8_t *data,
                                                  int     len);

/*
 * Rtcp connection configuration parameters.
 */
typedef struct {
    MmConnStatsUpdateRtcpReceivedCb updateRtcpReceiveCb;
    MmConnStatsUpdateRtcpSendCb     updateRtcpSendCb;
    void                            *cookie;
} MmConnStatsConfig;

/*
 * mmConnStats structure definition
 */
struct MmConnRtpStats {
    struct MmConn *mmConn;          /**< Parent class */
    int           session;          /**< Session number */
    spinlock_t    lock;             /**< Lock */
    RTPHANDLE     rtpHandle;        /* The RTCP Statistics handle */
};


typedef struct MmConnRtpStats  *MmConnStatsHndl;

/*########################################################################
#                                                                       #
#  FUNCTION PROTOTYPES                                                  #
#                                                                       #
########################################################################*/

/**
 * Set trace level of all multimedia switch relay connections.
 *
 * This function makes it possible to modify the trace level of all relay connections. This trace level is also dependant on the trace level which was used to compile the code.
 *
 * \since v1.0
 *
 * \pre none.
 *
 * \post The trace level will be the requested tracelevel if it not violates with the compile time trace level.
 *
 * \param [in] level Trace level.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The tracelevel has been  successfully set.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnStatsSetTraceLevel(MmPbxTraceLevel level);

/**
 * Initialise mmConnStats component of mmswitch kernel module.
 *
 * This function initialises the mmConnStats component of the mmswitch kernel module.
 *
 * \since v1.0
 *
 * \pre None.
 *
 * \post The mmConnStats component of mmswitch is initialised.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR the mmConnStats component has been initialised successfully.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnStatsInit(void);

/**
 * Deinitialise mmConnStats component of mmswitch kernel module.
 *
 * This function deinitialises the mmConnStats component of the mmswitch kernel module.
 *
 * \since v1.0
 *
 * \pre None.
 *
 * \post The mmConnStats component of mmswitch is deinitialised.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR the mmConnStats component has been deinitialised successfully.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnStatsDeinit(void);

MmPbxError mmConnStatsCreateSession(MmConnHndl        mmConn,
                                    MmConnStatsConfig *config);
MmPbxError mmConnStatsCloseSession(void *cookie);

MmPbxError mmConnStatsResetSession(void *cookie);
#endif /* #ifdef MMCONNSTATS_P_H */
