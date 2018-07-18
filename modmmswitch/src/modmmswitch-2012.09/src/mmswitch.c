/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/** \file
 * Multimedia Switch kernel module.
 *
 * \version v1.0
 *
 *************************************************************************/


/*
 * Define tracing prefix, needs to be defined before includes.
 */
#define MODULE_NAME    "MMSWITCH"
/*########################################################################
#                                                                        #
#  HEADER (INCLUDE) SECTION                                              #
#                                                                        #
########################################################################*/
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <net/sock.h>
#include <linux/socket.h>
#include <linux/file.h>
#include <linux/udp.h>
#include <net/ip.h>
#include <linux/jiffies.h>
#include <linux/kfifo.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/kthread.h>

#include "mmswitch_netlink_p.h"
#include "mmconnuser_netlink_p.h"
#include "mmconn_p.h"
#include "mmconn_netlink_p.h"
#include "mmconnuser_p.h"
#include "mmconnkernel_p.h"
#include "mmconnmulticast_p.h"
#include "mmconntone_p.h"
#include "mmconnrelay_p.h"
#include "mmconnrtcp_p.h"

/*########################################################################
#                                                                        #
#  MACROS/DEFINES                                                        #
#                                                                        #
########################################################################*/
/* This is changed from 150(mapped to 3s in 8khz codec, and 1.5s in 16khz) to 
   avoid crash in abacus endurance test. It's related with timestamp in dsp assert */
#define NUM_PACKETS_LOST    25
/*########################################################################
#                                                                        #
#  TYPES                                                                 #
#                                                                        #
########################################################################*/

/*########################################################################
#                                                                        #
#  PRIVATE GLOBALS                                                       #
#                                                                        #
########################################################################*/
static int _traceLevel = MMPBX_TRACELEVEL_ERROR;

/**
 * The _wqThread is a dedciated thread that is habdling  work queue jobs.
 */
static struct task_struct *_wqThread = NULL;
/**
 * This semaphore is used as a wake-up mechanism for the worq queue thread.
 * Whenever somebody has work for the thread, this semaphore is up'ed.
 */
static struct semaphore _wqSem;
/**
 * Cleanup mechanism to synchronize the stopping of the thread.
 */
static struct semaphore _exitWqSem;
/**
 * The queue for work queues.
 */
static LIST_HEAD(_workQueueList);
/**
 * The lock for accessing _workQueueList.  The queue is accessed from the work queue thread itself
 * (which is reading from it) and the netlink stack context (when a netlink command arrives).
 * The protected section should be very fast.
 */
static spinlock_t _wqLock;

/**
 * The _writeThread is taking socket write actions from the _writeQueue,
 * and subsequently writing the data to that socket.  It makes the data
 * transfer asynchronous from the caller/sender's context.
 */
static struct task_struct *_writeThread = NULL;
/**
 * This semaphore is used as a wake-up mechanism for the write thread.
 * Whenever somebody has work for the thread, this semaphore is up'ed.
 */
static struct semaphore _writeSem;
/**
 * This lock is taken by the _writeThread to indicate it is busy with
 * some socket.  The whole section of taking a socket write action
 * from the queue, and the actual sending on the socket, is protected.
 * Protecting this whole section makes it possible to cleanup the
 * socket safely from another thread, whenever needed.
 */
static struct semaphore _writeThreadSocketActionsMutex;
/**
 * The queue with socket write actions.
 */
static LIST_HEAD(_writeQueue); /**< Queue for all data which we want to write to a socket */
/**
 * The lock for accessing the queue.  The queue is accessed from the write thread itself
 * (which is reading from it), but also possibly from interrupt context (if the packets arrive
 * from the Broadcom DSP/driver for instance).  The protected section should be very fast.
 */
static spinlock_t _listReadLock;
/**
 * Cleanup mechanism to synchronize the stopping of the thread.
 */
static struct semaphore _exitReadSem;

/**
 * The _readThread is taking socket read actions form the _readQueue,
 * and subsequently performing the actual read action from that socket.
 * Finally it calls a (previously registered) call-back associated
 * with the socket to deliver the data to the correct module (typically
 * another connection in the switch).
 * The _readThread has similar locks then the _writeThread.
 */
