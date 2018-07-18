/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2015-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/** \file
 *      RTP private data structure / function declarations
 *
 * \version v1.0
 *
 *************************************************************************/
#ifndef  MMCONNBRCMSTATS_P_H
#define  MMCONNBRCMSTATS_P_H

/*########################################################################
#                                                                       #
#  HEADER (INCLUDE) SECTION                                             #
#                                                                       #
########################################################################*/
#include "mmconn.h"
#include "mmconnrtcp.h"

/*########################################################################
#                                                                       #
#  MACROS/DEFINES                                                       #
#                                                                       #
########################################################################*/
#define RTP_VERSION    2            /* Protocol version */

/* To indicate that the RTCPXR parameter is not available */
#define RTCPXR_UNAVAILABLE_VAL    127
#define RTCPXR_BT_UPDATED(val, bt)    (val & (1 << bt))

/* Define the Fix Block Types Length, multiple of 32 bits*/

/* RTCPXR_BT1 & 2 */
#define RTCPXR_MIN_RLE_BLOCKTYPE_SIZE     3 /* 3 * 32 bits: SSRC = 1 Long, Begin & End Seq = 1 Long , Info & NULL Chunk = 1 Long*/
#define RTCPXR_MIN_RLE_CHUNKS_SIZE        1 /* 1 * 32 bits: Info & NULL Chunk = 1 Long, no Bit Vector chunk */
#define RTCPXR_RUNLENGTH_CHUNK_TYPE       0
#define RTCPXR_BITVECTOR_CHUNK_TYPE       1

/* RTCPXR_BT 3 */
#define RTCPXR_MIN_PACKET_RECEIPT_TIME_REPORT_SIZE    2 /* in 32 bits SSRC = 1 Long, Begin & End Seq = 1 Long */

/* RTCPXR_BT 4 */
#define RTPCPXR_RECEIVER_REF_TIME_REPORT_SIZE    2  /* in 32 bits */

/* RTCPXR BT 5 */

/* RTCPXR BT 6 */
#define RTPCPXR_LENGTH_BLOCKTYPE_6    9
#define RTCPXR_TTL_IPV4               1
#define RTCPXR_TTL_IPV6               2

/* RTCPXR BT 7 */
/* Signal Related Metrics [RFC3611] Chapter 4.7.4 */
#define RTPCPXR_SIGNAL_RELATED_METRICS_BLOCK_SIZE     8

#define RTCPXR_GAP_THRESHOLD                          16

/* RX Configuration parameters [RFC3611] Chapter 4.7.6 */
#define RTCPXR_RXCONFIG_PARAM_STANDARD        0x03
#define RTCPXR_RXCONFIG_PARAM_ENHANCED        0x02
#define RTCPXR_RXCONFIG_PARAM_DISABLED        0x01
#define RTCPXR_RXCONFIG_PARAM_UNSPECIFIED     0

#ifdef __BIG_ENDIAN
#define RTCPXR_GET_RXCONFIG_PLC(val)          ((val >> 6) & 0x03)
#define RTCPXR_GET_RXCONFIG_JBA(val)          ((val >> 4) & 0x03)
#define RTCPXR_GET_RXCONFIG_JBA_RATE(val)     (val & 0x0f)
#else
#define RTCPXR_GET_RXCONFIG_PLC(val)          (val & 0x03)
#define RTCPXR_GET_RXCONFIG_JBA(val)          ((val >> 2) & 0x03)
#define RTCPXR_GET_RXCONFIG_JBA_RATE(val)     ((val >> 4) & 0x0f)
#endif


/* Call quality or Transmission Quality Metrics [RFC3611] Chapter 4.7.5 */
#define RTCPXR_RFACTOR_LOW_QUALITY      0
#define RTCPXR_RFACTOR_MED_QUALITY      50
#define RTCPXR_RFACTOR_BEST_QUALITY     100
#define RTCPXR_RFACTOR_UNAVAILABLE      127

#define RTCPXR_MOS_LOW_QUALITY          10
#define RTCPXR_MOS_BEST_QUALITY         50
#define RTCPXR_MOS_UNAVAILABLE          127

/* Receiver Configuration byte [RFC3611] Chapter 4.7.6 */
#define RTCPXR_PACKET_LOSS_CONCEALMENT_STANDARD       0x3
#define RTCPXR_PACKET_LOSS_CONCEALMENT_ENHANCED       0x2
#define RTCPXR_PACKET_LOSS_CONCEALMENT_DISABLED       0x1
#define RTCPXR_PACKET_LOSS_CONCEALMENT_UNSPECIFIED    0

