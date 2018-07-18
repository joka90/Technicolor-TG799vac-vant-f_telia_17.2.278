/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/**
 * Multimedia PBX common header file description
 *
 * \file mmcommon.h
 * \version v1.0
 */

#ifndef __MM_COMMON_H_
#define __MM_COMMON_H_

/*########################################################################
#                                                                       #
#  HEADER (INCLUDE) SECTION                                             #
#                                                                       #
########################################################################*/


/*########################################################################
#                                                                       #
#  MACROS/DEFINES                                                       #
#                                                                       #
########################################################################*/
#ifndef MAX
#define MAX(a, b)    (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b)    (((a) <= (b)) ? (a) : (b))
#endif

#ifndef TRACE_LEVEL
#define TRACE_LEVEL    2
#endif

/* this is copied from mmtrace.h*/
# define TRACE_LEVEL_VARIABLE    _traceLevel
# define MMPBX_TRACEDEF(x)    static MmPbxTraceLevel TRACE_LEVEL_VARIABLE = (x)
# define MMPBX_TRACESET(x)    TRACE_LEVEL_VARIABLE = (x)
# define MMPBX_TRACEGET()     (TRACE_LEVEL_VARIABLE)

/** Define trace levels **/
/* It's not allowed to use enum's in macro's (at least not in gcc) so we need to map them on defines */
#ifndef _MMPBX_TRACELEVEL_NONE
#define _MMPBX_TRACELEVEL_NONE    0
#endif
#ifndef _MMPBX_TRACELEVEL_ERROR
#define _MMPBX_TRACELEVEL_ERROR    1
#endif
#ifndef _MMPBX_TRACELEVEL_CRIT
#define _MMPBX_TRACELEVEL_CRIT    2
#endif
#ifndef _MMPBX_TRACELEVEL_DEBUG
#define _MMPBX_TRACELEVEL_DEBUG    3
#endif
#ifndef _MMPBX_TRACELEVEL_INFO
#define _MMPBX_TRACELEVEL_INFO    4
#endif

/* Tracing macro's */
#ifndef TRACE_FUNCTION
#define TRACE_FUNCTION    printk
#endif

#ifdef FORCED_TRACE_LEVEL
#define TRACE_LEVEL_VARIABLE    FORCED_TRACE_LEVEL
#else
#define TRACE_LEVEL_VARIABLE    _traceLevel
#endif

#ifdef MODULE_NAME
#define CAPTION    "[" MODULE_NAME "]"
#endif