static struct task_struct *_readThread = NULL;
static struct semaphore   _readThreadSocketActionsMutex;
static struct semaphore   _readSem;
static LIST_HEAD(_readQueue); /**< Queue for all data which we want to write to a socket */
static spinlock_t       _listWriteLock;
static struct semaphore _exitWriteSem;

/**
 * The network socket is created, and also deleted, from user-space.
 * In the meantime, we are also working with it: we increase the reference
 * count before we start working on this socket, and decrease it again whenever
 * we are finished.  That way we are guaranteed the kernel keeps the socket alive
 * as long as we need it.
 * Whenever we are done with this socket, we want to be 100% sure our
 * call-back is not executed anymore before we continue the cleanup.
 * Since at this point the socket is still open/active (until it is closed from
 * user-space), we need a special protection system.
 */
static spinlock_t _socketActivationLock;

static int _exit = 0;

/*########################################################################
#                                                                        #
#  PRIVATE FUNCTION PROTOTYPES                                           #
#                                                                        #
########################################################################*/
static void write_space_socket_stub(struct sock *sk);
static void change_state_socket_stub(struct sock *sk);
static void data_ready_socket(struct sock *sk,
                              int         bytes);
static int mmSwitchWriteSocketThreadFunc(void *thread_data);
static int mmSwitchReadSocketThreadFunc(void *thread_data);
static int mmSwitchWorkQueueThreadFunc(void *thread_data);

static void timer_cb(unsigned long data);

/*########################################################################
#                                                                        #
#   PUBLIC FUNCTION DEFINITIONS                                          #
#                                                                        #
########################################################################*/

/**
 *
 */
MmPbxError mmSwitchSetTraceLevel(MmPbxTraceLevel level)
{
    _traceLevel = level;
    MMPBX_TRC_INFO("New trace level : %s\n", mmPbxGetTraceLevelString(level));


    return MMPBX_ERROR_NOERROR;
}

/**
 *
 */
MmPbxError mmSwitchPrepareSocket(MmSwitchSock *mmSwitchSock,
                                 int          sockFd)
{
    MmPbxError    ret = MMPBX_ERROR_NOERROR;
    int           err = 0;
    unsigned long flags;

    do {
        MMPBX_TRC_INFO("sockFd: %d\n", sockFd);
        if (sockFd >= 0) {
            /* Get the socket slot (= struct) from the file descriptor created in user space.
             * The file handle passed in gets locked (via fd_get) --> reference count will become 2.
             * We need to call sockfd_put() whenever we are done with it!
             */
            mmSwitchSock->sock = sockfd_lookup(sockFd, &err);
            if (!mmSwitchSock->sock) {
                MMPBX_TRC_ERROR("sockfd_lookup failed with: %d", err);
                ret = MMPBX_ERROR_INTERNALERROR;
                break;
            }


            spin_lock_irqsave(&_socketActivationLock, flags);

            /* backup socket callbacks */
            mmSwitchSock->orig_sk_write_space   = mmSwitchSock->sock->sk->sk_write_space;
            mmSwitchSock->orig_sk_state_change  = mmSwitchSock->sock->sk->sk_state_change;
            mmSwitchSock->orig_sk_data_ready    = mmSwitchSock->sock->sk->sk_data_ready;

            /* Replace some of the internal socket members */
            mmSwitchSock->sock->sk->sk_user_data    = NULL;
            mmSwitchSock->sock->sk->sk_state_change = change_state_socket_stub;
            /* After the next line our callback is activated!  But it will block/wait on _socketActivationLock. */
            mmSwitchSock->sock->sk->sk_data_ready   = data_ready_socket;
            mmSwitchSock->sock->sk->sk_write_space  = write_space_socket_stub;
            mmSwitchSock->sock->sk->sk_user_data    = (void *)mmSwitchSock;
            /* Need to set the socket allocation to atomic */
            mmSwitchSock->sock->sk->sk_allocation = GFP_ATOMIC;

            spin_unlock_irqrestore(&_socketActivationLock, flags);
        }

        return err;
    } while (0);

    return ret;
}

/**
 *
 */
