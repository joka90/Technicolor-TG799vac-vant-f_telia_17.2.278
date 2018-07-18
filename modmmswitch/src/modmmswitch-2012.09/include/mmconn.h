/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/** \file
 * Multimedia switch connection API.
 *
 * A multimedia switch connection is a source/sink of a multimedia stream.
 *
 * \version v1.0
 *
 *************************************************************************/
#ifndef  MMCONN_INC
#define  MMCONN_INC

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

#ifndef RTPPACKET_GET_SEQNUM
#define RTPPACKET_GET_SEQNUM(rtp)    ((rtp)->seq[0] << 8 | (rtp)->seq[1]);  /**< RtpHeader helper macro: Get sequence number field within RTP header. */
#endif

#ifndef RTPPACKET_SET_SEQNUM
#define RTPPACKET_SET_SEQNUM(rtp, val)                 \
    ((rtp)->seq[0] = (uint8_t)(((val) & 0xff00) >> 8), \
     (rtp)->seq[1] = (uint8_t)((val) & 0x00ff));                      /**< RtpHeader helper macro: Set sequence number field within RTP header. */
#endif

#ifndef RTPPACKET_GET_TIMESTAMP
#define RTPPACKET_GET_TIMESTAMP(rtp)           \
    ((rtp)->ts[0] << 24 | (rtp)->ts[1] << 16 | \
     (rtp)->ts[2] << 8 | (rtp)->ts[3]);                                     /**< RtpHeader helper macro: Get timestamp field within RTP header. */
#endif

#ifndef RTPPACKET_SET_TIMESTAMP
#define RTPPACKET_SET_TIMESTAMP(rtp, val)                  \
    ((rtp)->ts[0] = (uint8_t)(((val) & 0xff000000) >> 24), \
     (rtp)->ts[1] = (uint8_t)(((val) & 0x00ff0000) >> 16), \
     (rtp)->ts[2] = (uint8_t)(((val) & 0x0000ff00) >> 8),  \
     (rtp)->ts[3] = (uint8_t)((val) & 0x000000ff));                   /**< RtpHeader helper macro: Set timestamp field within RTP header. */
#endif

#ifndef RTPPACKET_GET_SSRC
#ifdef __BIG_ENDIAN
#define RTPPACKET_GET_SSRC(rtp)                    \
    ((rtp)->ssrc[0] << 24 | (rtp)->ssrc[1] << 16 | \
     (rtp)->ssrc[2] << 8 | (rtp)->ssrc[3]);                                 /**< RtpHeader helper macro: Get SSRC field within RTP header. */
#else
#define RTPPACKET_GET_SSRC(rtp)                    \
    ((rtp)->ssrc[3] << 24 | (rtp)->ssrc[2] << 16 | \
     (rtp)->ssrc[1] << 8 | (rtp)->ssrc[0]);                                 /**< RtpHeader helper macro: Get SSRC field within RTP header. */
#endif /* #ifdef __BIG_ENDIAN */
#endif

#ifndef RTPPACKET_SET_SSRC
#ifdef __BIG_ENDIAN
#define RTPPACKET_SET_SSRC(rtp, val)                         \
    ((rtp)->ssrc[0] = (uint8_t)(((val) & 0xff000000) >> 24), \
     (rtp)->ssrc[1] = (uint8_t)(((val) & 0x00ff0000) >> 16), \
     (rtp)->ssrc[2] = (uint8_t)(((val) & 0x0000ff00) >> 8),  \
     (rtp)->ssrc[3] = (uint8_t)((val) & 0x000000ff));                 /**< RtpHeader helper macro: Set SSRC field within RTP header. */
#else
#define RTPPACKET_SET_SSRC(rtp, val)                         \
    ((rtp)->ssrc[3] = (uint8_t)(((val) & 0xff000000) >> 24), \
     (rtp)->ssrc[2] = (uint8_t)(((val) & 0x00ff0000) >> 16), \
     (rtp)->ssrc[1] = (uint8_t)(((val) & 0x0000ff00) >> 8),  \
     (rtp)->ssrc[0] = (uint8_t)((val) & 0x000000ff));                 /**< RtpHeader helper macro: Set SSRC field within RTP header. */
