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

#ifndef TRANSFORMER_BACKEND_MESSAGE_H
#define TRANSFORMER_BACKEND_MESSAGE_H

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define PRIVATE static
#define HIDDEN __attribute__((visibility("hidden")))

/**
* The different transformer proxy message types. This list need to be kept in sync with the one found
* in transformer/msg.lua
*/
typedef enum {
    GENERIC_ERROR = 1,  //!< Error response
    GPV_REQ,            //!< GetParameterValues request
    GPV_RESP,           //!< GetParameterValues response
    SPV_REQ,            //!< SetParameterValues request
    SPV_RESP,           //!< SetParameterValues response
    APPLY,              //!< Apply
    ADD_REQ,            //!< AddObject request
    ADD_RESP,           //!< AddObject response
    DEL_REQ,            //!< DeleteObject request
    DEL_RESP,           //!< DeleteObject response
    GPN_REQ,            //!< GetParameterNames request
    GPN_RESP,           //!< GetParameterNames response
    RESOLVE_REQ,        //!< Resolve request
    RESOLVE_RESP,       //!< Resolve response
    SUBSCRIBE_REQ,      //!< Subscribe request
    SUBSCRIBE_RESP,     //!< Subscribe response
    UNSUBSCRIBE_REQ,    //!< Unsubscribe request
    UNSUBSCRIBE_RESP,   //!< Unsubscribe response
    EVENT,              //!< Event
    GPL_REQ,            //!< GetParameterList request
    GPL_RESP,           //!< GetParameterList response
    GPC_REQ,            //!< GetCount request
    GPC_RESP,           //!< GetCount response
    UNKNOWN_TYPE,       //!< Sentinel
} msgType;

#define MAX_MESSAGE_SIZE  (0x8400) // Max message size is 33K

typedef struct {
    msgType type;
    int index; // Index of the pointer in the data buffer
    int buff_size; // Size of the current buffer
    uint8_t* buffer; // Current buffer
    ssize_t max_size;
    char uuid[16];
    uint8_t tmpByte; // Temporary placeholder for a byte when decoding.
    bool byte_set; // The temporary byte is set.
    bool no_response; //true if no response is expected
    bool is_last; //true if last response message
} message_ctx;

HIDDEN void message_init_encode(message_ctx* msg, msgType type, int max_msg_size, void* msg_uuid);
HIDDEN bool message_init_decode(message_ctx* msg);
HIDDEN void message_copy(message_ctx* from, message_ctx* to);
HIDDEN void message_destruct(message_ctx* msg);

HIDDEN bool encode_GPV_REQ(message_ctx* msg, const char* path);
HIDDEN bool encode_SPV_REQ(message_ctx* msg, const char* path, const char* value);
HIDDEN bool encode_APPLY(message_ctx* msg);
HIDDEN bool encode_ADD_REQ(message_ctx* msg, const char* path);
HIDDEN bool encode_DEL_REQ(message_ctx* msg, const char* path);
HIDDEN bool encode_GPN_REQ(message_ctx* msg, const char* path, uint16_t level);
HIDDEN bool encode_RESOLVE_REQ(message_ctx* msg, const char* path, const char* key);
HIDDEN bool encode_SUBSCRIBE_REQ(message_ctx* msg, const char* path, const char* address, uint8_t subscr_type, uint8_t options);
HIDDEN bool encode_UNSUBSCRIBE_REQ(message_ctx* msg, uint16_t id);
HIDDEN bool encode_GPL_REQ(message_ctx* msg, const char* path);
HIDDEN bool encode_GPC_REQ(message_ctx* msg, const char* path);

typedef struct {
    int errcode;
    const char* errmsg;
} error_response;
HIDDEN bool decode_ERROR(message_ctx* msg, error_response* resp);
typedef struct {
    const char* path;
    const char* param;
    const char* value;
    const char* type;
} gpv_response;
HIDDEN bool decode_GPV_RESP(message_ctx* msg, gpv_response* resp);
typedef struct {
    int errcode;
    const char* errmsg;
    const char* path;
} spv_response;
HIDDEN bool decode_SPV_RESP(message_ctx* msg, spv_response* resp);
HIDDEN bool decode_ADD_RESP(message_ctx* msg, const char** resp);
HIDDEN bool decode_DEL_RESP(message_ctx* msg);
typedef struct {
    const char* path;
    const char* name;
    bool writable;
} gpn_response;
HIDDEN bool decode_GPN_RESP(message_ctx* msg, gpn_response* resp);
HIDDEN bool decode_RESOLVE_RESP(message_ctx* msg, const char** resp);
typedef struct {
    int id;
    const char* path;
} subscribe_response;
HIDDEN bool decode_SUBSCRIBE_RESP(message_ctx* msg, subscribe_response* resp);
HIDDEN bool decode_UNSUBSCRIBE_RESP(message_ctx* msg);
typedef struct {
    int id;
    const char* path;
    uint8_t eventmask;
    const char* value;
} event_response;
HIDDEN bool decode_EVENT(message_ctx* msg, event_response* resp);
typedef struct {
    const char *path;
    const char *param;
} gpl_response;
HIDDEN bool decode_GPL_RESP(message_ctx *msg, gpl_response *resp);
HIDDEN bool decode_GPC_RESP(message_ctx *msg, int *count);

#endif