MmPbxError mmSwitchCleanupSocket(MmSwitchSock *mmSwitchSock)
{
    MmPbxError              ret = MMPBX_ERROR_NOERROR;
    unsigned long           flags;
    MmSwitchSockQueueEntry  *current_entry  = NULL;
    MmSwitchSockQueueEntry  *next_entry     = NULL;

    /**
     * The _readQueue is under our control:  it is populated by the data_ready_socket() call-back
     * that we registered ourselves.
     *
     * However the population of the _writeQueue is not under our control; it happens via mmSwitchWriteSocket()
     * which can be called from everywhere, with any MmSwitchSock reference.
     *
     * !!!! We assume that from this moment onwards, nobody will call mmSwitchWriteSocket() anymore on the mmSwitchSock
     * passed in with this cleanup function. !!!!
     *
     * In practice this means that the connection using this socket (for instance an RTCP connection) should be correctly
     * disconnected from the partner connection (for instance a Kernel connection) indirectly calling the mmSwitchWriteSocket()
     * function via mmConnChildWriteCb() (or even mmConnRtcpSendRtcp()) of this connection.
     */

    /* Take global thread locks!  This will make sure the threads will not be busy with any sockets. */

    while (down_interruptible(&_writeThreadSocketActionsMutex) != 0) {
    }
    while (down_interruptible(&_readThreadSocketActionsMutex) != 0) {
    }

    if (mmSwitchSock->sock != NULL) {
        /**
         * First make sure no call-backs are called anymore!
         * We need to add some protection, since our original data_ready_socket call-back could be in the middle of a run on another core.
         */
        spin_lock_irqsave(&_socketActivationLock, flags);
        /*  socket callbacks */
        mmSwitchSock->sock->sk->sk_state_change = mmSwitchSock->orig_sk_state_change;
        mmSwitchSock->sock->sk->sk_data_ready   = mmSwitchSock->orig_sk_data_ready;
        mmSwitchSock->sock->sk->sk_write_space  = mmSwitchSock->orig_sk_write_space;
        mmSwitchSock->sock->sk->sk_user_data    = NULL;
        spin_unlock_irqrestore(&_socketActivationLock, flags);
        /* We are done with the socket!  Release the lock we took earlier on the file handle. */
        sockfd_put(mmSwitchSock->sock);
        mmSwitchSock->sock = NULL;
    }

    mmSwitchSock->cb      = NULL;
    mmSwitchSock->mmConn  = NULL;
    memset(&mmSwitchSock->sockError, 0, sizeof(MmSwitchError));
    /* Remove references to this socket from the read and write queues */
    spin_lock_irqsave(&_listWriteLock, flags);
    list_for_each_entry_safe(current_entry, next_entry, &_writeQueue, list)
    {
        if (current_entry->mmSwitchSock == mmSwitchSock) {
            list_del(&current_entry->list);
            kfree(current_entry);
        }
    }
    spin_unlock_irqrestore(&_listWriteLock, flags);

    spin_lock_irqsave(&_listReadLock, flags);
    list_for_each_entry_safe(current_entry, next_entry, &_readQueue, list)
    {
        if (current_entry->mmSwitchSock == mmSwitchSock) {
            list_del(&current_entry->list);
            kfree(current_entry);
        }
    }
    spin_unlock_irqrestore(&_listReadLock, flags);

    /* Release global thread locks:  The threads can continue playing with other sockets now. */
    up(&_writeThreadSocketActionsMutex);
    up(&_readThreadSocketActionsMutex);

    return ret;
}

/**
 *
 */
MmPbxError mmSwitchRegisterSocketCb(MmSwitchSock        *mmSwitchSock,
                                    MmSwitchSocketCb    cb,
                                    MmConnHndl          conn,
                                    MmConnPacketHeader  *header)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    mmSwitchSock->cb      = cb;
    mmSwitchSock->mmConn  = conn;
    memcpy(&mmSwitchSock->header, header, sizeof(MmConnPacketHeader));
    memset(&mmSwitchSock->sockError, 0, sizeof(MmSwitchError));
    return ret;
}

/**
 *
 */
