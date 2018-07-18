/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/** \file
 * Multimedia switch multicast connection API.
 *
 * A multicast connection can be used to multicast a source stream to multiple multicast connection sinks.
 *
 * \version v1.0
 *
 *************************************************************************/


/*
 * Define tracing prefix, needs to be defined before includes.
 */
#define MODULE_NAME    "MMCONNMULTICAST"
/*########################################################################
#                                                                        #
#  HEADER (INCLUDE) SECTION                                              #
#                                                                        #
########################################################################*/
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/module.h>

#include "mmconnmulticast_p.h"
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
    MmConnMulticastHndl sink;
    struct list_head    list;
} MmConnMulticastSinkEntry;

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
MmPbxError mmConnMulticastInit(void)
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
MmPbxError mmConnMulticastDeinit(void)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    _initialised = FALSE;

    return ret;
}

/**
 *
 */
MmPbxError mmConnMulticastSetTraceLevel(MmPbxTraceLevel level)
{
    _traceLevel = level;
    MMPBX_TRC_INFO("New trace level : %s\n", mmPbxGetTraceLevelString(level));


    return MMPBX_ERROR_NOERROR;
}

/**
 *
 */
MmPbxError mmConnMulticastConstruct(MmConnMulticastHndl *multicast)
{
    MmPbxError          ret = MMPBX_ERROR_NOERROR;
    MmConnMulticastHndl mmConnMulticastTemp = NULL;

    if (isInitialised() == FALSE) {
        ret = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (multicast == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    *multicast = NULL;


    do {
        /* Try to allocate another mmConnmulticast object instance */
        mmConnMulticastTemp = kmalloc(sizeof(struct MmConnMulticast), GFP_KERNEL);
        if (mmConnMulticastTemp == NULL) {
            ret = MMPBX_ERROR_NORESOURCES;
            MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(ret));
            break;
        }

        memset(mmConnMulticastTemp, 0, sizeof(struct MmConnMulticast));

        /* Prepare connection for usage */
        ret = mmConnPrepare((MmConnHndl)mmConnMulticastTemp, MMCONN_TYPE_MULTICAST);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnPrepare failed with: %s\n", mmPbxGetErrorString(ret));
            break;
        }

        /* Register Child Destruct callback, will be called before destruct of object */
        ret = mmConnRegisterChildDestructCb((MmConnHndl)mmConnMulticastTemp, mmConnChildDestructCb);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRegisterChildDestructCb failed with: %s\n", mmPbxGetErrorString(ret));
        }

        /* Register Child write callback, will be called to push data into mmConn */
        ret = mmConnRegisterChildWriteCb((MmConnHndl)mmConnMulticastTemp, mmConnChildWriteCb);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRegisterChildWriteCb failed with: %s\n", mmPbxGetErrorString(ret));
            break;
        }

        /* Initialise list head */
        INIT_LIST_HEAD(&mmConnMulticastTemp->sinks);

        *multicast = mmConnMulticastTemp;
        MMPBX_TRC_INFO("mmConnMulticast = %p\n", *multicast);
    } while (0);


    if (ret != MMPBX_ERROR_NOERROR) {
        if (mmConnMulticastTemp != NULL) {
            mmConnDestruct((MmConnHndl *)&mmConnMulticastTemp);
        }
    }

    return ret;
}

/**
 *
 */
MmPbxError mmConnMulticastAddSink(MmConnMulticastHndl source,
                                  MmConnMulticastHndl sink)
{
    MmPbxError                ret     = MMPBX_ERROR_NOERROR;
    MmConnMulticastSinkEntry  *entry  = NULL;


    if (source == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (sink == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    MMPBX_TRC_INFO("source = %lx\n , sink = %lx\n", (unsigned long int)source, (unsigned long int)sink);

    entry = (MmConnMulticastSinkEntry *) kmalloc(sizeof(MmConnMulticastSinkEntry), GFP_KERNEL);
    if (entry == NULL) {
        ret = MMPBX_ERROR_NORESOURCES;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(ret));
        return ret;
    }

    entry->sink = sink;
    list_add_tail(&entry->list, &source->sinks);

    return ret;
}

/**
 *
 */
MmPbxError mmConnMulticastRemoveSink(MmConnMulticastHndl  source,
                                     MmConnMulticastHndl  sink)
{
    MmPbxError                ret     = MMPBX_ERROR_NOERROR;
    MmConnMulticastSinkEntry  *entry  = NULL;
    struct list_head          *pos, *q;

    if (source == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (sink == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    MMPBX_TRC_INFO("source = %lx\n , sink = %lx\n", (unsigned long int)source, (unsigned long int)sink);

    list_for_each_safe(pos, q, &source->sinks)
    {
        entry = list_entry(pos, MmConnMulticastSinkEntry, list);
        if (entry->sink == sink) {
            list_del(pos);
            kfree(entry);
        }
    }


    return ret;
}

/*########################################################################
#                                                                        #
#   EXPORTS                                                              #
#                                                                        #
########################################################################*/

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
    MmPbxError                err     = MMPBX_ERROR_NOERROR;
    MmConnMulticastHndl       source  = (MmConnMulticastHndl) conn;
    MmConnMulticastSinkEntry  *entry  = NULL;
    struct list_head          *pos    = NULL;

    /* Write to registered sinks */
    list_for_each(pos, &source->sinks)
    {
        entry = list_entry(pos, MmConnMulticastSinkEntry, list);
        MMPBX_TRC_INFO("sink: %lx\n", (unsigned long int) entry->sink)
        err = mmConnWrite((MmConnHndl)entry->sink, header, buff, bytes);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnWrite failed with error: %s\n", mmPbxGetErrorString(err));
        }
    }

    return err;
}

/**
 *
 */
static MmPbxError mmConnChildDestructCb(MmConnHndl mmConn)
{
    MmPbxError err = MMPBX_ERROR_NOERROR;

    return err;
}
