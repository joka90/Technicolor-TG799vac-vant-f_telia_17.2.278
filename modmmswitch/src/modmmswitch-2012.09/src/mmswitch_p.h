/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/** \file
 * Multimedia Switch.
 *
 * \version v1.0
 *
 *************************************************************************/

#ifndef MMSWITCH_INC
#define MMSWITCH_INC

/*########################################################################
#                                                                       #
#  HEADER (INCLUDE) SECTION                                             #
#                                                                       #
########################################################################*/
#include <linux/version.h>
#include <linux/socket.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/socket.h>
#include <linux/kfifo.h>

#include "mmcommon.h"
#include "mmconn_p.h"

/*########################################################################
#                                                                       #
#  MACROS/DEFINES                                                       #
#                                                                       #
########################################################################*/

#ifndef MMPBX_MAGIC_NR
#define MMPBX_MAGIC_NR    0xAC1C0001
#endif

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE    0
#endif

#define MAX_MMSWITCHSOCK_DATA_SIZE    1500

/*########################################################################
#                                                                       #
#  TYPES                                                                #
#                                                                       #
########################################################################*/
typedef void (*MmSwitchTimerCb)(MmConnHndl mmConn);

/*
 * Internal timer structure
 */
typedef struct {
    MmSwitchTimerCb   mmSwitchTimerCb;
    MmConnHndl        mmConn;
    struct timer_list timer;
    unsigned int      active;
    int               period;
    int               periodic;
} MmSwitchTimer;

typedef struct {
    int packetLost;
    int writeError;
} MmSwitchError;

#define TIMER_IDLE      0
#define TIMER_ACTIVE    0xC53AA35C


/**
 * Callback function to receive data from a socket.
 *
 * A mmConn which wants to receive data from a socket should  call this function.
 * The data will be pushed to the connection.
 *
 * \since v1.0
 *
 * \pre \c NONE
 *
 * \post The connection has received the data.
 *
 * \param [in] mmConn Handle of connection instance.
 * \param[in] data which is pushed to the endpoint.
 * \param [in] header Header used to identify packet.
 * \param [in] bytes Number of bytes which were pushed.
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR The write action to the endpoint suceeded.
 * \todo Add other possible return values after implementation.
 */
typedef MmPbxError (*MmSwitchSocketCb)(MmConnHndl         mmConn,
                                       void               *data,
                                       MmConnPacketHeader *header,
                                       unsigned int       bytes);

typedef struct {
    struct socket       *sock;
    MmSwitchError       sockError;
    MmSwitchSocketCb    cb;
    MmConnHndl          mmConn;
    MmConnPacketHeader  header;
    void (*orig_sk_write_space)(struct sock *sk);
    void (*orig_sk_state_change)(struct sock *sk);
    void (*orig_sk_data_ready)(struct sock  *sk,
                               int          bytes);
} MmSwitchSock;

/*
 * Structure used as template for data which will be queued
 */
typedef struct {
    MmSwitchSock            *mmSwitchSock;
    uint8_t                 data[MAX_MMSWITCHSOCK_DATA_SIZE];
    unsigned int            datalen;
    struct sockaddr_storage dst;
    struct list_head        list;
} MmSwitchSockQueueEntry;

/*########################################################################
#                                                                       #
#  FUNCTION PROTOTYPES                                                  #
#                                                                       #
########################################################################*/

/**
 * Set trace level of multimedia switch.
 *
 * This function makes it possible to modify the trace level of the multimedia switch object. This trace level is also dependant on the trace level which was used to compile the code.
 *
 * \since v1.0
 *
 * \pre \c mmSwitch must be a valid handle.
 *
 * \post The trace level will be the requested tracelevel if it not violates with the compile time trace level.
 *
 * \param [in] level Trace level of multimedia switch.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The tracelevel has been  successfully set.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmSwitchSetTraceLevel(MmPbxTraceLevel level);

/**
 * Prepare socket, created in user space, for usage in kernel space.
 */
MmPbxError mmSwitchPrepareSocket(MmSwitchSock *mmSwitchSock,
                                 int          sockFd);

/**
 * Cleanup mmswitch socket object (cleanup before object get's destroyed)
 */
MmPbxError mmSwitchCleanupSocket(MmSwitchSock *mmSwitchSock);


/**
 * Register socket callback function, to receive data from socket.
 */
MmPbxError mmSwitchRegisterSocketCb(MmSwitchSock        *mmSwitchSock,
                                    MmSwitchSocketCb    cb,
                                    MmConnHndl          conn,
                                    MmConnPacketHeader  *header);

/**
 * Write to socket.
 */

MmPbxError mmSwitchWriteSocket(MmSwitchSock     *mmSwitchSock,
                               struct sockaddr  *dst,
                               void             *data,
                               unsigned int     len);

/**
 * Prepare timer for usage.
 */
MmPbxError mmSwitchPrepareTimer(MmSwitchTimer   *tmr,
                                MmSwitchTimerCb cb,
                                MmConnHndl      mmConn,
                                int             periodic);

/**
 * Destroy  timer.
 */
MmPbxError mmSwitchDestroyTimer(MmSwitchTimer *tmr);


/**
 * Start timer.
 */
MmPbxError mmSwitchStartTimer(MmSwitchTimer *tmr,
                              int           period);

/**
 * Stop timer.
 */
MmPbxError mmSwitchStopTimer(MmSwitchTimer *tmr);

/**
 * Restart timer.
 */
MmPbxError mmSwitchRestartTimer(MmSwitchTimer *tmr,
                                int           period);
#endif /* MMSWITCH_INC */
