/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/** \file
 * Multimedia switch kernel connection API.
 *
 * A kernel connection is a source/sink of a multimedia stream in kernel space.
 *
 * \version v1.0
 *
 *************************************************************************/


/*
 * Define tracing prefix, needs to be defined before includes.
 */
#define MODULE_NAME    "MMCONNKRNL"
/*########################################################################
#                                                                        #
#  HEADER (INCLUDE) SECTION                                              #
#                                                                        #
########################################################################*/
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/module.h>

#include "mmconnkernel_p.h"
#include "mmswitch_p.h"
#include "mmconn_p.h"
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
typedef struct {
    MmConnKrnlEventCb cb;
    struct list_head  list;
} MmConnKrnlEventCbEntry;

/*########################################################################
#                                                                        #
#  PRIVATE DATA MEMBERS                                                  #
#                                                                        #
########################################################################*/

static int          _traceLevel = MMPBX_TRACELEVEL_NONE;
static struct mutex _mutex;
static int          _initialised = FALSE;
static LIST_HEAD(mmConnKrnlEventCbList);

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
static MmPbxError mmConnChildXConnCb(MmConnHndl mmConn);
static MmPbxError mmConnChildDiscCb(MmConnHndl mmConn);
static MmPbxError constructKernelConnection(MmConnKrnlConfig  *config,
                                            MmConnKrnlHndl    *krnl);

/*########################################################################
#                                                                        #
#  FUNCTION DEFINITIONS                                                  #
#                                                                        #
########################################################################*/

/**
 *
 */
MmPbxError mmConnKrnlInit(void)
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
MmPbxError mmConnKrnlDeinit(void)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    MmConnKrnlEventCbEntry  *entry = NULL;
    struct list_head        *pos, *q;

    list_for_each_safe(pos, q, &mmConnKrnlEventCbList)
    {
        entry = list_entry(pos, MmConnKrnlEventCbEntry, list);
        list_del(pos);
        kfree(entry);
    }

    _initialised = FALSE;

    return ret;
}

/**
 *
 */
MmPbxError mmConnKrnlSetTraceLevel(MmPbxTraceLevel level)
{
    _traceLevel = level;
    MMPBX_TRC_INFO("New trace level : %s\n", mmPbxGetTraceLevelString(level));


    return MMPBX_ERROR_NOERROR;
}

/**
 *
 */
MmPbxError mmConnKrnlConstruct(MmConnKrnlConfig *config,
                               MmConnKrnlHndl   *krnl)
{
    MmPbxError      ret             = MMPBX_ERROR_NOERROR;
    MmConnKrnlHndl  mmConnKrnlTemp  = NULL;

#ifdef MMPBX_DSP_SUPPORT_RTCPXR
    MmConnKrnlHndl mmConnCtrlKrnlTemp = NULL;
#endif

    do {
        /* Create main Linux Kernel connection */
        config->isControlConn = false;
        ret = constructKernelConnection(config, &mmConnKrnlTemp);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(ret));
            break;
        }

#ifdef MMPBX_DSP_SUPPORT_RTCPXR
        config->isControlConn = true;
        ret = constructKernelConnection(config, &mmConnCtrlKrnlTemp);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(ret));
            break;
        }
        ret = mmConnSetConnControl((MmConnHndl)mmConnKrnlTemp, (MmConnHndl)mmConnCtrlKrnlTemp);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(ret));
            break;
        }
#endif
        *krnl = mmConnKrnlTemp;
    } while (0);

    if (ret != MMPBX_ERROR_NOERROR) {
        if (mmConnKrnlTemp != NULL) {
            mmConnDestruct((MmConnHndl *)&mmConnKrnlTemp);
        }
#ifdef MMPBX_DSP_SUPPORT_RTCPXR
        if (mmConnCtrlKrnlTemp != NULL) {
            mmConnDestruct((MmConnHndl *)&mmConnCtrlKrnlTemp);
        }
#endif
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(ret));
    }

    return ret;
}

/**
 *
 */
