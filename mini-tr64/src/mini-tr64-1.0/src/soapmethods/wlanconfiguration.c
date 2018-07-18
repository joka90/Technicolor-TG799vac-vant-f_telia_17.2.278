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
#include "wlanconfiguration.h"
#include "soaphelper.h"
#include "../upnpurns.h"
#include "wlanconfigurationdesc.h"
#include "../upnpsoap.h"
#include "../minitr064dpath.h"

#define BASEWLANTEMPLATE "InternetGatewayDevice.LANDevice.%d.WLANConfiguration.%d."
#define SETWLANTEMPLATE BASEWLANTEMPLATE "%%s"
#define SSIDWLANTEMPLATE BASEWLANTEMPLATE "SSID"
#define RADIOMODEWLANTEMPLATE BASEWLANTEMPLATE "RadioEnabled"

static void WLANCSet(struct upnphttp * h, const char * action, const struct argument args[]) {
    int laninstance = 0;
    int wlancinstance = 0;
    char* querytemplate = SETWLANTEMPLATE;
    char querystring[128];

    sscanf(h->req_buf, WLANC_PATTERN, &laninstance, &wlancinstance);

    if(laninstance <= 0 || wlancinstance <= 0) {
        SoapError(h, 402, "Invalid control URL");
        syslog(LOG_WARNING, "Invalid control URL");
        return;
    }

    snprintf(querystring, sizeof(querystring), querytemplate, laninstance, wlancinstance);
    genericSetter(h, SERVICE_TYPE_WLAN, action, querystring, WLANCVars, args);
}


void WLANCGetInfo(struct upnphttp * h, const char * action)
{
    int laninstance = 0;
    int wlancinstance = 0;
    char* querytemplate = BASEWLANTEMPLATE;
    char querystring[128];
    struct query_item query[] = {
            { .name = querystring },
            { .name = 0 }
    };

    sscanf(h->req_buf, WLANC_PATTERN, &laninstance, &wlancinstance);

    if(laninstance <= 0 || wlancinstance <= 0) {
        SoapError(h, 402, "Invalid control URL");
        syslog(LOG_WARNING, "Invalid control URL");
        return;
    }

    snprintf(querystring, sizeof(querystring), querytemplate, laninstance, wlancinstance);
    genericGetter(h, SERVICE_TYPE_WLAN, action, query, WLANCVars, WLANCGetInfoArgs);
}

void WLANCGetSSID(struct upnphttp * h, const char * action)
{
    int laninstance = 0;
    int wlancinstance = 0;
    char* querytemplate = SSIDWLANTEMPLATE;
    char querystring[128];
    struct query_item query[] = {
            { .name = querystring },
            { .name = 0 }
    };

    sscanf(h->req_buf, WLANC_PATTERN, &laninstance, &wlancinstance);

    if(laninstance <= 0 || wlancinstance <= 0) {
        SoapError(h, 402, "Invalid control URL");
        syslog(LOG_WARNING, "Invalid control URL");
        return;
    }

    snprintf(querystring, sizeof(querystring), querytemplate, laninstance, wlancinstance);
    genericGetter(h, SERVICE_TYPE_WLAN, action, query, WLANCVars, WLANCGetSSIDArgs);
}

void WLANCSetSSID(struct upnphttp * h, const char * action)
{
    WLANCSet(h, action, WLANCSetSSIDArgs);
}

void WLANCGetRadioMode(struct upnphttp * h, const char * action)
{
    int laninstance = 0;
    int wlancinstance = 0;
    char* querytemplate = RADIOMODEWLANTEMPLATE;
    char querystring[128];
    struct query_item query[] = {
            { .name = querystring },
            { .name = 0 }
    };

    sscanf(h->req_buf, WLANC_PATTERN, &laninstance, &wlancinstance);

    if(laninstance <= 0 || wlancinstance <= 0) {
        SoapError(h, 402, "Invalid control URL");
        syslog(LOG_WARNING, "Invalid control URL");
        return;
    }

    snprintf(querystring, sizeof(querystring), querytemplate, laninstance, wlancinstance);
    genericGetter(h, SERVICE_TYPE_WLAN, action, query, WLANCVars, WLANCGetRadioModeArgs);
}

void WLANCSetRadioMode(struct upnphttp * h, const char * action)
{
    WLANCSet(h, action, WLANCSetRadioModeArgs);
}

