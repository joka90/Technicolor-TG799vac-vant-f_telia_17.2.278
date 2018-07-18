/************** COPYRIGHT AND CONFIDENTIALITY INFORMATION *********************
**                                                                          **
** Copyright (c) 2010 Technicolor                                           **
** All Rights Reserved                                                      **
**                                                                          **
** This program contains proprietary information which is a trade           **
** secret of TECHNICOLOR and/or its affiliates and also is protected as     **
** an unpublished work under applicable Copyright laws. Recipient is        **
** to retain this program in confidence and is not permitted to use or      **
** make copies thereof other than as permitted in a written agreement       **
** with TECHNICOLOR, UNLESS OTHERWISE EXPRESSLY ALLOWED BY APPLICABLE LAWS. **
**                                                                          **
******************************************************************************/

/** \file
* Error codes used throughout backend and plugins.
*
************************************************************************/

#ifndef TRANSFORMER_ERROR_H
#define TRANSFORMER_ERROR_H

#include <syslog.h>

/**
* Enumeration of error codes.
*
* \remark must be kept in sync with #cwmp_errors !!!
*/
typedef enum {
    TR064_ERR_NO_ERROR = 0,
    TR064_ERR_INVALID_ACTION = 401,
    TR064_ERR_INVALID_ARGS   = 402,
    TR064_ERR_ACTION_FAILED  = 501,
    TR064_ERR_INVALID_VALUE  = 600,
    TR064_ERR_OUT_OF_MEMORY  = 603,
    TR064_ERR_WRITE_ACCESS_DISABLED = 898,
} tr064_e_error;

#endif //TRANSFORMER_ERROR_Hs