MmPbxError mmConnKrnlRegisterEventCb(MmConnKrnlEventCb cb)
{
    MmPbxError              err     = MMPBX_ERROR_NOERROR;
    MmConnKrnlEventCbEntry  *entry  = NULL;

    entry = (MmConnKrnlEventCbEntry *) kmalloc(sizeof(MmConnKrnlEventCbEntry), GFP_KERNEL);
    if (entry == NULL) {
        err = MMPBX_ERROR_NORESOURCES;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }
    entry->cb = cb;
    list_add_tail(&entry->list, &mmConnKrnlEventCbList);


    return err;
}

/**
 *
 */
MmPbxError mmConnKrnlUnregisterEventCb(MmConnKrnlEventCb cb)
{
    MmPbxError              err = MMPBX_ERROR_NOERROR;
    MmConnKrnlEventCbEntry  *entry;
    struct list_head        *pos, *q;

    list_for_each_safe(pos, q, &mmConnKrnlEventCbList)
    {
        entry = list_entry(pos, MmConnKrnlEventCbEntry, list);
        if (entry && (entry->cb == cb)) {
            list_del(pos);
            kfree(entry);
        }
    }

    return err;
}

/**
 *
 */
const char * mmConnKrnlEventString(MmConnKrnlEventType eventType)
{
    switch (eventType) {
        case MMCONNKRNL_EVENT_CONSTRUCT:
            return "EVENT_CONSTRUCT";

        case MMCONNKRNL_EVENT_CTRL_CONSTRUCT:
            return "EVENT_CTRL_CONSTRUCT";

        case MMCONNKRNL_EVENT_DESTRUCT:
            return "EVENT_DESTRUCT";

        case MMCONNKRNL_EVENT_XCONN:
            return "EVENT_XCONN";

        case MMCONNKRNL_EVENT_DISC:
            return "EVENT_DISC";
    }

    return "Uknown_Event";
}

/*########################################################################
#                                                                        #
#   EXPORTS                                                              #
#                                                                        #
########################################################################*/
EXPORT_SYMBOL(mmConnKrnlRegisterEventCb);
EXPORT_SYMBOL(mmConnKrnlUnregisterEventCb);
EXPORT_SYMBOL(mmConnKrnlEventString);

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
static MmPbxError mmConnChildWriteCb(MmConnHndl         conn,
                                     MmConnPacketHeader *header,
                                     uint8_t            *buff,
                                     unsigned int       bytes)
{
    MmPbxError err = MMPBX_ERROR_NOERROR;

    if (conn->mmConnWriteCb != NULL) {
        err = conn->mmConnWriteCb(conn, buff, header, bytes);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnWriteCb failed with: %s\n", mmPbxGetErrorString(err));
        }
    }

    return err;
}

/**
 *
 */
static MmPbxError mmConnChildDestructCb(MmConnHndl mmConn)
{
    MmPbxError              err         = MMPBX_ERROR_NOERROR;
    MmConnKrnlHndl          mmConnKrnl  = (MmConnKrnlHndl) mmConn;
    MmConnKrnlEventCbEntry  *entry      = NULL;
    struct list_head        *pos        = NULL;

    /*Call registerd callbacks to notify construction of connection */
    list_for_each(pos, &mmConnKrnlEventCbList)
    {
        entry = list_entry(pos, MmConnKrnlEventCbEntry, list);
        if (entry) {
            entry->cb(MMCONNKRNL_EVENT_DESTRUCT, mmConnKrnl->config.endpointId, mmConnKrnl);
        }
    }

    return err;
}

/**
 *
 */
static MmPbxError mmConnChildXConnCb(MmConnHndl mmConn)
{
    MmPbxError              err         = MMPBX_ERROR_NOERROR;
    MmConnKrnlHndl          mmConnKrnl  = (MmConnKrnlHndl) mmConn;
    MmConnKrnlEventCbEntry  *entry      = NULL;
    struct list_head        *pos        = NULL;

    /*Call registered callbacks to notify cross-connect of connection */
    list_for_each(pos, &mmConnKrnlEventCbList)
    {
        entry = list_entry(pos, MmConnKrnlEventCbEntry, list);
        if (entry) {
            entry->cb(MMCONNKRNL_EVENT_XCONN, mmConnKrnl->config.endpointId, mmConnKrnl);
        }
    }

    return err;
}

/**
 *
 */
