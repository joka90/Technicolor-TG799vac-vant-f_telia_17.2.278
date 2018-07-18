/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/** \file
 * Kernel space multimedia switch connection API.
 *
 * A multimedia switch connection is a source/sink of a multimedia stream.
 *
 * \version v1.0
 *
 *************************************************************************/

/*
 * Define tracing prefix, needs to be defined before includes.
 */
#define MODULE_NAME    "MMCONN"
/*########################################################################
#                                                                        #
#  HEADER (INCLUDE) SECTION                                              #
#                                                                        #
########################################################################*/
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/workqueue.h>

#include "mmswitch_p.h"
#include "mmconn_p.h"
#include "mmconn_netlink_p.h"

/*########################################################################
#                                                                        #
#  MACROS/DEFINES                                                        #
#                                                                        #
########################################################################*/


/*########################################################################
#                                                                        #
#  TYPES                                                                 #
#                                                                        #
########################################################################*/


/*########################################################################
#                                                                        #
#  PRIVATE DATA MEMBERS                                                  #
#                                                                        #
########################################################################*/

static int  _traceLevel   = MMPBX_TRACELEVEL_ERROR;
static int  _initialised  = FALSE;

/*########################################################################
#                                                                       #
#  PRIVATE FUNCTION PROTOTYPES                                          #
#                                                                       #
########################################################################*/
static void objLock(MmConnHndl conn);
static void objRelease(MmConnHndl conn);
static int isInitialised(void);
static int hasTargets(MmConnHndl mmConn);
static int isTarget(MmConnHndl  mmConn,
                    MmConnHndl  target);
static MmPbxError mmConnRemoveTarget(MmConnHndl mmConn,
                                     MmConnHndl target);
static MmPbxError mmConnAddTarget(MmConnHndl  mmConn,
                                  MmConnHndl  target);

#ifndef NDEBUG
static int isValidMmConn(MmConnHndl mmConn);
#endif /*NDEBUG*/

/*########################################################################
#                                                                        #
#  FUNCTION DEFINITIONS                                                  #
#                                                                        #
########################################################################*/


/**
 *
 */
MmPbxError mmConnInit(void)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    _initialised = TRUE;
    return ret;
}

/**
 *
 */
MmPbxError mmConnSetTraceLevel(MmPbxTraceLevel level)
{
    _traceLevel = level;
    MMPBX_TRC_INFO("New trace level : %s\n", mmPbxGetTraceLevelString(level));


    return MMPBX_ERROR_NOERROR;
}

/**
 *
 */
MmPbxError mmConnPrepare(MmConnHndl mmConn,
                         MmConnType type)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    if (isInitialised() == FALSE) {
        ret = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(ret));
        return ret;
    }

    /* Initialise mmConn specific lock */
    spin_lock_init(&mmConn->lock);

    ret = mmConnGeNlPrepare(mmConn);
    if (ret != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnGeNlPrepare failed with error: %s\n", mmPbxGetErrorString(ret));
        return ret;
    }



    mmConn->type    = type;
    mmConn->target  = NULL;
    mmConn->control = NULL;
    mmConn->mmConnChildDestructCb = NULL;
    mmConn->mmConnChildXConnCb    = NULL;
    mmConn->mmConnChildDiscCb     = NULL;
    mmConn->mmConnChildWriteCb    = NULL;
    mmConn->mmConnWriteCb         = NULL;
    mmConn->mmConnDspCtrlCb       = NULL;
    mmConn->stats.ingressRtpPt    = -1;
#ifndef NDEBUG
    mmConn->magic = MMPBX_MAGIC_NR;
#endif /*NDEBUG*/

    return ret;
}

/**
 *
 */
