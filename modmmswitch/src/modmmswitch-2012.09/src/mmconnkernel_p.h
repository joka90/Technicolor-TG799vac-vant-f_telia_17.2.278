/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/** \file
 * Private multimedia switch kernel connection API.
 *
 * \version v1.0
 *
 *************************************************************************/
#ifndef  MMCONNKERNEL_P_INC
#define  MMCONNKERNEL_P_INC

/*########################################################################
#                                                                       #
#  HEADER (INCLUDE) SECTION                                             #
#                                                                       #
########################################################################*/
#include "mmconnkernel.h"
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

/**
 * Kernel connection configuration parameters.
 */
typedef struct {
    unsigned int  endpointId;                   /**< Unique identifier of kernel space endpoint */
    bool          isControlConn;                /**< Indication for  control connection */
} MmConnKrnlConfig;

/*
 * MmConnKrnl structure definition
 */
struct MmConnKrnl {
    struct MmConn     mmConn;                           /**< Parent class */
    MmConnKrnlConfig  config;                           /**< Kernel connection specific configuration */
};

/*########################################################################
#                                                                       #
#  FUNCTION PROTOTYPES                                                  #
#                                                                       #
########################################################################*/
/**
 * Set trace level of all multimedia switch kernel connections.
 *
 * This function makes it possible to modify the trace level of all kernel connections. This trace level is also dependant on the trace level which was used to compile the code.
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
MmPbxError mmConnKrnlSetTraceLevel(MmPbxTraceLevel level);

/**
 * Initialise mmConnKrnl component of mmswitch kernel module.
 *
 * This function initialises the mmConnKrnl component of the mmswitch kernel module.
 *
 * \since v1.0
 *
 * \pre None.
 *
 * \post The mmConnKrnl component of mmswitch is initialised.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR the mmConnKrnl component has been initialised successfully.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnKrnlInit(void);

/**
 * Deinitialise mmConnKrnl component of mmswitch kernel module.
 *
 * This function deinitialises the mmConnKrnl component of the mmswitch kernel module.
 *
 * \since v1.0
 *
 * \pre None.
 *
 * \post The mmConnKrnl component of mmswitch is deinitialised.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR the mmConnKrnl component has been deinitialised successfully.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnKrnlDeinit(void);

/**
 * Constructor of a multimedia switch kernel connection instance.
 *
 * This function is the constructor of a multimedia switch kernel connection instance.
 * A kernel connection is a source/sink of a multimedia stream in kernel space.
 *
 * \since v1.0
 *
 * \pre \c connGr must be a valid handle.
 *
 * \post \c krnl contains the handle of a valid multimedia switch kernel connection instance.
 *
 * \param [in] config Configuration of kernel connection instance of multimedia switch.
 * \param [out] krnl Handle of kernel connection instance of multimedia switch.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR A kernel connection handle has been successfully retrieved and is not NULL.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnKrnlConstruct(MmConnKrnlConfig *config,
                               MmConnKrnlHndl   *krnl);
#endif   /* ----- #ifndef MMCONNKERNEL_P_INC  ----- */
