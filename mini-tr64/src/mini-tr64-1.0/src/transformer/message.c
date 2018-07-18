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
#include "message.h"

#define MESSAGE_TRACE        "MESSAGE: "

PRIVATE void revert_encoding(message_ctx* msg, int old_index) {
    int diff = msg->index - old_index;
    memset(&msg->buffer[old_index],0,diff);
    msg->index = old_index;
    msg->buff_size = old_index;
}

// precondition: msg != NULL
void message_init_encode(message_ctx* msg, msgType type, int max_msg_size, void* msg_uuid){
    if (msg == NULL || max_msg_size < 1 || (msg_uuid != NULL && max_msg_size<17)) {
        // The message should already be allocated and it needs at least a type.
        return;
    }
    if(msg->max_size < max_msg_size) {
        // A bigger max_msg_size, free buffer and calloc a new one.
        msg->max_size = max_msg_size;
        free(msg->buffer);
        msg->buffer = (uint8_t *) calloc(max_msg_size + 1, sizeof(uint8_t));
        if (!msg->buffer) {
            free(msg);
            syslog(LOG_ERR, "%s message_init_encode: message buffer calloc failed", MESSAGE_TRACE);
            return;
        }
    } else {
        // Max buffer size doesn't need to change, clear buffer.
        memset(msg->buffer, 0, msg->max_size + 1);
    }
    msg->index = 0;
    msg->buff_size = 0;
    msg->type = type;
    msg->buffer[msg->index] = msg->type;
    msg->index += 1;
    msg->buff_size += 1;
    if (msg_uuid != NULL){
        memcpy(&msg->buffer[msg->index], msg_uuid,16);
        memcpy(&msg->uuid, msg_uuid, 16);
        msg->index += 16;
        msg->buff_size += 16;
    }
    msg->byte_set = false;
    msg->no_response = false;
}

// The received message needs to be in buffer and buff_size needs to be set correctly.
// make sure the message buffer is unchanged so it is safe to call this
// function more than once for the message
bool message_init_decode(message_ctx* msg) {
    if (msg == NULL || msg->buff_size < 1) {
        // A message needs at least a type.
        return false;
    }
    if(msg->max_size < msg->buff_size) {
        // A bigger buffer is present, increase max_size
        msg->max_size = msg->buff_size;
    }
    // set is_last to high bit of msg code
    msg->is_last = (msg->buffer[0] & 0x80)!=0;

    // set msg type code
    uint8_t type = msg->buffer[0] & 0x7F;
    if ((type>=GENERIC_ERROR) && (type<UNKNOWN_TYPE)) {
        msg->type = (msgType)type;
    } else {
        msg->type = UNKNOWN_TYPE;
    }
    //TODO: Based on type, retrieve UUID.
    msg->index = 1; // Tag byte is parsed, if UUID is parsed, move index.
    msg->byte_set = false;

    return true;
}

// prerequisite: to != NULL
void message_copy(message_ctx* from, message_ctx* to) {
    if (from == NULL || to == NULL ) {
        // Both contexts should have been allocated.
        return;
    }
    // copy all
    *to = *from;

    // clone dynamic fields
    if( from->max_size>0 ) {
        // use plain malloc here as initializing is not needed.
        //the buffer will be overwritten immediatly
        to->buffer = (uint8_t *) malloc(from->max_size + 1);
        if (!to->buffer) {
            syslog(LOG_ERR, "%s message_copy: failed to allocate destination buffer", MESSAGE_TRACE);
            return;
        }
        memcpy(to->buffer, from->buffer, from->max_size);
    }
    else {
        to->buffer = NULL;
    }
}

void message_destruct(message_ctx* msg) {
    free(msg->buffer);
    msg->buffer = NULL;
}

PRIVATE bool encode_number(message_ctx* msg, uint16_t number){
    if ((msg->index + 2) >= msg->max_size) {
        // Encode buffer full.
        return false;
    }
    uint8_t* buf = msg->buffer;
    buf[msg->index] = number >> 8;
    buf[msg->index + 1] = number & 0x00ff;
    msg->index += 2;
    msg->buff_size += 2;
    return true;
}

