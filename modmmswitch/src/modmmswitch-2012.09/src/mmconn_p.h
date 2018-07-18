/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/** \file
 * Private multimedia switch connection API.
 *
 * \version v1.0
 *
 *************************************************************************/
#ifndef  MMCONN_P_INC
#define  MMCONN_P_INC

/*########################################################################
#                                                                       #
#  HEADER (INCLUDE) SECTION                                             #
#                                                                       #
########################################################################*/
#include <linux/version.h>
#include <linux/workqueue.h>
#include <linux/kfifo.h>

#include "mmcommon.h"
#include "mmconn.h"

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
 * Callback function for child specific cleanup when destructing mmConn.
 *
 * The callback function will be called when we are destructing an mmConn, it can be used for child specif cleanup.
 *
 * \since v1.0
 *
 * \pre \c mmConn must be a valid handle
 *
 * \post The child connection has prepared itself for being destructed.
 *
 * \param[in] mmConn Connection handle.
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR The child connection has handled it's destruct specific actions successfully.
 * \todo Add other possible return values after implementation.
 */

typedef MmPbxError (*MmConnChildDestructCb)(MmConnHndl mmConn);

/**
 * Callback function for child specific actions when we cross-connect the connection.
 *
 * The callback function will be called after we  cross-connect an mmConn, it can be used for child specifc actions.
 *
 * \since v1.0
 *
 * \pre \c mmConn must be a valid handle
 *
 * \post The child connection is notified about the cross-connect.
 *
 * \param[in] mmConn Connection handle.
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR The child connection has handled it's specific actions successfully.
 * \todo Add other possible return values after implementation.
 */

typedef MmPbxError (*MmConnChildXConnCb)(MmConnHndl mmConn);

/**
 * Callback function for child specific actions when have disconnected the connection.
 *
 * The callback function will be called when we have disconnected an mmConn, it can be used for child specif actions.
 *
 * \since v1.0
 *
 * \pre \c mmConn must be a valid handle
 *
 * \post The child connection has been notified about the disconnect.
 *
 * \param[in] mmConn Connection handle.
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR The child connection has handled it's specific actions successfully.
 * \todo Add other possible return values after implementation.
 */

typedef MmPbxError (*MmConnChildDiscCb)(MmConnHndl mmConn);

/**
 * Multimedia connection types.
 */
typedef enum {
    MMCONN_TYPE_RTCP = 0,   /**< RTCP connection. */
    MMCONN_TYPE_RELAY,      /**< Relay connection. */
    MMCONN_TYPE_USER,       /**< User Connection. */
    MMCONN_TYPE_KERNEL,     /**< Kernel connection. */
    MMCONN_TYPE_TONE,       /**< Tone connection. */
    MMCONN_TYPE_MULTICAST,  /**< Multicast connection. */
} MmConnType;

/**
 * Multimedia connection  mode types.
 */
typedef enum {
    MMCONN_MODE_SNDO,   /**< Send only */
    MMCONN_MODE_RCVO,   /**< Receive only */
    MMCONN_MODE_SNDRX,  /**< Send-Receive */
    MMCONN_MODE_INACT,  /**< Inactive */
} MmConnMode;

/**
 * Multimedia connection statistics.
 */
typedef struct {
    int ingressRtpPt;         /**< RTP payload type for incomming media */
} MmConnStats;


/**
 * Callback function for child specific write (data push) implementation.
 *
 * The callback function will be called when we are writing to an mmConn, it can be used to call a child specific implementation.
 *
 * \since v1.0
 *
 * \pre \c mmConn must be a valid handle
 *
 * \post The child connection has prepared itself for being destructed.
 *
 * \param[in] mmConn Connection  handle.
 * \param[in] header Packet header.
 * \param[in] buff Data to push.
 * \param[in] bytes Number of bytes to write.
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR The child connection has pushed the data.
 * \todo Add other possible return values after implementation.
 */

