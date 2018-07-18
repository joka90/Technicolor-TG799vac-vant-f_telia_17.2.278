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

#ifndef SOAPMETHODS_WLANCONFIGURATION_H_INCLUDED
#define SOAPMETHODS_WLANCONFIGURATION_H_INCLUDED

#include "../upnphttp.h"

void WLANCGetInfo(struct upnphttp * h, const char * action);
void WLANCGetSSID(struct upnphttp * h, const char * action);
void WLANCSetSSID(struct upnphttp * h, const char * action);
void WLANCGetRadioMode(struct upnphttp * h, const char * action);
void WLANCSetRadioMode(struct upnphttp * h, const char * action);
void WLANCGetBeaconAdvertisement(struct upnphttp * h, const char * action);
void WLANCGetChannelInfo(struct upnphttp * h, const char * action);
void WLANCGetSecurityKeys(struct upnphttp * h, const char * action);
void WLANCGetBasBeaconSecurityProperties(struct upnphttp * h, const char * action);
void WLANCGetWPABeaconSecurityProperties(struct upnphttp * h, const char * action);
void WLANCGet11iBeaconSecurityProperties(struct upnphttp * h, const char * action);
void WLANCSetEnable(struct upnphttp * h, const char * action);
void WLANCSetBeaconAdvertisement(struct upnphttp * h, const char * action);
void WLANCSetChannel(struct upnphttp * h, const char * action);
void WLANCSetSecurityKeys(struct upnphttp * h, const char * action);
void WLANCSetBeaconType(struct upnphttp * h, const char * action);
void WLANCGetBeaconType(struct upnphttp * h, const char * action);

#endif
