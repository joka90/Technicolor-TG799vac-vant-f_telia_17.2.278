/************** COPYRIGHT AND CONFIDENTIALITY INFORMATION *********************
**                                                                          **
** Copyright (c) 2013 Technicolor                                           **
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

/**
* TR-064 Transformer Proxy Client
*
*/

#ifndef __TRANSFORMER_PROXY_H__
#define __TRANSFORMER_PROXY_H__

#include <sys/un.h>
#include <stdint.h>
#include <stdbool.h>
#include "message.h"

#define HIDDEN __attribute__((visibility("hidden")))

#define PROXY_TRACE        "TRANSFORMER_PROXY: "

typedef struct
{
    int (*create)(void);
    int (*close)(int sock);
    ssize_t (*recv)(int sock, void *buf, size_t len);
    ssize_t (*send)(int sock, const void *buf, size_t len);
} SocketInterface;


/**
* Transformer proxy client context structure.
*/
typedef struct {
    int socket;
    message_ctx msg; // Message context to communicate with Transformer.
    message_ctx event_msg; // Message context to receive Transformer events.

    const SocketInterface * sock;
} transformer_proxy_ctx;

/**
* request callback function.
* The prototype for the result callback and the error callback is the same.
*
* @param cookie the cookie passed in the to the request function
* @return true if successful, false if not
*/
typedef bool (*proxy_request_callback_fn)(void* cookie);


/**
* Send the request currently in the request buffer to the transformer proxy.
* @param cookie    a data pointer passed to the callback function
* @param cb        the callback function to be called when receiving a response
* @param error_cb  the callback function to be called when receiving an error
* @return whether sending the request and handling the responses was successful
*
* The callback will be called synchronously so the ctx pointer need only be
* valid during the call. This makes it possible to use a pointer to a local
* variable. (no need for malloc)
*/
HIDDEN bool transformer_proxy_request(
        transformer_proxy_ctx *proxy,
        void *cookie,
        proxy_request_callback_fn cb,
        proxy_request_callback_fn error_cb);

/**
* Initialize the transformer proxy client. The provided proxy object can't be null, storage needs
* to be initialized in advance, contents doesn't need to be initialized to zero.
* @param proxy  the cwmp transformer proxy context object
* @return whether initialization was successful
*/
HIDDEN bool transformer_proxy_init(transformer_proxy_ctx *proxy);

/**
* Cleanup the connection to the transformer proxy.
* @param proxy  the cwmp transformer proxy context object
* @return whether finalization was successful
*/
HIDDEN bool transformer_proxy_fini(transformer_proxy_ctx *proxy);


#endif