MmPbxError mmConnXConn(MmConnHndl source,
                       MmConnHndl target)
{
    MmPbxError err = MMPBX_ERROR_NOERROR;

    if (isInitialised() == FALSE) {
        err = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    if (source == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    if (target == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    MMPBX_TRC_INFO("source = %lx, target = %lx\n", (unsigned long int)source, (unsigned long int)target);

    do {
        /* Update source */
        do {
            objLock(source);

#ifndef NDEBUG
            if (isValidMmConn(source) == FALSE) {
                err = MMPBX_ERROR_INTERNALERROR;
                MMPBX_TRC_ERROR("invalid MmConn\n");
                break;
            }
#endif      /*NDEBUG */

            /* Create cross-connection */
            err = mmConnAddTarget(source, target);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("mmConnAddTarget failed with: %s\n", mmPbxGetErrorString(err));
                err = MMPBX_ERROR_INTERNALERROR;
                break;
            }
        } while (0);

        objRelease(source);
        if (err != MMPBX_ERROR_NOERROR) {
            break;
        }

        /* Update target */
        do {
            objLock(target);

#ifndef NDEBUG
            if (isValidMmConn(target) == FALSE) {
                err = MMPBX_ERROR_INTERNALERROR;
                MMPBX_TRC_ERROR("invalid MmConn\n");
                break;
            }
#endif      /*NDEBUG */

            /* Create cross-connection */
            mmConnAddTarget(target, source);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("mmConnAddTarget failed with: %s\n", mmPbxGetErrorString(err));
                err = MMPBX_ERROR_INTERNALERROR;
                break;
            }
        } while (0);

        objRelease(target);
        if (err != MMPBX_ERROR_NOERROR) {
            break;
        }

        /*
         * Perform child specific actions.
         * This can't be done when locked because can cause deadlock.
         */
        if (source->mmConnChildXConnCb != NULL) {
            source->mmConnChildXConnCb(source);
        }

        if ((target->mmConnChildXConnCb != NULL) && (source != target)) {
            target->mmConnChildXConnCb(target);
        }
        /*
         * Now cross-connect the control connections, if possible!
         */
        if ((source->control != NULL) && (target->control != NULL)) {
            MMPBX_TRC_INFO("Cross-connecting Control connection source: %p, target: %p\n", (void *)source->control, (void *)target->control);
            err = mmConnXConn(source->control, target->control);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("Error cross-connecting control connections %p and %p: %s\n", (void *)source->control, (void *)target->control, mmPbxGetErrorString(err));
                break;
            }
        }
    } while (0);

    return err;
}

/**
 *
 */
MmPbxError mmConnDisc(MmConnHndl conn)
{
    MmPbxError  err = MMPBX_ERROR_NOERROR;
    MmConnHndl  target;

    if (isInitialised() == FALSE) {
        err = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    if (conn == NULL) {
        err = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }

    do {
        objLock(conn);
#ifndef NDEBUG
        if (isValidMmConn(conn) == FALSE) {
            err = MMPBX_ERROR_INTERNALERROR;
            MMPBX_TRC_ERROR("invalid MmConn\n");
            objRelease(conn);
            break;
        }
#endif  /*NDEBUG */
        if (hasTargets(conn) == FALSE) {
            /*Connection has no targets, so already disconnected*/
            objRelease(conn);
            break;
        }

        target = conn->target;
        MMPBX_TRC_INFO("mmConn = %lx target = %lx\n", (unsigned long int)conn, (unsigned long int)target);
        /*Disconnect*/
        err = mmConnRemoveTarget(conn, target);
        objRelease(conn);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRemoveTarget failed with: %s\n", mmPbxGetErrorString(err));
            err = MMPBX_ERROR_INTERNALERROR;
            break;
        }

        objLock(target);
#ifndef NDEBUG
        if (isValidMmConn(target) == FALSE) {
            err = MMPBX_ERROR_INTERNALERROR;
            MMPBX_TRC_ERROR("invalid MmConn\n");
            objRelease(target);
            break;
        }
#endif  /*NDEBUG */
        err = mmConnRemoveTarget(target, conn);
        objRelease(target);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRemoveTarget failed with: %s\n", mmPbxGetErrorString(err));
            err = MMPBX_ERROR_INTERNALERROR;
            break;
        }

        /*
         * Perform child specific actions.
         * This can't be done when locked because can cause deadlock.
         */
        if (conn->mmConnChildDiscCb != NULL) {
            conn->mmConnChildDiscCb(conn);
        }

        if ((target->mmConnChildDiscCb != NULL) && (conn != target)) {
            target->mmConnChildDiscCb(target);
        }

        /*
         * Disconnect the control connection, if needed.
         */
        if (conn->control != NULL) {
            MMPBX_TRC_INFO("Disconnecting Control connection %p\n", (void *) conn->control);
            err = mmConnDisc(conn->control);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("Error disconnecting control connection %p: %s\n", conn->control, mmPbxGetErrorString(err));
                break;
            }
        }
    } while (0);

    return err;
}

/**
 *
 */
