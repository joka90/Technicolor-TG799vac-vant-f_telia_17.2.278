/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/** \file
 * Multimedia kernel space tone connection API.
 *
 * A tone connection is used to generate a tone.
 * A tone can be generated using a tone pattern table.
 *
 * \version v1.0
 *
 *************************************************************************/

#ifndef  MMCONNTONE_INC
#define  MMCONNTONE_INC

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

/**
 * Maximum number of patterns in tone pattern table.
 */
#define MMCONNTONE_MAX_TONE_PATTERNS    12  /**< Maximum number of tone pattern within one MmConnTonePatternTable */

#define ENCODING_NAME_LENGTH            16 /**< Maximum media encoding name length */

/*########################################################################
#                                                                       #
#  TYPES                                                                #
#                                                                       #
########################################################################*/

/**
 * Tone types.
 */
typedef enum {
    MMCONNTONE_BUSY = 0,              /**< Busy tone */
    MMCONNTONE_CALLWAITING,           /**< Call waiting tone */
    MMCONNTONE_CALLHOLD,              /**< Call hold tone */
    MMCONNTONE_BARGEIN,               /**< Barge-in tone */
    MMCONNTONE_CONFIRMATION,          /**< Confirmation tone */
    MMCONNTONE_CONGESTION,            /**< Congestion tone */
    MMCONNTONE_DIAL,                  /**< Dial tone */
    MMCONNTONE_MWI,                   /**< Message waiting indication tone */
    MMCONNTONE_MWI_SPECIAL,           /**< Message waiting indication tone with special tone*/
    MMCONNTONE_NONE,                  /**< None */
    MMCONNTONE_REJECTION,             /**< Rejection tone */
    MMCONNTONE_RELEASE,               /**< Release tone */
    MMCONNTONE_REMOTECALLHOLD,        /**< Remote call hold tone */
    MMCONNTONE_REMOTECALLWAITING,     /**< Remote call waiting tone */
    MMCONNTONE_RINGBACK,              /**< Ringback tone */
    MMCONNTONE_SPECIALDIAL,           /**< Special dial tone */
    MMCONNTONE_STUTTERDIAL,           /**< Stutter dial tone */
    MMCONNTONE_WARNING,               /**< Warning tone */
    MMCONNTONE_EXTRA1,                /**< Extra1 tone */
    MMCONNTONE_EXTRA2,                /**< Extra2 tone */
} MmConnToneType;

/**
 * Discription of a part of a tone.
 */
typedef struct {
    int             id;                             /**< Pattern Id. */
    int             on;                             /**< Enable/Disable pattern. */
    unsigned short  freq1;                          /**< First frequency in Hz. */
    unsigned short  freq2;                          /**< Second frequency in Hz. */
    unsigned short  freq3;                          /**< Third frequency in Hz. */
    unsigned short  freq4;                          /**< Fourth Frequency in Hz. */
    int8_t          power1;                         /**< Power of first frequency in dBm0.  */
    int8_t          power2;                         /**< Power of second frequency in dBm0.  */
    int8_t          power3;                         /**< Power of third frequency in dBm0.  */
    int8_t          power4;                         /**< Power of fourth frequency in dBm0.  */
    int             duration;                       /**< Duration of this pattern. */
    int             nextId;                         /**< Id of next pattern, except if maxLoops != 0 and after looping has finished. See nextIdAfterLoops. */
    int             maxLoops;                       /**< Number of times to repeat pattern, if maxLoops != 0, we enter/restart a loop. */
    int             nextIdAfterLoops;               /**< Id of pattern to use after looping maxLoops times this pattern. */
} MmConnTonePattern;

/**
 * Tone pattern table.
 * This structure is intended to program a driver with one tone (the one
 * that is going to be played).
 */
typedef struct {
    unsigned int      size;                                             /**< nr of patterns in this table. */
    MmConnTonePattern pattern[MMCONNTONE_MAX_TONE_PATTERNS];            /**< pattern description. */
} MmConnTonePatternTable;

/**
 * Tone connection configuration parameters.
 */
typedef struct {
    MmConnToneType          type;                               /**< Tone type */
    char                    encodingName[ENCODING_NAME_LENGTH]; /**< Encoding name as described in RFC3551 and http://www.iana.org/assignments/rtp-parameters */
    unsigned int            packetPeriod;                       /**< Packet period */
    MmConnTonePatternTable  toneTable;                          /**< Tone pattern table. */
    unsigned int            endpointId;                         /**< Unique identifier of kernel space endpoint (tone generator) */
} MmConnToneConfig;

/**
 * Kernel connection event types.
 */

typedef enum {
    MMCONNTONE_EVENT_CONSTRUCT = 0,     /**< A new tone connection is constructed and has a valid pattern table. */
    MMCONNTONE_EVENT_DESTRUCT,          /**< A tone connection will be destructed. */
    MMCONNTONE_EVENT_XCONN,             /**< A tone connection is cross-connected with another connection. */
    MMCONNTONE_EVENT_DISC,              /**< A tone connection is disconnected. */
} MmConnToneEventType;

/**
 * Callback function to listen for mmConnTone events.
 *
 * A kernel space endpoint can use this callback function to take appropriate actions when
 * a tone connection sends and event with endpoint id witch matches the endpoint id of the kernel space endpoint.
 *
 * \since v1.0
 *
 * \pre \c NONE
 *
 * \post The endpoint has handled the event.
 *
 * \param[in] type Event type.
 * \param [in] id Endpoint id.
 * \param [in] mmConnToneConfig mmConnTone configuration (tone description).
 * \param [in] mmConnToneHndl mmConnTone handle.
 *
 * \return ::MmPbxError
 * \retval MMPBX_ERROR_NOERROR The event has been handled successfully.
 * \todo Add other possible return values after implementation.
 */
typedef MmPbxError (*MmConnToneEventCb)(MmConnToneEventType type,
                                        unsigned int        id,
                                        MmConnToneConfig    *mmConnToneConfig,
                                        MmConnToneHndl      mmConnTone);


/*########################################################################
#                                                                       #
#  FUNCTION PROTOTYPES                                                  #
#                                                                       #
########################################################################*/

/**
 * Register callback for multimedia switch tone connection events.
 *
 * This callback function can be registered to listen for events on tone connections.
 *
 * \since v1.0
 *
 * \pre \c cb must be a valid callback function of type ::MmConnToneEventCb.
 *
 * \post The endpoint has handled the event.
 *
 * \param [in] cb Tone connection event callback function.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The callback function has been successfully registered.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnToneRegisterEventCb(MmConnToneEventCb cb);

/**
 * Unregister callback for multimedia switch tone connection events.
 *
 * Unregister callback function to listen for events on tone connections.
 *
 * \since v1.0
 *
 * \pre \c cb must be a valid callback function of type ::MmConnToneEventCb.
 *
 * \post The callback function will no longer be called.
 *
 * \param [in] cb Tone connection event callback function.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The callback function has been successfully unregistered.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnToneUnregisterEventCb(MmConnToneEventCb cb);
#endif   /* ----- #ifndef MMCONNTONE_INC  ----- */
