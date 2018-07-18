/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/** \file
 * Multimedia kernel space switch kernel connection API.
 *
 * A kernel connection is a source/sink of a multimedia stream in kernel space.
 *
 * \version v1.0
 *
 *************************************************************************/

#ifndef  MMCONNKERNEL_INC
#define  MMCONNKERNEL_INC

/*########################################################################
#                                                                       #
#  HEADER (INCLUDE) SECTION                                             #
#                                                                       #
########################################################################*/
#include "mmconntypes.h"
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

/**
 * Kernel connection event types.
 */

typedef enum {
    MMCONNKRNL_EVENT_CONSTRUCT = 0,     /**< A new kernel connection is constructed. */
    MMCONNKRNL_EVENT_CTRL_CONSTRUCT,    /**< A new kernel control connection is constructed. */
    MMCONNKRNL_EVENT_DESTRUCT,          /**< A kernel connection will be destructed. */
    MMCONNKRNL_EVENT_XCONN,             /**< A kernel connection is cross-connected with another connection. */
    MMCONNKRNL_EVENT_DISC,              /**< A kernel connection is disconnected. */
} MmConnKrnlEventType;

/**
 * Callback function to listen for mmConnKrnl events.
 *
 * A kernel space endpoint can use this callback function to take appropriate actions when
 * a kernel connection was constructed witch matches the endpoint id of the kernel space endpoint.
 *
 * \since v1.0
 *
 * \pre \c NONE
 *
 * \post The endpoint has handled the event.
 *
 * \param[in] type Event type.
 * \param [in] id Endpoint id.
 * \param [in] MmConnKrnlHndl mmConnKrnl.
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR The event has been handled successfully.
 * \todo Add other possible return values after implementation.
 */
typedef MmPbxError (*MmConnKrnlEventCb)(MmConnKrnlEventType type,
                                        unsigned int        id,
                                        MmConnKrnlHndl      mmConnKrnl);


/*########################################################################
#                                                                       #
#  FUNCTION PROTOTYPES                                                  #
#                                                                       #
########################################################################*/


/**
 * Register callback for multimedia switch kernel connection events.
 *
 * This callback function can be registered to listen for events on kernel connections.
 *
 * \since v1.0
 *
 * \pre \c cb must be a valid callback function of type ::MmConnKrnlEventCb.
 *
 * \post The endpoint has handled the event.
 *
 * \param [in] cb Kernel connection event callback function.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The callback function has been successfully registered.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnKrnlRegisterEventCb(MmConnKrnlEventCb cb);

/**
 * Unregister callback for multimedia switch kernel connection events.
 *
 * Unregister callback function to listen for events on kernel connections.
 *
 * \since v1.0
 *
 * \pre \c cb must be a valid callback function of type ::MmConnKrnlEventCb.
 *
 * \post The callback function will no longer be called.
 *
 * \param [in] cb Kernel connection event callback function.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The callback function has been successfully unregistered.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnKrnlUnregisterEventCb(MmConnKrnlEventCb cb);


/**
 * Get a string-representation of a ::MmConnKrnlEventType
 *
 * \since v1.0
 *
 * \param [in] eventType
 *
 * \return string-representation of ::MmConnKrnlEventType
 */
const char * mmConnKrnlEventString(MmConnKrnlEventType eventType);
#endif   /* ----- #ifndef MMCONNKERNEL_INC  ----- */