MmPbxError mmConnDestruct(MmConnHndl *mmConn)
{
    MmPbxError  ret           = MMPBX_ERROR_NOERROR;
    MmConnHndl  mmConnOld     = NULL;
    MmConnHndl  mmControlConn = NULL;

    if (isInitialised() == FALSE) {
        ret = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (*mmConn == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (hasTargets(*mmConn) == TRUE) {
        ret = mmConnDisc(*mmConn);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnDisc failed with error: %s\n", mmPbxGetErrorString(ret));
        }
    }

    /*
     * Perform child specific cleanup.
     * This can't be done when locked because can cause deadlock.
     */
    if ((*mmConn)->mmConnChildDestructCb != NULL) {
        (*mmConn)->mmConnChildDestructCb(*mmConn);
    }

    ret = mmConnGeNlDestroy(*mmConn);
    if (ret != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnGeNlDestroy failed with error: %s\n", mmPbxGetErrorString(ret));
    }

    objLock(*mmConn);

#ifndef NDEBUG
    if (isValidMmConn(*mmConn) == FALSE) {
        ret = MMPBX_ERROR_INTERNALERROR;
        MMPBX_TRC_ERROR("invalid MmConn\n");
        objRelease(*mmConn);
        return ret;
    }
#endif /*NDEBUG */

    /* Unregister child specific actions */
    (*mmConn)->mmConnChildWriteCb = NULL;
    (*mmConn)->mmConnChildXConnCb = NULL;
    (*mmConn)->mmConnChildDiscCb  = NULL;

    /*Unregister write callback of endpoint */
    (*mmConn)->mmConnWriteCb = NULL;

    /* Unregister DSP control callback */
    (*mmConn)->mmConnDspCtrlCb = NULL;

    mmControlConn = (*mmConn)->control;

#ifndef NDEBUG
    (*mmConn)->magic = 0;
#endif /*NDEBUG*/

    /*Unregister child destruct cb */
    (*mmConn)->mmConnChildDestructCb = NULL;

    MMPBX_TRC_INFO("mmConn = %lx\n", (unsigned long int) *mmConn);

    mmConnOld = *mmConn;
    *mmConn   = NULL;

    objRelease(mmConnOld);

    /* Destroy control connection. */
    if (mmControlConn != NULL) {
        MMPBX_TRC_INFO("Destructing Control connection %p\n", mmControlConn);
        ret = mmConnDestruct(&mmControlConn);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("Error destroying control connection %p: %s\n", mmControlConn, mmPbxGetErrorString(ret));
            return ret;
        }
    }

    /* Delete the connection */
    kfree(mmConnOld);

    return ret;
}

/**
 *
 */
MmPbxError mmConnRegisterChildDestructCb(MmConnHndl             mmConn,
                                         MmConnChildDestructCb  cb)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    objLock(mmConn);
    mmConn->mmConnChildDestructCb = cb;
    objRelease(mmConn);

    return ret;
}

/**
 *
 */
MmPbxError mmConnRegisterChildDiscCb(MmConnHndl         mmConn,
                                     MmConnChildDiscCb  cb)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    objLock(mmConn);
    mmConn->mmConnChildDiscCb = cb;
    objRelease(mmConn);

    return ret;
}

/**
 *
 */
MmPbxError mmConnRegisterChildXConnCb(MmConnHndl          mmConn,
                                      MmConnChildXConnCb  cb)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    objLock(mmConn);
    mmConn->mmConnChildXConnCb = cb;
    objRelease(mmConn);

    return ret;
}

/**
 *
 */
MmPbxError mmConnRegisterDspCtrlCb(MmConnHndl       mmConn,
                                   MmConnDspCtrlCb  cb)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    objLock(mmConn);
    mmConn->mmConnDspCtrlCb = cb;
    objRelease(mmConn);

    return ret;
}

/**
 * Push data into mmConn.
 */