MmPbxError mmSwitchWriteSocket(MmSwitchSock     *mmSwitchSock,
                               struct sockaddr  *dst,
                               void             *data,
                               unsigned int     len)
{
    MmPbxError              err             = MMPBX_ERROR_NOERROR;
    MmSwitchSockQueueEntry  *sockQueueEntry = NULL;
    unsigned long           flags;

    /*
     * We will send data to socket asynchronously.
     * This is done to avoid "BUG scheduling while atomic", triggered within sock_sendmsg.
     * This means that it's possible that this function is called multiple times before
     * the data is really send, that's why we queue the data.
     */

    if (len > MAX_MMSWITCHSOCK_DATA_SIZE) {
        err = MMPBX_ERROR_NORESOURCES;
        MMPBX_TRC_ERROR("len > MAX_MMSWITCHSOCK_DATA_SIZE\n");
        return err;
    }

    /*
     * Check whether the socket is still in use.
     */

    if ((mmSwitchSock->cb != NULL)) {
        /* Try to allocate another sockQueueEntry object instance */
        sockQueueEntry = kmalloc(sizeof(MmSwitchSockQueueEntry), GFP_ATOMIC);
        if (sockQueueEntry == NULL) {
            err = MMPBX_ERROR_NORESOURCES;
            MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
            return err;
        }

        /* Add reference to mmswitchSock */
        sockQueueEntry->mmSwitchSock = mmSwitchSock;

        /* copy data */
        memcpy(&sockQueueEntry->dst, dst, sizeof(struct sockaddr_storage));
        sockQueueEntry->datalen = len;
        memcpy(sockQueueEntry->data, data, len);

        /*Add sockQueueEntry to write queue */
        INIT_LIST_HEAD(&sockQueueEntry->list);
        spin_lock_irqsave(&_listWriteLock, flags);
        list_add_tail(&sockQueueEntry->list, &_writeQueue);
        spin_unlock_irqrestore(&_listWriteLock, flags);

        /*Release semaphore of socket write thread */
        if (!_exit) {
            up(&_writeSem);
        }
    }

    return err;
}

/**
 *
 */
MmPbxError mmSwitchPrepareTimer(MmSwitchTimer   *tmr,
                                MmSwitchTimerCb cb,
                                MmConnHndl      mmConn,
                                int             periodic)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    if (tmr == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_INFO("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (cb == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_INFO("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (mmConn == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_INFO("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (tmr) {
        tmr->mmSwitchTimerCb  = cb;
        tmr->mmConn           = mmConn;
        tmr->active           = TIMER_IDLE;
        tmr->periodic         = periodic;
        init_timer(&tmr->timer);
        tmr->timer.function = timer_cb;
        tmr->timer.data     = (unsigned long)tmr;
    }

    return ret;
}

/**
 *
 */
MmPbxError mmSwitchDestroyTimer(MmSwitchTimer *tmr)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    if (tmr == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_INFO("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (tmr->active == TIMER_ACTIVE) {
        tmr->active = TIMER_IDLE;
        tmr->mmConn = NULL;
        del_timer_sync(&tmr->timer);
    }

    return ret;
}

/**
 *  Start timer, period in ms.
 */
MmPbxError mmSwitchStartTimer(MmSwitchTimer *tmr,
                              int           period)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    if (tmr == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_INFO("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (period == 0) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_INFO("%s, period == 0\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    tmr->period         = period;
    tmr->timer.expires  = jiffies + HZ * period / 1000;
    tmr->active         = TIMER_ACTIVE;
    add_timer(&tmr->timer);

    return ret;
}

/**
 *
 */
MmPbxError mmSwitchStopTimer(MmSwitchTimer *tmr)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    if (tmr == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_INFO("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (tmr->active == TIMER_ACTIVE) {
        tmr->active = TIMER_IDLE;
        del_timer_sync(&tmr->timer);
    }

    return ret;
}

/**
 *
 */

MmPbxError mmSwitchRestartTimer(MmSwitchTimer *tmr,
                                int           period)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    do {
        ret = mmSwitchStopTimer(tmr);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmSwitchStopTimer failed with: %s\n", mmPbxGetErrorString(ret));
            break;
        }

        ret = mmSwitchStartTimer(tmr, period);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmSwitchStartTimer failed with: %s\n", mmPbxGetErrorString(ret));
            break;
        }


        return ret;
    } while (0);

    return ret;
}

/*########################################################################
#                                                                        #
#   PRIVATE FUNCTION DEFINITIONS                                         #
#                                                                        #
########################################################################*/

/**
 *
 */
