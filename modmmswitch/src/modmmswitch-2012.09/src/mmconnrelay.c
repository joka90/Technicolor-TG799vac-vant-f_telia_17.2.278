/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/** \file
 * Multimedia switch relay connection API.
 *
 * A relay connection relays a stream to a network socket.
 *
 * \version v1.0
 *
 *************************************************************************/


/*
 * Define tracing prefix, needs to be defined before includes.
 */
#define MODULE_NAME    "MMCONNRELAY"
/*########################################################################
#                                                                        #
#  HEADER (INCLUDE) SECTION                                              #
#                                                                        #
########################################################################*/
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/net.h>
#include <linux/file.h>
#include <linux/string.h>

#include "mmconnrelay_p.h"
#include "mmswitch_p.h"
#include "mmconn_p.h"
#include "mmconn_netlink_p.h"

/*########################################################################
#                                                                        #
#  MACROS/DEFINES                                                        #
#                                                                        #
########################################################################*/
#define PACKET_TMR_RESTART_COUNT    24
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

static int  _traceLevel   = MMPBX_TRACELEVEL_NONE;
static int  _initialised  = FALSE;

/*########################################################################
#                                                                       #
#  PRIVATE FUNCTION PROTOTYPES                                          #
#                                                                       #
########################################################################*/
static void mmSwitchTimerCb(MmConnHndl mmConn);
static int isInitialised(void);
static MmPbxError mmConnChildWriteCb(MmConnHndl         conn,
                                     MmConnPacketHeader *header,
                                     uint8_t            *buff,
                                     unsigned int       bytes);

static MmPbxError mmConnChildDestructCb(MmConnHndl mmConn);
static MmPbxError mmSwitchSocketCb(MmConnHndl         mmConn,
                                   void               *data,
                                   MmConnPacketHeader *header,
                                   unsigned int       bytes);
static void mmConnRelayGetRemoteAddr(MmConnRelayHndl          mmConnRelay,
                                     struct sockaddr_storage  *remoteAddr);

/*########################################################################
#                                                                        #
#  FUNCTION DEFINITIONS                                                  #
#                                                                        #
########################################################################*/

/**
 *
 */
MmPbxError mmConnRelayInit(void)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;


    _initialised = TRUE;

    return ret;
}

/**
 *
 */
MmPbxError mmConnRelayDeinit(void)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    _initialised = FALSE;

    return ret;
}

/**
 *
 */
MmPbxError mmConnRelaySetTraceLevel(MmPbxTraceLevel level)
{
    _traceLevel = level;
    MMPBX_TRC_INFO("New trace level : %s\n", mmPbxGetTraceLevelString(level));


    return MMPBX_ERROR_NOERROR;
}

/**
 *
 */
MmPbxError mmConnRelayConstruct(MmConnRelayConfig *config,
                                MmConnRelayHndl   *relay)
{
    MmPbxError      ret             = MMPBX_ERROR_NOERROR;
    MmConnRelayHndl mmConnRelayTemp = NULL;

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

    if (relay == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\r\n", mmPbxGetErrorString(ret));
        return ret;
    }

    *relay = NULL;

    do {
        /* Try to allocate another mmConnRelay object instance */
        mmConnRelayTemp = kmalloc(sizeof(struct MmConnRelay), GFP_KERNEL);
        if (mmConnRelayTemp == NULL) {
            ret = MMPBX_ERROR_NORESOURCES;
            MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(ret));
            break;
        }

        memset(mmConnRelayTemp, 0, sizeof(struct MmConnRelay));

        /* Prepare connection for usage */
        ret = mmConnPrepare((MmConnHndl)mmConnRelayTemp, MMCONN_TYPE_RELAY);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnPrepare failed with: %s\n", mmPbxGetErrorString(ret));
            break;
        }

        /* Register Child Destruct callback, will be called before destruct of object */
        ret = mmConnRegisterChildDestructCb((MmConnHndl)mmConnRelayTemp, mmConnChildDestructCb);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRegisterChildDestructCb failed with: %s\n", mmPbxGetErrorString(ret));
        }

        /* Register Child write callback, will be called to push data into mmConn */
        ret = mmConnRegisterChildWriteCb((MmConnHndl)mmConnRelayTemp, mmConnChildWriteCb);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmConnRegisterChildWriteCb failed with: %s\n", mmPbxGetErrorString(ret));
            break;
        }
        mmConnRelayTemp->config.localSockFd = config->localSockFd;
        memcpy(&mmConnRelayTemp->config.remoteAddr, (void *)&config->remoteAddr, sizeof(struct sockaddr_storage));
