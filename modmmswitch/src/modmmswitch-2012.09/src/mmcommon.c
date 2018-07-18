/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/*########################################################################
#                                                                       #
#  HEADER (INCLUDE) SECTION                                             #
#                                                                       #
########################################################################*/
#include <linux/module.h>

#include "mmcommon.h"
/*########################################################################
#                                                                       #
#  TYPES, DEFINES AND STRUCTS                                           #
#                                                                       #
########################################################################*/

/*########################################################################
#                                                                       #
#  PRIVATE DATA MEMBERS                                                 #
#                                                                       #
########################################################################*/

/*########################################################################
#                                                                       #
#  PRIVATE FUNCTION PROTOTYPES                                          #
#                                                                       #
########################################################################*/

/*########################################################################
#                                                                       #
#  PRIVATE FUNCTION IMPLEMENTATION                                      #
#                                                                       #
########################################################################*/

/*########################################################################
#                                                                       #
#  PUBLIC FUNCTION IMPLEMENTATION                                       #
#                                                                       #
########################################################################*/

const char *mmPbxGetErrorString(MmPbxError error)
{
    switch (error) {
        case MMPBX_ERROR_NOERROR:
            return "MMPBX_ERROR_NOERROR";

        case MMPBX_ERROR_NOMEMORY:
            return "MMPBX_ERROR_NOMEMORY";

        case MMPBX_ERROR_NORESOURCES:
            return "MMPBX_ERROR_NORESOURCES";

        case MMPBX_ERROR_INVALIDSTATE:
            return "MMPBX_ERROR_INVALIDSTATE";

        case MMPBX_ERROR_INVALIDTYPE:
            return "MMPBX_ERROR_INVALIDTYPE";

        case MMPBX_ERROR_INVALIDPARAMS:
            return "MMPBX_ERROR_INVALIDPARAMS";

        case MMPBX_ERROR_INVALIDCONFIG:
            return "MMPBX_ERROR_INVALIDCONFIG";

        case MMPBX_ERROR_UNEXPECTEDCONDITION:
            return "MMPBX_ERROR_UNEXPECTEDCONDITION";

        case MMPBX_ERROR_INTERNALERROR:
            return "MMPBX_ERROR_INTERNALERROR";

        case MMPBX_ERROR_EAGAIN:
            return "MMPBX_ERROR_EAGAIN";

        case MMPBX_ERROR_NOTIMPLEMENTED:
            return "MMPBX_ERROR_NOTIMPLEMENTED";

        case MMPBX_ERROR_NOTSUPPORTED:
            return "MMPBX_ERROR_NOTSUPPORTED";

        case MMPBX_ERROR_ABORTED:
            return "MMPBX_ERROR_ABORTED";

        case MMPBX_ERROR_UNKNOWN:
            return "MMPBX_ERROR_UNKNOWN";
    }

    return "Unknown MmPbxError value";
}

const char *mmPbxGetTraceLevelString(MmPbxTraceLevel traceLevel)
{
    switch (traceLevel) {
        case MMPBX_TRACELEVEL_NONE:
            return "MMPBX_TRACELEVEL_NONE";

        case MMPBX_TRACELEVEL_ERROR:
            return "MMPBX_TRACELEVEL_ERROR";

        case MMPBX_TRACELEVEL_CRIT:
            return "MMPBX_TRACELEVEL_CRIT";

        case MMPBX_TRACELEVEL_DEBUG:
            return "MMPBX_TRACELEVEL_DEBUG";

        case MMPBX_TRACELEVEL_INFO:
            return "MMPBX_TRACELEVEL_INFO";
    }

    return "Unknown MmPbxTraceLevel value";
}

/*########################################################################
#                                                                       #
#  EXPORTS                                                              #
#                                                                       #
########################################################################*/
EXPORT_SYMBOL(mmPbxGetErrorString);
EXPORT_SYMBOL(mmPbxGetTraceLevelString);