typedef MmPbxError (*MmConnChildWriteCb)(MmConnHndl         mmConn,
                                         MmConnPacketHeader *header,
                                         uint8_t            *buff,
                                         unsigned int       bytes);

/*
 * Struct MmConn definition.
 */
struct MmConn {
    MmConnType            type;
    MmConnHndl            target;
    MmConnHndl            control;
    void                  *cookie;
    spinlock_t            lock;
    unsigned long         flags;
    MmConnChildDestructCb mmConnChildDestructCb;
    MmConnChildXConnCb    mmConnChildXConnCb;
    MmConnChildDiscCb     mmConnChildDiscCb;
    MmConnChildWriteCb    mmConnChildWriteCb;
    MmConnWriteCb         mmConnWriteCb;
    MmConnDspCtrlCb       mmConnDspCtrlCb;
    MmConnStats           stats;
    spinlock_t            event_fifo_lock;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
    struct kfifo          event_fifo;
#else
    struct kfifo          *event_fifo;
#endif

#ifndef NDEBUG
    unsigned long         magic;
#endif /*NDEBUG*/
};


/*
 * MmConnUsr Asyncx Work related data
 */
struct MmConnAsyncWork {
    MmConnHndl          mmConn;
    struct work_struct  work;                             /**< Work queue work */
};

/*
 * MmConnUsr Work Queue related data
 */