#ifdef MMPBX_DSP_SUPPORT_RTCPXR
        mmConnRelayTemp->config.cookie = config->cookie;
        mmConnRelayTemp->config.updateRtcpReceiveCb = config->updateRtcpReceiveCb;
        mmConnRelayTemp->config.updateRtcpSendCb    = config->updateRtcpSendCb;
#endif

        ret = mmSwitchPrepareSocket(&mmConnRelayTemp->localSock, config->localSockFd);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmSwitchPrepareSocket failed with: %s\n", mmPbxGetErrorString(ret));
            break;
        }

        memcpy(&mmConnRelayTemp->config.header, (void *)&config->header, sizeof(MmConnPacketHeader));
        ret = mmSwitchRegisterSocketCb(&mmConnRelayTemp->localSock, mmSwitchSocketCb, (MmConnHndl) mmConnRelayTemp, &config->header);
        if (ret != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmSwitchRegisterSocketCb failed with: %s\n", mmPbxGetErrorString(ret));
            break;
        }

        MMPBX_TRC_INFO("mediaTimeout: %d\n", config->mediaTimeout);
        mmConnRelayTemp->config.mediaTimeout = config->mediaTimeout;

        /* Prepare timer for usage (if needed) */
        if ((config->mediaTimeout) > 0) {
            ret = mmSwitchPrepareTimer(&mmConnRelayTemp->mmSwitchTimer, mmSwitchTimerCb, (MmConnHndl)mmConnRelayTemp, FALSE);
            if (ret != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("mmSwitchPrepareTimer failed with: %s\n", mmPbxGetErrorString(ret));
                break;
            }
        }

        *relay = mmConnRelayTemp;
        MMPBX_TRC_INFO("mmConnRelay = %p\n", *relay);
    } while (0);


    if (ret != MMPBX_ERROR_NOERROR) {
        if (mmConnRelayTemp != NULL) {
            mmConnDestruct((MmConnHndl *)&mmConnRelayTemp);
        }
    }

    return ret;
}

/**
 *
 */
MmPbxError mmConnRelayModRemoteAddr(MmConnRelayHndl         relay,
                                    struct sockaddr_storage *remoteAddr)
{
    MmPbxError ret = MMPBX_ERROR_NOERROR;

    if (isInitialised() == FALSE) {
        ret = MMPBX_ERROR_INVALIDSTATE;
        MMPBX_TRC_CRIT("%s\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (relay == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(ret));
        return ret;
    }

    if (remoteAddr == NULL) {
        ret = MMPBX_ERROR_INVALIDPARAMS;
        MMPBX_TRC_ERROR("%s\n", mmPbxGetErrorString(ret));
        return ret;
    }

    MMPBX_TRC_INFO("called\n");

    memcpy(&relay->config.remoteAddr, (void *)remoteAddr, sizeof(struct sockaddr_storage));

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
static void mmConnRelayGetRemoteAddr(MmConnRelayHndl          mmConnRelay,
                                     struct sockaddr_storage  *remoteAddr)
{
    memcpy(remoteAddr, &mmConnRelay->config.remoteAddr, sizeof(struct sockaddr_storage));
}

/**
 *
 */
static void mmSwitchTimerCb(MmConnHndl mmConn)
{
    MmPbxError  ret = MMPBX_ERROR_NOERROR;
    MmConnEvent event;

    event.type  = MMCONN_EV_INGRESS_MEDIA_TIMEOUT;
    event.parm  = 0;

    /*Send event to user space*/
    ret = mmConnSendEvent(mmConn, &event);
    if (ret != MMPBX_ERROR_NOERROR) {
        ret = MMPBX_ERROR_INTERNALERROR;
        MMPBX_TRC_ERROR("mmConnSendEvent failed with error: %s\n", mmPbxGetErrorString(ret));
    }
}

/**
 *
 */
static MmPbxError mmConnChildWriteCb(MmConnHndl         conn,
                                     MmConnPacketHeader *header,
                                     uint8_t            *buff,
                                     unsigned int       bytes)
{
    MmPbxError              err         = MMPBX_ERROR_NOERROR;
    MmConnRelayHndl         mmConnRelay = (MmConnRelayHndl) conn;
    struct sockaddr_storage dst;

    mmConnRelayGetRemoteAddr(mmConnRelay, &dst);
    if (dst.ss_family == AF_UNSPEC) {
        //Remote address not known, discard write
        return err;
    }
    do {
        err = mmSwitchWriteSocket(&mmConnRelay->localSock, (struct sockaddr *)&mmConnRelay->config.remoteAddr, buff, bytes);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmSwitchWriteSocket failed with error: %s\n", mmPbxGetErrorString(err));
            break;
        }
#ifdef MMPBX_DSP_SUPPORT_RTCPXR
        if ((mmConnRelay->config.updateRtcpSendCb != NULL) && (mmConnRelay->config.cookie != NULL)) {
            err = mmConnRelay->config.updateRtcpSendCb(mmConnRelay->config.cookie, buff, bytes);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("updateRtcpSendCb failed to parse RTCP frame; error: %s\n", mmPbxGetErrorString(err));
            }
        }
        else {
            MMPBX_TRC_INFO("No updateRtcpSendCb callback is installed\n\r");
        }
#endif
    } while (0);