#if TRACE_LEVEL >= _MMPBX_TRACELEVEL_ERROR
#define MMPBX_TRC_ERROR(str, args ...)                                 \
    if (TRACE_LEVEL_VARIABLE >= _MMPBX_TRACELEVEL_ERROR) {             \
        TRACE_FUNCTION("\e[31m%-16s:E: %s:%d - " str "\e[0m", CAPTION, \
                       __func__, __LINE__, ## args); }
#else
#define MMPBX_TRC_ERROR(args ...)
#endif
#if TRACE_LEVEL >= _MMPBX_TRACELEVEL_CRIT
#define MMPBX_TRC_CRIT(str, args ...)                                  \
    if (TRACE_LEVEL_VARIABLE >= _MMPBX_TRACELEVEL_CRIT) {              \
        TRACE_FUNCTION("\e[35m%-16s:C: %s:%d - " str "\e[0m", CAPTION, \
                       __func__, __LINE__, ## args); }
#else
#define MMPBX_TRC_CRIT(args ...)
#endif
#if TRACE_LEVEL >= _MMPBX_TRACELEVEL_DEBUG
#define MMPBX_TRC_DEBUG(str, args ...)                                 \
    if (TRACE_LEVEL_VARIABLE >= _MMPBX_TRACELEVEL_DEBUG) {             \
        TRACE_FUNCTION("\e[34m%-16s:D: %s:%d - " str "\e[0m", CAPTION, \
                       __func__, __LINE__, ## args); }
#else
#define MMPBX_TRC_DEBUG(args ...)
#endif
#if TRACE_LEVEL >= _MMPBX_TRACELEVEL_INFO
#define MMPBX_TRC_INFO(str, args ...)                     \
    if (TRACE_LEVEL_VARIABLE >= _MMPBX_TRACELEVEL_INFO) { \
        TRACE_FUNCTION("%-16s:I: %s:%d - " str, CAPTION,  \
                       __func__, __LINE__, ## args); }
#else
#define MMPBX_TRC_INFO(args ...)
#endif

/*########################################################################
#                                                                       #
#  TYPES                                                                #
#                                                                       #
########################################################################*/

/**
 * \brief All possible error codes that mmpbx functions can return
 * \since 1.0
 */
typedef enum {
    /** \brief Function is executed sucessfully
     * \since 1.0
     */
    MMPBX_ERROR_NOERROR,

    /** \brief A memory error has occured
     * \since 1.0
     */
    MMPBX_ERROR_NOMEMORY,

    /** \brief Not enough resourses are available
     * \since 1.0
     */
    MMPBX_ERROR_NORESOURCES,

    /** \brief An object is in an invalid state
     * \since 1.0
     */
    MMPBX_ERROR_INVALIDSTATE,

    /** \brief An object is from an invalid type
     * \since 1.0
     */
    MMPBX_ERROR_INVALIDTYPE,

    /** \brief A parameter has an incorrect value
     * \since 1.0
     */
    MMPBX_ERROR_INVALIDPARAMS,

    /** \brief The configuration is invalid
     * \since 1.0
     */
    MMPBX_ERROR_INVALIDCONFIG,

    /** \brief An unexpected condition has occurred
     * \since 1.0
     */
    MMPBX_ERROR_UNEXPECTEDCONDITION,

    /** \brief An internal error has occured
     * \since 1.0
     */
    MMPBX_ERROR_INTERNALERROR,

    /** \brief A duplicate has been found and is not allowed
     * \since 1.0
     */
    MMPBX_ERROR_EAGAIN,

    /** \brief This functionality is not implemented
     * \since 1.0
     */
    MMPBX_ERROR_NOTIMPLEMENTED,

    /** \brief This functionality is currently not yet supported
     * \since v1.0
     */
    MMPBX_ERROR_NOTSUPPORTED,

    /** \brief The function has aborted
     * \since 1.0
     */
    MMPBX_ERROR_ABORTED,

    /** \brief An unknown error has occured
     * \since 1.0
     */
    MMPBX_ERROR_UNKNOWN
} MmPbxError;

/**
 * \brief The tracelevels of the mmpbx libraries
 * \since 1.0
 */
typedef enum {
    /** \brief No traces should be used
     * \since 1.0
     */
    MMPBX_TRACELEVEL_NONE   = _MMPBX_TRACELEVEL_NONE,

    /** \brief Only error traces are displayed
     * \since 1.0
     */
    MMPBX_TRACELEVEL_ERROR  = _MMPBX_TRACELEVEL_ERROR,

    /** \brief Error traces and critical traces are displayed
     * \since 1.0
     */
    MMPBX_TRACELEVEL_CRIT   = _MMPBX_TRACELEVEL_CRIT,

    /** \brief Error, critical and debugging traces are displayed
     * \since 1.0
     */
    MMPBX_TRACELEVEL_DEBUG  = _MMPBX_TRACELEVEL_DEBUG,

    /** \brief Error, critical, debugging and informational traces are displayed
     * \since 1.0
     */
    MMPBX_TRACELEVEL_INFO   = _MMPBX_TRACELEVEL_INFO
} MmPbxTraceLevel;


/*########################################################################
#                                                                       #
#  FUNCTION PROTOTYPES                                                  #
#                                                                       #
########################################################################*/

/**
 * \name Enum string representations
 *
 * All these functions return string representations of enumeration values
 *
 * \{
 */

/**
 * \brief Return a string representation of ::MmPbxError
 *
 * This function returns a string representation of a ::MmPbxError enumeration
 *
 * \since v1.0
 *
 * \pre None
 *
 * \post None
 *
 * \param[in] error The error where a string representation is needed of
 *
 * \return const char *
 * \retval Return value is always a valid string, NULL will never be returned
 */
const char *mmPbxGetErrorString(MmPbxError error);

/**
 * \brief Return a string representation of ::MmPbxTraceLevel
 *
 * This function returns a string representation of a ::MmPbxTraceLevel enumeration
 *
 * \since v1.0
 *
 * \pre None
 *
 * \post None
 *
 * \param[in] traceLevel The trace level where a string representation is needed of
 *
 * \return const char *
 * \retval Return value is always a valid string, NULL will never be returned
 */
const char *mmPbxGetTraceLevelString(MmPbxTraceLevel traceLevel);

/**
 * \}
 */
#endif /* __MMPBX_COMMON_H_ */
