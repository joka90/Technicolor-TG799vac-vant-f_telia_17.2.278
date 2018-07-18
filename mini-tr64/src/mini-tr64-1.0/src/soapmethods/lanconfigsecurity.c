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
#include <string.h>
#include <sys/syslog.h>
#include <openssl/md5.h>
#include "lanconfigsecurity.h"
#include "soaphelper.h"
#include "../upnpurns.h"
#include "lanconfigsecuritydesc.h"
#include "../upnpsoap.h"
#include "../upnpglobalvars.h"
#include "../upnphttpdigest.h"

#define MAX_PASSWORD_LEN (128)

void LCSSetConfigPassword(struct upnphttp * h, const char * action)
{
    struct query_item query[] = {
            { .name = "uci.minitr064d.password.dslfconfig", .value = 0 },
            { 0 }
    };
    struct cookiestruct cookie = {
            .h = h,
            .action = action,
            .entries = 0,
            .args = SetConfigPasswordArgs,
            .varlist = LCSVars,
            .entry = 0,
            .urn = SERVICE_TYPE_LCS,
    };
    char errormsg[512];
    char password[MAX_PASSWORD_LEN];
    char* startpos;
    char* endpos;
    size_t valuelen;

    // Extract password value
    startpos = strstr(h->req_buf, "<NewPassword>");
    if(!startpos) {
        snprintf(errormsg, sizeof(errormsg), "Missing parameter NewPassword");
        SoapError(h, 501, errormsg);
        return;
    }
    endpos = strstr(startpos + 13, "</NewPassword>");
    if(!endpos) {
        snprintf(errormsg, sizeof(errormsg), "Missing parameter NewPassword");
        SoapError(h, 501, errormsg);
        return;
    }
    if(endpos == startpos + 13) {
        snprintf(errormsg, sizeof(errormsg), "Cannot set an empty password");
        SoapError(h, 501, errormsg);
        return;
    }
    valuelen = endpos - startpos - 13;

    if(valuelen >= MAX_PASSWORD_LEN) {
        snprintf(errormsg, sizeof(errormsg), "Password too long");
        SoapError(h, 501, errormsg);
        return;
    }

    memcpy(password, startpos + 13, valuelen);
    password[valuelen] = '\0';

    computeHA1("dslf-config", password, "minitr064d", password_dsl_config);

    syslog(LOG_DEBUG, "Setting password_dslconfig to %s",password_dsl_config);
    query[0].value = password_dsl_config;

    setpv(query, &genericSetterCb, &genericSetterErrorCb, &cookie);
}