#define RTCPXR_JITTER_BUFFER_ADAPTIVE                 0x3
#define RTCPXR_JITTER_BUFFER_NONADAPTIVE              0x2
#define RTCPXR_JITTER_BUFFER_RESERVED                 0x1
#define RTCPXR_JITTER_BUFFER_UNKNOWN                  0

/*########################################################################
#                                                                       #
#  TYPES                                                                #
#                                                                       #
########################################################################*/
typedef enum {
    RTCP_SR   = 200,
    RTCP_RR   = 201,
    RTCP_SDES = 202,
    RTCP_BYE  = 203,
    RTCP_APP  = 204,
    RTCP_XR   = 207
} RTCPTYPE;

typedef enum {
    RTCP_SDES_END   = 0,
    RTCP_SDES_CNAME = 1,
    RTCP_SDES_NAME  = 2,
    RTCP_SDES_EMAIL = 3,
    RTCP_SDES_PHONE = 4,
    RTCP_SDES_LOC   = 5,
    RTCP_SDES_TOOL  = 6,
    RTCP_SDES_NOTE  = 7,
    RTCP_SDES_PRIV  = 8
} RTCPSDESTYPE;

/*
 * RTCP common header word
 */
typedef struct {
#ifdef __BIG_ENDIAN
    unsigned int    version     : 2;  /* protocol version */
    unsigned int    padding     : 1;  /* padding flag */
    unsigned int    count       : 5;  /* varies by packet type */
#else
    unsigned int    count       : 5;  /* varies by packet type */
    unsigned int    padding     : 1;  /* padding flag */
    unsigned int    version     : 2;  /* protocol version */
#endif
    unsigned int    packetType  : 8;  /* RTCP packet type */
    unsigned short  length;           /* pkt len in words, w/o this word */
} RTCPCOMMON;

/*
** Reception report block
*/
typedef struct {
    unsigned long ssrc;           /* data source being reported */
    unsigned int  fraction  : 8;  /* fraction lost since last SR/RR */
    int           lost      : 24; /* cumul. no. pkts lost (signed!) */
    unsigned long last_seq;       /* extended last seq. no. received */
    unsigned long jitter;         /* interarrival jitter */
    unsigned long lsr;            /* last SR packet from this source */
    unsigned long dlsr;           /* delay since last SR packet */
} RTCPRR;

/*
** SDES item
*/
typedef struct {
    uint8_t type;                   /* type of item (RTCPSDESTYPE) */
    uint8_t length;                 /* length of item (in octets) */
    uint8_t data[1];                /* text, not null-terminated */
} RTCPSDESITEM;


/*
 * RTCP-XR Block Type header word
 *
 * The Header of the each RTCP-XR Block types could differ depending of the
 * purpose of the Block Types itself
 *
 */
typedef struct {
    unsigned char   blockTypeId;     /* Block Type [1..7] */
    union {
#ifdef __BIG_ENDIAN
        /* Normal/Common Type Summary header*/
        unsigned char commonHeader;   /* Block Type Summary header or reserved (BT4, BT5, BT7)*/

        /* Block Type Header Summary with Thinning parameters (BT1, BT2, BT3) */
        struct {
            unsigned char reserved1 : 4;    /* Reserved */
            unsigned char thinning  : 4;    /* Thinning (T) */
        }             thinningHeader;

        /* Block Type Header for Summary report purposes (BT6) */
        struct {
            unsigned char loss      : 1;  /* Loss Report flag */
            unsigned char duplicate : 1;  /* Duplicate report flag */
            unsigned char jitter    : 1;  /* Jitter Flag */
            unsigned char toh       : 2;  /* TTL or Hop Limit flag */
            unsigned char reserved2 : 3;  /* Reserved */
        }             reportHeader;
#else
        /* Normal Type Summary */
        unsigned char header;   /* Block Type Summary or reserved (BT4, BT5, BT7)*/

        /* Block Type Header Summary with Thinning parameters (BT1, BT2, BT3) */
        struct {
            unsigned char thinning  : 4;    /* Thinning (T) */
            unsigned char reserved1 : 4;    /* Reserved */
        }             thinningHeader;

        /* Block Type Header for Summary report purposes (BT6) */
        struct {
            unsigned char reserved2 : 3;  /* Reserved */
            unsigned char toh       : 2;  /* TTL or Hop Limit flag */
            unsigned char jitter    : 1;  /* Jitter Flag */
            unsigned char duplicate : 1;  /* Duplicate report flag */
            unsigned char loss      : 1;  /* Loss Report flag */
        }             reportHeader;
#endif
    }               typeSpecific;
    unsigned short  blockLength;         /* varies by packet type */
}RTCPXRCOMMON;

