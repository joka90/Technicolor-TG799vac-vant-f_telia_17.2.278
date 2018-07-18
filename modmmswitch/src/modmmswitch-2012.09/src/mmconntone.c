/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/** \file
 * Multimedia switch tone connection API.
 *
 * A tone connection is used to generate a tone from kernel space.
 *
 * \version v1.0
 *
 *************************************************************************/


/*
 * Define tracing prefix, needs to be defined before includes.
 */
#define MODULE_NAME    "MMCONNTONE"
/*########################################################################
#                                                                        #
#  HEADER (INCLUDE) SECTION                                              #
#                                                                        #
########################################################################*/
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/module.h>

#include "mmconntone_p.h"
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
    MmConnToneEventCb cb;
    struct list_head  list;
} MmConnToneEventCbEntry;

/*########################################################################
#                                                                        #
#  PRIVATE DATA MEMBERS                                                  #
#                                                                        #
########################################################################*/

static int          _traceLevel = MMPBX_TRACELEVEL_NONE;
static struct mutex _mutex;
static int          _initialised = FALSE;
static LIST_HEAD(mmConnToneEventCbList);

/*########################################################################
#                                                                       #
#  PRIVATE FUNCTION PROTOTYPES                                          #
#                                                                       #
########################################################################*/

static int isInitialised(void);
static void varLock(void);
static void varRelease(void);
static MmPbxError mmConnChildWriteCb(MmConnHndl         conn,
                                     MmConnPacketHeader *header,
                                     uint8_t            *buff,
                                     unsigned int       bytes);

static MmPbxError mmConnChildDestructCb(MmConnHndl mmConn);
static MmPbxError mmConnChildXConnCb(MmConnHndl mmConn);
static MmPbxError mmConnChildDiscCb(MmConnHndl mmConn);

/*########################################################################
#                                                                        #
#  FUNCTION DEFINITIONS                                                  #
#                                                                        #
########################################################################*/

/**
 *
 */
MmPbxError mmConnToneInit(void)
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
MmPbxError mmConnToneDeinit(void)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    MmConnToneEventCbEntry  *entry = NULL;
    struct list_head        *pos, *q;

    list_for_each_safe(pos, q, &mmConnToneEventCbList)
    {
        entry = list_entry(pos, MmConnToneEventCbEntry, list);
        list_del(pos);
        kfree(entry);
    }

    _initialised = FALSE;

    return ret;
}

/**
 *
 */
MmPbxError mmConnToneSetTraceLevel(MmPbxTraceLevel level)
{
    _traceLevel = level;
    MMPBX_TRC_INFO("New trace level : %s\n", mmPbxGetTraceLevelString(level));


    return MMPBX_ERROR_NOERROR;
}

/**
 *
 */
MmPbxError mmConnToneConstruct(MmConnToneConfig *config,
                               MmConnToneHndl   *tone)
{
    MmPbxError              ret             = MMPBX_ERROR_NOERROR;
    MmConnToneHndl          mmConnToneTemp  = NULL;
    MmConnToneEventCbEntry  *entry          = NULL;
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

    if (tone == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    *tone = NULL;

    varLock();

    do {
        /* Try to allocate another mmConnTone object instance */
        mmConnToneTemp = kmalloc(sizeof(struct MmConnTone), GFP_KERNEL);
        if (mmConnToneTemp == NULL) {
            ret = MMPBX_ERROR_NORESOURCES;
            MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(ret));
            break;
        }

        memset(mmConnToneTemp, 0, sizeof(struct MmConnTone));

        /* Prepare connection for usage */
        ret = mmConnPrepare((MmConnHndl)mmConnToneTemp, MMCONN_TYPE_TONE);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnPrepare failed with: %s\n", mmPbxGetErrorString(ret));
            break;
        }

        /* Register Child Destruct callback, will be called before destruct of object */
        ret = mmConnRegisterChildDestructCb((MmConnHndl)mmConnToneTemp, mmConnChildDestructCb);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRegisterChildDestructCb failed with: %s\n", mmPbxGetErrorString(ret));
        }

        /* Register Child cross-connect callback, will be called after cross-connect */
        ret = mmConnRegisterChildXConnCb((MmConnHndl)mmConnToneTemp, mmConnChildXConnCb);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRegisterChildXConnCb failed with: %s\n", mmPbxGetErrorString(ret));
        }

        /* Register Child disconnect callback, will be called after disconnect */
        ret = mmConnRegisterChildDiscCb((MmConnHndl)mmConnToneTemp, mmConnChildDiscCb);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRegisterChildDiscCb failed with: %s\n", mmPbxGetErrorString(ret));
        }

        /* Save configuration */
        mmConnToneTemp->config.type = config->type;
        strncpy(mmConnToneTemp->config.encodingName, config->encodingName, ENCODING_NAME_LENGTH);
        mmConnToneTemp->config.packetPeriod   = config->packetPeriod;
        mmConnToneTemp->config.toneTable.size = config->toneTable.size;
        mmConnToneTemp->config.endpointId     = config->endpointId;

        /* Register Child write callback, will be called to push data into mmConn */
        ret = mmConnRegisterChildWriteCb((MmConnHndl)mmConnToneTemp, mmConnChildWriteCb);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRegisterChildWriteCb failed with: %s\n", mmPbxGetErrorString(ret));
            break;
        }

        *tone = mmConnToneTemp;
        MMPBX_TRC_INFO("mmConnTone = %p\n", *tone);
    } while (0);

    varRelease();

    if (ret != MMPBX_ERROR_NOERROR) {
        if (mmConnToneTemp != NULL) {
            mmConnDestruct((MmConnHndl *)&mmConnToneTemp);
        }
    }
    else {
        /*Call registered callbacks to notify construction of connection (if pattern table size = 0) */
        if ((*tone)->config.toneTable.size == 0) {
            list_for_each(pos, &mmConnToneEventCbList)
            {
                entry = list_entry(pos, MmConnToneEventCbEntry, list);
                entry->cb(MMCONNTONE_EVENT_CONSTRUCT, (*tone)->config.endpointId, &(*tone)->config, *tone);
            }
        }
    }

    return ret;
}

