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
#include "../upnpurns.h"
#include "../upnpsoap.h"
#include "soaphelper.h"
#include "wanpppconnection.h"
#include "wanpppconnectiondesc.h"
#include "../minitr064dpath.h"

#define BASEPPPTEMPLATE "InternetGatewayDevice.WANDevice.%d.WANConnectionDevice.%d.WANPPPConnection.%d."
#define SETPPPTEMPLATE BASEPPPTEMPLATE "%%s"
#define PPPUSERTEMPLATE BASEPPPTEMPLATE "Username"

static int getInstances(const char* req_buf, int* waninst, int* wancinst, int* wanpppinst) {
    return sscanf(req_buf, WANPPP_PATTERN, waninst, wancinst, wanpppinst);
}

static void WANPPPSet(struct upnphttp * h, const char * action, const struct argument args[])
{
    char* querytemplate = SETPPPTEMPLATE;
    char querystring[128];
    int waninst;
    int wancinst;
    int wanpppinst;

    if(getInstances(h->req_buf, &waninst, &wancinst, &wanpppinst) != 3) {
        SoapError(h, 501, "Invalid ctl path");
        return;
    }
    snprintf(querystring, sizeof(querystring), querytemplate, waninst, wancinst, wanpppinst);
    genericSetter(h, SERVICE_TYPE_WANPPP, action, querystring, WANPPPVars, args);
}


void WANPPPSetUserName(struct upnphttp * h, const char * action)
{
    WANPPPSet(h, action, WANPPPSetUserNameArgs);
}

void WANPPPSetPassword(struct upnphttp * h, const char * action)
{
    WANPPPSet(h, action, WANPPPSetPasswordArgs);
}

void WANPPPGetUserName(struct upnphttp * h, const char * action) {
    char* querytemplate = PPPUSERTEMPLATE;
    char querystring[128];
    int waninst;
    int wancinst;
    int wanpppinst;
    struct query_item query[] = {
            { .name = querystring },
            { .name = 0 }
    };

    if(getInstances(h->req_buf, &waninst, &wancinst, &wanpppinst) != 3) {
        SoapError(h, 501, "Invalid ctl path");
        return;
    }
    snprintf(querystring, sizeof(querystring), querytemplate, waninst, wancinst, wanpppinst);

    genericGetter(h, SERVICE_TYPE_WANPPP, action, query, WANPPPVars, WANPPPGetUserNameArgs);
}

#define CSPPPTEMPLATE BASEPPPTEMPLATE "ConnectionStatus"
#define LCEPPPTEMPLATE BASEPPPTEMPLATE "LastConnectionError"
#define UTPPPTEMPLATE BASEPPPTEMPLATE "Uptime"

void WANPPPGetStatusInfo(struct upnphttp * h, const char * action) {
    char csquerystring[128];
    char lcequerystring[128];
    char utquerystring[128];
    int waninst;
    int wancinst;
    int wanpppinst;
    struct query_item query[] = {
            { .name = csquerystring },
            { .name = lcequerystring },
            { .name = utquerystring },
            { .name = 0 }
    };

    if(getInstances(h->req_buf, &waninst, &wancinst, &wanpppinst) != 3) {
        SoapError(h, 501, "Invalid ctl path");
        return;
    }
    snprintf(csquerystring, sizeof(csquerystring), CSPPPTEMPLATE, waninst, wancinst, wanpppinst);
    snprintf(lcequerystring, sizeof(lcequerystring), LCEPPPTEMPLATE, waninst, wancinst, wanpppinst);
    snprintf(utquerystring, sizeof(utquerystring), UTPPPTEMPLATE, waninst, wancinst, wanpppinst);

    genericGetter(h, SERVICE_TYPE_WANPPP, action, query, WANPPPVars, WANPPPGetStatusInfoArgs);
}

void WANPPPGetInfo(struct upnphttp * h, const char * action) {
    char* querytemplate = BASEPPPTEMPLATE;
    char querystring[128];
    int waninst;
    int wancinst;
    int wanpppinst;
    struct query_item query[] = {
            { .name = querystring },
            { .name = 0 }
    };

    if(getInstances(h->req_buf, &waninst, &wancinst, &wanpppinst) != 3) {
        SoapError(h, 501, "Invalid ctl path");
        return;
    }
    snprintf(querystring, sizeof(querystring), querytemplate, waninst, wancinst, wanpppinst);

    genericGetter(h, SERVICE_TYPE_WANPPP, action, query, WANPPPVars, WANPPPGetInfoArgs);
}

#define CTPPPTEMPLATE BASEPPPTEMPLATE "ConnectionType"
#define PCTPPPTEMPLATE BASEPPPTEMPLATE "PossibleConnectionTypes"