static void timer_cb(unsigned long data)
{
    MmSwitchTimer *t = (MmSwitchTimer *)data;

    if ((t != NULL) && (t->active == TIMER_ACTIVE) && (t->mmConn != NULL)) {
        /* Call callback */
        t->mmSwitchTimerCb(t->mmConn);

        /* restart if periodic */
        if (t->periodic) {
            if (t->active == TIMER_ACTIVE) {
                t->timer.expires = jiffies + HZ * t->period / 1000;
                add_timer(&t->timer);
            }
        }
    }
}

/**
 *
 */
static void write_space_socket_stub(struct sock *sk)
{
    /* Not implemented */
}

/**
 *
 */
static void change_state_socket_stub(struct sock *sk)
{
    /* Not implemented */
}

/**
 *
 */
static void data_ready_socket(struct sock *sk,
                              int         bytes)
{
    MmSwitchSock            *mmSwitchSock   = NULL;
    MmSwitchSockQueueEntry  *sockQueueEntry = NULL;
    unsigned long           flags;
    unsigned long           flags_activationLock;

    spin_lock_irqsave(&_socketActivationLock, flags_activationLock);

    if (sk->sk_data_ready != data_ready_socket) {
        MMPBX_TRC_DEBUG("Socket call-back already de-activated!\n");
        goto data_ready_socket_exit;
    }
    if (sk->sk_user_data == NULL) {
        MMPBX_TRC_ERROR("Socket user data not filled in!\n");
        goto data_ready_socket_exit;
    }
    mmSwitchSock = (MmSwitchSock *) sk->sk_user_data;
    if (mmSwitchSock->cb == NULL) {
        /* No call-back configured, so no need to load the queue. */
        goto data_ready_socket_exit;
    }
    /* Try to allocate another sockQueueEntry object instance */
    sockQueueEntry = kmalloc(sizeof(MmSwitchSockQueueEntry), GFP_ATOMIC);
    if (sockQueueEntry == NULL) {
        MMPBX_TRC_ERROR("kmalloc failed\n");
        goto data_ready_socket_exit;
    }
    /* Copy data into entry */
    sockQueueEntry->mmSwitchSock = mmSwitchSock;
    /* Add sockQueueEntry to read queue */
    INIT_LIST_HEAD(&sockQueueEntry->list);
    spin_lock_irqsave(&_listReadLock, flags);
    list_add_tail(&sockQueueEntry->list, &_readQueue);
    spin_unlock_irqrestore(&_listReadLock, flags);
    /* Wake up socket read thread!  Some work needs to be done. */
    if (!_exit) {
        up(&_readSem);
    }

data_ready_socket_exit:

    spin_unlock_irqrestore(&_socketActivationLock, flags_activationLock);
}

static int mmSwitchWriteSocketThreadFunc(void *thread_data)
{
    int                     ret = 0;
    struct kvec             vec;
    struct msghdr           msg;
    MmSwitchSockQueueEntry  *sockQueueEntry = NULL;
    unsigned long           flags;


    while (1) {
        /* Attempt to acquire semaphore */
        ret = down_interruptible(&_writeSem);

        if (ret != 0) {
            MMPBX_TRC_DEBUG("signal received, semaphore not acquired...\n");
            continue;
        }

        if (down_interruptible(&_writeThreadSocketActionsMutex) != 0) {
            MMPBX_TRC_DEBUG("signal received, mutex not acquired...\n");
            up(&_writeSem);
            continue;
        }

        while (1) {
            sockQueueEntry = NULL;
            spin_lock_irqsave(&_listWriteLock, flags);
            if (!list_empty(&_writeQueue)) {
                sockQueueEntry = list_first_entry(&_writeQueue, MmSwitchSockQueueEntry, list);
                list_del(&sockQueueEntry->list);
            }
            spin_unlock_irqrestore(&_listWriteLock, flags);

            if (!sockQueueEntry) {
                break;
            }
            if (_exit) {
                kfree(sockQueueEntry);
                break;
            }

            if (sockQueueEntry->mmSwitchSock->sock != NULL) {
                /* Do not forget these memsets or kernel_sendmsg will fail */
                memset(&vec, 0, sizeof(struct kvec));
                memset(&msg, 0, sizeof(struct msghdr));

                vec.iov_base    = sockQueueEntry->data;
                vec.iov_len     = sockQueueEntry->datalen;
                msg.msg_flags   = MSG_DONTWAIT | MSG_NOSIGNAL;
                msg.msg_name    = &sockQueueEntry->dst;
                msg.msg_namelen = (sockQueueEntry->dst.ss_family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);

/* If for some reason the function kernel_sendmsg returns an error, then wait some time (wait for 150 packets) then retry to send.
 */
                if (sockQueueEntry->mmSwitchSock->sockError.writeError == 1) {
                    if (sockQueueEntry->mmSwitchSock->sockError.packetLost < NUM_PACKETS_LOST) {
                        sockQueueEntry->mmSwitchSock->sockError.packetLost++;
                    }
                    else {
                        ret = kernel_sendmsg(sockQueueEntry->mmSwitchSock->sock, &msg, &vec, 1, vec.iov_len);
                        if (ret < 0) {
                            sockQueueEntry->mmSwitchSock->sockError.packetLost = 0;
                        }
                        else {
                            sockQueueEntry->mmSwitchSock->sockError.writeError  = 0;
                            sockQueueEntry->mmSwitchSock->sockError.packetLost  = 0;
                        }
                    }
                }
                else {
                    ret = kernel_sendmsg(sockQueueEntry->mmSwitchSock->sock, &msg, &vec, 1, vec.iov_len);
                    if (ret < 0) {
                        MMPBX_TRC_ERROR("kernel_sendmsg failed with error %d\n", ret);
                        sockQueueEntry->mmSwitchSock->sockError.writeError  = 1;
                        sockQueueEntry->mmSwitchSock->sockError.packetLost  = 0;
                    }
                }
            }

            /*Destruct */
            kfree(sockQueueEntry);
        }

        up(&_writeThreadSocketActionsMutex);
        if (_exit) {
            MMPBX_TRC_DEBUG("exited \n");
            break;
        }
    }

    up(&_exitWriteSem);

    do_exit(0);

    return 0;
}

