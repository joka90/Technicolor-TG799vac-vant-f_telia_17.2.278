/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/** \file
 * Multimedia switch RTCP connection API.
 *
 * An RTCP connection relays media, and generates RTCP if media is of type RTP .
 *
 * \version v1.0
 *
 *************************************************************************/

#ifndef  MMCONNRTCP_INC
#define  MMCONNRTCP_INC

/*########################################################################
#                                                                       #
#  HEADER (INCLUDE) SECTION                                             #
#                                                                       #
########################################################################*/
#include "mmconntypes.h"
#include "mmcommon.h"
/*########################################################################
#                                                                       #
#  MACROS/DEFINES                                                       #
#                                                                       #
########################################################################*/
/** \brief Maximum number of saved RTCP frame  */
#define MMCONN_RTCP_MAX_SAVED_RTCPXR_FRAME    2

/**
 * RTP/RTCP stack statistics
 */
/* will not be used if MMPBX_DSP_SUPPORT_RTCPXR is defined */
typedef struct {
    unsigned long meanE2eDelay;             /**< Mean end-to-end delay in microseconds, measured by RTP instance. */
    unsigned long worstE2eDelay;            /**< Worst end-to-end delay in microseconds, measured by RTP instance. */
    unsigned long rxJitter;                 /**< jitter in RX stream, measured by RTP instance. \n Jitter is measured in timestamp units (0.125ms for 8 kHz codecs). */
    uint64_t      rxTotalPacketsLost;       /**< Total number of lost packets in RX stream, measured by RTP instance. */
    uint64_t      remoteTxRtpPackets;       /**< Number of RTP packets sent, measured by remote party. */
    uint64_t      remoteTxRtpBytes;         /**< Number of RTP bytes send, measured by remote party.*/
    uint64_t      remoteRxTotalPacketsLost; /**< Total number of lost packets in RX stream, measured by remote party.   */
    unsigned long remoteRxJitter;           /**< Mean Jitter in RX stream, measured by remote party.\n Jitter is measured in timestamp units (0.125ms for 8 kHz codecs).*/
    unsigned long remoteRxWorstJitter;      /**< Worst jitter in RX stream, measured by remote party.\n Measured in timestamp units (0.125 ms for 8kHz codecs).*/
    unsigned long currentE2eDelay;          /**< end-to-end delay in microseconds, measured by RTP instance. */
    unsigned long meanRemoteRxJitter;       /**< Mean Jitter in RX stream, measured by remote party.\n Jitter is measured in timestamp units (0.125 ms for 8kHz codecs).*/
    unsigned long meanRxJitter;             /**< Mean Jitter in RX stream, measured by RTCP instance. \n Jitter is measred in timestamp units (0.125 ms for 8kHz codecs).*/
} MmConnRtcpStackStats;

/*
 * RTP/RTCP (XR) statistics
 */