#endif /* #ifdef __BIG_ENDIAN */
#endif


/*########################################################################
#                                                                       #
#  TYPES                                                                #
#                                                                       #
########################################################################*/

/**
 * RTP header template, can be used to interprete data.
 */
typedef struct {
#ifdef __BIG_ENDIAN
    uint8_t version : 2;  /**< protocol version */
    uint8_t p       : 1;  /**< padding flag */
    uint8_t x       : 1;  /**< header extension flag */
    uint8_t cc      : 4;  /**< CSRC count */
    uint8_t m       : 1;  /**< marker bit */
    uint8_t pt      : 7;  /**< payload type */
#else
    uint8_t cc      : 4;  /**< CSRC count */
    uint8_t x       : 1;  /**< header extension flag */
    uint8_t p       : 1;  /**< padding flag */
    uint8_t version : 2;  /**< protocol version */
    uint8_t pt      : 7;  /**< payload type */
    uint8_t m       : 1;  /**< marker bit */
#endif
    uint8_t seq[2];       /**< 16-bit sequence number */
    uint8_t ts[4];        /**< 32-bit timestamp */
    uint8_t ssrc[4];      /**< synchronization source */
} MmConnRtpHeader;

/**
 * Multimedia connection packet types
 */

typedef enum {
    MMCONN_PACKET_TYPE_RAW = 0, /**< RAW media. */
    MMCONN_PACKET_TYPE_RTP,     /**< RTP media. */
    MMCONN_PACKET_TYPE_RTCP,    /**< RTCP media. */
    MMCONN_PACKET_TYPE_UDPTL,   /**< UDPTL media. */
} MmConnPacketType;

/**
 * Multimedia connection packet header description.
 */
typedef struct {
    MmConnPacketType type; /**< Packet type */
} MmConnPacketHeader;

/**
 * Multimedia event types.
 */
typedef enum {
    MMCONN_EV_INGRESS_RTP_MEDIA = 0,      /**< Incoming (connection -> switch) media. Parm will hold payload type */
    MMCONN_EV_INGRESS_MEDIA_TIMEOUT,      /**< No media received for a certain time (for IP connections). Parm has no meaning for this event. */
    MMCONN_EV_DONE,                       /**< Something is done. Parm has no meaning for this event. */
} MmConnEventType;

/**
 * Multimedia event types.
 */
typedef struct {
    MmConnEventType type;       /**< Event Type */
    int             parm;       /**< Optional parameter of event */
} MmConnEvent;

/*
 * DSP statistics parameters as defined by Broadcom docs.:
 *      DSLcChange-TMR102-RDS.pdf file, Endpoint statistics.
 * The parameters are mapped 1 on 1 with EPZCNXSTATS structure and EPZT38CNXSTATS structure
 */
struct MmConnDspRtpStatsParm {
    unsigned int    txpkts;                 /**< Packets Sent */
    unsigned int    txbytes;                /**< Octets Sent */
    unsigned int    rxpkts;                 /**< Packets Received */
    unsigned int    rxbytes;                /**< Octets Received */
    unsigned int    lost;                   /**< tPackets lost */
    unsigned int    discarded;              /**< Discarded packets */
    unsigned int    txRtcpPkts;             /**< Number of ingress RTCP packets */
    unsigned int    rxRtcpPkts;             /**< Number of egress RTPC packets */
    unsigned short  txRtcpXrPkts;           /**< Number of ingress RTCP XR packets */
    unsigned short  rxRtcpXrPkts;           /**< Number of egress RTCP XR packets */
    unsigned short  jitterBufferOverruns;   /**< Number of jitter buffer overruns */
    unsigned short  jitterBufferUnderruns;  /**< Number of jitter buffer underruns */
    unsigned int    jitter;                 /**< Interarrival jitter estimate from RTP service (in ms) */
    unsigned short  peakJitter;             /**< Peak jitter/holding time from voice PVE service (in ms) */
    unsigned short  jitterBufferMin;        /**< Jitter buffer minimum delay */
    unsigned short  jitterBufferAverage;    /**< Jitter buffer average delay */
    unsigned short  jitterBufferMax;        /**< Jitter buffer absolute maximum delay */
    unsigned int    latency;                /**< Avg. tx delay (in ms) */
    unsigned short  peakLatency;            /**< Peak tx delay (in ms) */
    unsigned short  mosLQ;                  /**< MOS-listening quality (value *10) */
    unsigned short  mosCQ;                  /**< MOS-conversational quality (value*10) */
    unsigned short  reserved;
    unsigned int    txpg;                   /**< T38: Pages sent */
    unsigned int    rxpg;                   /**< T38: Pages received */
};

