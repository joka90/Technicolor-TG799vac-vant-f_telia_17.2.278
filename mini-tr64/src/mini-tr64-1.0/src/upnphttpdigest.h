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

#ifndef UPNPHTTPDIGEST_H_INCLUDED
#define UPNPHTTPDIGEST_H_INCLUDED

#include "upnphttp.h"

void computeHA1(const char* username, const char* password, const char* realm, char *digest);
int buildWWWAuthenticateHeader(struct upnphttp* h);
void processAuthorizationHeader(struct upnphttp* h, char *header);
int checkAuthentication(const char *HA1, const char *nonce, const char *method,
        const char *uri, const char *response, const char *qop, const char *nc, const char *cnonce);

#endif