MmPbxError mmConnWrite(MmConnHndl         conn,
                       MmConnPacketHeader *header,
                       uint8_t            *buff,
                       unsigned int       bytes)
{
    MmPbxError  ret = MMPBX_ERROR_NOERROR;
    MmConnEvent event;
    MmConnHndl  target  = NULL;
    int         pt      = 0;

    if (isInitialised() == FALSE) {
        ret = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (conn == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (header == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (buff == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    objLock(conn);
#ifndef NDEBUG
    if (isValidMmConn(conn) == FALSE) {
        ret = MMPBX_ERROR_INTERNALERROR;
        MMPBX_TRC_ERROR("invalid MmConn\n");
        objRelease(conn);
        return ret;
    }
#endif /*NDEBUG */

    if (hasTargets(conn) == FALSE) {
        /* Ignore if connection is not cross-connected */
        ret = MMPBX_ERROR_NOERROR;
        objRelease(conn);
        return ret;
    }

    /*Send Event*/
    if (header->type == MMCONN_PACKET_TYPE_RTP) {
        if (bytes < sizeof(MmConnRtpHeader)) {
            /* Ignore if to small */
            ret = MMPBX_ERROR_NOERROR;
            objRelease(conn);
            return ret;
        }
        pt = ((MmConnRtpHeader *)buff)->pt;
        /* Ignore comfort noise (pt 13: comfort noise) */
        if (pt != 13) {
            /* Only send event if payload type changed */
            if (conn->stats.ingressRtpPt != pt) {
                event.type  = MMCONN_EV_INGRESS_RTP_MEDIA;
                event.parm  = ((MmConnRtpHeader *)buff)->pt;

                /*Send event to user space*/
                ret = mmConnSendEvent(conn, &event);
                if (ret != MMPBX_ERROR_NOERROR) {
                    ret = MMPBX_ERROR_INTERNALERROR;
                    MMPBX_TRC_ERROR("mmConnSendEvent failed with error: %s\n", mmPbxGetErrorString(ret));
                }
            }
            /*Save Ingress RTP payload type */
            conn->stats.ingressRtpPt = pt;
        }
    }

    target = conn->target;
    objRelease(conn);

    /*Call child specific write function, to push data into mmConn */

    /* The following code is not safe
     * we never lock target, so it may already be destructed
     * as we release conn, it may also cause disconnection
     * By adding these extar check can't 100% avoid the above condition, but lock target may introduce a lot of problem.
     * So take Jurgen's suggestion to add this
     */
    if ((target != NULL) && (target->mmConnChildWriteCb != NULL) && isTarget(target, conn)) {
        target->mmConnChildWriteCb(target, header, buff, bytes);
    }
    return ret;
}

/**
 *
 */
MmPbxError mmConnSetCookie(MmConnHndl conn,
                           void       *cookie)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    if (isInitialised() == FALSE) {
        ret = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (conn == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(ret));
        return ret;
    }

    /* We allow NULL as cookie */

    objLock(conn);
#ifndef NDEBUG
    if (isValidMmConn(conn) == FALSE) {
        ret = MMPBX_ERROR_INTERNALERROR;
        MMPBX_TRC_ERROR("invalid MmConn\n");
        objRelease(conn);
        return ret;
    }
#endif /*NDEBUG */
    conn->cookie = cookie;
    objRelease(conn);

    return ret;
}

/**
 *
 */
MmPbxError mmConnGetCookie(MmConnHndl conn,
                           void       **cookie)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    if (isInitialised() == FALSE) {
        ret = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (conn == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (cookie == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(ret));
        return ret;
    }

    objLock(conn);
#ifndef NDEBUG
    if (isValidMmConn(conn) == FALSE) {
        ret = MMPBX_ERROR_INTERNALERROR;
        MMPBX_TRC_ERROR("invalid MmConn\n");
        objRelease(conn);
        return ret;
    }
#endif /*NDEBUG */
    memcpy(cookie, &conn->cookie, sizeof(void *));
    objRelease(conn);

    return ret;
}

/*
 *
 */
MmPbxError mmConnSetConnControl(MmConnHndl  mmConn,
                                MmConnHndl  conn)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    objLock(mmConn);
    mmConn->control = conn;
    objRelease(mmConn);

    return ret;
}

/*
 *
 */
MmPbxError mmConnGetConnControl(MmConnHndl  mmConn,
                                MmConnHndl  *conn)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    objLock(mmConn);
    *conn = mmConn->control;
    objRelease(mmConn);

    return ret;
}

/*
 * Forward command to the DSP callback handler
 */
MmPbxError mmConnDspControl(MmConnHndl        mmConn,
                            void              *param,
                            MmConnDspCommand  cmd)
{
    MmPbxError  ret     = MMPBX_ERROR_NOERROR;
    MmConnHndl  target  = NULL;

    if (isInitialised() == FALSE) {
        ret = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (mmConn == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    objLock(mmConn);
#ifndef NDEBUG
    if (isValidMmConn(mmConn) == FALSE) {
        ret = MMPBX_ERROR_INTERNALERROR;
        MMPBX_TRC_ERROR("invalid MmConn\n");
        objRelease(mmConn);
        return ret;
    }
#endif /*NDEBUG */

    if (hasTargets(mmConn) == FALSE) {
        /* Ignore if connection is not cross-connected */
        ret = MMPBX_ERROR_INVALIDPARAMS;
        objRelease(mmConn);
        return ret;
    }
    target = mmConn->target;
    objRelease(mmConn);

    if (target->type != MMCONN_TYPE_KERNEL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_INFO("DSP statistics only available on Kernel connection: %s\n\r", mmPbxGetErrorString(ret));
        return ret;
    }

    if ((target != NULL) && (target->mmConnDspCtrlCb != NULL) && isTarget(target, mmConn)) {
        ret = target->mmConnDspCtrlCb(target, param, cmd);
    }
    return ret;
}

/**
 * Register child specific implementation, to push data.
 */
MmPbxError mmConnRegisterChildWriteCb(MmConnHndl          mmConn,
                                      MmConnChildWriteCb  cb)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    objLock(mmConn);
    mmConn->mmConnChildWriteCb = cb;
    objRelease(mmConn);

    return ret;
}

/**
 *  Handle to push data into endpoint.
 */
MmPbxError mmConnRegisterWriteCb(MmConnHndl     conn,
                                 MmConnWriteCb  cb)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    objLock(conn);
    conn->mmConnWriteCb = cb;
    objRelease(conn);

    return ret;
}

/**
 * Handle to push data into endpoint.
 */
MmPbxError mmConnUnregisterWriteCb(MmConnHndl     conn,
                                   MmConnWriteCb  cb)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    objLock(conn);
    conn->mmConnWriteCb = NULL;
    objRelease(conn);

    return ret;
}