/*
 * RTCP-XR Report Block Type 1 and Block Type 2
 *
 * Block Type 1: Loss RLE Report Block
 * Ref. [RFC3611] chapter 4.1: Loss RLE Report Block
 * Represent the detailed reporting upon individual receipt or loss frames.
 *
 * Block Type 2: Duplicate RLE Report Block
 * Ref. [RFC3611] chapter 4.2: Duplicate RLE Report Block
 * Represent the detailed reporting upon individual Duplication of frames.
 */
typedef struct {
    union {
#ifdef __BIG_ENDIAN
        /* Run Length Chunk */
        struct {
            unsigned short  chunkType : 1;  /* A zero identifies this as a run length chunk */
            unsigned short  runType   : 1;  /* Zero indicates a run of 0s. One indicates a run of 1s */
            unsigned short  runLength : 14; /* A value between 1 - 16,383. */
        }               runLengthChunk;
        /* Bit Vector Chunk */
        struct {
            unsigned short  chunkType : 1;  /* A one identifies this as a bit vector chunk */
            unsigned short  bitVector : 15; /* each bit will represent the sequnce number that is lost or duplicated */
        }               bitVectorChunk;
#else
        /* Run Length Chunk */
        struct {
            unsigned short  runLength : 14; /* A value between 1 - 16,383. */
            unsigned short  runType   : 1;  /* Zero indicates a run of 0s. One indicates a run of 1s */
            unsigned short  chunkType : 1;  /* A zero identifies this as a run length chunk */
        }               runLengthChunk;
        /* Bit Vector Chunk */
        struct {
            unsigned short  bitVector : 15; /* each bit will represent the sequnce number that is lost or duplicated */
            unsigned short  chunkType : 1;  /* A one identifies this as a bit vector chunk */
        }               bitVectorChunk;
#endif
        /* Terminating NULL Chunk */
        unsigned short  nullChunk;          /* This chunk is all zero */
    }chunkInfo;
} RLE_CHUNK_INFO;

typedef struct {
    unsigned long   ssrc;               /* SSRC of source */
    unsigned short  beginSeq;           /* The first sequence number that this block reports on */
    unsigned short  endSeq;             /* The last sequence number that this block reports on plus one */
    RLE_CHUNK_INFO  runLengthChunk[1];  /* Loss/Duplicate RLE Chunk information, variable length but at least 1 block */
} RTCPXR_BT1;

/*
 * RTCP-XR Report Block Type 3
 * Ref. [RFC3611] chapter 4.3: Packet Receipt Times Report Block
 * Will reports the packet reception timer per-sequence number
 */
typedef struct {
    unsigned long   ssrc;           /* SSRC of source */
    unsigned short  beginSeq;       /* The first sequence number that this block reports on */
    unsigned short  endSeq;         /* The last sequence number that this block reports on plus one */
    unsigned long   rxTime[1];      /* Receipt time of packet begin seq (mod 65536) ; variable-length list */
} RTCPXR_BT3;

/*
 * RTCP-XR Report Block Type 4
 * Ref. [RFC3611] chapter 4.4: Receiver Reference Time Report
 * Represent the sender timestamp of the NTP timestamp of RTCP Sender Report
 *
 * Block Length is fix to 2*long
 */
typedef struct {
    unsigned long msw;          /* NTP timestamp; most significant word */
    unsigned long lsw;          /* NTP timestamp; least significant word */
} RTCPXR_BT4;

/*
 * RTCP-XR Report Block Type 5
 * Ref. [RFC3611] chapter 4.5: DLRR Report Block
 * Represent the Delay since the Last Receiver Reference (DLRR) Time Report Block (BT4)
 *
 * This is a reply of sending BT4;
 * Could exist of multiple sub-blocks from different sources
 */
typedef struct {
    unsigned long ssrc;         /* SSRC of receiver */
    unsigned long lrr;          /* The middle of 32 bits out of 64 in the NTP timestamp */
    unsigned long dlrr;         /* Delay since Last RR (DLRR); express in unit 1/65536 sec  */
}DLRR_SUB_BLOCK;

typedef struct {
    DLRR_SUB_BLOCK subBlocks[1];       /* DLRR report block of each receiver, variable length */
} RTCPXR_BT5;