PRIVATE bool encode_string(message_ctx* msg, const char* str){
    if (!str)
        return false;

    uint16_t s_len = strlen(str);
    if (!encode_number(msg, s_len)){
        return false;
    }
    if ((msg->index + s_len) >= msg->max_size) {
        // Encode buffer full.
        return false;
    }
    uint8_t* buf = msg->buffer;
    memcpy(&buf[msg->index], str, s_len);
    msg->index += s_len;
    msg->buff_size += s_len;
    return true;
}

PRIVATE bool encode_byte(message_ctx* msg, uint8_t byte){
    if ((msg->index + 1) >= msg->max_size) {
        // Encode buffer full.
        return false;
    }
    uint8_t* buf = msg->buffer;
    buf[msg->index] = byte;
    msg->index += 1;
    msg->buff_size += 1;
    return true;
}

static bool encode_param_request(message_ctx *msg, const char *path, const char *method)
{
    //CWMP_LOG_DBG(MESSAGE_TRACE, "encode_%s: %s", method,  path);
    int old_index = msg->index;
    if (!encode_string(msg, path)){
        syslog(LOG_ERR, "%s encode_%s: reverting encoding!", MESSAGE_TRACE, method);
        revert_encoding(msg, old_index);
        return false;
    }
    return true;
}

bool encode_GPV_REQ(message_ctx* msg, const char* path)
{
    return encode_param_request(msg, path, "GPV_REQ");
}

bool encode_GPL_REQ(message_ctx* msg, const char* path)
{
    return encode_param_request(msg, path, "GPL_REQ");
}

bool encode_GPC_REQ(message_ctx* msg, const char* path)
{
    return encode_param_request(msg, path, "GPC_REQ");
}


bool encode_SPV_REQ(message_ctx* msg, const char* path, const char* value){
    int old_index = msg->index;
    bool success = encode_string(msg, path) &&
            encode_string(msg, value);
    if (!success){
        syslog(LOG_ERR, "%s encode_SPV_REQ: reverting encoding!", MESSAGE_TRACE);
        revert_encoding(msg, old_index);
    }
    return success;
}

bool encode_APPLY(message_ctx* msg) {
    msg->no_response = true;
    return true;
}

bool encode_ADD_REQ(message_ctx* msg, const char* path) {
    int old_index = msg->index;
    bool success = encode_string(msg, path);
    if (!success){
        syslog(LOG_ERR, "%s encode_ADD_REQ: reverting encoding!", MESSAGE_TRACE);
        revert_encoding(msg, old_index);
    }
    return success;
}

bool encode_DEL_REQ(message_ctx* msg, const char* path) {
    int old_index = msg->index;
    if (!encode_string(msg, path)) {
        syslog(LOG_ERR, "%s encode_DEL_REQ: reverting encoding!", MESSAGE_TRACE);
        revert_encoding(msg, old_index);
        return false;
    }
    return true;
}

bool encode_GPN_REQ(message_ctx* msg, const char* path, uint16_t level) {
    int old_index = msg->index;
    bool success = encode_string(msg, path) &&
            encode_number(msg, level);
    if (!success){
        syslog(LOG_ERR, "%s encode_GPN_REQ: reverting encoding!", MESSAGE_TRACE);
        revert_encoding(msg, old_index);
    }
    return success;
}

bool encode_RESOLVE_REQ(message_ctx* msg, const char* path, const char* key) {
    int old_index = msg->index;
    bool success = encode_string(msg, path) &&
            encode_string(msg, key);
    if (!success){
        syslog(LOG_ERR, "%s encode_RESOLVE_REQ: reverting encoding!", MESSAGE_TRACE);
        revert_encoding(msg, old_index);
    }
    return success;
}

bool encode_SUBSCRIBE_REQ(message_ctx* msg, const char* path, const char* address, uint8_t subscr_type, uint8_t options){
    int old_index = msg->index;
    bool success = encode_string(msg, path) &&
            encode_string(msg, address) &&
            encode_byte(msg, subscr_type) &&
            encode_byte(msg, options);
    if (!success){
        syslog(LOG_ERR, "%s encode_SUBSCRIBE_REQ: reverting encoding!", MESSAGE_TRACE);
        revert_encoding(msg, old_index);
    }
    return success;
}

bool encode_UNSUBSCRIBE_REQ(message_ctx* msg, uint16_t id) {
    return encode_number(msg, id);
}