MmPbxError mmSwitchInitWork(struct work_struct *work, work_func_t func)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    if ((work == NULL) || (func == NULL)) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s failed with error %s\n", __func__, mmPbxGetErrorString(ret));
        return ret;
    }

    INIT_LIST_HEAD(&(work)->entry);
    work->func = func;

    return ret;
}

MmPbxError mmSwitchScheduleWork(struct work_struct *work)
{
    MmPbxError    ret   = MMPBX_ERROR_NOERROR;
    unsigned long flags = 0;

    if ((work == NULL) || (work->func == NULL)) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s failed with error %s\n", __func__, mmPbxGetErrorString(ret));
        return ret;
    }

    /* list_add() will give us a LIFO (Last In 1st Out). list_add_tail() will give us a FIFO (1st In 1st Out)*/
    spin_lock_irqsave(&_wqLock, flags);
    list_add_tail(&work->entry, &_workQueueList);
    spin_unlock_irqrestore(&_wqLock, flags);
    up(&_wqSem);

    return ret;
}

static int mmSwitchWorkQueueThreadFunc(void *thread_data)
{
    while (1) {
        int                 ret   = 0;
        unsigned long       flags = 0;
        struct work_struct  *work = NULL;

        /* Attempt to acquire semaphore */
        ret = down_interruptible(&_wqSem);
        if (ret != 0) {
            MMPBX_TRC_DEBUG("signal received, semaphore not acquired...\n");
            continue;
        }

        spin_lock_irqsave(&_wqLock, flags);
        if (!list_empty(&_workQueueList)) {
            work = list_first_entry(&_workQueueList, struct work_struct, entry);
            list_del(&work->entry);
        }
        spin_unlock_irqrestore(&_wqLock, flags);


        if ((work != NULL) && (work->func != NULL)) {
            work->func(work);
        }

        if (_exit) {
            MMPBX_TRC_DEBUG("%s: exited\n", __func__);
            break;
        }
    }

    up(&_exitWqSem);

    do_exit(0);

    return 0;
}

