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

#include <syslog.h>
#include "../upnpglobalvars.h"
#include "helper.h"

/**
* info passed to the gpv callback function.
* This is setup by the cwmp_view_getparametervalues_exec function
*/
struct rpc_info {
    transformer_proxy_ctx *ctx;
    transformer_getparametervalues_result_cb result_gpv_cb;
    transformer_getparameternames_result_cb  result_gpn_cb;
    transformer_setparametervalues_result_cb result_spv_cb;
    transformer_generic_error_cb error_cb;
    void *cookie;
};

/**
* encode the namelist in the view_ctx to a correctly typed request
*
* \param view_ctx the view context
* \param type the message type
* \returns true if all parameters in the list were correctly encoded.
*          false otherwise.
* Note that the type in the list is ignored.
*/
static bool encode_msg_paramlist(message_ctx* msg, msgType type, const struct query_item params[])
{
    message_init_encode(msg, type, MAX_MESSAGE_SIZE, &transformer_uuid);

    int idx = 0;
    struct query_item item = params[idx];
    while(item.name) {
        bool encoded = false;
        switch( type ) {
            case GPV_REQ:
                encoded = encode_GPV_REQ(msg, item.name);
                break;
            case GPN_REQ:
                encoded = encode_GPN_REQ(msg, item.name, item.level);
                break;
            case SPV_REQ:
                encoded = encode_SPV_REQ(msg, item.name, item.value);
                break;
            default:
                break;
        }
        if (!encoded) {
            syslog(LOG_ERR, "%s Failed to encode complete request, using subset", PROXY_TRACE);
            return false;
        }
        idx++;
        item = params[idx];
    }
    return true;
}

/**
* Convert a transformer error code to a TR-064 error code.
* @param error             the transformer error code
* @param tr064_error       the TR-064 error code
*/
static void transformererror_to_tr064error(unsigned int transformererror, tr064_e_error *tr064_error)
{
    switch (transformererror) {
        // 9000 Method not supported
        case 9000:
            *tr064_error = TR064_ERR_INVALID_ACTION;
            break;

        case 9003: // 9003 Invalid Arguments
        case 9005: // 9005 Invalid Parameter Name
        case 9006: // 9006 Invalid Parameter Type
            *tr064_error = TR064_ERR_INVALID_ARGS;
            break;

        // 9004 Resource exceeded
        case 9004:
            *tr064_error = TR064_ERR_OUT_OF_MEMORY;
            break;

        // 9007 Invalid Parameter Value
        case 9007:
            *tr064_error = TR064_ERR_INVALID_VALUE;
            break;

        // 9008 Set non-writable parameter
        case 9008:
            *tr064_error = TR064_ERR_WRITE_ACCESS_DISABLED;
            break;

        case 9001: // 9001 Request Denied
        case 9002: // 9002 Internal Error
        default:
            *tr064_error = TR064_ERR_ACTION_FAILED;
            break;
    }
}

static bool parse_transformer_error(transformer_proxy_ctx *ctx, char **error_str, uint16_t *error)
{
    message_ctx* msg = &ctx->msg;
    if (msg->type != GENERIC_ERROR) {
        syslog(LOG_ERR, "%s invalid response type: expected %d, got %d", PROXY_TRACE, GENERIC_ERROR, msg->type);
        return false;
    }

    error_response resp = { 0 };
    // Try to extract an error response.
    if(!decode_ERROR(msg, &resp)){
        syslog(LOG_ERR, "%s %s: could not decode response message", PROXY_TRACE, "parse_error");
        return false;
    }
    *error = resp.errcode;
    *error_str = (char *)resp.errmsg;

    return true;
}

static bool transformer_generic_error_callback(void *cookie)
{
    struct rpc_info *info = (struct rpc_info*)cookie;

    uint16_t transformer_error;
    char *transformer_error_str;

    if(!parse_transformer_error(info->ctx, &transformer_error_str, &transformer_error)) {
        return false;
    }

    if (info->error_cb) {
        tr064_e_error tr064_error;

        syslog(LOG_DEBUG, "%s invoking error callback", PROXY_TRACE);
        transformererror_to_tr064error(transformer_error, &tr064_error);
        info->error_cb(tr064_error, transformer_error_str, info->cookie);
    }

    return true;
}

