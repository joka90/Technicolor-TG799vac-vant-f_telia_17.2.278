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

#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include <stdbool.h>

#include "proxy.h"

#define RECEIVE_TIMEOUT         60  // seconds
#define PROXY_SERVER_PATH       "transformer"
#define ABSTRACT_SUN_LEN        ((socklen_t) (((struct sockaddr_un *) 0)->sun_path) + sizeof(PROXY_SERVER_PATH))

/**
* Handle a response from Transformer.
* @param proxy     the cwmp transformer proxy context object
* @param cookie    cookie passed to callback
* @param cb        the callback function to be called when receiving a response
* @param error_cb  the callback function to be called when receiving an error
* @return Whether processing the response was successful
*/
static bool transformer_proxy_handle_response(
        transformer_proxy_ctx *proxy,
        void *cookie,
        proxy_request_callback_fn cb,
        proxy_request_callback_fn error_cb)
{
    // Unix datagrams will be read one by one, no concatenation or partial packets.
    if( !message_init_decode(&proxy->msg) ) {
        syslog(LOG_ERR, "%s response msg can not be handled, init decode failed.", PROXY_TRACE);
        return false;
    }
    syslog(LOG_DEBUG, "%s [transformer_proxy_handle_response] type: %d, last=%d", PROXY_TRACE, proxy->msg.type, proxy->msg.is_last);
    switch (proxy->msg.type) {
        case GPV_RESP:
        case SPV_RESP:
        case GPN_RESP:
        case GPC_RESP:
            if (cb) {
                return cb(cookie);
            }
            return true;
            break;
        case GENERIC_ERROR:
            if (error_cb) {
                return error_cb(cookie);
            }
            return true;
            break;
        default:
            syslog(LOG_ERR, "%s invalid message %d received", PROXY_TRACE, proxy->msg.type);
            break;
    }

    return false;
}

bool transformer_proxy_request(
        transformer_proxy_ctx *proxy,
        void *cookie,
        proxy_request_callback_fn cb,
        proxy_request_callback_fn error_cb)
{
    // init socket if not done yet
    if (proxy->socket < 0) {
        proxy->socket = proxy->sock->create();
        if (proxy->socket < 0)
            return false;
    }
    // Check if there is something to be done
    message_ctx* msg = &proxy->msg;
    if (!msg || msg->buffer == NULL || msg->buff_size == 0) {
        return false;
    }

    // send the request to Transformer
    syslog(LOG_DEBUG, "%s sending request type %d", PROXY_TRACE, msg->buffer[0]);
    msg->buffer[0] |= 0x80;
    if (proxy->sock->send(proxy->socket, msg->buffer, msg->index) < 0) {
        syslog(LOG_DEBUG, "%s first send() attempt failed: %s", PROXY_TRACE, strerror(errno));
        // send() failed: close the socket, create a new one and retry once
        proxy->sock->close(proxy->socket);
        proxy->socket = proxy->sock->create();
        if (proxy->socket < 0) {
            return false;
        }
        if (proxy->sock->send(proxy->socket, msg->buffer, msg->index) < 0) {
            syslog(LOG_ERR, "%s send() failed: %s", PROXY_TRACE, strerror(errno));
            return false;
        }
    }

    if (msg->no_response) {
        syslog(LOG_WARNING, "%s not expecting a response", PROXY_TRACE);
        return true;
    }

    // Start receiving responses from Transformer.
    // make sure the msg buffer is large enough
    if (msg->max_size < MAX_MESSAGE_SIZE) {
        free(msg->buffer);
        msg->buffer = (uint8_t *) calloc(MAX_MESSAGE_SIZE + 1, sizeof(uint8_t));
        msg->max_size = MAX_MESSAGE_SIZE;
        msg->index = 0;
        msg->buff_size = 0;
    }

    bool done = false;
    bool error = false;
    while (!done && !error)
    {
        msg->buff_size = proxy->sock->recv(proxy->socket, msg->buffer, msg->max_size);

        if( msg->buff_size>0 ) {
            //receive succeeded
            // Handle the response message, report error if that fails
            error = !transformer_proxy_handle_response(proxy, cookie, cb, error_cb);
            done = msg->is_last;
        }
        else {
            // even though cwmpd registers its signal handlers with the SA_RESTART flag
            // this does not restart recv() when the socket has a timeout set
            // (see signal(7)) so we have to explicitly handle the EINTR case
            // an interrupted recv is not an error.
            error = !( (msg->buff_size<0) && (errno==EINTR) );
        }
    }

    // handle any error
    if( error ) {
        // an error occured during the receive loop
        if( msg->buff_size>0 ) {
            syslog(LOG_ERR, "%s response message was not handled correctly", PROXY_TRACE);
        }
        else if( msg->buff_size < 0 ) {
            if( (errno==EAGAIN) || (errno==EWOULDBLOCK)) {
                syslog(LOG_ERR, "%s recv() timed out", PROXY_TRACE);
            }
            else {
                syslog(LOG_ERR, "%s recv() failed: %s", PROXY_TRACE, strerror(errno));
            }
        }
        else {
            syslog(LOG_ERR, "%s connection closed by transformer", PROXY_TRACE);
        }
        syslog(LOG_WARNING, "%s Closing connection due to previous error", PROXY_TRACE);
        proxy->sock->close(proxy->socket);
        proxy->socket = -1;
        return false;
    }

    return true;
}

