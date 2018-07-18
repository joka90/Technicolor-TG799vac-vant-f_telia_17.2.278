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
#include "wanipconnection.h"
#include "wanipconnectiondesc.h"
#include "../minitr064dpath.h"

#define BASEIPTEMPLATE "InternetGatewayDevice.WANDevice.%d.WANConnectionDevice.%d.WANIPConnection.%d."

static int getInstances(const char* req_buf, int* waninst, int* wancinst, int* wanipinst) {
    return sscanf(req_buf, WANIP_PATTERN, waninst, wancinst, wanipinst);
}

#define CSIPTEMPLATE BASEIPTEMPLATE "ConnectionStatus"
#define LCEIPTEMPLATE BASEIPTEMPLATE "LastConnectionError"
#define UTIPTEMPLATE BASEIPTEMPLATE "Uptime"

void WANIPGetStatusInfo(struct upnphttp * h, const char * action) {
    char csquerystring[128];
    char lcequerystring[128];
    char utquerystring[128];
    int waninst;
    int wancinst;
    int wanipinst;
    struct query_item query[] = {
            { .name = csquerystring },
            { .name = lcequerystring },
            { .name = utquerystring },
            { .name = 0 }
    };

    if(getInstances(h->req_buf, &waninst, &wancinst, &wanipinst) != 3) {
        SoapError(h, 501, "Invalid ctl path");
        return;
    }
    snprintf(csquerystring, sizeof(csquerystring), CSIPTEMPLATE, waninst, wancinst, wanipinst);
    snprintf(lcequerystring, sizeof(lcequerystring), LCEIPTEMPLATE, waninst, wancinst, wanipinst);
    snprintf(utquerystring, sizeof(utquerystring), UTIPTEMPLATE, waninst, wancinst, wanipinst);

    genericGetter(h, SERVICE_TYPE_WANIP, action, query, WANIPVars, WANIPGetStatusInfoArgs);
}

void WANIPGetInfo(struct upnphttp * h, const char * action) {
    char* querytemplate = BASEIPTEMPLATE;
    char querystring[128];
    int waninst;
    int wancinst;
    int wanipinst;
    struct query_item query[] = {
            { .name = querystring },
            { .name = 0 }
    };

    if(getInstances(h->req_buf, &waninst, &wancinst, &wanipinst) != 3) {
        SoapError(h, 501, "Invalid ctl path");
        return;
    }
    snprintf(querystring, sizeof(querystring), querytemplate, waninst, wancinst, wanipinst);

    genericGetter(h, SERVICE_TYPE_WANIP, action, query, WANIPVars, WANIPGetInfoArgs);
}

#define CTIPTEMPLATE BASEIPTEMPLATE "ConnectionType"
#define PCTIPTEMPLATE BASEIPTEMPLATE "PossibleConnectionTypes"

void WANIPGetConnectionTypeInfo(struct upnphttp * h, const char * action) {
    char ctquerystring[128];
    char pctquerystring[128];
    int waninst;
    int wancinst;
    int wanipinst;
    struct query_item query[] = {
            { .name = ctquerystring },
            { .name = pctquerystring },
            { .name = 0 }
    };

    if(getInstances(h->req_buf, &waninst, &wancinst, &wanipinst) != 3) {
        SoapError(h, 501, "Invalid ctl path");
        return;
    }
    snprintf(ctquerystring, sizeof(ctquerystring), CTIPTEMPLATE, waninst, wancinst, wanipinst);
    snprintf(pctquerystring, sizeof(pctquerystring), PCTIPTEMPLATE, waninst, wancinst, wanipinst);

    genericGetter(h, SERVICE_TYPE_WANIP, action, query, WANIPVars, WANIPGetConnectionTypeInfoArgs);
}

#define EIIPPTEMPLATE BASEIPTEMPLATE "ExternalIPAddress"

void WANIPGetExternalIPAddress(struct upnphttp * h, const char * action) {
    char* querytemplate = EIIPPTEMPLATE;
    char querystring[128];
    int waninst;
    int wancinst;
    int wanipinst;
    struct query_item query[] = {
            { .name = querystring },
            { .name = 0 }
    };

    if(getInstances(h->req_buf, &waninst, &wancinst, &wanipinst) != 3) {
        SoapError(h, 501, "Invalid ctl path");
        return;
    }
    snprintf(querystring, sizeof(querystring), querytemplate, waninst, wancinst, wanipinst);

    genericGetter(h, SERVICE_TYPE_WANIP, action, query, WANIPVars, WANIPGetExternalIPAddressArgs);
}