void WLANCGetBeaconAdvertisement(struct upnphttp * h, const char * action) {
    int laninstance = 0;
    int wlancinstance = 0;
    char* querytemplate = BASEWLANTEMPLATE "BeaconAdvertisementEnabled";
    char querystring[128];
    struct query_item query[] = {
            { .name = querystring },
            { .name = 0 }
    };

    sscanf(h->req_buf, WLANC_PATTERN, &laninstance, &wlancinstance);

    if(laninstance <= 0 || wlancinstance <= 0) {
        SoapError(h, 402, "Invalid control URL");
        syslog(LOG_WARNING, "Invalid control URL");
        return;
    }

    snprintf(querystring, sizeof(querystring), querytemplate, laninstance, wlancinstance);
    genericGetter(h, SERVICE_TYPE_WLAN, action, query, WLANCVars, WLANCGetBeaconAdvertisementArgs);
}
void WLANCGetChannelInfo(struct upnphttp * h, const char * action) {
    int laninstance = 0;
    int wlancinstance = 0;
    char* querytemplate = BASEWLANTEMPLATE "Channel";
    char* querytemplate2 = BASEWLANTEMPLATE "PossibleChannels";
    char querystring[128];
    char querystring2[128];
    struct query_item query[] = {
            { .name = querystring },
            { .name = querystring2 },
            { .name = 0 }
    };

    sscanf(h->req_buf, WLANC_PATTERN, &laninstance, &wlancinstance);

    if(laninstance <= 0 || wlancinstance <= 0) {
        SoapError(h, 402, "Invalid control URL");
        syslog(LOG_WARNING, "Invalid control URL");
        return;
    }

    snprintf(querystring, sizeof(querystring), querytemplate, laninstance, wlancinstance);
    snprintf(querystring2, sizeof(querystring2), querytemplate2, laninstance, wlancinstance);
    genericGetter(h, SERVICE_TYPE_WLAN, action, query, WLANCVars, WLANCGetChannelInfoArgs);
}
static void getSecurityKeysCb(const char *path, const char *param, const char *value, void *cookie) {
    struct cookiestruct *ctx = cookie;

    if (!ctx) {
        syslog(LOG_ERR, "NULL context data, aborting");
        return;
    }

    if(ctx->entry > 5) {
        syslog(LOG_ERR, "Too many entries, cannot store %s", param);
        return;
    }

    ctx->entries[ctx->entry].name = strdup(param);
    ctx->entries[ctx->entry].value = strdup(value);
    ctx->entry++;
}
void WLANCGetSecurityKeys(struct upnphttp * h, const char * action) {
     int laninstance = 0;
     int wlancinstance = 0;
     int entry=0;
     char body[512] = "";
     int bodylen = 0;
     struct respentry entries[6] = { { 0 } };
     char* querytemplate  = BASEWLANTEMPLATE "WEPKey.";
     char* querytemplate2 = BASEWLANTEMPLATE "PreSharedKey.1.PreSharedKey";
     char* querytemplate3 = BASEWLANTEMPLATE "KeyPassphrase";
     char querystring[128], querystring2[128], querystring3[128];
     struct query_item query[] = {
             { .name = querystring },
             { .name = querystring2 },
             { .name = querystring3 },
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

     sscanf(h->req_buf, WLANC_PATTERN, &laninstance, &wlancinstance);

     if(laninstance <= 0 || wlancinstance <= 0) {
         SoapError(h, 402, "Invalid control URL");
         syslog(LOG_WARNING, "Invalid control URL");
         return;
     }

     snprintf(querystring, sizeof(querystring), querytemplate, laninstance, wlancinstance);
     snprintf(querystring2, sizeof(querystring2), querytemplate2, laninstance, wlancinstance);
     snprintf(querystring3, sizeof(querystring3), querytemplate3, laninstance, wlancinstance);

     if (h->ssl)
     {   // HTTPS
         getpv(query, &getSecurityKeysCb, &GenericErrorCb, &cookie);

         if (cookie.entry != 6)
         {
            SoapError(h, TR064_ERR_ACTION_FAILED, "Could not retrieve all parameters");
            syslog(LOG_ERR, "WLANCGetSecurityKeys - Could not retrieve all parameters (%d should be 5)", cookie.entry);
            goto gsk_cleanup;
         }
         bodylen = snprintf(body, sizeof(body), "<u:GetSecurityKeysResponse " "xmlns:u=\"" SERVICE_TYPE_WLAN "\">"
                                          "<NewWEPKey0><![CDATA[%s]]></NewWEPKey0>"
                                          "<NewWEPKey1><![CDATA[%s]]></NewWEPKey1>"
                                          "<NewWEPKey2><![CDATA[%s]]></NewWEPKey2>"
                                          "<NewWEPKey3><![CDATA[%s]]></NewWEPKey3>"
                                          "<NewPreSharedKey><![CDATA[%s]]></NewPreSharedKey>"
                                          "<NewKeyPassphrase><![CDATA[%s]]></NewKeyPassphrase>"
                                      "</u:GetSecurityKeysResponse>",
                                      cookie.entries[0].value,
                                      cookie.entries[1].value,
                                      cookie.entries[2].value,
                                      cookie.entries[3].value,
                                      cookie.entries[4].value,
                                      cookie.entries[5].value);
     }
     else
     {  // HTTP => Return empty strings
         bodylen = snprintf(body, sizeof(body), "<u:GetSecurityKeysResponse " "xmlns:u=\"" SERVICE_TYPE_WLAN "\">"
                                      "<NewWEPKey0></NewWEPKey0>"
                                      "<NewWEPKey1></NewWEPKey1>"
                                      "<NewWEPKey2></NewWEPKey2>"
                                      "<NewWEPKey3></NewWEPKey3>"
                                      "<NewPreSharedKey></NewPreSharedKey>"
                                      "<NewKeyPassphrase></NewKeyPassphrase>"
                                      "</u:GetSecurityKeysResponse>");
     }
     BuildSendAndCloseSoapResp(h, body, bodylen);
     gsk_cleanup:
         for (entry=0; entry<cookie.entry; entry++)
         {
             free(cookie.entries[entry].name);
             free(cookie.entries[entry].value);
         }
}
void WLANCGetBasBeaconSecurityProperties(struct upnphttp * h, const char * action) {
    int laninstance = 0;
    int wlancinstance = 0;
    char* querytemplate = BASEWLANTEMPLATE "BasicEncryptionModes";
    char* querytemplate2 = BASEWLANTEMPLATE "BasicAuthenticationMode";
    char querystring[128];
    char querystring2[128];
    struct query_item query[] = {
            { .name = querystring },
            { .name = querystring2 },
            { .name = 0 }
    };

    sscanf(h->req_buf, WLANC_PATTERN, &laninstance, &wlancinstance);

    if(laninstance <= 0 || wlancinstance <= 0) {
        SoapError(h, 402, "Invalid control URL");
        syslog(LOG_WARNING, "Invalid control URL");
        return;
    }

    snprintf(querystring, sizeof(querystring), querytemplate, laninstance, wlancinstance);
    snprintf(querystring2, sizeof(querystring2), querytemplate2, laninstance, wlancinstance);
    genericGetter(h, SERVICE_TYPE_WLAN, action, query, WLANCVars, WLANCGetBasBeaconSecurityPropertiesArgs);
}
void WLANCGetWPABeaconSecurityProperties(struct upnphttp * h, const char * action) {
    int laninstance = 0;
    int wlancinstance = 0;
    char* querytemplate = BASEWLANTEMPLATE "WPAEncryptionModes";
    char* querytemplate2 = BASEWLANTEMPLATE "WPAAuthenticationMode";
    char querystring[128];
    char querystring2[128];
    struct query_item query[] = {
            { .name = querystring },
            { .name = querystring2 },
            { .name = 0 }
    };

    sscanf(h->req_buf, WLANC_PATTERN, &laninstance, &wlancinstance);

    if(laninstance <= 0 || wlancinstance <= 0) {
        SoapError(h, 402, "Invalid control URL");
        syslog(LOG_WARNING, "Invalid control URL");
        return;
    }

    snprintf(querystring, sizeof(querystring), querytemplate, laninstance, wlancinstance);
    snprintf(querystring2, sizeof(querystring2), querytemplate2, laninstance, wlancinstance);
    genericGetter(h, SERVICE_TYPE_WLAN, action, query, WLANCVars, WLANCGetWPABeaconSecurityPropertiesArgs);
}
void WLANCGet11iBeaconSecurityProperties(struct upnphttp * h, const char * action) {
    int laninstance = 0;
    int wlancinstance = 0;
    char* querytemplate = BASEWLANTEMPLATE "IEEE11iEncryptionModes";
    char* querytemplate2 = BASEWLANTEMPLATE "IEEE11iAuthenticationMode";
    char querystring[128];
    char querystring2[128];
    struct query_item query[] = {
            { .name = querystring },
            { .name = querystring2 },
            { .name = 0 }
    };

    sscanf(h->req_buf, WLANC_PATTERN, &laninstance, &wlancinstance);

    if(laninstance <= 0 || wlancinstance <= 0) {
        SoapError(h, 402, "Invalid control URL");
        syslog(LOG_WARNING, "Invalid control URL");
        return;
    }

    snprintf(querystring, sizeof(querystring), querytemplate, laninstance, wlancinstance);
    snprintf(querystring2, sizeof(querystring2), querytemplate2, laninstance, wlancinstance);
    genericGetter(h, SERVICE_TYPE_WLAN, action, query, WLANCVars, WLANCGet11iBeaconSecurityPropertiesArgs);
}
void WLANCSetEnable(struct upnphttp * h, const char * action) {
    WLANCSet(h, action, WLANCSetEnableArgs);
}
void WLANCSetBeaconAdvertisement(struct upnphttp * h, const char * action) {
    WLANCSet(h, action, WLANCSetBeaconAdvertisementArgs);
}
void WLANCSetChannel(struct upnphttp * h, const char * action) {
    WLANCSet(h, action, WLANCSetChannelArgs);
}
void WLANCSetSecurityKeys(struct upnphttp * h, const char * action) {
    int laninstance = 0;
    int wlancinstance = 0;
    char *querytemplate = BASEWLANTEMPLATE "KeyPassphrase";
    char querystring[128];
    char *prefix = NULL, *postfix=NULL;
    char *startpos = NULL,  *endpos = NULL;
    size_t prefixlen;
    char save;
    struct query_item query[2] = { { 0 } };
    struct cookiestruct cookie = {
                .h = h,
                .action = action,
                .entries = 0,
                .args = 0,
                .varlist = 0,
                .entry = 0,
                .urn = SERVICE_TYPE_WLAN,
    };


    sscanf(h->req_buf, WLANC_PATTERN, &laninstance, &wlancinstance);

    if(laninstance <= 0 || wlancinstance <= 0) {
        SoapError(h, 402, "Invalid control URL");
        syslog(LOG_WARNING, "Invalid control URL");
        return;
    }

    snprintf(querystring, sizeof(querystring), querytemplate, laninstance, wlancinstance);
    query[0].name = querystring;

    prefix = "<NewKeyPassphrase>";
    postfix = "</NewKeyPassphrase>";
    prefixlen = strlen(prefix);

    startpos = strstr(h->req_buf, "<NewKeyPassphrase>");
    if(!startpos) {
        SoapError(h, 501, "Missing argument: NewKeyPassphrase");
        return;
    }
    endpos = strstr(startpos + prefixlen, postfix);
    if(!endpos) {
        SoapError(h, 501, "Missing argument: NewKeyPassphrase");
        return;
    }
    query[0].value = startpos + prefixlen;
    save = *endpos;
    *endpos=0;
    setpv(query, genericSetterCb, &genericSetterErrorCb, &cookie);
    *endpos = save;
}
void WLANCSetBeaconType(struct upnphttp * h, const char * action) {
    WLANCSet(h, action, WLANCSetBeaconTypeArgs);
}
void WLANCGetBeaconType(struct upnphttp * h, const char * action) {
    int laninstance = 0;
    int wlancinstance = 0;
    char* querytemplate = BASEWLANTEMPLATE "BeaconType";
    char querystring[128];
    struct query_item query[] = {
            { .name = querystring },
            { .name = 0 }
    };

    sscanf(h->req_buf, WLANC_PATTERN, &laninstance, &wlancinstance);

    if(laninstance <= 0 || wlancinstance <= 0) {
        SoapError(h, 402, "Invalid control URL");
        syslog(LOG_WARNING, "Invalid control URL");
        return;
    }

    snprintf(querystring, sizeof(querystring), querytemplate, laninstance, wlancinstance);
    genericGetter(h, SERVICE_TYPE_WLAN, action, query, WLANCVars, WLANCGetBeaconTypeArgs);
}