/*
 * RTCP-XR Report Block Type 6
 * Ref. [RFC3611] chapter 4.6: Statistics Summary Report
 * Represent the summary of the previous blocks, although nit very detailed.
 */
typedef struct {
    unsigned long   ssrc;               /* SSRC of the sender */
    unsigned short  beginSeq;           /* Begin Sequence */
    unsigned short  endSeq;             /* End of Sequence */
    unsigned long   lostPackets;        /* Number of lost packets */
    unsigned long   duplicatedPackets;  /* Number of duplicated packets */
    unsigned long   minJitter;          /* Minimum relative transmit time between 2 frames */
    unsigned long   maxJitter;          /* Maximum relative transmit time between 2 frames */
    unsigned long   meanJitter;         /* Mean relative transmit time between 2 frames */
    unsigned long   devJitter;          /* The standard deviation of the relative
                                         * transmit time between each 2 frames series */
    uint8_t         minToH;             /* Minimum TTL or Hop Limit of data packets */
    uint8_t         maxToH;             /* Maximum TTL or Hop Limit of data packets */
    uint8_t         meanToH;            /* Mean TTL or Hop Limit of data packets */
    uint8_t         devToH;             /* The standard deviation of TTL or Hop Limit
                                           of data packets */
} RTCPXR_BT6;

/*
 * RTCP-XR Report Block Type 7
 * Ref. [RFC3611] chapter 4.7: VoIP Metrics Report Block
 * Represent the metrics for monitoring voice over IP calls.
 */
typedef struct {
    unsigned long   ssrc;             /* SSRC of the sender */

    /* Packet Loss and Discard Metrics [RFC3611] Chapter 4.7.1 */
    uint8_t         lossRate;         /* The fraction of RTP data frames lost
                                       * lossRate = (packetLost/expected packets) * 256
                                       */
    uint8_t         discardedRate;    /* The fraction of RTP data frames discarded
                                       * discRate = (packetDiscarded/expected packets) * 256
                                       */

    /* Burst Metrics [RFC3611] Chapter 4.7.2 */
    uint8_t         burstDensity;     /* The fraction of RTP data packets within periods since
                                       * the beginning of reception.
                                       * burstDensity = ((packetsLost + packetsDiscarded)/expected packets within burstPeriod) * 256
                                       */
    uint8_t         gapDensity;       /* The fraction of RTP data packets within inter-burst gaps since
                                       * the beginning of reception.
                                       * gapDensity = ((packetsLost + packetsDiscarded)/expected packets within GapPeriod) * 256
                                       */
    unsigned short  burstDuration;    /* The mean duration, in milliseconds, of the busrt periods */
    unsigned short  gapDuration;      /* The mean duartion, in milliseconds, of the gap periods */

    /* Delay Metrics [RFC3611] Chapter 4.7.3 */
    unsigned short  roundTripDelay;   /* Round Trip Delay, in milliseconds */
    unsigned short  endSystemDelay;   /* Delay introduced by sample, jitter buffer, encode/decode, in milliseconds. It's used to calculate one way delay */

    /* Signal Related Metrics [RFC3611] Chapter 4.7.4 */
    int8_t          signalLevel;      /* The voice relative level, in dBm. */
    int8_t          noiseLevel;       /* The noise relative level, in dBm. */
    uint8_t         rerl;             /* Residual Echo Return Loss. */
    uint8_t         gapMin;           /* The Gap Threshold, Value to determine if a gap exists. */

    /* Call quality or Transmission Quality Metrics [RFC3611] Chapter 4.7.5 */
    uint8_t         RFactor;          /* A voice quality metrics describing the segment of the call that is
                                       * carried over this RTP session
                                       * rFactor < 50 is unusable; 50 < rFactor < 100 a good quality
                                       * A value of 127 indicated that the value is unavailable.
                                       * ITU-T G.107 and ETSI TS101 329-5
                                       */
    uint8_t         externalRFactor;  /* A voice quality metric describing the call that is carried over a
                                       * network segment external to the RTP segment,
                                       * for example cellular network.
                                       * Interpreted the same as rFactor
                                       */
    uint8_t         mosLQ;            /* Estimated Mean Option Score for Listening Quality,
                                       * a metric on a scale from 1 to 5, in which 5 repesent
                                       * excelent and 1 represent unacceptable.
                                       * The value is an integer between 10 and 50,
                                       * value is corresponding to MOS * 10.
                                       */
    uint8_t         mosCQ;            /* Estimated Mean Option Score for Conversational Quality,
                                       * a metric on a scale from 1 to 5, in which 5 repesent
                                       * excelent and 1 represent unacceptable.
                                       * The value is an integer between 10 and 50,
                                       * value is corresponding to MOS * 10.
                                       */
    uint8_t         rxconfig;
    uint8_t         reserved;

    /* Jitter Buffers Parameters [RFC3611] Chapter 4.7.7 */
    unsigned short  jitterBufferNominal;          /* Current Jitter Buffer Nominal Delay, in msecs */
    unsigned short  jitterBufferMax;              /* Current Maximum Jitter Buffer Delay, in msecs */
    unsigned short  jitterBufferWorstCase;        /* Current Worst Case Jitter Buffer Delay, in msecs */
} RTCPXR_BT7;

