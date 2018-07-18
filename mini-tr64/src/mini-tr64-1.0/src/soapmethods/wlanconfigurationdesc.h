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

#ifndef WLANCONFIGURATIONDESC_H_INCLUDED
#define WLANCONFIGURATIONDESC_H_INCLUDED

#include "../upnpdescgen.h"

/* Read TR064 spec - WLANConfiguration */
static const struct argument WLANCGetInfoArgs[] =
        {
                {2, 0, 0}, /* out */
                {2, 1, 0}, /* out */
                {2, 2, 0}, /* out */
                {2, 3, 0}, /* out */
                {2, 4, 0}, /* out */
                {2, 5, 0}, /* out */
                {2, 13, 0}, /* out */
                {2, 14, 0}, /* out */
                {2, 15, 0}, /* out */
                {2, 20, 0}, /* out */
                {2, 21, 0}, /* out */
                {0, 0, 0},
        };

static const struct argument WLANCSetEnableArgs[] =
        {
                {1, 0, 0}, /* in */
                {0, 0, 0},
        };


static const struct argument WLANCSetConfigArgs[] =
        {
                {1, 2, 0}, /* in */
                {1, 3, 0}, /* in */
                {1, 4, 0}, /* in */
                {1, 5, 0}, /* in */
                {1, 13, 0}, /* in */
                {1, 20, 0}, /* in */
                {1, 21, 0}, /* in */
                {0, 0, 0},
        };

static const struct argument WLANCSetSSIDArgs[] =
        {
                {1, 4, 0},
                {0, 0, 0},
        };

static const struct argument WLANCGetSSIDArgs[] =
        {
                {2, 4, 0},
                {0, 0, 0},
        };
static const struct argument WLANCSetRadioModeArgs[] =
        {
                {1, 37, 0},
                {0, 0, 0},
        };

static const struct argument WLANCGetRadioModeArgs[] =
        {
                {2, 37, 0},
                {0, 0, 0},
        };

static const struct argument WLANCGetBeaconAdvertisementArgs[] =
        {
                {2, 39, 0},
                {0, 0, 0},
        };

static const struct argument WLANCGetChannelInfoArgs[] =
        {
                {2, 3, 0},
                {2, 49, 0},
                {0, 0, 0},
        };
static const struct argument WLANCGetSecurityKeysArgs[] =
        {
                {2, 7, "WEPKey0"},
                {2, 7, "WEPKey1"},
                {2, 7, "WEPKey2"},
                {2, 7, "WEPKey3"},
                {2, 11, 0},
                {2, 12, 0},
                {0, 0, 0},
        };
static const struct argument WLANCGetBasBeaconSecurityPropertiesArgs[] =
        {
                {2, 20, 0},
                {2, 21, 0},
                {0, 0, 0},
        };
static const struct argument WLANCGetWPABeaconSecurityPropertiesArgs[] =
        {
                {2, 22, 0},
                {2, 23, 0},
                {0, 0, 0},
        };
static const struct argument WLANCGet11iBeaconSecurityPropertiesArgs[] =
        {
                {2, 28, 0},
                {2, 29, 0},
                {0, 0, 0},
        };
static const struct argument WLANCSetBeaconAdvertisementArgs[] =
        {
                {1, 39, 0},
                {0, 0, 0},
        };
static const struct argument WLANCSetChannelArgs[] =
        {
                {1, 3, 0},
                {0, 0, 0},
        };
static const struct argument WLANCSetSecurityKeysArgs[] =
        {
                {1, 7, "WEPKey0"},
                {1, 7, "WEPKey1"},
                {1, 7, "WEPKey2"},
                {1, 7, "WEPKey3"},
                {1, 11, 0},
                {1, 12, 0},
                {0, 0, 0},
        };
static const struct argument WLANCSetBeaconTypeArgs[] =
        {
                {1, 5, 0},
                {0, 0, 0},
        };
static const struct argument WLANCGetBeaconTypeArgs[] =
        {
                {2, 5, 0},
                {0, 0, 0},
        };


static const struct action WLANCActions[] =
        {
                {"SetRadioMode", WLANCSetRadioModeArgs}, /* Req  */
                {"GetRadioMode", WLANCGetRadioModeArgs}, /* Req  */
                {"SetSSID", WLANCSetSSIDArgs}, /* Req  */
                {"GetSSID", WLANCGetSSIDArgs}, /* Req  */
                {"SetEnable", WLANCSetEnableArgs}, /* Req  */
                {"GetInfo", WLANCGetInfoArgs}, /* Req  */
                {"SetConfig", WLANCSetConfigArgs}, /* Req  */
                {"GetBeaconAdvertisement", WLANCGetBeaconAdvertisementArgs},
                {"GetChannelInfo", WLANCGetChannelInfoArgs},
                {"GetSecurityKeys", WLANCGetSecurityKeysArgs},
                {"GetBasBeaconSecurityProperties", WLANCGetBasBeaconSecurityPropertiesArgs},
                {"GetWPABeaconSecurityProperties", WLANCGetWPABeaconSecurityPropertiesArgs},
                {"Get11iBeaconSecurityProperties", WLANCGet11iBeaconSecurityPropertiesArgs},
                {"SetBeaconAdvertisement", WLANCSetBeaconAdvertisementArgs},
                {"SetChannel", WLANCSetChannelArgs},
                {"SetSecurityKeys", WLANCSetSecurityKeysArgs},
                {"SetBeaconType", WLANCSetBeaconTypeArgs},
                {"GetBeaconType", WLANCGetBeaconTypeArgs},
                {0, 0}
        };