/**
* Callback function used when receiving parameters from transformer.
* @param cookie the cookie passed to cwmp_view_getparametervalues_result_cb
*               This may never be NULL;
* @return true if ok, false otherwise
*/
static bool transformer_gpv_callback(void *cookie)
{
    struct rpc_info *info = (struct rpc_info*)cookie;

    // Initialize the response buffer
    message_ctx* msg = &info->ctx->msg;
    if (msg->type != GPV_RESP) {
        syslog(LOG_ERR, "%s invalid response type: expected %d, got %d", PROXY_TRACE, GPV_RESP, msg->type);
        return false;
    }

    gpv_response resp = { 0 };
    // Keep extracting GPV responses from the message till the last one is reached.
    while( decode_GPV_RESP(msg, &resp) ) {
        size_t length = strlen(resp.path);
        char path[length + 1];
        strcpy(path, resp.path);
        //delete the ending dot from the path, if present
        if( path[length - 1] == '.') {
            path[length - 1] = 0;
        }
        // Send the parameter to Transformer
        if (info->result_gpv_cb) {
            info->result_gpv_cb(path, resp.param, resp.value, info->cookie);
        }
    }
    return true;
}

void getpv(const struct query_item params[], transformer_getparametervalues_result_cb result_cb, transformer_generic_error_cb error_cb, void* cookie)
{
    message_ctx* msg = &transformer_proxy.msg;
    bool error = false;

    // Check whether the buffer has been initialized before sending the request
    if( !encode_msg_paramlist(msg, GPV_REQ, params) ) {
        syslog(LOG_ERR, "%s GPV_exec: failed to encode request", PROXY_TRACE);
        error = true;
        goto cleanup;
    }
    struct rpc_info info = {
            .ctx = &transformer_proxy,
            .result_gpv_cb = result_cb,
            .error_cb = error_cb,
            .cookie = cookie,
    };

    // Send the request to the transformer proxy.
    // The call will return when the last response has been processed.
    if (!transformer_proxy_request(&transformer_proxy, &info, transformer_gpv_callback, transformer_generic_error_callback)) {
        syslog(LOG_ERR, "%s %s failed", PROXY_TRACE, "GPV_exec");
        error = true;
        goto cleanup;
    }

cleanup:
    if (error && error_cb)
        error_cb(TR064_ERR_ACTION_FAILED, "Internal Error", cookie);

    if(msg != NULL) {
        //We are finished with the message, reset.
        msg->buff_size = 0;
        msg->index = 0;
        msg->type = UNKNOWN_TYPE;
    }
}


/**
* Callback function used when receiving parameter names from transformer.
* @param cookie the data passecto the request, may not be NULL
* @return true if ok, false otherwise
*/
static bool transformer_gpn_callback(void *cookie)
{
    struct rpc_info *info = (struct rpc_info*)cookie;

    message_ctx* msg = &info->ctx->msg;
    if (msg->type != GPN_RESP) {
        syslog(LOG_ERR, "%s invalid response type: expected %d, got %d", PROXY_TRACE, GPN_RESP, msg->type);
        return false;
    }

    gpn_response resp = { 0 };
    // Keep extracting GPN responses from the message till the last one is reached.
    while( decode_GPN_RESP(msg, &resp) ) {
        size_t length = strlen(resp.path);
        char path[length + 1];
        strcpy(path, resp.path);
        //delete the ending dot from the path, if present
        if( path[length - 1] == '.') {
            path[length - 1] = 0;
        }
        // Send the parameter to CWMP
        if (info->result_gpn_cb) {
            info->result_gpn_cb(path, resp.name, info->cookie);
        }
    }

    return true;
}