/* eXtended Report (XR) */
typedef struct {
    RTCPXRCOMMON  common;       /* RTCP-XR common header */
    union {
        RTCPXR_BT1  bt1;        /* Block Type 1 */
        RTCPXR_BT1  bt2;        /* Block Type 2 Use the same format as BT1*/
        RTCPXR_BT3  bt3;        /* Block Type 3 */
        RTCPXR_BT4  bt4;        /* Block Type 4 */
        RTCPXR_BT5  bt5;        /* Block Type 5 */
        RTCPXR_BT6  bt6;        /* Block Type 6 */
        RTCPXR_BT7  bt7;        /* Block Type 7 */
    }             blockTypes;
}RTCPXR;

/*
** One RTCP packet
** Taken from Brcm implementation
*/
typedef struct {
    RTCPCOMMON  common;            /* RTCP common header */
    union {
        /* sender report (SR) */
        struct RTCPSR {
            unsigned long ssrc;         /* sender generating this report */
            unsigned long ntp_sec;      /* NTP timestamp */
            unsigned long ntp_frac;
            unsigned long rtp_ts;       /* RTP timestamp */
            unsigned long psent;        /* packets sent */
            unsigned long osent;        /* octets sent */
            RTCPRR        rr[1];        /* variable-length list */
        } sr;

        /* reception report (RR) */
        struct {
            unsigned long ssrc;         /* receiver generating this report */
            RTCPRR        rr[1];        /* variable-length list */
        } rr;

        /* source description (SDES) */
        struct RTCPSDES {
            unsigned long src;          /* first SSRC/CSRC */
            RTCPSDESITEM  item[1];      /* list of SDES items */
        } sdes;

        /* eXtended Reporting (XR) */
        struct {
            unsigned long ssrc;       /* sender/receiver generating this report */
            RTCPXR        xr[1];      /* Variable length list */
        } xr;

        /* BYE */
        struct {
            unsigned long src[1];       /* list of sources */
            /* can't express trailing text for reason */
        } bye;
    }           r;
} RTCPPACKET;

typedef struct RTCPSDES   RTCPSDES;
typedef struct RTCPSR     RTCPSR;

/*
** Per-source state information
** Taken from Brcm implementation
*/
typedef struct {
    unsigned short  max_seq;        /* highest seq. number seen */
    unsigned long   cycles;         /* shifted count of seq. number cycles */
    unsigned long   base_seq;       /* base seq number */
    unsigned long   bad_seq;        /* last 'bad' seq number + 1 */
    unsigned long   probation;      /* sequ. packets till source is valid */
    unsigned long   received;       /* packets received */
    unsigned long   expected_prior; /* packet expected at last interval */
    unsigned long   received_prior; /* packet received at last interval */
    unsigned long   transit;        /* relative trans time for prev pkt */
    unsigned long   jitter;         /* estimated jitter */

    unsigned long   roundtrip;    /* round trip time */
    unsigned long   lsr;          /* last sender report */
    unsigned long   dlsr;         /* delay since last sender report */
    unsigned long   rsr;          /* time that last sender report is received */
} RTPSRC;

