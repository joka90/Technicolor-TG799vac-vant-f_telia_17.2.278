/************** COPYRIGHT AND CONFIDENTIALITY INFORMATION ********************
**                                                                          **
** Copyright (c) 2014 Technicolor                                           **
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

#ifndef TRANSFORMER_BACKEND_HELPER_H
#define TRANSFORMER_BACKEND_HELPER_H

#include <stdbool.h>

#include "error.h"
#include "proxy.h"

struct query_item
{
    char *name;
    char *value;
    int level;
};

/***************************
 * CALLBACKS
 **************************/

/**
* Generic error callback used by the API functions
*
* \param[in] errorcode An integer representing the error code
* \param[in] errorstr A string describing the error
* \param[in] cookie A pointer to opaque context information passed along by the view implementation.
*
*  \return No return value
*/
typedef void (*transformer_generic_error_cb)(tr064_e_error errorcode, const char *errorstr, void *cookie);

/**
* Result callback for GetParameterValues RPC
*
* \param[in] path A string representing the datamodel parameter path
* \param[in] param A string representing the datamodel parameter name
* \param[in] value A string representing the datamodel parameter value
* \param[in] cookie A pointer to opaque context information passed along by the view implementation.
*
*  \return No return value
*/
typedef void (*transformer_getparametervalues_result_cb)(const char *path, const char *param, const char *value, void *cookie);

/**
* Specific result callback for GetParameterNames RPC
*
* \param[in] paramPath A string containing the parameter path
* \param[in] paramName A string containing the parameter name
* \param[in] cookie A pointer to opaque context information passed along by the view implementation.
*
*  \return No return value
*/
typedef void (*transformer_getparameternames_result_cb) (const char* paramPath, const char* paramName, void *cookie);

/**
* Specific result callback for SetParameterValues RPC
*
* \param[in] cookie A pointer to opaque context information passed along by the view implementation.
*
*  \return No return value
*/
typedef void (*transformer_setparametervalues_result_cb) (void *cookie);


/***************************
 * RPCs
 **************************/
void getpv(const struct query_item params[], transformer_getparametervalues_result_cb result_cb, transformer_generic_error_cb error_cb, void* cookie);
void getpn(const struct query_item params[], transformer_getparameternames_result_cb  result_cb, transformer_generic_error_cb error_cb, void *cookie);
void setpv(const struct query_item params[], transformer_setparametervalues_result_cb result_cb, transformer_generic_error_cb error_cb, void *cookie);

bool apply(void);
#endif