/**
 * Handle that represents the DSP Rtp Statistics parameters
 *
 * \since v1.0
 */
typedef struct MmConnDspRtpStatsParm  *MmConnDspRtpStatsParmHndl;

/*
 * DSP command
 */
typedef enum {
    MMCONN_DSP_GET_STATS,                   /**< Get the DSP statistics */
    MMCONN_DSP_RESET_STATS                  /**< Reset the DSP statistics */
}MmConnDspCommand;

/**
 * Callback function to write data to an endpoint.
 *
 * An endpoint which wants to receive data from a connection instance of multimedia switch can implement this callback function which allows the connection to push data to the endpoint.
 *
 * \since v1.0
 *
 * \pre \c NONE
 *
 * \post The endpoint has handled the data push.
 *
 * \param [in] conn Handle of connection instance.
 * \param[in] data which is pushed to the endpoint.
 * \param [in] header Header used to identify packet.
 * \param [in] bytes Number of bytes which were pushed.
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR The write action to the endpoint suceeded.
 * \todo Add other possible return values after implementation.
 */
typedef MmPbxError (*MmConnWriteCb)(MmConnHndl          conn,
                                    void                *data,
                                    MmConnPacketHeader  *header,
                                    unsigned int        bytes);

/**
 * Callback function to retrieve the Dsp Statistics from the Linux Connection
 *
 * An Rtp connection who want to retrieve the DSP statistics could call this function.
 * The statistics can only be obtained if the RTCP and the Linux connection are cross connected.
 * If the Linux Connection registered a callback to access the DSP statistics then the requested statistics will be returned.
 *
 * \since v1.0
 *
 * \pre \c mmConn must be a valid RTCP or Relay handle.
 * \pre \c param optional parameters that could represent a valid handle of
 *               MmConnDspRtpStatsParm structure.
 * \pre \c cmd must be one of the MmConnDspCommand.
 *
 * \param [in] mmConn Rtcp switch connection handle.
 * \param [out] param could be a MmConnDspRtpStatsParm structure handle or NULL
 * \param [in] MmConnDspCommand command to the DSP.
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR The command is successfully executed
 * \todo Add other possible return values after implementation.
 */
typedef MmPbxError (*MmConnDspCtrlCb)(MmConnHndl        mmConn,
                                      void              *param,
                                      MmConnDspCommand  cmd);

/*########################################################################
#                                                                       #
#  FUNCTION PROTOTYPES                                                  #
#                                                                       #
########################################################################*/