void WANPPPGetConnectionTypeInfo(struct upnphttp * h, const char * action) {
    char ctquerystring[128];
    char pctquerystring[128];
    int waninst;
    int wancinst;
    int wanpppinst;
    struct query_item query[] = {
            { .name = ctquerystring },
            { .name = pctquerystring },
            { .name = 0 }
    };

    if(getInstances(h->req_buf, &waninst, &wancinst, &wanpppinst) != 3) {
        SoapError(h, 501, "Invalid ctl path");
        return;
    }
    snprintf(ctquerystring, sizeof(ctquerystring), CTPPPTEMPLATE, waninst, wancinst, wanpppinst);
    snprintf(pctquerystring, sizeof(pctquerystring), PCTPPPTEMPLATE, waninst, wancinst, wanpppinst);

    genericGetter(h, SERVICE_TYPE_WANPPP, action, query, WANPPPVars, WANPPPGetConnectionTypeInfoArgs);
}

#define EIPPPPTEMPLATE BASEPPPTEMPLATE "ExternalIPAddress"

void WANPPPGetExternalIPAddress(struct upnphttp * h, const char * action) {
    char* querytemplate = EIPPPPTEMPLATE;
    char querystring[128];
    int waninst;
    int wancinst;
    int wanpppinst;
    struct query_item query[] = {
            { .name = querystring },
            { .name = 0 }
    };

    if(getInstances(h->req_buf, &waninst, &wancinst, &wanpppinst) != 3) {
        SoapError(h, 501, "Invalid ctl path");
        return;
    }
    snprintf(querystring, sizeof(querystring), querytemplate, waninst, wancinst, wanpppinst);

    genericGetter(h, SERVICE_TYPE_WANPPP, action, query, WANPPPVars, WANPPPGetExternalIPAddressArgs);
}

#define NAMEPPPTEMPLATE BASEPPPTEMPLATE "Name"


static void getNameCb(const char *path, const char *param, const char *value, void *cookie) {
    struct cookiestruct *ctx = cookie;
    size_t valuelen = 0;
    char *entryval;

    if (!ctx) {
        syslog(LOG_ERR, "NULL context data, aborting");
        return;
    }

    if(ctx->entry >= 1) { // Want to keep a last one set at 0;
        syslog(LOG_ERR, "Too many entries, cannot store %s", param);
        return;
    }

    if(!strcmp(param, "Name")) {
        valuelen = strlen(value);
        entryval = malloc(valuelen+1);
        if(!entryval) {
            syslog(LOG_ERR, "Could not allocate buffer to store value for %s (len=%d)", param, (int) valuelen);
            return;
        }

        strncpy(entryval, value, valuelen);
        entryval[valuelen] = 0; // Trailing 0 to be sure
        ctx->entries[ctx->entry].name = "Name";
        ctx->entries[ctx->entry].value = entryval; // This one must be freed at the end
        ctx->entry++;
        return;
    }
}
/*
    To bring the connection up / down, we'll use ifup, ifdown <name>
    To do so, we'll retrieve the name of the connection using the IGD Name parameter
    This becomes invalid as soon as the Name is changed by the ACS but today I don't have any better way to do it
    and changing the name is very rare.
 */

void IfUpDownOnIntf(struct upnphttp * h, const char * action, const char * command) {
    char* querytemplate = NAMEPPPTEMPLATE;
    char querystring[128];
    int waninst;
    int wancinst;
    int wanpppinst;
    struct respentry entries[2] = { { 0 } };
    char cl[256];
    int bodylen;
    char body[512];
    struct query_item query[] = {
            { .name = querystring },
            { .name = 0 }
    };
    struct cookiestruct cookie = {
            .h = h,
            .action = action,
            .entries = entries,
            .args = 0,
            .varlist = 0,
            .entry = 0,
            .urn = 0,
    };
    static const char resp[] =
            "<u:%sResponse "
                    "xmlns:u=\"%s\">"
                    "</u:%sResponse>";


    if(getInstances(h->req_buf, &waninst, &wancinst, &wanpppinst) != 3) {
        SoapError(h, 501, "Invalid ctl path");
        return;
    }
    snprintf(querystring, sizeof(querystring), querytemplate, waninst, wancinst, wanpppinst);

    getpv(query, &getNameCb, &GenericErrorCb, &cookie);

    if(cookie.entry != 1) {
        SoapError(h, 501, "Invalid ctl path");
        return;
    }

    snprintf(cl, sizeof(cl), "%s %s", command, entries[0].value);
    cl[sizeof(cl)-1] = 0;

    // Call command - will wait until it returns
    system(cl);

    free(entries[0].value); // Only one value was allocated

    bodylen = snprintf(body, sizeof(body), resp,
            action, SERVICE_TYPE_WANPPP, action);
    BuildSendAndCloseSoapResp(h, body, bodylen);

    return;
}
void WANPPPRequestConnection(struct upnphttp * h, const char * action) {
    IfUpDownOnIntf(h, action, "ifup");
}
void WANPPPForceTermination(struct upnphttp * h, const char * action) {
    IfUpDownOnIntf(h, action, "ifdown");
}
