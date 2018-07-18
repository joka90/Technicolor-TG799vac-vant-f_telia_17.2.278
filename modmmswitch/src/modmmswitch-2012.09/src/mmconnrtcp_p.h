/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/** \file
 * Private multimedia switch rtcp connection API.
 *
 * \version v1.0
 *
 *************************************************************************/
#ifndef  MMCONNRTCP_P_INC
#define  MMCONNRTCP_P_INC

/*########################################################################
#                                                                       #
#  HEADER (INCLUDE) SECTION                                             #
#                                                                       #
########################################################################*/
#include <linux/socket.h>
#include <linux/types.h>

#include "mmconnrtcp.h"
#include "mmconnrelay_p.h"
#include "mmswitch_p.h"
#include "mmconn_p.h"

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
 * RTCP instance statistics
 */
typedef struct {
    /* Local statisticstics, measured by the DSP or RTP instance*/
    uint64_t      callLength;                   /**< Length of the call in milliseconds.*/
    uint64_t      txRtpPackets;                 /**< RTP packets sent. */
    uint64_t      txRtpBytes;                   /**< RTP bytes sent. */
    uint64_t      rxRtpPackets;                 /**< RTP packets received. */
    uint64_t      rxRtpBytes;                   /**< RTP bytes received. */
    uint64_t      rxTotalPacketsLost;           /**< Total number of lost packets in RX stream. */
    uint64_t      rxTotalPacketsDiscarded;      /**< Total number of discarded packets in RX stream. */
    unsigned long rxPacketsLostRate;            /**< Packets Lost Rate */
    unsigned long rxPacketsDiscardedRate;       /**< Discarded Packets Rate */
    unsigned long signalLevel;                  /**< The Voice signal relative level in dBm */
    unsigned long noiseLevel;                   /**< The Noise level in dBm */
    unsigned long RERL;                         /**< Residual Echo Return Loss */
    unsigned long RFactor;                      /**< Voice quality metric in R-Factor */
    unsigned long externalRFactor;              /**< Voice quality metric in R-Factor for external network */
    unsigned long mosLQ;                        /**< MOS Score Line Quality (value * 10)*/
    unsigned long mosCQ;                        /**< MOS Score Conversational Quality (value * 10)*/
    unsigned long meanE2eDelay;                 /**< Mean end-to-end delay in microseconds. */
    unsigned long worstE2eDelay;                /**< Worst end-to-end delay in microseconds. */
    unsigned long currentE2eDelay;              /**< Current end-to-end delay in microseconds. */
    unsigned long rxJitter;                     /**< jitter in RX stream. \n Jitter is measured in timestamp units (0.125ms for 8 kHz codecs). */
    unsigned long rxMinJitter;                  /**< Min jitter in RX stream. \n Jitter is measured in timestamp units (0.125ms for 8 kHz codecs). */
    unsigned long rxMaxJitter;                  /**< Max jitter in RX stream. \n Jitter is measured in timestamp units (0.125ms for 8 kHz codecs). */
    unsigned long rxDevJitter;                  /**< Dev jitter in RX stream. \n Jitter is measured in timestamp units (0.125ms for 8 kHz codecs). */
    unsigned long meanRxJitter;                 /**< Mean Jitter in RX stream. \n Jitter is measred in timestamp units (0.125 ms for 8kHz codecs).*/
    unsigned long worstRxJitter;                /**< Worst Jitter in RX stream. \n Jitter is measred in timestamp units (0.125 ms for 8kHz codecs).*/
    unsigned long jitterBufferOverruns;         /**< Number of Jitter Buffer overruns */
    unsigned long jitterBufferUnderruns;        /**< Number of Jitter buffer underruns */
    /* Remote party statistics, measured by the remote party */
    uint64_t      remoteTxRtpPackets;           /**< Number of RTP packets sent. */
    uint64_t      remoteTxRtpBytes;             /**< Number of RTP bytes send.*/
    uint64_t      remoteRxTotalPacketsLost;     /**< Total number of lost packets in RX stream.   */
    unsigned long remoteRxPacketsLostRate;      /**< Lost packets rate in RX stream.   */
    unsigned long remoteRxPacketsDiscardedRate; /**< Total number of discared packets in RX stream.   */
    unsigned long remoteSignalLevel;            /**< Signal Level in dBm in RX stream. */
    unsigned long remoteNoiseLevel;             /**< Noise level in dBm  in RX stream. */
    unsigned long remoteRERL;                   /**< Residual Echo Return Loss */
    unsigned long remoteRFactor;                /**< Voice quality metric in R-Factor */
    unsigned long remoteExternalRFactor;        /**< Voice quality metric in R-Factor for external network */
    unsigned long remoteMosLQ;                  /**< MOS Score Line Quality (value * 10)*/
    unsigned long remoteMosCQ;                  /**< MOS Score Conversational Quality (value * 10)*/
    unsigned long remoteMeanE2eDelay;           /**< end-to-end delay in microseconds. */
    unsigned long remoteWorstE2eDelay;          /**< end-to-end delay in microseconds. */
    unsigned long remoteCurrentE2eDelay;        /**< end-to-end delay in microseconds. */
    unsigned long remoteRxJitter;               /**< Mean Jitter in RX stream.\n Jitter is measured in timestamp units (0.125ms for 8 kHz codecs).*/
    unsigned long minRemoteRxJitter;            /**< Min jitter in RX stream. \n Jitter is measured in timestamp units (0.125ms for 8 kHz codecs). */
    unsigned long maxRemoteRxJitter;            /**< Max jitter in RX stream. \n Jitter is measured in timestamp units (0.125ms for 8 kHz codecs). */
    unsigned long devRemoteRxJitter;            /**< Dev jitter in RX stream. \n Jitter is measured in timestamp units (0.125ms for 8 kHz codecs). */
    unsigned long meanRemoteRxJitter;           /**< Mean Jitter in RX stream. \n Jitter is measred in timestamp units (0.125 ms for 8kHz codecs).*/
    unsigned long remoteRxWorstJitter;          /**< Worst jitter in RX stream.\n Measured in timestamp units (0.125 ms for 8kHz codecs).*/
    /*AT&T extension*/
    uint64_t      txRtcpPackets;            /**< RTCP packets sent. */
    uint64_t      rxRtcpPackets;            /**< RTCP packets received. */
    unsigned long localSumFractionLoss;     /**< Sum of tx fraction loss */
    unsigned long localSumSqrFractionLoss;  /**< Sum of square of tx fraction loss */
    unsigned long remoteSumFractionLoss;    /**< Sum of tx fraction loss */
    unsigned long remoteSumSqrFractionLoss; /**< Sum of square of tx fraction loss */
    unsigned long localSumJitter;           /**< summary of tx Interarrival jitter */
    unsigned long localSumSqrJitter;        /**< Sum of square of tx Interarrival jitter */
    unsigned long remoteSumJitter;          /**< Sum of tx Interarrival jitter*/
    unsigned long remoteSumSqrJitter;       /**< summary of square of tx Interarrival jitter */
    unsigned long sumRoundTripDelay;        /**< Sum of Round Trip Delay */
    unsigned long sumSqrRoundTripDelay;     /**< Sum of Square of Max Round Trip Delay */
    unsigned long maxOneWayDelay;           /**< Max One Way Delay */
    unsigned long sumOneWayDelay;           /**< Sum of One Way Delay */
    unsigned long sumSqrOneWayDelay;        /**< Sum of Square of One Way Delay */
    /* RTCP frame buffer */
    unsigned long txRtcpFrameLength[MMCONN_RTCP_MAX_SAVED_RTCPXR_FRAME]; /**< Length of tx RTCP frame*/
    uint8_t       *txRtcpFrameBuffer[MMCONN_RTCP_MAX_SAVED_RTCPXR_FRAME];/**< Buffer of tx RTCP frame*/
    unsigned long rxRtcpFrameLength[MMCONN_RTCP_MAX_SAVED_RTCPXR_FRAME];/**< Length of rx RTCP frame*/
    uint8_t       *rxRtcpFrameBuffer[MMCONN_RTCP_MAX_SAVED_RTCPXR_FRAME];/**< Buffer of rx RTCP frame*/
} MmConnRtcpStats;