static MmPbxError mmConnChildDiscCb(MmConnHndl mmConn)
{
    MmPbxError              err         = MMPBX_ERROR_NOERROR;
    MmConnKrnlHndl          mmConnKrnl  = (MmConnKrnlHndl) mmConn;
    MmConnKrnlEventCbEntry  *entry      = NULL;
    struct list_head        *pos        = NULL;

    /*Call registered callbacks to notify cross-connect of connection */
    list_for_each(pos, &mmConnKrnlEventCbList)
    {
        entry = list_entry(pos, MmConnKrnlEventCbEntry, list);
        if (entry) {
            entry->cb(MMCONNKRNL_EVENT_DISC, mmConnKrnl->config.endpointId, mmConnKrnl);
        }
    }

    return err;
}

/**
 *
 */
static MmPbxError constructKernelConnection(MmConnKrnlConfig  *config,
                                            MmConnKrnlHndl    *krnl)
{
    MmPbxError              ret             = MMPBX_ERROR_NOERROR;
    MmConnKrnlHndl          mmConnKrnlTemp  = NULL;
    MmConnKrnlEventCbEntry  *entry          = NULL;
    struct list_head        *pos;

    if (isInitialised() == FALSE) {
        ret = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (config == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (krnl == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    *krnl = NULL;


    do {
        /* Try to allocate another mmConnkrnl object instance */
        mmConnKrnlTemp = kmalloc(sizeof(struct MmConnKrnl), GFP_KERNEL);
        if (mmConnKrnlTemp == NULL) {
            ret = MMPBX_ERROR_NORESOURCES;
            MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(ret));
            break;
        }

        memset(mmConnKrnlTemp, 0, sizeof(struct MmConnKrnl));

        /* Prepare connection for usage */
        ret = mmConnPrepare((MmConnHndl)mmConnKrnlTemp, MMCONN_TYPE_KERNEL);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnPrepare failed with: %s\n", mmPbxGetErrorString(ret));
            break;
        }

        /* Register Child Destruct callback, will be called before destruct of object */
        ret = mmConnRegisterChildDestructCb((MmConnHndl)mmConnKrnlTemp, mmConnChildDestructCb);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRegisterChildDestructCb failed with: %s\n", mmPbxGetErrorString(ret));
        }

        /* Register Child cross-connect callback, will be called after cross-connect */
        ret = mmConnRegisterChildXConnCb((MmConnHndl)mmConnKrnlTemp, mmConnChildXConnCb);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRegisterChildXConnCb failed with: %s\n", mmPbxGetErrorString(ret));
        }

        /* Register Child disconnect callback, will be called after disconnect */
        ret = mmConnRegisterChildDiscCb((MmConnHndl)mmConnKrnlTemp, mmConnChildDiscCb);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRegisterChildDiscCb failed with: %s\n", mmPbxGetErrorString(ret));
        }

        /* Store endpoint id */
        mmConnKrnlTemp->config.endpointId     = config->endpointId;
        mmConnKrnlTemp->config.isControlConn  = config->isControlConn;

        /* Register Child write callback, will be called to push data into mmConn */
        ret = mmConnRegisterChildWriteCb((MmConnHndl)mmConnKrnlTemp, mmConnChildWriteCb);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRegisterChildWriteCb failed with: %s\n", mmPbxGetErrorString(ret));
            break;
        }

        *krnl = mmConnKrnlTemp;
        MMPBX_TRC_INFO("mmConnKrnl = %p\n", *krnl);
    } while (0);


    if (ret != MMPBX_ERROR_NOERROR) {
        if (mmConnKrnlTemp != NULL) {
            mmConnDestruct((MmConnHndl *)&mmConnKrnlTemp);
        }
    }
    else {
        /*Call registerd callbacks to notify construction of connection */
        list_for_each(pos, &mmConnKrnlEventCbList)
        {
            MmConnKrnlEventType eventType = MMCONNKRNL_EVENT_CONSTRUCT;

            entry = list_entry(pos, MmConnKrnlEventCbEntry, list);
            if (config->isControlConn == true) {
                eventType = MMCONNKRNL_EVENT_CTRL_CONSTRUCT;
            }
            if (entry) {
                entry->cb(eventType, (*krnl)->config.endpointId, *krnl);
            }
        }
    }

    return ret;
}