static const struct stateVar WLANCVars[] =
        {
/* 0 */         {"Enable", 1, 0, 0, 0}, /* Required */
                {"Status", 0, 0, 0, 0}, /* Required */  // ENUM
                {"MaxBitRate", 0, 0, 0, 0}, /* Required */
                {"Channel", 6, 0, 0, 0}, /* Required */
                {"SSID", 0, 0, 0, 0}, /* Required */
/* 5 */         {"BeaconType", 0, 0, 0, 0}, /* Required */
                {"WEPKeyIndex", 6, 0, 0, 0}, /* Required */
                {"WEPKey", 0, 0, 0, 0}, /* Required */
                {"WEPEncryptionLevel", 0, 0, 0, 0}, /* Optional */
                {"PreSharedKeyIndex", 6, 0, 0, 0}, /* Required */
/* 10 */        {"AssociatedDeviceMACAddress", 0, 0, 0, 0}, /* Required */
                {"PreSharedKey", 0, 0, 0, 0}, /* Required */
                {"KeyPassphrase", 0, 0, 0, 0}, /* Required */
                {"MACAddressControlEnabled", 1, 0, 0, 0}, /* Required */
                {"Standard", 0, 0, 0, 0}, /* Required */ // ENUM
/* 15 */        {"BSSID", 0, 0, 0, 0}, /* Required */
                {"TotalBytesSent", 3, 0, 0, 0}, /* Required */
                {"TotalBytesReceived", 3, 0, 0, 0}, /* Required */
                {"TotalPacketsSent", 3, 0, 0, 0}, /* Optional */
                {"TotalPacketsReceived", 3, 0, 0, 0}, /* Required */
/* 20 */        {"BasicEncryptionModes", 0, 0, 0, 0}, /* Required */
                {"BasicAuthenticationMode", 0, 0, 0, 0}, /* Required */
                {"WPAEncryptionModes", 0, 0, 0, 0}, /* Required */
                {"WPAAuthenticationMode", 0, 0, 0, 0}, /* Required */
                {"PossibleChannels", 0, 0, 0, 0}, /* Required */
/* 25 */        {"BasicDataTransmitRates", 0, 0, 0, 0}, /* Required */
                {"OperationalDataTransmitRates", 0, 0, 0, 0}, /* Required */
                {"PossibleDataTransmitRates", 0, 0, 0, 0}, /* Required */
                {"IEEE11iEncryptionModes", 0, 0, 0, 0}, /* O */
                {"IEEE11iAuthenticationMode", 0, 0, 0, 0}, /* O */
/* 30 */        {"TotalAssociations", 2, 0, 0, 0}, /* R */
                {"AssociatedDeviceMACAddress", 0, 0, 0, 0}, /* R */
                {"AssociatedDeviceIPAddress", 0, 0, 0, 0}, /* R */
                {"AssociatedDeviceAuthenticationState", 1, 0, 0, 0}, /* R */
                {"LastRequestedUnicastCipher", 0, 0, 0, 0}, /* O */
/* 35 */        {"LastRequestedMulticastCipher", 0, 0, 0, 0}, /* O */
                {"LastPMKId", 0, 0, 0, 0}, /* O */
                {"RadioEnabled", 1, 0, 0, 0}, /* R */
                {"InsecureOOBAccessEnabled", 1, 0, 0, 0}, /* O */
                {"BeaconAdvertisementEnabled", 1, 0, 0, 0}, /* O */
/* 40 */        {"LocationDescription", 0, 0, 0, 0}, /* O */
                {"RegulatoryDomain", 0, 0, 0, 0}, /* O */
                {"TotalPSKFailures", 3, 0, 0, 0}, /* O */
                {"TotalIntegrityFailures", 3, 0, 0, 0}, /* O */
                {"ChannelsInUse", 0, 0, 0, 0}, /* O */
/* 45 */        {"DeviceOperationMode", 0, 0, 0, 0}, /* O */
                {"DistanceFromRoot", 6, 0, 0, 0}, /* O */
                {"PeerBSSID", 0, 0, 0, 0}, /* O */
/* 48 */        {"AuthenticationServiceMode", 0, 0, 0, 0}, /* O */
                {"PossibleChannels", 0, 0, 0, 0},
                {0, 0, 0, 0, 0}
        };

static const struct serviceDesc scpdWLANC =
        { WLANCActions, WLANCVars };

#endif
