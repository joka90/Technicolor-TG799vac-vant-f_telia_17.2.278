/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/** \file
 * Private multimedia switch multicast connection API.
 *
 * \version v1.0
 *
 *************************************************************************/
#ifndef  MMCONNMULTICAST_P_INC
#define  MMCONNMULTICAST_P_INC

/*########################################################################
#                                                                       #
#  HEADER (INCLUDE) SECTION                                             #
#                                                                       #
########################################################################*/
#include "mmconn_p.h"

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

/*
 * MmConnMulticast structure definition
 */
struct MmConnMulticast {
    struct MmConn     mmConn;                           /**< Parent class */
    struct list_head  sinks;
};

/*########################################################################
#                                                                       #
#  FUNCTION PROTOTYPES                                                  #
#                                                                       #
########################################################################*/

/**
 * Initialise mmConnMulticast component of mmswitch library.
 *
 * This function initialises the mmConnMulticast component of the mmswitch library.
 *
 * \since v1.0
 *
 * \pre None.
 *
 * \post The mmConnMulticast component of mmswitch is initialised.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR the mmConnMulticast component has been initialised successfully.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnMulticastInit(void);

/**
 * Deinitialise mmConnMulticast component of mmswitch kernel module.
 *
 * This function deinitialises the mmConnMulticast component of the mmswitch kernel module.
 *
 * \since v1.0
 *
 * \pre None.
 *
 * \post The mmConnMulticast component of mmswitch is deinitialised.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR the mmConnMulticast component has been deinitialised successfully.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnMulticastDeinit(void);

/**
 * Set trace level of all multimedia switch multicast connections.
 *
 * This function makes it possible to modify the trace level of all multicast connections. This trace level is also dependant on the trace level which was used to compile the code.
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
MmPbxError mmConnMulticastSetTraceLevel(MmPbxTraceLevel level);

/**
 * Constructor of a multimedia switch multicast connection instance.
 *
 * This function is the constructor of a multimedia switch multicast connection instance.
 * A multicast connection can be a source/sink of multiple media streams.
 *
 * \since v1.0
 *
 * \pre \c none.
 *
 * \post \c multicast contains the handle of a valid multimedia switch multicast connection instance.
 *
 * \param [out] multicast Handle of multicast connection instance of multimedia switch.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR A multicast connection handle has been successfully retrieved and is not NULL.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnMulticastConstruct(MmConnMulticastHndl *multicast);

/**
 * Add sink to a source  multicast connection.
 *
 * This function adds a sink to a source  multicast connection.
 * A sink can only be a multicast connection, no other connection types are allowed.
 *
 * \since v1.0
 *
 * \pre \c source must be a valid multicast connection handle.
 * \pre \c sink must be a valid multicast connection handle.
 *
 * \post The sink will be a sink of the source multicast connection.
 *
 * \param [in] source Handle of source multicast connection instance.
 * \param [in] sink Handle of sink multicast connection instance.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The multicast connection will be added as a sink of the source multicast connection..
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnMulticastAddSink(MmConnMulticastHndl source,
                                  MmConnMulticastHndl sink);

/**
 * Remove sink from a source  multicast connection.
 *
 * This function removes a sink from a source  multicast connection.
 *
 * \since v1.0
 *
 * \pre \c source must be a valid multicast connection handle.
 * \pre \c sink must be a valid multicast connection handle.
 *
 * \post The sink will no longer receive data from the source multicast connection.
 *
 * \param [in] source Handle of source multicast connection instance.
 * \param [in] sink Handle of sink multicast connection instance, which needs to be removed.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The sink will no longer receive data from the source.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnMulticastRemoveSink(MmConnMulticastHndl  source,
                                     MmConnMulticastHndl  sink);
#endif   /* ----- #ifndef MMCONNMULTICAST_P_INC  ----- */
