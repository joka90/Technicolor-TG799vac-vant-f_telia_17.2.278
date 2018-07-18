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

#ifndef WANCOMMONINTERFACECONFIGDESC_H_INCLUDED
#define WANCOMMONINTERFACECONFIGDESC_H_INCLUDEDs

#include "../upnpdescgen.h"

/* Read TR064 spec - WANCommonInterfaceConfig */
static const struct argument SetEnabledForInternetArgs[] =
        {
                {1, 13, 0}, /* in */
                {0, 0, 0},
        };

static const struct argument GetEnabledForInternetArgs[] =
        {
                {2, 13, 0}, /* out */
                {0, 0, 0},
        };


static const struct argument GetCommonLinkPropertiesArgs[] =
        {
                {2, 0, 0}, /* out */
                {2, 1, 0}, /* out */
                {2, 2, 0}, /* out */
                {2, 3, 0}, /* out */
                {0, 0, 0},
        };

static const struct argument GetWANAccessProviderArgs[] =
        {
                {2, 4, 0}, /* out */
                {0, 0, 0},
        };
static const struct argument GetMaximumActiveConnectionsArgs[] =
        {
                {2, 5, 0}, /* out */
                {0, 0, 0},
        };
static const struct argument GetTotalBytesSentArgs[] =
        {
                {2, 9, 0}, /* out */
                {0, 0, 0},
        };
static const struct argument GetTotalBytesReceivedArgs[] =
        {
                {2, 10, 0}, /* out */
                {0, 0, 0},
        };
static const struct argument GetTotalPacketsSentArgs[] =
        {
                {2, 11, 0}, /* out */
                {0, 0, 0},
        };
static const struct argument GetTotalPacketsReceivedArgs[] =
        {
                {2, 12, 0}, /* out */
                {0, 0, 0},
        };

static const struct argument GetNumbberOfActiveConnectionsArgs[] =
        {
                {2, 6, 0}, /* out */
                {0, 0, 0},
        };

static const struct argument GetSpecificActiveConnectionArgs[] =
        {
                {1, 8, 0}, /* in */
                {2, 7, 0}, /* out */
                {0, 0, 0},
        };

static const struct argument GetActiveConnectionArgs[] =
        {
                {1, 6, 0}, /* in -- TODO: special name NewActiveConnectionIndex */
                {2, 7, 0}, /* out */
                {2, 8, 0}, /* out */
                {0, 0, 0},
        };

static const struct action WCICActions[] =
        {
                {"SetEnabledForInternet", SetEnabledForInternetArgs}, /* O/S  */
                {"GetEnabledForInternet", GetEnabledForInternetArgs}, /* O */
                {"GetCommonLinkProperties", GetCommonLinkPropertiesArgs}, /* Req  */
                {"GetWANAccessProvider", GetWANAccessProviderArgs}, /* O  */
                {"GetMaximumActiveConnections", GetMaximumActiveConnectionsArgs}, /* O  */
                {"GetTotalBytesSent", GetTotalBytesSentArgs}, /* Req  */
                {"GetTotalBytesReceived", GetTotalBytesReceivedArgs}, /* Req  */
                {"GetTotalPacketsSent", GetTotalPacketsSentArgs}, /* Req  */
                {"GetTotalPacketsReceived", GetTotalPacketsReceivedArgs}, /* Req  */
                {"GetNumbberOfActiveConnections", GetNumbberOfActiveConnectionsArgs}, /* O  */
                {"GetSpecificActiveConnection", GetSpecificActiveConnectionArgs}, /* O  */
                {"GetActiveConnection", GetActiveConnectionArgs}, /* O  */
                {0, 0}
        };

static const struct stateVar WCICVars[] =
        {
                {"WANAccessType", 0, 0, 0, 0}, /* Required */
                {"Layer1UpstreamMaxBitRate", 3, 0, 0, 0}, /* Required */
                {"Layer1DownstreamMaxBitRate", 3, 0, 0, 0}, /* Required */
                {"PhysicalLinkStatus", 0, 0, 0, 0}, /* Required */
                {"WANAccessProvider", 0, 0, 0, 0}, /* Optional */
                {"MaximumActiveConnections", 2, 0, 0, 0}, /* Optional */
                {"NumberOfActiveConnections", 2, 0, 0, 0}, /* Optional */
                {"ActiveConnectionDeviceContainer", 0, 0, 0, 0}, /* Optional */
                {"ActiveConnectionServiceID", 0, 0, 0, 0}, /* Optional */
                {"TotalBytesSent", 3, 0, 0, 0}, /* Required */
                {"TotalBytesReceived", 3, 0, 0, 0}, /* Required */
                {"TotalPacketsSent", 3, 0, 0, 0}, /* Required */
                {"TotalPacketsReceived", 3, 0, 0, 0}, /* Required */
                {"EnabledForInternet", 1, 0, 0, 0}, /* Optional */
                {0, 0, 0, 0, 0}
        };

static const struct serviceDesc scpdWCIC =
        { WCICActions, WCICVars };

#endif