typedef struct {/* Connection statistics */
    /* connection statistics from the local endpoint's perspective */
    unsigned long localPktSent;               /* Packets sent */
    unsigned long localOctetSent;             /* Octets sent */
    unsigned long localPktRecvd;              /* Packets received */
    unsigned long localOctetRecvd;            /* Octets received */
    unsigned long localPktLost;               /* Packet lost */
    unsigned long localPktLostRate;           /* Packet loss rate */
    unsigned long localPktDiscarded;          /* Packet discarded */
    unsigned long localPktDiscardedRate;      /* Packet discarded Rate */
    unsigned long localJitter;                /* Interarrival jitter estimate (in ms) */
    unsigned long localMinJitter;             /* Minimum Interarrival jitter estimate (in ms) */
    unsigned long localMaxJitter;             /* Maximum Interarrival jitter estimate (in ms) */
    unsigned long localDevJitter;             /* Standard Deviation Interarrival jitter estimate (in ms) */
    unsigned long localJitterBufferOverruns;  /* Overrun count */
    unsigned long localJitterBufferUnderruns; /* Underrun count */
    unsigned long localLatency;               /* Round Trip Delay (in ms) */
    unsigned long locaMaxLatency;             /* Max Round Trip Delay (in ms) */
    unsigned long localSignalLevel;           /* Signal Level in dBm */
    unsigned long localNoiseLevel;            /* Noise level in dBm */
    unsigned long localRERL;                  /* Residual Echo Return Loss */
    unsigned long localRFactor;               /* R-Factor in voice quality */
    unsigned long localExternalRFactor;       /* External R-Factor quality from external network */
    unsigned long localMosLQ;                 /* Mean Option Score for Listening Quality, value * 10 */
    unsigned long localMosCQ;                 /* Mean Option Score for Conversational Quality, value * 10 */
    unsigned long localJitterBuffer;          /* Current Jitter Buffer Nominal Delay, in msecs */
    unsigned long localMaxJitterBuffer;       /* Maximum Jitter Buffer Nominal Delay, in msecs */
    unsigned long localWorstJitterBuffer;     /* Worst case Jitter Buffer Nominal Delay, in msecs */

    unsigned long localAverageJitter;         /* Average local interarrival jitter estimate (in ms) */
    unsigned long localAverageLatency;        /* Average local avg tx delay (in ms) */

    /* connection statistics from the remote endpoint's perspective */
    unsigned long remotePktSent;            /* Packets sent */
    unsigned long remoteOctetSent;          /* Octets sent */
    unsigned long remotePktRecvd;           /* Packets received */
    unsigned long remoteOctetRecvd;         /* Octets received */
    unsigned long remotePktLost;            /* Packet lost */
    unsigned long remotePktLostRate;        /* Packet loss rate */
    unsigned long remotePktDiscardedRate;   /* Packet discarded Rate */
    unsigned long remoteJitter;             /* Interarrival jitter estimate (in ms) */
    unsigned long remoteMinJitter;          /* Minimum Interarrival jitter estimate (in ms) */
    unsigned long remoteMaxJitter;          /* Maximum Interarrival jitter estimate (in ms) */
    unsigned long remoteDevJitter;          /* Standard Deviation Interarrival jitter estimate (in ms) */
    unsigned long remoteLatency;            /* Round Trip Delay (in ms) */
    unsigned long remoteMaxLatency;         /* Max Round Trip Delay (in ms) */
    unsigned long remoteSignalLevel;        /* Signal Level in dBm */
    unsigned long remoteNoiseLevel;         /* Noise level in dBm */
    unsigned long remoteRERL;               /* Residual Echo Return Loss */
    unsigned long remoteRFactor;            /* R-Factor in voice quality */
    unsigned long remoteExternalRFactor;    /* External R-Factor quality from external network */
    unsigned long remoteMosLQ;              /* Mean Option Score for Listening Quality, value * 10 */
    unsigned long remoteMosCQ;              /* Mean Option Score for Conversational Quality, value * 10 */
    unsigned long remoteJitterBuffer;       /* Current Jitter Buffer Nominal Delay, in msecs */
    unsigned long remoteMaxJitterBuffer;    /* Maximum Jitter Buffer Nominal Delay, in msecs */
    unsigned long remoteWorstJitterBuffer;  /* Worst case Jitter Buffer Nominal Delay, in msecs */

    unsigned long remoteAverageJitter;        /* Average local interarrival jitter estimate (in ms) */
    unsigned long remoteAverageLatency;       /* Average local avg tx delay (in ms) */

    unsigned long rtcpPktsReceived;           /* Number of RTCP packets received */
    unsigned long rtcpXrPktsReceived;         /* Number of RTCP-XR packets received */
    unsigned long rtcpPktsSent;               /* Number of RTCP packets Sent */
    unsigned long rtcpXrPktsSent;             /* Number of RTCP-XR packets sent */

    /*AT&T extension*/
    unsigned long localSumFractionLoss;     /*summary of tx fraction loss */
    unsigned long localSumSqrFractionLoss;  /*summary of square of tx fraction loss */
    unsigned long remoteSumFractionLoss;    /*summary of tx fraction loss */
    unsigned long remoteSumSqrFractionLoss; /*summary of square of tx fraction loss */
    unsigned long localSumJitter;           /*summary of tx Interarrival jitter */
    unsigned long localSumSqrJitter;        /*Sum of Square of tx Interarrival jitter */
    unsigned long remoteSumJitter;          /*Sum of tx Interarrival jitter*/
    unsigned long remoteSumSqrJitter;       /*Sum of square of tx Interarrival jitter */
    unsigned long sumRoundTripDelay;        /*Sum of Round Trip Delay */
    unsigned long sumSqrRoundTripDelay;     /*Sum of Square of Max Round Trip Delay */
    unsigned long maxOneWayDelay;           /*Max One Way Delay */
    unsigned long sumOneWayDelay;           /*Sum of One Way Delay */
    unsigned long sumSqrOneWayDelay;        /*Sum of Square of One Way Delay */

    /* RTCP frame buffer */
    unsigned long txRtcpFrameLength[MMCONN_RTCP_MAX_SAVED_RTCPXR_FRAME];  /**<Saved sent RTCP frame buffer length */
    uint8_t       *txRtcpFrameBuffer[MMCONN_RTCP_MAX_SAVED_RTCPXR_FRAME]; /**<Saved sent RTCP frame buffer content */
    unsigned long rxRtcpFrameLength[MMCONN_RTCP_MAX_SAVED_RTCPXR_FRAME];  /**<Saved received RTCP frame buffer length */
    uint8_t       *rxRtcpFrameBuffer[MMCONN_RTCP_MAX_SAVED_RTCPXR_FRAME]; /**<Saved received RTCP frame buffer content */
} MmConnRtcpCnxStats;