/*
 * MmConnRtcp structure definition
 */
/**
 * Rtcp connection configuration parameters.
 */
typedef struct {
    int                     localMediaSockFd; /**< File Descriptor of local media socket, created in user space */
    int                     localRtcpSockFd;  /**< File Descriptor of local RTCP socket, created in user space */
    struct sockaddr_storage remoteMediaAddr;  /**< Remote Media Address info, generated in user space */
    struct sockaddr_storage remoteRtcpAddr;   /**< Remote RTCP Address info, generated in user space */
    MmConnPacketHeader      header;           /**<Header to add when we receive data from socket */
    unsigned int            mediaTimeout;     /**< Media timeout (ms, 0 == disable) */
    unsigned int            mediaMuteTime;    /**< Mute incoming media for x ms ( 0 == disable) */
    unsigned int            rtcpBandwidth;    /**< RTCP bandwidth */
    unsigned int            genRtcp;          /**< Do we need to generate RTCP? (0==FALSE, 1==TRUE) */
} MmConnRtcpConfig;

/**
 * Rtcp connection configuration parameters.
 */
struct MmConnRtcp {
    struct MmConn     mmConn;                   /**< Parent class */
    MmConnRtcpConfig  config;                   /**< Configuration */
    MmConnRtcpStats   stats;                    /**< Statistics */
    spinlock_t        lock;                     /**< Lock */
    MmSwitchSock      localMediaSock;           /**< local Media Socket */
    MmSwitchSock      localRtcpSock;            /**< local Rtcp Socket */
    MmSwitchTimer     mediaTimeoutTimer;        /**< Media timeout timer */
    MmSwitchTimer     rtcpTimer;                /**< RTCP transmit timer */
    int               packetCounter;            /**< Packet counter, used to detect media timeout */
    unsigned long     mediaMuteTimeout;         /**< Timeout value, used to mute first x ms of media */
    unsigned int      rtcpTimerTimeout;         /**<  RTCP transmit timer timeout value */
#ifdef MMPBX_DSP_SUPPORT_RTCPXR
    void              *statsCookie;             /**< Cookie to be used by RTP/RTCP statistics */
#else
    void              *stackCookie;             /**< Cookie to be used by RTP/RTCP stack */
#endif
    uint64_t          callStartJiffies;         /**< Jiffies when first byte is send to the network */
};