/*########################################################################
#                                                                        #
#   EXPORTS                                                              #
#                                                                        #
########################################################################*/
EXPORT_SYMBOL(mmConnSetConnControl);
EXPORT_SYMBOL(mmConnGetConnControl);
EXPORT_SYMBOL(mmConnSetCookie);
EXPORT_SYMBOL(mmConnGetCookie);
EXPORT_SYMBOL(mmConnWrite);
EXPORT_SYMBOL(mmConnRegisterWriteCb);
EXPORT_SYMBOL(mmConnUnregisterWriteCb);
EXPORT_SYMBOL(mmConnRegisterDspCtrlCb);

/*########################################################################
#                                                                        #
#   PRIVATE FUNCTION DEFINITIONS                                         #
#                                                                        #
########################################################################*/

/**
 *
 */
static int isInitialised(void)
{
    return _initialised;
}

/**
 *
 */
static void objLock(MmConnHndl conn)
{
    spin_lock_irqsave(&conn->lock, conn->flags);
}

/**
 *
 */
static void objRelease(MmConnHndl conn)
{
    spin_unlock_irqrestore(&conn->lock, conn->flags);
}

/**
 *
 */
static int hasTargets(MmConnHndl mmConn)
{
    int ret = TRUE;

    if (mmConn->target == NULL) {
        ret = FALSE;
    }

    return ret;
}

/**
 *
 */
static int isTarget(MmConnHndl  mmConn,
                    MmConnHndl  target)
{
    int ret = TRUE;

    if (mmConn->target != target) {
        ret = FALSE;
    }

    return ret;
}

/**
 *
 */
static MmPbxError mmConnAddTarget(MmConnHndl  mmConn,
                                  MmConnHndl  target)
{
    if (isTarget(mmConn, target) == FALSE) {
        mmConn->target = target;
    }
    return MMPBX_ERROR_NOERROR;
}

/**
 *
 */
static MmPbxError mmConnRemoveTarget(MmConnHndl mmConn,
                                     MmConnHndl target)
{
    if (isTarget(mmConn, target) == TRUE) {
        mmConn->target = NULL;
    }
    return MMPBX_ERROR_NOERROR;
}

#ifndef NDEBUG
static int isValidMmConn(MmConnHndl mmConn)
{
    if ((mmConn->magic) == MMPBX_MAGIC_NR) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}
#endif /*NDEBUG */
