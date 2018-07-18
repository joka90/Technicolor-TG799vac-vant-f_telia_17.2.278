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
#include <sys/syslog.h>
#include "wandsllinkconfig.h"
#include "soaphelper.h"
#include "../upnpurns.h"
#include "../upnpsoap.h"
#include "wandslinterfaceconfigdesc.h"
#include "wandsllinkconfigdesc.h"
#include "../minitr064dpath.h"

void WDLCGetInfo(struct upnphttp * h, const char * action)
{
    int waninstance = 0;
    int wancinstance = 0;
    char* querytemplate = "InternetGatewayDevice.WANDevice.%d.WANConnectionDevice.%d.WANDSLLinkConfig.";
    char querystring[128];
    struct query_item query[] = {
            { .name = querystring },
            { .name = 0 }
    };

    sscanf(h->req_buf, WDLC_PATTERN, &waninstance, &wancinstance);

    if(waninstance <= 0 || wancinstance <=0) {
        SoapError(h, 402, "Invalid control URL");
        syslog(LOG_WARNING, "Invalid control URL");
        return;
    }

    snprintf(querystring, sizeof(querystring), querytemplate, waninstance, wancinstance);
    genericGetter(h, SERVICE_TYPE_WDLC, action, query, WDLCVars, WDLCGetInfoArgs);
}

void WDLCGetDSLLinkInfo(struct upnphttp * h, const char * action) {
    int waninstance = 0;
    int wancinstance = 0;
    char* querytemplate = "InternetGatewayDevice.WANDevice.%d.WANConnectionDevice.%d.WANDSLLinkConfig.LinkType";
    char querystring[128];
    struct query_item query[] = {
            { .name = querystring },
            { .name = 0 }
    };

    sscanf(h->req_buf, WDLC_PATTERN, &waninstance, &wancinstance);

    if(waninstance <= 0 || wancinstance <=0) {
        SoapError(h, 402, "Invalid control URL");
        syslog(LOG_WARNING, "Invalid control URL");
        return;
    }

    snprintf(querystring, sizeof(querystring), querytemplate, waninstance, wancinstance);
    genericGetter(h, SERVICE_TYPE_WDLC, action, query, WDLCVars, WDLCGetDSLLinkInfoArgs);
}
void WDLCGetDestinationAddress(struct upnphttp * h, const char * action) {
    int waninstance = 0;
    int wancinstance = 0;
    char* querytemplate = "InternetGatewayDevice.WANDevice.%d.WANConnectionDevice.%d.WANDSLLinkConfig.DestinationAddress";
    char querystring[128];
    struct query_item query[] = {
            { .name = querystring },
            { .name = 0 }
    };

    sscanf(h->req_buf, WDLC_PATTERN, &waninstance, &wancinstance);

    if(waninstance <= 0 || wancinstance <=0) {
        SoapError(h, 402, "Invalid control URL");
        syslog(LOG_WARNING, "Invalid control URL");
        return;
    }

    snprintf(querystring, sizeof(querystring), querytemplate, waninstance, wancinstance);
    genericGetter(h, SERVICE_TYPE_WDLC, action, query, WDLCVars, WDLCGetDestinationAddressArgs);
}
void WDLCGetATMEncapsulation(struct upnphttp * h, const char * action) {
    int waninstance = 0;
    int wancinstance = 0;
    char* querytemplate = "InternetGatewayDevice.WANDevice.%d.WANConnectionDevice.%d.WANDSLLinkConfig.ATMEncapsulation";
    char querystring[128];
    struct query_item query[] = {
            { .name = querystring },
            { .name = 0 }
    };

    sscanf(h->req_buf, WDLC_PATTERN, &waninstance, &wancinstance);

    if(waninstance <= 0 || wancinstance <=0) {
        SoapError(h, 402, "Invalid control URL");
        syslog(LOG_WARNING, "Invalid control URL");
        return;
    }

    snprintf(querystring, sizeof(querystring), querytemplate, waninstance, wancinstance);
    genericGetter(h, SERVICE_TYPE_WDLC, action, query, WDLCVars, WDLCGetATMEncapsulationArgs);
}
void WDLCSetDestinationAddress(struct upnphttp * h, const char * action) {
    char* querytemplate = "InternetGatewayDevice.WANDevice.%d.WANConnectionDevice.%d.WANDSLLinkConfig.DestinationAddress";
    char querystring[128];
    int waninstance = 0;
    int wancinstance = 0;

    if(sscanf(h->req_buf, WDLC_PATTERN, &waninstance, &wancinstance) != 2) {
        SoapError(h, 501, "Invalid ctl path");
        return;
    }
    snprintf(querystring, sizeof(querystring), querytemplate, waninstance, wancinstance);
    genericSetter(h, SERVICE_TYPE_WDLC, action, querystring, WDLCVars, WDLCSetDestinationAddressArgs);
}