/**
 * Write packet to a multimedia switch  connection instance.
 *
 * This function needs to be used to write data to a connection instance.
 *
 * \since v1.0
 *
 * \pre \c conn must be a valid handle.
 *
 * \post The content of \c buff has been written to the connection.
 *
 * \param [in] conn Handle of connection instance.
 * \param [in] header Header used to identify packet.
 * \param [in] buff Buffer which contains data to write.
 * \param [in] bytes Number of bytes to write.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR Data successfully written.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnWrite(MmConnHndl         conn,
                       MmConnPacketHeader *header,
                       uint8_t            *buff,
                       unsigned int       bytes);

/**
 * Set cookie attached to this connection instance.
 *
 * This function sets the cookie.
 *
 * \since v1.0
 *
 * \pre \c conn must be a valid handle.
 *
 * \post \c cookie contains  the cookie of the \c conn connection object.
 *
 * \param [in] conn Multimedia switch connection handle.
 * \param [out] cookie Endpoint defined cookie.
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR The cookie has been successfully set.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnSetCookie(MmConnHndl conn,
                           void       *cookie);

/**
 * Get cookie attached to this connection instance.
 *
 * This function returns the cookie.
 *
 * \since v1.0
 *
 * \pre \c conn must be a valid handle.
 *
 * \post \c cookie contains  the cookie of the \c conn connection object.
 *
 * \param [in] conn Multimedia switch connection handle.
 * \param [out] cookie Endpoint defined cookie.
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR The cookie has been successfully returned.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnGetCookie(MmConnHndl conn,
                           void       **cookie);

/**
 * Set Control connection attached to this connection instance.
 *
 * Attach connection control to the main connection.
 *
 * \since v1.0
 *
 * \pre \c mmConn and conn must be a valid handle.
 *
 * \post \c control connection is attached to this connection instance.
 *
 * \param [in] mmConn master connection handle.
 * \param [in] conn control connection handle
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR The countrol connection has been successfully attached.
 */
MmPbxError mmConnSetConnControl(MmConnHndl  mmConn,
                                MmConnHndl  conn);

/**
 * Get Control connection attached to this connection instance.
 *
 * \since v1.0
 *
 * \pre \c mmConn and conn must be a valid handle.
 *
 * \post \c returning the connection control attached to this connection instance.
 *
 * \param [in] mmConn master connection handle.
 * \param [out] conn control connection handle
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR The countrol connection has been successfully retrieved.
 */
MmPbxError mmConnGetConnControl(MmConnHndl  mmConn,
                                MmConnHndl  *conn);

/**
 * Send an event to the user space counterpart of mmConn.
 *
 * This function can be used to send an event to the user space counterpart of mmConn.
 *
 * \since v1.0
 *
 * \pre \c mmConn must be a valid handle.
 *
 * \post The event has been end to the user space counterpart of mmConn.
 *
 * \param [in] mmConn Handle of connection instance.
 * \param [in] event Event to send.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR Event successfully sent.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnSendEvent(MmConnHndl   mmConn,
                           MmConnEvent  *event);

/**
 * Register write callback for a multimedia switch  connection instance.
 *
 * This callback function can be registered by an endpoint to get an event when data is pushed to the endpoint.
 *
 * \since v1.0
 *
 * \pre \c conn must be a valid handle.
 * \pre \c cb must be a valid callback function of type ::MmConnWriteCb.
 *
 * \post The endpoint has handled the event.
 *
 * \param [in] conn Handle of multimedia switch connection instance.
 * \param [in] cb Write callback function.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The callback function has been successfully registered.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRegisterWriteCb(MmConnHndl     conn,
                                 MmConnWriteCb  cb);

/**
 * Unregister write callback for a multimedia switch connection instance.
 *
 * This function unregisters a write callback function from a connection instance.
 *
 * \since v1.0
 *
 * \pre \c conn must be a valid handle.
 *
 * \post The callback function will no longer be called.
 *
 * \param [in] conn Handle of multimedia switch connection instance.
 * \param [in] cb Write callback function.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The callback function has been successfully unregistered.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnUnregisterWriteCb(MmConnHndl     conn,
                                   MmConnWriteCb  cb);

#ifdef MMPBX_DSP_SUPPORT_RTCPXR
/**
 * Register a callback for handling the DSP operation.
 *
 * This callback function usually will be registered by the Linux connection.
 * The callback will be an access point to process a DSP command.
 *
 * \since v1.0
 *
 * \pre \c mmConn must be a valid handle.
 * \pre \c cb must be a valid callback function of type :MmConnDspCtrlCb.
 *
 * \param [in] mmConn Handle of Linux Connection connection instance.
 * \param [in] cb Write callback function.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The callback function has been successfully registered.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnRegisterDspCtrlCb(MmConnHndl       mmConn,
                                   MmConnDspCtrlCb  cb);
#endif
#endif   /* ----- #ifndef MMCONN_INC  ----- */