    return err;
}

/**
 *
 */
static MmPbxError mmConnChildDestructCb(MmConnHndl mmConn)
{
    MmPbxError      err         = MMPBX_ERROR_NOERROR;
    MmConnRelayHndl mmConnRelay = (MmConnRelayHndl) mmConn;

    if (mmConnRelay->config.mediaTimeout > 0) {
        err = mmSwitchDestroyTimer(&mmConnRelay->mmSwitchTimer);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("mmSwitchDestroyTimer failed with: %s\n", mmPbxGetErrorString(err));
        }
    }

    err = mmSwitchCleanupSocket(&mmConnRelay->localSock);
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmSwitchCleanupSocket failed with: %s\n", mmPbxGetErrorString(err));
    }

    return err;
}

/**
 * Function to handle received data from socket.
 */
static MmPbxError mmSwitchSocketCb(MmConnHndl         mmConn,
                                   void               *data,
                                   MmConnPacketHeader *header,
                                   unsigned int       bytes)
{
    MmPbxError      err         = MMPBX_ERROR_NOERROR;
    MmConnRelayHndl mmConnRelay = (MmConnRelayHndl) mmConn;

    err = mmConnWrite(mmConn, header, data, bytes);
    if (err != MMPBX_ERROR_NOERROR) {
        MMPBX_TRC_ERROR("mmConnWrite failed with: %s\n", mmPbxGetErrorString(err));
    }
#ifdef MMPBX_DSP_SUPPORT_RTCPXR
    if ((mmConnRelay->config.updateRtcpReceiveCb != NULL) && (mmConnRelay->config.cookie != NULL)) {
        err = mmConnRelay->config.updateRtcpReceiveCb(mmConnRelay->config.cookie, data, bytes);
        if (err != MMPBX_ERROR_NOERROR) {
            MMPBX_TRC_ERROR("updateRtcpReceiveCb failed to parse Received RTCP frame; with: %s\n", mmPbxGetErrorString(err));
        }
    }
    else {
        MMPBX_TRC_INFO("No updateRtcpReceiveCb Callback is installed\n\r");
    }
#endif

    if (mmConnRelay->config.mediaTimeout > 0) {
        mmConnRelay->packetCounter++;
        if (mmConnRelay->packetCounter > PACKET_TMR_RESTART_COUNT) {
            err = mmSwitchRestartTimer(&mmConnRelay->mmSwitchTimer, mmConnRelay->config.mediaTimeout);
            if (err != MMPBX_ERROR_NOERROR) {
                MMPBX_TRC_ERROR("mmSwitchReStartTimer failed with: %s\n", mmPbxGetErrorString(err));
            }
            mmConnRelay->packetCounter = 0;
        }
    }
    return err;
}
