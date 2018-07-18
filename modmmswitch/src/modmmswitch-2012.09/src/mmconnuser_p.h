/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/** \file
 * Private multimedia switch user connection API.
 *
 * \version v1.0
 *
 *************************************************************************/
#ifndef  MMCONNUSER_P_INC
#define  MMCONNUSER_P_INC

/*########################################################################
#                                                                       #
#  HEADER (INCLUDE) SECTION                                             #
#                                                                       #
########################################################################*/
#include <linux/version.h>
#include <net/genetlink.h>
#include <linux/workqueue.h>
#include <linux/kfifo.h>

#include "mmconn_p.h"
/*########################################################################
#                                                                       #
#  MACROS/DEFINES                                                       #
#                                                                       #
########################################################################*/

/**
 * Maximum number of Generic Netlink operations
 * for a certain family.
 */
#define MMCONNUSR_MAX_GENL_OPS    2


#define MAX_MMCONNUSR_DATA_SIZE    3000
/*########################################################################
#                                                                       #
#  TYPES                                                                #
#                                                                       #
########################################################################*/

/*
 * Structure used as template for data which will be queued
 */
typedef struct {
    uint8_t             data[MAX_MMCONNUSR_DATA_SIZE];      /**< Work queue info */
    unsigned int        datalen;                            /**< Work queue info */
    MmConnPacketHeader  header;                             /**< Work queue info */
} MmConnUsrQueueEntry;

/*
 * MmConnUsr structure definition
 */
struct MmConnUsr {
    struct MmConn       mmConn;                           /**< Parent class */
    struct genl_family  geNlFam;                          /**< Generic Netlink family description */
    struct genl_ops     geNlOps[MMCONNUSR_MAX_GENL_OPS];  /**< Generic Netlink Operations */
    u32                 geNlPid;                          /**< Generic netlink socket identifier */
    u32                 geNlSeq;                          /**< Generic netlink sequence number */
    spinlock_t          fifo_lock;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
    struct kfifo        fifo;
#else
    struct kfifo        *fifo;
#endif
    MmConnUsrQueueEntry mmConnUsrQueueEntry; /*o Internally used as template for data which will be queued */
};

/*
 * MmConnUsr Work Queue related data
 */
struct MmConnUsrNlAsyncReply {
    MmConnUsrHndl       mmConnUsr;
    struct work_struct  work;                             /**< Work queue work */
    u32                 geNlPid;                          /**< Generic netlink socket identifier */
    u32                 geNlSeq;                          /**< Generic netlink sequence number */
};

/*########################################################################
#                                                                       #
#  FUNCTION PROTOTYPES                                                  #
#                                                                       #
########################################################################*/
/**
 * Set trace level of all multimedia switch user connections.
 *
 * This function makes it possible to modify the trace level of all user connections. This trace level is also dependant on the trace level which was used to compile the code.
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
MmPbxError mmConnUsrSetTraceLevel(MmPbxTraceLevel level);

/**
 * Initialise mmConnUsr component of mmswitch kernel module.
 *
 * This function initialises the mmConnUsr component of the mmswitch kernel module.
 *
 * \since v1.0
 *
 * \pre None.
 *
 * \post The mmConnUsr component of mmswitch is initialised.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR the mmConnUsr component has been initialised successfully.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnUsrInit(void);

/**
 * Deinitialise mmConnUsr component of mmswitch kernel module.
 *
 * This function deinitialises the mmConnUsr component of the mmswitch kernel module.
 *
 * \since v1.0
 *
 * \pre None.
 *
 * \post The mmConnUsr component of mmswitch is deinitialised.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR the mmConnUsr component has been deinitialised successfully.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnUsrDeinit(void);

/**
 * Constructor of a multimedia switch user connection instance.
 *
 * This function is the constructor of a multimedia switch user connection instance.
 * A user connection is a source/sink of a multimedia stream in user space.
 *
 * \since v1.0
 *
 * \pre \c connGr must be a valid handle.
 *
 * \post \c usr contains the handle of a valid multimedia switch user connection instance.
 *
 * \param [out] usr Handle of user connection instance of multimedia switch.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR A user connection handle has been successfully retrieved and is not NULL.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnUsrConstruct(MmConnUsrHndl     *usr,
                              struct genl_info  *info);
#endif   /* ----- #ifndef MMCONNUSER_P_INC  ----- */