/*
** Per-session RTCP statistics
** Taken from Brcm implementation
*/
typedef struct RTCPSTAT {
    uint8_t         bPaddingByte;   /* padding octet */
    unsigned short  wExtensionByte; /* extension octet */
    unsigned short  wCSRCByte;      /* CSRC octet */

    /* local and remote packet loss rate */
    unsigned long   dwPrevLocalNumLost;
    unsigned long   dwPrevLocalLastSeqNum;
    unsigned long   dwPrevRemoteNumLost;
    unsigned long   dwPrevRemoteLastSeqNum;

    unsigned short  wRemotePktLossRate;
    unsigned short  wLocalPktLossRate;

    unsigned int    bCalRemotePktLossRate;        /* flag to indicate that a recalculation of the remote pkt loss rate is needed */
    unsigned int    bCalLocalPktLossRate;         /* flag to indicate that a recalculation of the local pkt loss rate is needed */

    unsigned int    bFirstRtcpReceived;
    unsigned int    bFirstRtcpSent;

    /* cumulative connection statistics from previous RTP sessions */
    unsigned long   dwPrevCumLocalOctetSent;            /* cumulative octet count of data sent */
    unsigned long   dwPrevCumLocalPacketSent;           /* cumulative packet count of data sent */
    unsigned long   dwPrevCumLocalOctetRecvd;           /* cumulative octet count of data received */
    unsigned long   dwPrevCumLocalPacketRecvd;          /* cumulative packet count of data received */
    int             dwPrevCumLocalPacketLost;           /* cumulative number of packets lost */
    unsigned long   dwPrevCumRemoteOctetSent;           /* cumulative octet count of data sent */
    unsigned long   dwPrevCumRemotePacketSent;          /* cumulative packet count of data sent */
    int             dwPrevCumRemotePacketLost;          /* cumulative number of packets lost */
    unsigned long   dwPrevCumLocalPacketDisc;           /* cumulative number of packets discarded */

    unsigned long   dwPrevCumLocalRtcpPktRcvd;          /* cumulative packet count of RTCP received */
    unsigned long   dwPrevCumumLocalRtcpPktSent;        /* cumulative packet count of RTCP sent */
    unsigned long   dwPrevCumLocalRtcpXrPktSent;        /* cumulative packet count of RTCP-XR sent */
    unsigned long   dwPrevCumLocalRtcpXrPktRcvd;        /* cumulatibe packet count of RTCP-XR received */
    unsigned long   dwPrevCumLocalOverruns;             /* cumulative Jitter Buffer Overruns */
    unsigned long   dwPrevCumLocalUnderruns;            /* cumulative Jitter Buffer Underruns */
    unsigned long   dwPrevCumLocalMosLQ;                /* cumulative Mos Line Quality */
    unsigned long   dwPrevCumLocalMosCQ;                /* cumulative Mos COnversational Quality */

    /* average statistics */
    unsigned long   localAverageJitter;                 /* average local interarrival jitter */
    unsigned long   remoteAverageJitter;                /* average remote interarrival jitter */
    unsigned long   localAverageLatency;                /* average local latency */

    /* delta connection statistics from the local endpoint */
    unsigned long   dwLocalOctetSent;                   /* delta octet count of data sent */
    unsigned long   dwLocalPacketSent;                  /* delta packet count of data sent */
    unsigned long   dwLocalOctetRecvd;                  /* delta octet count of data received */
    unsigned long   dwLocalPacketRecvd;                 /* delta packet count of data received */
    int             dwLocalPacketLost;                  /* delta number of packets lost */
    unsigned long   dwLocalJitter;                      /* inter-packet arrival jitter */
    unsigned long   dwLocalLatency;                     /* latency */
    unsigned long   dwLocalOverruns;                    /* jitter buffer overrun count */
    unsigned long   dwLocalUnderruns;                   /* jitter buffer underrun count */

    /* delta connection statistics from the remote endpoint */
    unsigned long   dwRemoteOctetSent;                  /* delta octet count of data sent */
    unsigned long   dwRemotePacketSent;                 /* delta packet count of data sent */
    int             dwRemotePacketLost;                 /* delta number of packets lost */
    unsigned long   dwRemoteJitter;                     /* inter-packet arrival jitter */

    unsigned int    rxRecv;                             /* Indicate if RTP packets are received */
    unsigned short  wRecvSeqNum;                        /* Current rx sequence number */
    unsigned long   ssrc;                               /* Remote side SSRC (network byte ordering) */

    unsigned int    rtcpRRRecv;                         /* Indicates if RTCP RR packets are received */
    unsigned int    rtcpSRRecv;                         /* Indicates if RTCP SR packets are received */
    unsigned int    rtcpSRSent;                         /* Indicates if RTCP packets are sent */
    unsigned long   lastntpsec;                         /* Last ntp sec received */
    unsigned long   lastntpfrac;                        /* Last ntp fraction received */

    unsigned int    pktsent;                            /* Indicate if an RTP packet is sent between
                                                           the RTCP interval */
    unsigned long   lastrtpts;                          /* Last RTP time stamp sent */
    int             lastrtpticks;                       /* System ticks for the last RTP time stamp */
    char            cname[16];                          /* Local IPv4 address (for SDES/CNAME) */

    /* (xxx.xxx.xxx.xxx + \0) */
    int             cnamelen;                           /* CNAME length */
    RTPSRC          source;
    /*AT&T extension*/
    unsigned long   localSumFractionLoss;     /*summary of tx fraction loss */
    unsigned long   localSumSqrFractionLoss;  /*summary of square of tx fraction loss */
    unsigned long   remoteSumFractionLoss;    /*summary of rx fraction loss */
    unsigned long   remoteSumSqrFractionLoss; /*summary of square of rx fraction loss */
    unsigned long   localSumJitter;           /*summary of tx jitter */
    unsigned long   localSumSqrJitter;        /*summary of square of tx jitter */
    unsigned long   remoteSumJitter;          /*summary of rx jitter */
    unsigned long   remoteSumSqrJitter;       /*summary of square of rx jitter */
} RTCPSTAT;