/*########################################################################
#                                                                       #
#  TYPES                                                                #
#                                                                       #
########################################################################*/

/**
 * Callback function which will be called to allow an RTP/RTCP stack to create a new RTP & RTCP session.
 *
 * An RTP/RTCP stack implementer can use this callback function to create it's RTP and RTCP session.
 * The implementer of this callback function can set the value of a cookie within \c mmConnRtcp.
 * This cookie allows the implementer to have a reference to RTP/RTCP stack specific data structure.
 *
 * \since v1.0
 *
 * \pre \c NONE
 *
 * \post RTP & RTCP session are created.
 *
 * \param [in] mmConnRtcp MmConnRtcp handle.
 * \param [in] rtcpBandwidth  Allowed bandwidth for RTCP.
 * \param [out] cookie RTP/RTCP stack specific reference for RTP & RTCP session.
 * \param [out] rtcpTimerTimeout Timeout value of RTCP transmit timer.
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR RTP & RTCP sessions are created successfully.
 * \todo Add other possible return values after implementation.
 */
typedef MmPbxError (*MmConnRtcpStackCreateSessionsCb)(MmConnRtcpHndl  mmConnRtcp,
                                                      unsigned int    rtcpBandwidth,
                                                      void            **cookie,
                                                      unsigned int    *rtcpTimerTimeout);

/**
 * Callback function which will be called to allow an RTP/RTCP stack to destroy an RTP & RTCP session.
 *
 * An RTP/RTCP stack implementer can use this callback function to destroy it's RTP and RTCP session.
 *
 * \since v1.0
 *
 * \pre Sessions should first be created before they can be destroyed.
 *
 * \post RTP & RTCP session,which was identified by means of \c cookie,  are destroyed.
 *
 * \param [in] mmConnRtcp mmConnRtcp handle.
 * \param [in] cookie RTP/RTCP stack specific reference for RTP & RTCP session.
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR RTP & RTCP sessions are destroyed successfully.
 * \todo Add other possible return values after implementation.
 */
typedef MmPbxError (*MmConnRtcpStackDestroySessionsCb)(MmConnRtcpHndl mmConnRtcp,
                                                       void           *cookie);

