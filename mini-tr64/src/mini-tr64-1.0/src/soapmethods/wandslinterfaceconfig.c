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
#include "wandslinterfaceconfig.h"
#include "soaphelper.h"
#include "../upnpurns.h"
#include "../upnpsoap.h"
#include "wandslinterfaceconfigdesc.h"
#include "../minitr064dpath.h"

void WDICGetInfo(struct upnphttp * h, const char * action)
{
    int waninstance = 0;
    char* querytemplate = "InternetGatewayDevice.WANDevice.%d.WANDSLInterfaceConfig.";
    char querystring[64];
    struct query_item query[] = {
            { .name = querystring },
            { .name = 0 }
    };

    sscanf(h->req_buf, WDIC_PATTERN, &waninstance);

    if(waninstance <= 0) {
        SoapError(h, 402, "Invalid control URL");
        syslog(LOG_WARNING, "Invalid control URL");
        return;
    }
    
    snprintf(querystring, sizeof(querystring), querytemplate, waninstance);
    genericGetter(h, SERVICE_TYPE_WDIC, action, query, WDICVars, WDICGetInfoArgs);
}
