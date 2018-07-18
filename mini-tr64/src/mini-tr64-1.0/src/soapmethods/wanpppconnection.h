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

#ifndef SOAPMETHODS_WANPPPCONNECTION_H_INCLUDED
#define SOAPMETHODS_WANPPPCONNECTION_H_INCLUDED

#include "../upnphttp.h"

void WANPPPSetUserName(struct upnphttp * h, const char * action);
void WANPPPSetPassword(struct upnphttp * h, const char * action);
void WANPPPGetUserName(struct upnphttp * h, const char * action);
void WANPPPGetStatusInfo(struct upnphttp * h, const char * action);
void WANPPPGetInfo(struct upnphttp * h, const char * action);
void WANPPPGetConnectionTypeInfo(struct upnphttp * h, const char * action);
void WANPPPGetExternalIPAddress(struct upnphttp * h, const char * action);
void WANPPPRequestConnection(struct upnphttp * h, const char * action);
void WANPPPForceTermination(struct upnphttp * h, const char * action);

#endif

