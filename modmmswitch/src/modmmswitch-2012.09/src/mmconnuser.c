/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/** \file
 * Multimedia switch user connection API.
 *
 * A user connection is a source/sink of a multimedia stream in user space.
 *
 * \version v1.0
 *
 *************************************************************************/


/*
 * Define tracing prefix, needs to be defined before includes.
 */
#define MODULE_NAME    "MMCONNUSR"
/*########################################################################
#                                                                        #
#  HEADER (INCLUDE) SECTION                                              #
#                                                                        #
########################################################################*/
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/kfifo.h>

#include "mmconnuser_p.h"
#include "mmconnuser_netlink_p.h"
#include "mmswitch_p.h"
#include "mmconn_p.h"
/*########################################################################
#                                                                        #
#  MACROS/DEFINES                                                        #
#                                                                        #
########################################################################*/
#define MMCONNUSR_FIFO_SIZE    (2 * PAGE_SIZE)

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

static int          _traceLevel = MMPBX_TRACELEVEL_NONE;
static struct mutex _mutex;
static int          _initialised = FALSE;

/*########################################################################
#                                                                       #
#  PRIVATE FUNCTION PROTOTYPES                                          #
#                                                                       #
########################################################################*/

static int isInitialised(void);

static MmPbxError mmConnChildWriteCb(MmConnHndl         conn,
                                     MmConnPacketHeader *header,
                                     uint8_t            *buff,
                                     unsigned int       bytes);
static MmPbxError mmConnChildDestructCb(MmConnHndl mmConn);

/*########################################################################
#                                                                        #
#  FUNCTION DEFINITIONS                                                  #
#                                                                        #
########################################################################*/

/**
 *
 */
MmPbxError mmConnUsrInit(void)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    /* Add data mutex */
    mutex_init(&_mutex);

    _initialised = TRUE;

    return ret;
}

/**
 *
 */
MmPbxError mmConnUsrDeinit(void)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    _initialised = FALSE;

    return ret;
}

/**
 *
 */
MmPbxError mmConnUsrSetTraceLevel(MmPbxTraceLevel level)
{
    MmPbxError err = MMPBX_ERROR_NOERROR;

    _traceLevel = level;
    MMPBX_TRC_INFO("New trace level : %s\n", mmPbxGetTraceLevelString(level));

    err = mmConnUsrGeNlSetTraceLevel(level);
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnUsrGeNlSetTraceLevel failed with: %s\n", mmPbxGetErrorString(err));
    }

    return MMPBX_ERROR_NOERROR;
}

/**
 *
 */
MmPbxError mmConnUsrConstruct(MmConnUsrHndl *usr, struct genl_info *info)
{
    MmPbxError    ret           = MMPBX_ERROR_NOERROR;
    MmConnUsrHndl mmConnUsrTemp = NULL;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
    int err = 0;
#endif

    if (isInitialised() == FALSE) {
        ret = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (usr == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    *usr = NULL;

    do {
        /* Try to allocate another mmConnUsr object instance */
        mmConnUsrTemp = kmalloc(sizeof(struct MmConnUsr), GFP_KERNEL);
        if (mmConnUsrTemp == NULL) {
            ret = MMPBX_ERROR_NORESOURCES;
            MMPBX_TRC_ERROR("%s\r\n", mmPbxGetErrorString(ret));
            break;
        }

        memset(mmConnUsrTemp, 0, sizeof(struct MmConnUsr));

        spin_lock_init(&mmConnUsrTemp->fifo_lock);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
        err = kfifo_alloc(&mmConnUsrTemp->fifo, MMCONNUSR_FIFO_SIZE, GFP_KERNEL);
        if (err != 0) {
            ret = MMPBX_ERROR_NORESOURCES;
            MMPBX_TRC_ERROR("kfifo_alloc failed with error: %d\n", err);
            break;
        }
#else
        mmConnUsrTemp->fifo = kfifo_alloc(MMCONNUSR_FIFO_SIZE, GFP_KERNEL, &mmConnUsrTemp->fifo_lock);
#endif

        /* Prepare connection for usage */
        ret = mmConnPrepare((MmConnHndl)mmConnUsrTemp, MMCONN_TYPE_USER);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnPrepare failed with: %s\n", mmPbxGetErrorString(ret));
            break;
        }

        /* Register Child Destruct callback, will be called before destruct of object */
        ret = mmConnRegisterChildDestructCb((MmConnHndl)mmConnUsrTemp, mmConnChildDestructCb);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRegisterChildDestructCb failed with: %s\n", mmPbxGetErrorString(ret));
        }

        /* Register Child write callback, will be called to push data into mmConn */
        ret = mmConnRegisterChildWriteCb((MmConnHndl)mmConnUsrTemp, mmConnChildWriteCb);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRegisterChildWriteCb failed with: %s\n", mmPbxGetErrorString(ret));
            break;
        }

        /* Create dedicated generic netlink family for this user connection */
        ret = mmConnUsrGeNlCreateFam(mmConnUsrTemp, info);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnUsrGeNlCreateFam failed with: %s\n", mmPbxGetErrorString(ret));
            break;
        }
        *usr = mmConnUsrTemp;
        MMPBX_TRC_INFO("mmConnUsr = %p\n", *usr);
    } while (0);


    if (ret != MMPBX_ERROR_NOERROR) {
        if (mmConnUsrTemp != NULL) {
            mmConnDestruct((MmConnHndl *)&mmConnUsrTemp);
        }
    }

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
static int isInitialised(void)
{
    return _initialised;
}

/**
 *
 */
static MmPbxError mmConnChildDestructCb(MmConnHndl mmConn)
{
    MmPbxError    err       = MMPBX_ERROR_NOERROR;
    MmConnUsrHndl mmConnUsr = (MmConnUsrHndl) mmConn;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
    kfifo_free(&mmConnUsr->fifo);
#else
    kfifo_free(mmConnUsr->fifo);
#endif


    return err;
}

/**
 *
 */
static MmPbxError mmConnChildWriteCb(MmConnHndl         conn,
                                     MmConnPacketHeader *header,
                                     uint8_t            *buff,
                                     unsigned int       bytes)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    ret = mmConnUsrGeNlWrite((MmConnUsrHndl)conn, header, buff, bytes);
    if (ret != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnUsrGeNlWrite failed with: %s\n", mmPbxGetErrorString(ret));
    }

    return ret;
}