/*########################################################################
#                                                                       #
#  FUNCTION PROTOTYPES                                                  #
#                                                                       #
########################################################################*/
/**
 * Set trace level of all multimedia switch rtcp connections.
 *
 * This function makes it possible to modify the trace level of all relay connections. This trace level is also dependant on the trace level which was used to compile the code.
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
MmPbxError mmConnRtcpSetTraceLevel(MmPbxTraceLevel level);

/**
 * Initialise mmConnRtcp component of mmswitch kernel module.
 *
 * This function initialises the mmConnRtcp component of the mmswitch kernel module.
 *
 * \since v1.0
 *
 * \pre None.
 *
 * \post The mmConnRelay component of mmswitch is initialised.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR the mmConnRelay component has been initialised successfully.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnRtcpInit(void);

/**
 * Deinitialise mmConnRtcp component of mmswitch kernel module.
 *
 * This function deinitialises the mmConnRtcp component of the mmswitch kernel module.
 *
 * \since v1.0
 *
 * \pre None.
 *
 * \post The mmConnRtcp component of mmswitch is deinitialised.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR the mmConnRtcp component has been deinitialised successfully.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnRtcpDeinit(void);

/**
 * Constructor of a multimedia switch rtcp connection instance.
 *
 * This function is the constructor of a multimedia switch rtcp connection instance.
 *
 * \since v1.0
 *
 * \pre \c connGr must be a valid handle.
 *
 * \post \c rtcp contains the handle of a valid multimedia switch rtcp connection instance.
 *
 * \param [in] config Configuration of rtcp connection instance of multimedia switch.
 * \param [out] rtcp Handle of relay connection instance of multimedia switch.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR A rtcp connection handle has been successfully retrieved and is not NULL.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRtcpConstruct(MmConnRtcpConfig *config,
                               MmConnRtcpHndl   *rtcp);

/**
 * Modify Media Packet type of RTCP connection instance.
 *
 * This function allows the user to modify the media packet type of an RTCP connection instance.
 * This packet type is used as a label for all incoming traffic.
 *
 * \since v1.0
 *
 * \pre \c rtcp must be a valid handle.
 *
 * \post The packet type will be modfied.
 *
 * \param [in] rtcp Handle of RTCP connection instance.
 * \param [in] type Packet type.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR Parameter modified successfully.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnRtcpModMediaPacketType(MmConnRtcpHndl    rtcp,
                                        MmConnPacketType  type);

/**
 * Modify RTCP generator state of RTCP connection instance.
 *
 * This function allows the user enable/disable RTCP generation.
 *
 * \since v1.0
 *
 * \pre \c rtcp must be a valid handle.
 *
 * \post The state will be modfied.
 *
 * \param [in] rtcp Handle of RTCP connection instance.
 * \param [in] genRtcp RTCP generator state (0 == disabled, 1 == enabled)
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR Parameter modified successfully.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnRtcpModGenRtcp(MmConnRtcpHndl  rtcp,
                                unsigned int    genRtcp);

/**
 * Modify remote media socket address .
 *
 * This function allows the user to modify remote media socket address .
 *
 * \since v1.0
 *
 * \pre \c rtcp must be a valid handle.
 *
 * \post The remote media socket address will be modfied.
 *
 * \param [in] rtcp Handle of RTCP connection instance.
 * \param [in] remoteMediaAddr Remote Media socket address .
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR Parameter modified successfully.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnRtcpModRemoteMediaAddr(MmConnRtcpHndl          rtcp,
                                        struct sockaddr_storage *remoteMediaAddr);

/**
 * Modify remote RTCP socket address.
 *
 * This function allows the user to modify remote RTCP socket address info.
 *
 * \since v1.0
 *
 * \pre \c rtcp must be a valid handle.
 *
 * \post The remote RTCP socket address info will be modfied.
 *
 * \param [in] rtcp Handle of RTCP connection instance.
 * \param [in] remoteRtcpAddr Remote RTCP socket address info.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR Parameter modified successfully.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnRtcpModRemoteRtcpAddr(MmConnRtcpHndl           rtcp,
                                       struct sockaddr_storage  *remoteRtcpAddr);

/**
 * Modify RTCP generator RTCP bandwidth.
 *
 * This function allows the user to modify the bandwidth used for RTCP.
 * RTCP bandwidth should be 5% of RTP bandwidth.
 *
 * \since v1.0
 *
 * \pre \c rtcp must be a valid handle.
 *
 * \post The bandwidth will be modfied.
 *
 * \param [in] rtcp Handle of RTCP connection instance.
 * \param [in] rtcpBandwidth RTCP bandwitdh (bytes/second).
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR Parameter modified successfully.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnRtcpModRtcpBandwidth(MmConnRtcpHndl  rtcp,
                                      unsigned int    rtcpBandwidth);
#endif   /* ----- #ifndef MMCONNRTCP_P_INC  ----- */
