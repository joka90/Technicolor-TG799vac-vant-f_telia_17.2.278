/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
**                                                                      **
** Copyright (c) 2010-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
**                                                                      **
*************************************************************************/

/** \file
 * Private multimedia switch tone connection API.
 *
 * \version v1.0
 *
 *************************************************************************/
#ifndef  MMCONNTONE_P_INC
#define  MMCONNTONE_P_INC

/*########################################################################
#                                                                       #
#  HEADER (INCLUDE) SECTION                                             #
#                                                                       #
########################################################################*/
#include "mmconntone.h"
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

/*
 * MmConnTone structure definition
 */
struct MmConnTone {
    struct MmConn     mmConn;                     /**< Parent class */
    MmConnToneConfig  config;                     /**< Tone connection specific configuration */
    unsigned int      receivedPatterns;
};


/*########################################################################
#                                                                       #
#  FUNCTION PROTOTYPES                                                  #
#                                                                       #
########################################################################*/

/**
 * Set trace level of all multimedia switch tone connections.
 *
 * This function makes it possible to modify the trace level of all tone connections. This trace level is also dependant on the trace level which was used to compile the code.
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
MmPbxError mmConnToneSetTraceLevel(MmPbxTraceLevel level);

/**
 * Initialise mmConnTone component of mmswitch kernel module.
 *
 * This function initialises the mmConnTone component of the mmswitch kernel module.
 *
 * \since v1.0
 *
 * \pre None.
 *
 * \post The mmConnTone component of mmswitch is initialised.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR the mmConnTone component has been initialised successfully.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnToneInit(void);

/**
 * Deinitialise mmConnTone component of mmswitch kernel module.
 *
 * This function deinitialises the mmConnTone component of the mmswitch kernel module.
 *
 * \since v1.0
 *
 * \pre None.
 *
 * \post The mmConnTone component of mmswitch is deinitialised.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR the mmConnTone component has been deinitialised successfully.
 * \todo Add other possible return values after implementation
 */
MmPbxError mmConnToneDeinit(void);

/**
 * Constructor of a multimedia switch tone connection instance.
 *
 * This function is the constructor of a multimedia switch tone connection instance.
 * A multimedia switch tone connection is used to generate a tone.
 *
 * \since v1.0
 *
 * \pre \c NONE.
 *
 * \post \c tone Contains the handle of a valid multimedia switch tone connection instance.
 *
 * \param [in] config Configuration of tone connection instance of multimedia switch.
 * \param [out] tone Handle of tone connection instance of multimedia switch.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR A tone connection handle has been successfully retrieved and is not NULL.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnToneConstruct(MmConnToneConfig *config,
                               MmConnToneHndl   *tone);

/**
 * Save pattern into pattern table of mmConnTone.
 *
 * This function is used to save a pattern in the pattern table of mmConnTone.
 *
 * \since v1.0
 *
 * \pre \c mmConnTone must be a valid handle of type \c MmConnToneHndl.
 *
 * \post The pattern is saved in the table.
 *
 * \param [in] mmConnTone Handle of tone connection instance of multimedia switch.
 * \param [in] mmConnTonepattern Tone pattern.
 *
 * \return ::MmPbxError.
 * \retval MMPBX_ERROR_NOERROR The pattern has been saved successfully.
 * \todo Add other possible return values after implementation.
 */
MmPbxError mmConnToneSavePattern(MmConnToneHndl     tone,
                                 MmConnTonePattern  *pattern);
#endif   /* ----- #ifndef MMCONNTONE_P_INC  ----- */
