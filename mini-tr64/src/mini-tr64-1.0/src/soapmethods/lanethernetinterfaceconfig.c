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
#include "lanethernetinterfaceconfig.h"
#include "soaphelper.h"
#include "../upnpurns.h"
#include "lanethernetinterfaceconfigdesc.h"
#include "../upnpsoap.h"
#include "../minitr064dpath.h"

void LEICGetInfo(struct upnphttp * h, const char * action)
{
    int laninstance = 0;
    int leicinstance = 0;
    char* querytemplate = "InternetGatewayDevice.LANDevice.%d.LANEthernetInterfaceConfig.%d.";
    char querystring[64];
    struct query_item query[] = {
            { .name = querystring },
            { .name = 0 }
    };

    sscanf(h->req_buf, LEIC_PATTERN, &laninstance, &leicinstance);

    if(laninstance <= 0 || leicinstance <= 0) {
        SoapError(h, 402, "Invalid control URL");
        syslog(LOG_WARNING, "Invalid control URL");
        return;
    }

    snprintf(querystring, sizeof(querystring), querytemplate, laninstance, leicinstance);
    genericGetter(h, SERVICE_TYPE_LEIC, action, query, LEICVars, LEICGetInfoArgs);
}