/************************
* Decode
************************/

PRIVATE bool decode_byte(message_ctx* msg, uint8_t* by) {
    int index = msg->index;
    if (index + 1 > msg->buff_size){
        return false;
    }
    if (msg->byte_set) {
        *by = msg->tmpByte;
        msg->byte_set = false;
    } else {
        *by = (uint8_t) msg->buffer[index];
    }
    msg->index = index + 1;
    return true;
}

PRIVATE bool decode_number(message_ctx* msg, int* number) {
    // Read the path
    int index = msg->index;
    if (index + 2 > msg->buff_size) {
        syslog(LOG_ERR, "%s decode_number: Trying to read past buffer size: %d > %d", MESSAGE_TRACE, index + 2, msg->buff_size);
        return false;
    }
    uint8_t* buf = msg->buffer;
    uint8_t first_byte = buf[index];
    if (msg->byte_set) {
        first_byte = msg->tmpByte;
        msg->byte_set = false;
    }
    uint16_t length = (first_byte << 8) + buf[index + 1];
    *number = (int) length;
    msg->index = index + 2;
    return true;
}

PRIVATE bool decode_string(message_ctx* msg, const char** str) {
    int length, index;
    if (!decode_number(msg, &length) ||
            (index = msg->index, index + length > msg->buff_size)) {
        return false;
    }
    *str = (char *) &msg->buffer[index];
    index += length;
    msg->index = index;
    msg->tmpByte = msg->buffer[index];
    msg->byte_set = true;
    msg->buffer[index] = '\0'; // Null-terminate string.
    return true;
}

bool decode_ERROR(message_ctx* msg, error_response* resp) {
    return decode_number(msg, &resp->errcode) &&
            decode_string(msg, &resp->errmsg);
}

bool decode_GPV_RESP(message_ctx* msg, gpv_response* resp) {
    if (msg->index >= msg->buff_size) {
        return false;
    }
    return decode_string(msg, &resp->path) &&
            decode_string(msg, &resp->param) &&
            decode_string(msg, &resp->value) &&
            decode_string(msg, &resp->type);
}

bool decode_GPL_RESP(message_ctx *msg, gpl_response *resp)
{
    if( msg->buff_size <= msg->index ) {
        return false;
    }
    return decode_string(msg, &resp->path) &&
            decode_string(msg, &resp->param);
}

bool decode_GPC_RESP(message_ctx *msg, int *count)
{
    return decode_number(msg, count);
}


bool decode_SPV_RESP(message_ctx* msg, spv_response* resp){
    if (msg->index >= msg->buff_size) {
        return false;
    }
    return decode_number(msg, &resp->errcode) &&
            decode_string(msg, &resp->path) &&
            decode_string(msg, &resp->errmsg);
}

bool decode_ADD_RESP(message_ctx* msg, const char** resp) {
    return decode_string(msg, resp);
}

bool decode_DEL_RESP(message_ctx* msg) {
    return true;
}

bool decode_GPN_RESP(message_ctx* msg, gpn_response* resp) {
    int writ;
    if (msg->index >= msg->buff_size) {
        return false;
    }
    return decode_string(msg, &resp->path) &&
            decode_string(msg, &resp->name) &&
            decode_number(msg, &writ) &&
            (resp->writable = !(writ == 0), true);
}

bool decode_RESOLVE_RESP(message_ctx* msg, const char** resp) {
    return decode_string(msg, resp);
}

bool decode_SUBSCRIBE_RESP(message_ctx* msg, subscribe_response* resp) {
    bool decoded_id = false;
    if (!resp->id){
        if (decode_number(msg, &resp->id)){
            decoded_id = true;
            // Continue decoding in case there are non-eventable paths.
        } else {
            return false;
        }
    }
    if (msg->index < msg->buff_size) {
        return decode_string(msg, &resp->path);
    }
    return decoded_id;
}

bool decode_UNSUBSCRIBE_RESP(message_ctx* msg) {
    return true;
}

bool decode_EVENT(message_ctx* msg, event_response* resp) {
    return decode_number(msg, &resp->id) &&
            decode_string(msg, &resp->path) &&
            decode_byte(msg, &resp->eventmask) &&
            ((msg->index >= msg->buff_size) ||
                    decode_string(msg, &resp->value));
}