struct MmConnNlAsyncReply {
    MmConnHndl          mmConn;
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
 * Set trace level of all multimedia switch connection function calls.
 *
 * This function makes it possible to modify the trace level of all multimedia connection function calls. This trace level is also dependant on the trace level which was used to compile the code.
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
MmPbxError mmConnSetTraceLevel(MmPbxTraceLevel level);

/**
 * Initialise mmConn component of mmswitch kernel module.
 *
 * This function initialises the mmConn component of mmswitch kernel module.
 *
 * \since v1.0
 *
 * \pre None.
 *
 * \post The mmConn component of mmswitch is initialised.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR the mmConn component has been initialised successfully.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnInit(void);

/**
 * Prepare multimedia connection for usage.
 *
 * Connections are created using the constructors of the specialization types,
 * but some common initialisation needs to be done for every connection (e.a add to connection group).
 * This common initialisation is done using this function.
 * Every constructor of a specialization type should call this function.
 *
 * \since v1.0
 *
 * \pre mmConn must be a valid handle.
 *
 * \post The multimedia connection is ready to be used.
 *
 * \param[in] mmConn Handle of multimedia switch connection instance.
 * \param[in] type Connection type.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR the multimedia connection is ready to be used.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnPrepare(MmConnHndl mmConn,
                         MmConnType type);


/**
 * Register child specific destruct callback function.
 *
 * This callback function can be provided by a child connection to extend the default destruct behaviour.
 * This callback function will be called when we are destructing the connection.
 *
 * \since v1.0
 *
 * \pre \c mmConn must be a valid handle.
 * \pre \c cb must be a valid callback function of type ::MmConnChildDestructCb.
 *
 * \post The callback function will be registered to the multimedia switch connection  instance.
 *
 * \param [in] mmConn Multimedia switch connection handle.
 * \param [in] cb callback function.
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR The callback function has been  successfully registered.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRegisterChildDestructCb(MmConnHndl             mmConn,
                                         MmConnChildDestructCb  cb);

/**
 * Register child specific cross-connect callback function.
 *
 * This callback function can be provided by a child connection to extend the default cross-connect behaviour.
 * This callback function will be called when we will cross-connect the connection.
 *
 * \since v1.0
 *
 * \pre \c mmConn must be a valid handle.
 * \pre \c cb must be a valid callback function of type ::MmConnChildXConnCb.
 *
 * \post The callback function will be registered to the multimedia switch connection  instance.
 *
 * \param [in] mmConn Multimedia switch connection handle.
 * \param [in] cb callback function.
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR The callback function has been  successfully registered.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRegisterChildXConnCb(MmConnHndl          mmConn,
                                      MmConnChildXConnCb  cb);

/**
 * Register child specific disconnect callback function.
 *
 * This callback function can be provided by a child connection to extend the default disconnect behaviour.
 * This callback function will be called when we have disconnected the connection.
 *
 * \since v1.0
 *
 * \pre \c mmConn must be a valid handle.
 * \pre \c cb must be a valid callback function of type ::MmConnChildDiscCb.
 *
 * \post The callback function will be registered to the multimedia switch connection  instance.
 *
 * \param [in] mmConn Multimedia switch connection handle.
 * \param [in] cb callback function.
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR The callback function has been  successfully registered.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRegisterChildDiscCb(MmConnHndl         mmConn,
                                     MmConnChildDiscCb  cb);

/**
 * Register child specific write callback function.
 *
 * This callback function can be provided by a child connection to push data into the child connection.
 *
 * \since v1.0
 *
 * \pre \c mmConn must be a valid handle.
 * \pre \c cb must be a valid callback function of type ::MmConnChildWriteCb.
 *
 * \post The callback function will be registered to the multimedia switch connection  instance.
 *
 * \param [in] mmConn Multimedia switch connection handle.
 * \param [in] cb callback function.
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR The callback function has been  successfully registered.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRegisterChildWriteCb(MmConnHndl          mmConn,
                                      MmConnChildWriteCb  cb);

/**
 * Destructor of a multimedia switch connection instance.
 *
 * This function is the destructor of a multimedia switch connection object. This function should be used when the user is no longer interested in the multimedia switch connection functionality. The destructor will unregister all callbacks registered with the connection instance and will free all resources used for the connection instance. Callback functions registered with the connection instance will no longer be fired from that moment on. This function will also set the connection handle to NULL.
 *
 *
 * \since v1.0
 *
 * \pre \c conn must be a valid handle.
 *
 * \post The multimedia switch connection object has been properly removed and all memory and references that it held are correctly removed.
 *
 * \param [in] conn Handle of multimedia switch connection instance.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR the multimedia switch connection object has been successfully removed.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnDestruct(MmConnHndl *conn);

/**
 * Cross-connect 2 multimedia switch connection instances.
 *
 * This function will cross-connect 2 multimedia instances by registering the write function of the target as a write callback function of the source and by registering the write function of the source as a write callback function of the target.
 *
 * \since v1.0
 *
 * \pre \c source must be a valid handle.
 * \pre \c target must be a valid handle.
 *
 * \post Both instances will be cross-connected.
 *
 * \param [in] source Source multimedia switch connection.
 * \param [in] target Target multimedia switch connection.
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR The cross-connect action succeeded.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnXConn(MmConnHndl source,
                       MmConnHndl target);

/**
 * Disconnect multimedia switch connection instance.
 *
 * This function will undo a previous cross-connect action.
 *
 * \since v1.0
 *
 * \pre \c conn must be a valid handle.
 * \pre \c conn must connected with another connection instance.
 *
 * \post The connection instance will be disconnected.
 *
 * \param [in] conn Multimedia switch connection to disconnect.
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR The connection instance has been successfully disconnected.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnDisc(MmConnHndl conn);

/**
 * Function to forward a command to the Hardware DSP.
 *
 * This function will forward the cmd to the Hardware DSP.
 *
 * \since v1.0
 *
 * \pre \c mmConn must be a valid handle.
 * \pre \c mmConn must connected with another connection instance.
 *
 * \post The command will be forwarded to the DSP instance
 *
 * \param [in] mmConn   must be a valid connection instance
 * \param [out] param   Could be of type MmConnDspRtpStatsParm structure handle or NULL.
 * \param [in] cmd      command to the DSP.
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR The command has been forwarded to the Hanrdware DSP
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnDspControl(MmConnHndl        mmConn,
                            void              *param,
                            MmConnDspCommand  cmd);
#endif   /* ----- #ifndef MMCONN_P_INC  ----- */