/**
 * Callback function which will be called when the RTCP bandwidth configuration has changed.
 *
 * An RTP/RTCP stack implementer can use this callback function to updated the RTCP bandwith according to the configuration.
 *
 * \since v1.0
 *
 * \pre Sessions must first be created before we can modify the bandwith of the RTCP session.
 *
 * \post RTCP session,which was identified by means of \c cookie, has updated it's bandwidth.
 *
 * \param [in] mmConnRtcp MmConnRtcp handle.
 * \param [in] cookie RTP/RTCP stack specific reference for RTP & RTCP session.
 * \param [in] rtcpBandwidth  Allowed bandwidth for RTCP.
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR RTCP session updated successfully.
 * \todo Add other possible return values after implementation.
 */
typedef MmPbxError (*MmConnRtcpStackModRtcpBandwidthCb)(MmConnRtcpHndl  mmConnRtcp,
                                                        void            *cookie,
                                                        unsigned int    rtcpBandwidth);

/**
 * Callback function which will be called when we receive an RTP packet from the network.
 *
 * An RTP/RTCP stack implementer can use this callback function to notify the RTP/RTCP stack that we have received an RTP packet from the network.
 *
 * \since v1.0
 *
 * \pre RTP and RTCP sessions must first be created.
 *
 * \post Stack is aware of received RTP packet.
 *
 * \param [in] mmConnRtcp MmConnRtcp handle.
 * \param [in] cookie RTP/RTCP stack specific reference for RTP & RTCP session.
 * \param [in] data RTP packet which was received.
 * \param [in] bytes  size of RTP packet which was received.
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR Event handled successfully.
 * \todo Add other possible return values after implementation.
 */
typedef MmPbxError (*MmConnRtcpStackRtpReceiveCb)(MmConnRtcpHndl  mmConnRtcp,
                                                  void            *cookie,
                                                  uint8_t         *data,
                                                  unsigned int    bytes);

/**
 * Callback function which will be called when we send an RTP packet to the network.
 *
 * An RTP/RTCP stack implementer can use this callback function to notify the RTP/RTCP stack that we will send an RTP packet to the network.
 *
 * \since v1.0
 *
 * \pre RTP and RTCP sessions must first be created.
 *
 * \post Stack is aware of the RTP packet.
 *
 * \param [in] mmConnRtcp MmConnRtcp handle.
 * \param [in] cookie RTP/RTCP stack specific reference for RTP & RTCP session.
 * \param [in] data RTP packet which will be send.
 * \param [in] bytes  size of RTP packet which will be send.
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR Event handled successfully.
 * \todo Add other possible return values after implementation.
 */
typedef MmPbxError (*MmConnRtcpStackRtpSendCb)(MmConnRtcpHndl mmConnRtcp,
                                               void           *cookie,
                                               uint8_t        *data,
                                               unsigned int   bytes);

/**
 * Callback function which will be called when we receive an RTCP packet.
 *
 * An RTP/RTCP stack implementer can use this callback function to notify the RTP/RTCP stack that we have received an RTCP packet.
 *
 * \since v1.0
 *
 * \pre RTP and RTCP sessions must first be created.
 *
 * \post RTP/RTCP stack had handled the RTCP packet received event.
 *
 * \param [in] mmConnRtcp MmConnRtcp handle.
 * \param [in] cookie RTP/RTCP stack specific reference for RTP & RTCP session.
 * \param [in] data RTCP packet which was received.
 * \param [in] bytes  size of RTCP packet which was received.
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR Event handled successfully.
 * \todo Add other possible return values after implementation.
 */
typedef MmPbxError (*MmConnRtcpStackRtcpReceiveCb)(MmConnRtcpHndl mmConnRtcp,
                                                   void           *cookie,
                                                   uint8_t        *data,
                                                   unsigned int   bytes);