static int transformer_proxy_create_socket(void)
{
    // just to be sure create the socket with the close-on-exec flag set
    int s = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if( 0 <= s ) {
        struct sockaddr_un server_address;
        int on = 1;
        struct timeval rcvtimeout = { .tv_sec = RECEIVE_TIMEOUT };

        // Set socket option to include credentials in every call to the server
        setsockopt(s, SOL_SOCKET, SO_PASSCRED, &on, sizeof(on));

        // Set receive timeout so we don't hang indefinitely in case
        // Transformer never sends back a reply.
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &rcvtimeout, sizeof(rcvtimeout));

        // Connect to Transformer.
        memset(&server_address, 0, sizeof(struct sockaddr_un));
        server_address.sun_family = AF_UNIX;
        memcpy(&server_address.sun_path[1], PROXY_SERVER_PATH, sizeof(PROXY_SERVER_PATH) - 1);
        if (connect(s, (struct sockaddr *) &server_address, ABSTRACT_SUN_LEN) < 0) {
            syslog(LOG_WARNING, "%s connect() failed %s", PROXY_TRACE, strerror(errno));
            close(s);
            return -1;
        }
    }
    else {
        syslog(LOG_WARNING, "%s socket() failed %s", PROXY_TRACE, strerror(errno));
    }
    return s;
}

/**
* Initialize the transformer proxy client. The provided proxy object can't be null, storage needs
* to be initialized in advance, contents doesn't need to be initialized to zero.
* @param proxy  the cwmp transformer proxy context object
* @return whether initialization was successful
*/
bool transformer_proxy_init(transformer_proxy_ctx *proxy)
{
    static const SocketInterface def_socket_intf = {
            .create = transformer_proxy_create_socket,
            .close = close,
            .recv = read,
            .send = write
    };

    if( proxy->sock == NULL) {
        proxy->sock = &def_socket_intf;
    }

    // Don't create the socket just yet but do it lazily.
    // That way we're less dependent on Transformer being
    // up and running when cwmpd starts.
    // To be really independent we need socket activation
    // like systemd offers...
    proxy->socket = -1;
    return true;
}

/**
* Cleanup the connection to the transformer proxy.
* @param proxy  the cwmp transformer proxy context object
* @return whether finalization was successful
*/
bool transformer_proxy_fini(transformer_proxy_ctx *proxy)
{
    bool ret = true;

    if (proxy->socket >= 0) {
        ret = (proxy->sock->close(proxy->socket) == 0);
        proxy->socket = -1;
    }
    return ret;
}
