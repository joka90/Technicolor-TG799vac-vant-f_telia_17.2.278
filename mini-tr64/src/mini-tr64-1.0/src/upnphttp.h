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


#ifndef UPNPHTTP_H_INCLUDED
#define UPNPHTTP_H_INCLUDED

#include <netinet/in.h>
#include <sys/queue.h>

#include "config.h"

#include <openssl/ssl.h>

#if 0
/* according to "UPnP Device Architecture 1.0" */
#define UPNP_VERSION_STRING "UPnP/1.0"
#else
/* according to "UPnP Device Architecture 1.1" */
#define UPNP_VERSION_STRING "UPnP/1.1"
#endif

/* server: HTTP header returned in all HTTP responses : */
#define MINITR064D_SERVER_STRING	OS_VERSION " " UPNP_VERSION_STRING " MiniTR064d/" MINITR064D_VERSION

/*
 states :
  0 - waiting for data to read
  1 - waiting for HTTP Post Content.
  ...
  >= 100 - to be deleted
*/
enum httpStates {
    EWaitingForHttpRequest = 0,
    EWaitingForHttpContent,
    ESendingContinue,
    ESendingAndClosing,
    EToDelete = 100
};

enum httpCommands {
    EUnknown = 0,
    EGet,
    EPost,
    ESubscribe,
    EUnSubscribe
};

enum authStates {
    None,
    Stale,
    DSLConfig,
    DSLReset,
};

struct upnphttp {
    int socket;
    struct in_addr clientaddr;	/* client address */
#ifdef ENABLE_IPV6
	int ipv6;
	struct in6_addr clientaddr_v6;
#endif /* ENABLE_IPV6 */
	SSL * ssl;
    enum httpStates state;
    char HttpVer[16];
    /* request */
    char * req_buf;
    char accept_language[8];
    int req_buflen;
    int req_contentlen;
    int req_contentoff;     /* header length */
    enum httpCommands req_command;
    int req_soapActionOff;
    int req_soapActionLen;
    int respflags;				/* see FLAG_* constants below */
    /* response */
    char * res_buf;
    int res_buflen;
    int res_sent;
    int res_buf_alloclen;
    enum authStates authState;
    LIST_ENTRY(upnphttp) entries;
};

/* Include the "Timeout:" header in response */
#define FLAG_TIMEOUT	0x01
/* Include the "SID:" header in response */
#define FLAG_SID		0x02

/* If set, the POST request included a "Expect: 100-continue" header */
#define FLAG_CONTINUE	0x40

/* If set, the Content-Type is set to text/xml, otherwise it is text/xml */
#define FLAG_HTML		0x80

/* If set, the corresponding Allow: header is set */
#define FLAG_ALLOW_POST			0x100
#define FLAG_ALLOW_SUB_UNSUB	0x200

int init_ssl(void);
void free_ssl(void);

/* New_upnphttp() */
struct upnphttp *
        New_upnphttp(int);

void
InitSSL_upnphttp(struct upnphttp *);

/* CloseSocket_upnphttp() */
void
        CloseSocket_upnphttp(struct upnphttp *);

/* Delete_upnphttp() */
void
        Delete_upnphttp(struct upnphttp *);

/* Process_upnphttp() */
void
        Process_upnphttp(struct upnphttp *);

/* BuildHeader_upnphttp()
 * build the header for the HTTP Response
 * also allocate the buffer for body data */
void
        BuildHeader_upnphttp(struct upnphttp * h, int respcode,
        const char * respmsg,
        int bodylen);

/* BuildResp_upnphttp()
 * fill the res_buf buffer with the complete
 * HTTP 200 OK response from the body passed as argument */
void
        BuildResp_upnphttp(struct upnphttp *, const char *, int);

/* BuildResp2_upnphttp()
 * same but with given response code/message */
void
        BuildResp2_upnphttp(struct upnphttp * h, int respcode,
        const char * respmsg,
        const char * body, int bodylen);

int
        SendResp_upnphttp(struct upnphttp *);

/* SendRespAndClose_upnphttp() */
void
        SendRespAndClose_upnphttp(struct upnphttp *);

void
        Send401(struct upnphttp * h);

#endif