static int mmSwitchReadSocketThreadFunc(void *thread_data)
{
    int                     ret = 0;
    struct kvec             vec;
    struct msghdr           msg;
    MmSwitchSockQueueEntry  *sockQueueEntry = NULL;
    int                     size            = 0;
    unsigned long           flags;
    MmSwitchSocketCb        cb;
    MmConnHndl              mmConn;



    while (1) {
        /* Attempt to acquire semaphore */
        ret = down_interruptible(&_readSem);

        if (ret != 0) {
            MMPBX_TRC_DEBUG("signal received, semaphore not acquired...\n");
            continue;
        }

        if (down_interruptible(&_readThreadSocketActionsMutex) != 0) {
            MMPBX_TRC_DEBUG("signal received, mutex not acquired...\n");
            up(&_readSem);
            continue;
        }

        while (1) {
            sockQueueEntry = NULL;
            spin_lock_irqsave(&_listReadLock, flags);
            if (!list_empty(&_readQueue)) {
                sockQueueEntry = list_first_entry(&_readQueue, MmSwitchSockQueueEntry, list);
                list_del(&sockQueueEntry->list);
            }
            spin_unlock_irqrestore(&_listReadLock, flags);

            if (!sockQueueEntry) {
                break;
            }
            if (_exit) {
                kfree(sockQueueEntry);
                break;
            }

            cb      = sockQueueEntry->mmSwitchSock->cb;
            mmConn  = sockQueueEntry->mmSwitchSock->mmConn;

            if (cb != NULL) {
                memset(&vec, 0, sizeof(struct kvec));
                memset(&msg, 0, sizeof(struct msghdr));
                vec.iov_base  = sockQueueEntry->data;
                vec.iov_len   = sizeof(sockQueueEntry->data);

                size = kernel_recvmsg(sockQueueEntry->mmSwitchSock->sock, &msg, &vec, 1, vec.iov_len, MSG_DONTWAIT);
                if (size > 0) {
                    ret = cb(mmConn, sockQueueEntry->data, &sockQueueEntry->mmSwitchSock->header, size);
                    if (ret != MMPBX_ERROR_NOERROR) {
                        MMPBX_TRC_ERROR("sockQueueEntry->mmSwitchSock->cb failed with error: %d\n", ret);
                    }
                }
            }

            /*Destruct */
            kfree(sockQueueEntry);
        }

        up(&_readThreadSocketActionsMutex);

        if (_exit) {
            MMPBX_TRC_DEBUG("exited\n");
            break;
        }
    }

    up(&_exitReadSem);

    do_exit(0);

    return 0;
}

/**
 *
 */
static int __init mmSwitchInit(void)
{
    MmPbxError  err = MMPBX_ERROR_NOERROR;
    int         ret = 0;

    do {
        spin_lock_init(&_socketActivationLock);

        spin_lock_init(&_listWriteLock);
        spin_lock_init(&_listReadLock);
        spin_lock_init(&_wqLock);

        err = mmSwitchGeNlInit();
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmSwitchGenNlInit failed with: %s\n", mmPbxGetErrorString(ret));
            ret = -1;
            break;
        }

        err = mmConnUsrGeNlInit();
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnUsrGenNlInit failed with: %s\n", mmPbxGetErrorString(ret));
            ret = -1;
            break;
        }

        err = mmConnGeNlInit();
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConngeNlInit failed with: %s\n", mmPbxGetErrorString(ret));
            ret = -1;
            break;
        }
        err = mmConnInit();
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnInit failed with: %s\n", mmPbxGetErrorString(ret));
            ret = -1;
            break;
        }

        err = mmConnUsrInit();
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnUsrInit failed with: %s\n", mmPbxGetErrorString(ret));
            ret = -1;
            break;
        }

        err = mmConnKrnlInit();
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnKrnlInit failed with: %s\n", mmPbxGetErrorString(ret));
            ret = -1;
            break;
        }

        err = mmConnMulticastInit();
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnMulticastInit failed with: %s\n", mmPbxGetErrorString(ret));
            ret = -1;
            break;
        }

        err = mmConnToneInit();
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnToneInit failed with: %s\n", mmPbxGetErrorString(ret));
            ret = -1;
            break;
        }

        err = mmConnRelayInit();
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRelayInit failed with: %s\n", mmPbxGetErrorString(ret));
            ret = -1;
            break;
        }

        err = mmConnRtcpInit();
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRtcpInit failed with: %s\n", mmPbxGetErrorString(ret));
            ret = -1;
            break;
        }

#ifdef MMPBX_DSP_SUPPORT_RTCPXR
        err = mmConnStatsInit();
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnStatsInit failed with: %s\n", mmPbxGetErrorString(ret));
        }