/**
 * Callback function which will be called when RTCP transmit timer expires.
 *
 * This callback function will be called when the RTCP transmit timer expires.
 * The RTP/RTCP stack implementer can modify the period of the RTCP transmit timer within this callback. The timer will be restarted after this callback is handled.
 *
 * \since v1.0
 *
 * \pre \c NONE.
 *
 * \post RTP/RTCP stack had handled the RTCP timer callback.
 *
 * \param [in] mmConnRtcp MmConnRtcp handle.
 * \param [in] cookie RTP/RTCP Stack specific reference for RTP & RTCP session.
 * \param [out] rtcpTimerTimeout Timeout value of RTCP transmit timer.
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR Event handled successfully.
 * \todo Add other possible return values after implementation.
 */
typedef MmPbxError (*MmConnRtcpStackRtcpTimerCb)(MmConnRtcpHndl mmConnRtcp,
                                                 void           *cookie,
                                                 unsigned int   *rtcpTimerTimeout);

/**
 * Callback function which will be called to get the RTCP statistics.
 *
 * This callback function will be called to get the RTCP statistics
 *
 * \since v1.0
 *
 * \pre \c NONE.
 *
 * \post the staitistics is retrieved
 *
 * \param [in] cookie RTP/RTCP connection specific reference for RTP & RTCP statistics session.
 * \param [out] stats The RTCP statistics
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR Event handled successfully.
 * \todo Add other possible return values after implementation.
 */
typedef MmPbxError (*MmConnRtcpGetCnxStatsCb)(void                *cookie,
                                              MmConnRtcpCnxStats  *stats);


/*########################################################################
#                                                                       #
#  FUNCTION PROTOTYPES                                                  #
#                                                                       #
########################################################################*/