void getpn(const struct query_item params[], transformer_getparameternames_result_cb result_cb, transformer_generic_error_cb error_cb, void *cookie)
{
    message_ctx* msg = &transformer_proxy.msg;
    bool error = false;

    if( !encode_msg_paramlist(msg, GPN_REQ, params)) {
        syslog(LOG_ERR, "%s GPN: failed to create request message", PROXY_TRACE);
        error = true;
        goto cleanup;
    }

    struct rpc_info info =  {
            .ctx = &transformer_proxy,
            .result_gpn_cb = result_cb,
            .error_cb = error_cb,
            .cookie = cookie,
    };
    // Send the request to the transformer proxy.
    // The call will return when the last response has been processed.
    if (!transformer_proxy_request(&transformer_proxy, &info, transformer_gpn_callback, transformer_generic_error_callback)) {
        syslog(LOG_ERR, "%s %s failed", PROXY_TRACE, "GPN_exec");
        error = true;
        goto cleanup;
    }

cleanup:
    if (error && error_cb)
        error_cb(TR064_ERR_ACTION_FAILED, "Internal Error", cookie);

    if(msg != NULL) {
        //We are finished with the message, reset.
        msg->buff_size = 0;
        msg->index = 0;
        msg->type = UNKNOWN_TYPE;
    }
}

/**
* Callback function used when setting parameters in transformer.
* @param cookie the cookie passed to the request, can not be NULL
* @return true if OK, false otherwise
*/
static bool cwmp_spv_callback(void *cookie)
{
    struct rpc_info *info = (struct rpc_info*)cookie;
    tr064_e_error tr064_error = TR064_ERR_NO_ERROR;

    message_ctx* msg = &info->ctx->msg;
    if( msg->type != SPV_RESP ) {
        syslog(LOG_ERR, "%s invalid response type: expected %d, got %d", PROXY_TRACE, SPV_RESP, msg->type);
        return false;
    }

    // Check if the spv had an error
    spv_response resp = {};
    if (decode_SPV_RESP(msg, &resp)  && info->error_cb)
    {
        transformererror_to_tr064error(resp.errcode, &tr064_error);
        info->error_cb(tr064_error, resp.errmsg, info->cookie);
    }
    else
        info->result_spv_cb(info->cookie);

    return true;
}

void setpv(const struct query_item params[], transformer_setparametervalues_result_cb result_cb,
           transformer_generic_error_cb error_cb, void *cookie)
{
    message_ctx* msg = &transformer_proxy.msg;
    bool error = false;
    // Check whether the buffer has been initialized before sending the request
    if( !encode_msg_paramlist(msg, SPV_REQ, params)) {
        syslog(LOG_ERR, "%s SPV exec; failed to create request message", PROXY_TRACE);
        error = true;
        goto cleanup;
    }

    struct rpc_info info = {
            .ctx = &transformer_proxy,
            .result_spv_cb = result_cb,
            .error_cb = error_cb,
            .cookie = cookie
    };

    // Send the request to the transformer proxy. The call will return when the last response has been processed.
    // Errors will be included in the setParameterValues response message.
    // No need for an error callback, all errors are reported via cwmp_spv_callback
    if (!transformer_proxy_request(&transformer_proxy, &info, cwmp_spv_callback, NULL)) {
        syslog(LOG_ERR, "%s %s failed", PROXY_TRACE, "SPV_exec");
        error = true;
        goto cleanup;
    }

cleanup:
    if (error)
        error_cb(TR064_ERR_ACTION_FAILED, "Internal Error", cookie);

    if(msg != NULL) {
        //We are finished with the message, reset.
        msg->buff_size = 0;
        msg->index = 0;
        msg->type = UNKNOWN_TYPE;
    }
}

bool apply()
{
    // send an 'apply' message to Transformer so daemons
    // can be restarted, if needed, after changes to the datamodel
    syslog(LOG_DEBUG, "%s apply", PROXY_TRACE);
    message_ctx* msg = &transformer_proxy.msg;
    if (msg->type != APPLY) {
        // Initialize the request buffer if it has not been initialized yet
        message_init_encode(msg, APPLY, MAX_MESSAGE_SIZE, &transformer_uuid);
    }
    if (!encode_APPLY(msg)) {
        syslog(LOG_ERR, "%s %s: encoding failed", PROXY_TRACE, "apply");
        return false;
    }
    bool result = transformer_proxy_request(&transformer_proxy, NULL, NULL, NULL);
    //We are finished with the message, reset.
    msg->buff_size = 0;
    msg->index = 0;
    msg->type = UNKNOWN_TYPE;
    return result;
}