#endif

        /*Create dedicated kernel threads for socket handling*/
        sema_init(&_writeSem, 0);
        sema_init(&_writeThreadSocketActionsMutex, 1);
        _writeThread = kthread_run(mmSwitchWriteSocketThreadFunc, NULL, "mmswitchsock_w");

        if (_writeThread == NULL) {
            MMPBX_TRC_ERROR("kthread_run failed\n");
            BUG();
            break;
        }

        sema_init(&_readSem, 0);
        sema_init(&_readThreadSocketActionsMutex, 1);
        _readThread = kthread_run(mmSwitchReadSocketThreadFunc, NULL, "mmswitchsock_r");
        if (_readThread == NULL) {
            MMPBX_TRC_ERROR("kthread_run failed\n");
            BUG();
            break;
        }

        sema_init(&_wqSem, 0);
        _wqThread = kthread_run(mmSwitchWorkQueueThreadFunc, NULL, "mmswitch_wq");
        if (_wqThread == NULL) {
            MMPBX_TRC_ERROR("kthread_run failed\n");
            BUG();
            break;
        }

        sema_init(&_exitReadSem, 0);
        sema_init(&_exitWriteSem, 0);
        sema_init(&_exitWqSem, 0);

        return ret;
    } while (0);

    return ret;
}

/**
 *
 */
static void __exit mmSwitchExit(void)
{
    MmPbxError              err             = MMPBX_ERROR_NOERROR;
    MmSwitchSockQueueEntry  *sockQueueEntry = NULL;
    struct list_head        *pos, *q;

    MMPBX_TRC_DEBUG("Start mmSwitchExit  \n");

    _exit = 1;

    up(&_readSem);
    while (1) {
        if (down_interruptible(&_exitReadSem) == 0) {
            break;
        }
    }

    up(&_writeSem);
    while (1) {
        if (down_interruptible(&_exitWriteSem) == 0) {
            break;
        }
    }

    up(&_wqSem);
    while (1) {
        if (down_interruptible(&_exitWqSem) == 0) {
            break;
        }
    }

    MMPBX_TRC_DEBUG("Switch Read, Write and WorkQueue processes are terminated\n");

    err = mmSwitchGeNlDeinit();
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("Failed to deinitialize mmswitch communication channel\n");
    }

    err = mmConnGeNlDeinit();
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConngeNlDeInit failed with: %s\n", mmPbxGetErrorString(err));
    }

    err = mmConnUsrGeNlDeinit();
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("Failed to deinitialize mmConnUsr communication channel\n");
    }

    err = mmConnUsrDeinit();
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("Failed to deinitialize mmConnUsr channel\n");
    }

    err = mmConnKrnlDeinit();
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnKrnlDeinit failed with: %s\n", mmPbxGetErrorString(err));
    }

    err = mmConnMulticastDeinit();
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnMulticastDeInit failed with: %s\n", mmPbxGetErrorString(err));
    }

    err = mmConnToneDeinit();
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnToneDeinit failed with: %s\n", mmPbxGetErrorString(err));
    }

    err = mmConnRelayDeinit();
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnRelayDeinit failed with: %s\n", mmPbxGetErrorString(err));
    }

#ifdef MMPBX_DSP_SUPPORT_RTCPXR
    err = mmConnStatsDeinit();
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnStatsDeinit failed with: %s\n", mmPbxGetErrorString(err));
    }
#endif

    err = mmConnRtcpDeinit();
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnRtcpDeinit failed with: %s\n", mmPbxGetErrorString(err));
    }

    list_for_each_safe(pos, q, &_writeQueue)
    {
        sockQueueEntry = list_entry(pos, MmSwitchSockQueueEntry, list);

        /*Remove from list with all MmSwitchSockQueueEntries structures */
        list_del(&sockQueueEntry->list);
        /*Destruct */
        kfree(sockQueueEntry);
    }

    list_for_each_safe(pos, q, &_readQueue)
    {
        sockQueueEntry = list_entry(pos, MmSwitchSockQueueEntry, list);

        /*Remove from list with all MmSwitchSockQueueEntries structures */
        list_del(&sockQueueEntry->list);
        /*Destruct */
        kfree(sockQueueEntry);
    }

    MMPBX_TRC_DEBUG("mmSwitchExit successfully\n");
}

/*GPL license is needed to create dicated workqueue */
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jurgen Schoeters <jurgen.schoeters@technicolor.com>");
module_init(mmSwitchInit);
module_exit(mmSwitchExit);