/**
 * Register callback to create an RTP and RTCP session.
 *
 * This callback function can be registered to create an RTP & RTCP session.
 *
 * \since v1.0
 *
 * \pre \c cb must be a valid callback function of type ::MmConnRtcpStackCreateSessionsCb.
 *
 * \post The callback function has been registered.
 *
 * \param [in] cb Callback function.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The callback function has been successfully registered.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRtcpRegisterStackCreateSessionsCb(MmConnRtcpStackCreateSessionsCb cb);

/**
 * Unregister callback to create an RTP and RTCP session.
 *
 * Unregister callback function to create RTP and RTCP session.
 *
 * \since v1.0
 *
 * \pre \c cb must be a valid callback function of type ::MmConnRtcpStackCreateSessionsCb.
 *
 * \post The callback function will no longer be called.
 *
 * \param [in] cb Callback function.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The callback function has been successfully unregistered.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRtcpUnregisterStackCreateSessionsCb(MmConnRtcpStackCreateSessionsCb cb);

/**
 * Register callback to destroy an RTP and RTCP session.
 *
 * This callback function can be registered to destroy an RTP & RTCP session.
 *
 * \since v1.0
 *
 * \pre \c cb must be a valid callback function of type ::MmConnRtcpStackDestroySessionsCb.
 *
 * \post The callback function has been registered.
 *
 * \param [in] cb Callback function.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The callback function has been successfully registered.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRtcpRegisterStackDestroySessionsCb(MmConnRtcpStackDestroySessionsCb cb);

/**
 * Unregister callback to destroy RTP and RTCP session.
 *
 * Unregister callback function to destroy RTP and RTCP session.
 *
 * \since v1.0
 *
 * \pre \c cb must be a valid callback function of type ::MmConnRtcpStackDestroySessionsCb.
 *
 * \post The callback function will no longer be called.
 *
 * \param [in] cb Callback function.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The callback function has been successfully unregistered.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRtcpUnregisterStackDestroySessionsCb(MmConnRtcpStackDestroySessionsCb cb);

/**
 * Register callback function to allow modification of RTCP bandwidth.
 *
 * This callback function can be registered to allow modification of RTCP bandwidth.
 *
 * \since v1.0
 *
 * \pre \c cb must be a valid callback function of type ::MmConnRtcpStackModRtcpBandwidthCb.
 *
 * \post The callback function has been registered.
 *
 * \param [in] cb Callback function.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The callback function has been successfully registered.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRtcpRegisterStackModRtcpBandwidthCb(MmConnRtcpStackModRtcpBandwidthCb cb);

/**
 * Unregister callback to allow modification of RTCP bandwidth.
 *
 * Unregister callback function.
 *
 * \since v1.0
 *
 * \pre \c cb must be a valid callback function of type ::MmConnRtcpStackModRtcpBandwidthCb.
 *
 * \post The callback function will no longer be called.
 *
 * \param [in] cb Callback function.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The callback function has been successfully unregistered.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRtcpUnregisterStackModRtcpBandwidthCb(MmConnRtcpStackModRtcpBandwidthCb cb);

/**
 * Register callback function which will be called when \c mmConnRtcp sends an RTP packet to the network.
 *
 * This callback function can be registered to notify the stack about the sending of RTP.
 *
 * \since v1.0
 *
 * \pre \c cb must be a valid callback function of type ::MmConnRtcpStackRtpSendCb.
 *
 * \post The callback function has been registered.
 *
 * \param [in] cb Callback function.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The callback function has been successfully registered.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRtcpRegisterStackRtpSendCb(MmConnRtcpStackRtpSendCb cb);

/**
 * Unregister callback which will be called when \c mmConnRtcp sends an RTP packet to the network.
 *
 * Unregister callback function.
 *
 * \since v1.0
 *
 * \pre \c cb must be a valid callback function of type ::MmConnRtcpStackRtpSendCb.
 *
 * \post The callback function will no longer be called.
 *
 * \param [in] cb Callback function.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The callback function has been successfully unregistered.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRtcpUnregisterStackRtpSendCb(MmConnRtcpStackRtpSendCb cb);

/**
 * Register callback function which will be called when \c mmConnRtcp receives an RTP packet from the network.
 *
 * This callback function can be registered to notify the stack about the received RTP packet.
 *
 * \since v1.0
 *
 * \pre \c cb must be a valid callback function of type ::MmConnRtcpStackRtpReceiveCb.
 *
 * \post The callback function has been registered.
 *
 * \param [in] cb Callback function.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The callback function has been successfully registered.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRtcpRegisterStackRtpReceiveCb(MmConnRtcpStackRtpReceiveCb cb);

/**
 * Unregister callback which will be called when \c mmConnRtcp receives an RTP packet from the network.
 *
 * Unregister callback function.
 *
 * \since v1.0
 *
 * \pre \c cb must be a valid callback function of type ::MmConnRtcpStackRtpReceiveCb.
 *
 * \post The callback function will no longer be called.
 *
 * \param [in] cb Callback function.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The callback function has been successfully unregistered.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRtcpUnregisterStackRtpReceiveCb(MmConnRtcpStackRtpReceiveCb cb);

/**
 * Register callback function which will be called when \c mmConnRtcp received an RTCP packet from the network.
 *
 * This callback function can be registered to notify the stack about the received RTCP packet.
 *
 * \since v1.0
 *
 * \pre \c cb must be a valid callback function of type ::MmConnRtcpStackRtcpReceiveCb.
 *
 * \post The callback function has been registered.
 *
 * \param [in] cb Callback function.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The callback function has been successfully registered.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRtcpRegisterStackRtcpReceiveCb(MmConnRtcpStackRtcpReceiveCb cb);

/**
 * Unregister callback which will be called when \c mmConnRtcp received an RTCP packet from the network.
 *
 * Unregister callback function.
 *
 * \since v1.0
 *
 * \pre \c cb must be a valid callback function of type ::MmConnRtcpStackRtcpReceiveCb.
 *
 * \post The callback function will no longer be called.
 *
 * \param [in] cb Callback function.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The callback function has been successfully unregistered.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRtcpUnregisterStackRtcpReceiveCb(MmConnRtcpStackRtcpReceiveCb cb);

/**
 * Register callback function which will be called when the \c mmConnRtcp RTCP transmit timer expires .
 *
 * This callback function can be registered to notify the stack about the timer expire.
 *
 * \since v1.0
 *
 * \pre \c cb must be a valid callback function of type ::MmConnRtcpStackRtcpTimerCb.
 *
 * \post The callback function has been registered.
 *
 * \param [in] cb Callback function.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The callback function has been successfully registered.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRtcpRegisterStackRtcpTimerCb(MmConnRtcpStackRtcpTimerCb cb);

/**
 * Unregister callback which will be called when the \c mmConnRtcp RTCP transmit timer expires.
 *
 * Unregister callback function.
 *
 * \since v1.0
 *
 * \pre \c cb must be a valid callback function of type ::MmConnRtcpStackRtcpTimerCb.
 *
 * \post The callback function will no longer be called.
 *
 * \param [in] cb Callback function.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The callback function has been successfully unregistered.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRtcpUnregisterStackRtcpTimerCb(MmConnRtcpStackRtcpTimerCb cb);

/**
 * Register callback function which will be called to get the RTCP statistics
 *
 * This callback function can be registered to get the RTCP statistics
 *
 * \since v1.0
 *
 * \pre \c cb must be a valid callback function of type ::MmConnRtcpGetCnxStatsCb.
 *
 * \post The callback function has been registered.
 *
 * \param [in] cb Callback function.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The callback function has been successfully registered.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRtcpRegisterGetCnxStatsCb(MmConnRtcpGetCnxStatsCb cb);

/**
 * Unregister callback to retrieve the DSP statistics
 *
 * Unregister callback function.
 *
 * \since v1.0
 *
 * \pre \c cb must be a valid callback function of type ::MmConnRtcpGetCnxStatsCb.
 *
 * \post The callback function will no longer be called.
 *
 * \param [in] cb Callback function.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The callback function has been successfully unregistered.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRtcpUnregisterGetCnxStatsCb(MmConnRtcpGetCnxStatsCb cb);

/**
 * Send RTCP packet to network.
 *
 * This function needs to be used to send an RTCP packet to the network.
 *
 * \since v1.0
 *
 * \pre \c mmConnRtcp must be a valid handle.
 *
 * \post The content of \c buff has been send to the network.
 *
 * \param [in] mmConnRtcp Handle of RTCP connection instance.
 * \param [in] buff Buffer which contains data to send.
 * \param [in] bytes Number of bytes to send.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR Data successfully sent.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRtcpSendRtcp(MmConnRtcpHndl  mmConnRtcp,
                              uint8_t         *buff,
                              unsigned int    bytes);

/**
 * Update mmConnRtcp RTP/TCP stack statistics.
 *
 * This function needs to be used to update the statistics of a mmConnRtcp.
 *
 * \since v1.0
 *
 * \pre \c mmConnRtcp must be a valid handle.
 *
 * \post The content of \c buff has been send to the network.
 *
 * \param [in] mmConnRtcp Handle of RTCP connection instance.
 * \param [in] stats RTP/RTCP stack statistics.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR update successfully.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRtcpUpdateStackStats(MmConnRtcpHndl        mmConnRtcp,
                                      MmConnRtcpStackStats  *stats);

#ifdef MMPBX_DSP_SUPPORT_RTCPXR
/**
 * Reset the RTCP statistics including the RTCP-XR DSP HW statistics
 *
 * This function needs to be used to reset the statistics
 *
 * \since v1.0
 *
 * \pre \c mmConnRtcp must be a valid handle.
 *
 * \post The Internal statistics value will be erased
 *
 * \param [in] mmConnRtcp Handle of RTCP connection instance.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR update successfully.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRtcpResetStats(MmConnRtcpHndl mmConnRtcp);

/**
 * Update mmConnRtcp RTP/TCP statistics.
 *
 * This function needs to be used to update the statistics of a mmConnRtcp.
 *
 * \since v1.0
 *
 * \pre \c mmConnRtcp must be a valid handle.
 *
 * \post The content of \c buff has been send to the network.
 *
 * \param [in] mmConnRtcp Handle of RTCP connection instance.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR update successfully.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRtcpUpdateStats(MmConnRtcpHndl mmConnRtcp);
#endif
#endif   /* ----- #ifndef MMCONNRTCP_INC  ----- */