/**
 *
 */
MmPbxError mmConnToneRegisterEventCb(MmConnToneEventCb cb)
{
    MmPbxError              err     = MMPBX_ERROR_NOERROR;
    MmConnToneEventCbEntry  *entry  = NULL;

    entry = (MmConnToneEventCbEntry *) kmalloc(sizeof(MmConnToneEventCbEntry), GFP_KERNEL);
    if (entry == NULL) {
        err = MMPBX_ERROR_NORESOURCES;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(err));
        return err;
    }
    entry->cb = cb;
    list_add_tail(&entry->list, &mmConnToneEventCbList);


    return err;
}

/**
 *
 */
MmPbxError mmConnToneUnregisterEventCb(MmConnToneEventCb cb)
{
    MmPbxError              err = MMPBX_ERROR_NOERROR;
    MmConnToneEventCbEntry  *entry;
    struct list_head        *pos, *q;

    list_for_each_safe(pos, q, &mmConnToneEventCbList)
    {
        entry = list_entry(pos, MmConnToneEventCbEntry, list);
        if (entry->cb == cb) {
            list_del(pos);
            kfree(entry);
        }
    }

    return err;
}

/**
 *
 */
MmPbxError mmConnToneSavePattern(MmConnToneHndl     tone,
                                 MmConnTonePattern  *pattern)
{
    MmPbxError              err     = MMPBX_ERROR_NOERROR;
    MmConnToneEventCbEntry  *entry  = NULL;
    struct list_head        *pos;


    if (tone->receivedPatterns >= MMCONNTONE_MAX_TONE_PATTERNS) {
        err = MMPBX_ERROR_NORESOURCES;
        MMPBX_TRC_ERROR("receiving more patterns then what patterntable can hold\n");
        return err;
    }

    /*Save pattern */
    memcpy(&tone->config.toneTable.pattern[tone->receivedPatterns], pattern, sizeof(MmConnTonePattern));

    /*Increment received patterns counter */
    tone->receivedPatterns++;

    MMPBX_TRC_INFO("tone->receivedPatterns: %d\n", tone->receivedPatterns);

    /*Call registered callbacks to notify construction of connection (if pattern table size == tone->receivedPatterns) */
    if (tone->config.toneTable.size == tone->receivedPatterns) {
        list_for_each(pos, &mmConnToneEventCbList)
        {
            entry = list_entry(pos, MmConnToneEventCbEntry, list);
            entry->cb(MMCONNTONE_EVENT_CONSTRUCT, tone->config.endpointId, &tone->config, tone);
        }
    }

    return err;
}

/*########################################################################
#                                                                        #
#   EXPORTS                                                              #
#                                                                        #
########################################################################*/
EXPORT_SYMBOL(mmConnToneRegisterEventCb);
EXPORT_SYMBOL(mmConnToneUnregisterEventCb);

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
static void varLock()
{
    mutex_lock(&_mutex);
}

/**
 *
 */
static void varRelease()
{
    mutex_unlock(&_mutex);
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
    MmConnToneHndl          mmConnTone  = (MmConnToneHndl) mmConn;
    MmConnToneEventCbEntry  *entry      = NULL;
    struct list_head        *pos        = NULL;

    /*Call registered callbacks to notify construction of connection */
    list_for_each(pos, &mmConnToneEventCbList)
    {
        entry = list_entry(pos, MmConnToneEventCbEntry, list);
        entry->cb(MMCONNTONE_EVENT_DESTRUCT, mmConnTone->config.endpointId, &mmConnTone->config, mmConnTone);
    }


    return err;
}

/**
 *
 */
static MmPbxError mmConnChildXConnCb(MmConnHndl mmConn)
{
    MmPbxError              err         = MMPBX_ERROR_NOERROR;
    MmConnToneHndl          mmConnTone  = (MmConnToneHndl) mmConn;
    MmConnToneEventCbEntry  *entry      = NULL;
    struct list_head        *pos        = NULL;

    /*Call registered callbacks to notify cross-connect of connection */
    list_for_each(pos, &mmConnToneEventCbList)
    {
        entry = list_entry(pos, MmConnToneEventCbEntry, list);
        entry->cb(MMCONNTONE_EVENT_XCONN, mmConnTone->config.endpointId, &mmConnTone->config, mmConnTone);
    }


    return err;
}

/**
 *
 */
static MmPbxError mmConnChildDiscCb(MmConnHndl mmConn)
{
    MmPbxError              err         = MMPBX_ERROR_NOERROR;
    MmConnToneHndl          mmConnTone  = (MmConnToneHndl) mmConn;
    MmConnToneEventCbEntry  *entry      = NULL;
    struct list_head        *pos        = NULL;

    MMPBX_TRC_INFO("called, mmConnTone: %p\n", mmConnTone);

    /*Call registered callbacks to notify cross-connect of connection */
    list_for_each(pos, &mmConnToneEventCbList)
    {
        entry = list_entry(pos, MmConnToneEventCbEntry, list);
        entry->cb(MMCONNTONE_EVENT_DISC, mmConnTone->config.endpointId, &mmConnTone->config, mmConnTone);
    }


    return err;
}