/*
 * Refer to RFC3611 for explanation
 */
typedef struct {
    uint8_t *buf;           /* Storing the RTCP frames */
    int     len;            /* Length of the RTCP Buffer */
} RTCPBUFF;

typedef struct {
    unsigned int    rtcpXrUpdated;              /* bit 0 RTCP-XR is enabled
                                                 * bit 1-7 Updated Block type
                                                 */
    /* Loss Report Length Encoding (LRLE) (BT1) */
    unsigned short  lrleStartSeq;               /* LRLE Start Sequence */
    unsigned short  lrleEndSeq;                 /* LRLE End Sequence */
    uint8_t         lrleThinningValue;          /* Thinning factor, the one every 2^T packets are reported */
    uint8_t         lrleRunType;                /* Zero indicate a run for 0s. One indicate a run for 1s */
    unsigned short  lrleRunLength;              /* The length of the run bit vector, range 1 to 16,383 */
    int             lrleInfoLength;             /* The length of the bit vector chunks info buffer storage*/
    unsigned short  *lrleInfo;                  /* Storing the bit vectors chunks excluding the chunkType info (bit 0) */
    /* Duplicated Report Length Encoding (DRLE) (BT2) */
    unsigned short  drleStartSeq;               /* DRLE Start Sequence */
    unsigned short  drleEndSeq;                 /* DRLE End Sequence */
    uint8_t         drleThinningValue;          /* Thinning factor, the one every 2^T packets are reported */
    uint8_t         drleRunType;                /* Zero indicate a run for 0s. One indicate a run for 1s */
    unsigned short  drleRunLength;              /* The length of the run bit vector, range 1 to 16,383 */
    int             drleInfoLength;             /* The length of the bit vector chunks info buffer storage */
    unsigned short  *drleInfo;                  /* Storing the bit vectors chunks excluding the chunkType info (bit 0) */
    /* Packet Receipt Times Reports (BT3) */
    unsigned short  packetStartSeq;             /* Packet Start Sequence reported */
    unsigned short  packetEndSeq;               /* Packet End Sequence reported */
    unsigned int    packetThinningValue;        /* Thinning factor, the one every 2^T packets are reported */
    int             packetReceiptInfoLength;    /* The length of the packet receipts info buffer storage */
    unsigned long   *packetsReceiptInfo;        /* Storing all the reported packets receipt info */
    /* Receiver Reference Time Report (BT4) */
    unsigned long   receiptTimeReferenceMsw;    /* NTP timestamp (Most Significant Word) from RTCP sender report */
    unsigned long   receiptTimeReferenceLsw;    /* NTP timestamp (Leasr Significant Word) from RTCP sender report */
    /* Delay since Last Receiver Report (DLRR) Report (BT5) */
    int             dlrrNummerOfBlocks;         /* Number of DLRR blocks reported, one sub-block per Receiver Report */
    DLRR_SUB_BLOCK  *dlrrSubBlock;              /* Storing the Sub Blocks information */
    /* Statistics Summary (BT6)*/
    unsigned long   sumLostPackets;             /* Total number of Lost packets */
    unsigned long   duplicatedPackets;          /* Total number of Duplicate packets */
    /* Jitter measurement */
    unsigned long   minJitter;                  /* Minimum Jitter in msec */
    unsigned long   meanJitter;                 /* Current Jitter in msec */
    unsigned long   maxJitter;                  /* Maximum Jitter in msec */
    unsigned long   devJitter;                  /* Standard Deviation Jitter in msec */
    /* TTL or Hop Limit */
    unsigned int    ipValid;                    /* 0 = undefined; 1 = IPV4; 2 = IPV6 */
    uint8_t         minToH;                     /* Minimum TTL or Hop Limit of data packets */
    uint8_t         maxToH;                     /* Maximum TTL or Hop Limit of data packets */
    uint8_t         meanToH;                    /* Mean TTL or Hop Limit of data packets */
    uint8_t         devToH;                     /* The standard deviation of TTL or Hop Limit */
    /* VoIP Metrics Report (BT7) */
    /* Packet Loss and Discard Metrics */
    unsigned long   lossRate;                   /* Fraction of Loss frames rate */
    unsigned long   discardedRate;              /* Fraction if Discarded frames rate */
    /* Burst Metrics */
    unsigned long   burstDensity;               /* Fraction of Burst Density */
    unsigned long   gapDensity;                 /* Fraction of GAP Density */
    unsigned long   burstDuration;              /* Mean Burst duration in msec */
    unsigned long   gapDuration;                /* Mean GAP duration in msec */
    /* Delay Metrics */
    unsigned long   roundTripDelay;     /* Round Trip Delay */
    unsigned short  endSystemDelay;     /* Delay introduced by sample, jitter buffer, encode/decode, in milliseconds. It's used to calculate one way delay */

    /* Signal related Metrics */
    uint8_t         signalLevel;                /* Signal Level in dBm */
    uint8_t         noiseLevel;                 /* Noise level in dBm */
    uint8_t         RERL;                       /* Residual Echo Return Loss */
    uint8_t         gapMin;                     /* Gap Minimum */
    /* Call Quality or Transmission Quality Metrics */
    unsigned long   RFactor;                    /* R-Factor in voice quality */
    unsigned long   externalRFactor;            /* External R-Factor quality from external network */
    unsigned long   mosLQ;                      /* Mean Option Score for Listening Quality, value * 10 */
    unsigned long   mosCQ;                      /* Mean Option Score for Conversational Quality, value * 10 */
    /* Rx Config */
    uint8_t         rxConfigPLC;                /* Packet Loss Concealment */
    uint8_t         rxConfigJBA;                /* Jitter Buffer Adaptive */
    uint8_t         rxConfigJBRate;             /* Jitter Buffer Rate */
    /* Jitter Buffers Parameters [RFC3611] Chapter 4.7.7 */
    unsigned short  jitterBufferNominal;        /* Current Jitter Buffer Nominal Delay, in msecs */
    unsigned short  jitterBufferMax;            /* Current Maximum Jitter Buffer Delay, in msecs */
    unsigned short  jitterBufferWorstCase;      /* Current Worst Case Jitter Buffer Delay, in msecs */
    unsigned long   ssrc;
    /*AT&T extension*/
    unsigned long   maxRoundTripDelay;          /*Max Round Trip Delay */
    unsigned long   sumRoundTripDelay;          /*Sum of Max Round Trip Delay */
    unsigned long   sumSqrRoundTripDelay;       /*Sum of Square of Max Round Trip Delay */
    unsigned long   maxOneWayDelay;             /*Max Round Trip Delay */
    unsigned long   sumOneWayDelay;             /*Sum of Max Round Trip Delay */
    unsigned long   sumSqrOneWayDelay;          /*Sum of Square of Max Round Trip Delay */
} RTCPXRSTAT;

/* Per session control block */
typedef struct RTPHANDLE {
    unsigned long ssrc;                         /* SSRC value of the session */
    unsigned long essrc;                        /* SSRC value of the egress stream */
    /* RTCP & RTCP-XR statistics */
    RTCPSTAT      rtcpStat;                     /* RTCP statistics */
    RTCPXRSTAT    localRtcpXrStat;              /* Detail report statistics */
    RTCPXRSTAT    remoteRtcpXrStat;             /* Detail report statistics */
    /* Saving the last RTCP reports  */
    RTCPBUFF      rxRtcpBuf[MMCONN_RTCP_MAX_SAVED_RTCPXR_FRAME];
    int           lastRxRtcpBufIdx;             /* Index of the Last rtcpBuf RTCP buffer storages */
    RTCPBUFF      txRtcpBuf[MMCONN_RTCP_MAX_SAVED_RTCPXR_FRAME];
    int           lastTxRtcpBufIdx;             /* Index of the Last rtcpBuf RTCP buffer storages */
} RTPHANDLE;

/*########################################################################
#                                                                       #
#  FUNCTION PROTOTYPES                                                  #
#                                                                       #
########################################################################*/
#endif /* #ifdef _MMCONNBRCMSTATS_P_H_ */
