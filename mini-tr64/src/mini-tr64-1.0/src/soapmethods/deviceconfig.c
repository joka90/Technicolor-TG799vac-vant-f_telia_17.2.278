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

#include <stdio.h>
#include "../transformer/helper.h"
#include "soaphelper.h"
#include "../upnpurns.h"
#include "../upnpsoap.h"
#include "../upnpglobalvars.h"

static void DCActionCallback(void *cookie) {
    struct ctxdata *calldata = cookie;
    struct getinfostruct *gidata;
    char body[1024];
    int bodylen;
    static const char resp[] =
            "<u:%sResponse "
                    "xmlns:u=\"%s\">"
                    "</u:%sResponse>";

    if (!calldata) {
        syslog(LOG_ERR, "NULL context data, aborting");
        return;
    }
    gidata = calldata->data;

    bodylen = snprintf(body, sizeof(body), resp,
            calldata->action, SERVICE_TYPE_DC, calldata->action);
    BuildSendAndCloseSoapResp(calldata->h, body, bodylen);

}


static void DCActionErrorCallback(tr064_e_error errorcode, const char *errorstr, void *cookie) {
    struct ctxdata *ctx = cookie;

    if(!ctx) {
        syslog(LOG_ERR, "NULL context data, errorcode: %d errorstring= %s", errorcode, errorstr);
        return;
    }

    SoapError(ctx->h, errorcode, errorstr);
}

void DCReboot(struct upnphttp * h, const char * action)
{
    struct ctxdata cookie = {
            .h = h,
            .action = action,
            .data = NULL
    };
    struct query_item query[] = {
            { .name = "rpc.system.reboot", .value = "TR64" },
            { .name = 0 }
    };
    setpv(query, &DCActionCallback, &DCActionErrorCallback, &cookie);
}

void DCFactoryReset(struct upnphttp * h, const char * action)
{
    struct ctxdata cookie = {
            .h = h,
            .action = action,
            .data = NULL
    };
    struct query_item query[] = {
            { .name = "rpc.system.reset", .value = "1" },
            { .name = 0 }
    };
    setpv(query, &DCActionCallback, &DCActionErrorCallback, &cookie);
}

void DCConfigurationStarted(struct upnphttp * h, const char * action)
{
    char body[1024];
    char errormsg[512];
    int bodylen;
    static const char resp[] =
            "<u:%sResponse "
                    "xmlns:u=\"%s\">"
                    "</u:%sResponse>";
    char prefix[] = "<NewSessionID>";
    char postfix[] = "</NewSessionID>";
    char *startpos;
    char *endpos;
    size_t prefixlen;
    size_t valuelen;

    if(config_session_id[0] != '\0') {
        snprintf(errormsg, sizeof(errormsg), "Configuration session already active");
        SoapError(h, 501, errormsg);
        return;
    }

    prefixlen = strlen(prefix);
    startpos = strstr(h->req_buf, prefix);
    if(!startpos) {
        snprintf(errormsg, sizeof(errormsg), "Missing parameter NewSessionID");
        SoapError(h, 501, errormsg);
        return;
    }
    endpos = strstr(startpos + prefixlen, postfix);
    if(!endpos) {
        snprintf(errormsg, sizeof(errormsg), "Missing parameter NewSessionID");
        SoapError(h, 501, errormsg);
        return;
    }
    valuelen = endpos - startpos - prefixlen;
    if(valuelen > (sizeof(config_session_id)-1)) {
        snprintf(errormsg, sizeof(errormsg), "NewSessionID is too long");
        SoapError(h, 501, errormsg);
        return;
    }

    memcpy(config_session_id, startpos + prefixlen, valuelen);
    config_session_id[valuelen] = '\0';
    clock_gettime(CLOCK_MONOTONIC, &config_session_lastupdate);

    syslog(LOG_NOTICE, "Started configuration session for  id %s", config_session_id);

    bodylen = snprintf(body, sizeof(body), resp, action, SERVICE_TYPE_DC, action);
    BuildSendAndCloseSoapResp(h, body, bodylen);
}

void DCConfigurationFinished(struct upnphttp * h, const char * action)
{
    char body[1024];
    int bodylen;
    static const char resp[] =
            "<u:%sResponse "
                    "xmlns:u=\"%s\">"
                    "<NewStatus><![CDATA[%s]]></NewStatus>"
                    "</u:%sResponse>";

    if(config_session_id[0] == '\0') {
        SoapError(h, 402, "No ongoing configuration session");
        return;
    }
    syslog(LOG_NOTICE, "Finished configuration session for id %s", config_session_id);

    config_session_id[0] = '\0';

    if(apply() == true) {
        bodylen = snprintf(body, sizeof(body), resp,
                action, SERVICE_TYPE_DC, "ChangesApplied", action);
        BuildSendAndCloseSoapResp(h, body, bodylen);
    } else {
        SoapError(h, 501, "Could not apply changes");
    }
}
