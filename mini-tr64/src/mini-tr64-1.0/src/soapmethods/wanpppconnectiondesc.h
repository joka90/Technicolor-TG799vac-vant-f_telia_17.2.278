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

#ifndef WANPPPCONNECTIONDESC_H_INCLUDED
#define WANPPPCONNECTIONDESC_H_INCLUDED

#include "../upnpdescgen.h"

/* Read TR064 spec - WANDSLInterfaceConfig */
static const struct argument WANPPPSetRouteProtocolRxArgs[] =
        {
                {1, 41, 0},
                {0, 0, 0},
        };

static const struct argument WANPPPSetMaxMRUSizeArgs[] =
        {
                {1, 22, 0},
                {0, 0, 0},
        };
static const struct argument WANPPPSetMACAddressArgs[] =
        {
                {1, 33, 0},
                {0, 0, 0},
        };
static const struct argument WANPPPGetExternalIPAddressArgs[] =
        {
                {2, 20, 0},
                {0, 0, 0},
        };
static const struct argument WANPPPDeletePortMappingArgs[] =
        {
                {1, 27, 0},
                {1, 28, 0},
                {1, 30, 0},
                {0, 0, 0},
        };
static const struct argument WANPPPAddPortMappingArgs[] =
        {
                {1, 27, 0},
                {1, 28, 0},
                {1, 30, 0},
                {1, 29, 0},
                {1, 31, 0},
                {1, 25, 0},
                {1, 32, 0},
                {1, 26, 0},
                {0, 0, 0},
        };
static const struct argument WANPPPGetSpecificPortMappingEntryArgs[] =
        {
                {1, 27, 0},
                {1, 28, 0},
                {1, 30, 0},
                {2, 29, 0},
                {2, 31, 0},
                {2, 25, 0},
                {2, 32, 0},
                {2, 26, 0},
                {0, 0, 0},
        };
static const struct argument WANPPPGetGenericPortMappingEntryArgs[] =
        {
                {1, 24, 0},//todo: special name
                {2, 27, 0},
                {2, 28, 0},
                {2, 30, 0},
                {2, 29, 0},
                {2, 31, 0},
                {2, 25, 0},
                {2, 32, 0},
                {2, 26, 0},
                {0, 0, 0},
        };
static const struct argument WANPPPGetPortMappingNumberOfEntriesArgs[] =
        {
                {2, 24, 0},
                {0, 0, 0},
        };

static const struct argument WANPPPSetConnectionTriggerArgs[] =
        {
                {1, 12, 0},
                {0, 0, 0},
        };

static const struct argument WANPPPSetPPPoEServiceArgs[] =
        {
                {1, 39, 0},
                {1, 40, 0},
                {0, 0, 0},
        };

static const struct argument WANPPPGetNATRSIPStatusArgs[] =
        {
                {2, 13, 0},
                {2, 14, 0},
                {0, 0, 0},
        };
static const struct argument WANPPPGetWarnDisconnectDelayArgs[] =
        {
                {2, 11, 0},
                {0, 0, 0},
        };
static const struct argument WANPPPGetIdleDisconnectTimeArgs[] =
        {
                {2, 10, 0},
                {0, 0, 0},
        };
static const struct argument WANPPPGetAutoDisconnectTimeArgs[] =
        {
                {2, 9, 0},
                {0, 0, 0},
        };

static const struct argument WANPPPSetPasswordArgs[] =
        {
                {1, 16, 0},
                {0, 0, 0},
        };
static const struct argument WANPPPSetUserNameArgs[] =
        {
                {1, 15, 0},
                {0, 0, 0},
        };
static const struct argument WANPPPGetUserNameArgs[] =
        {
                {2, 15, 0},
                {0, 0, 0},
        };
static const struct argument WANPPPGetPPPAuthenticationProtocolArgs[] =
        {
                {2, 19, 0},
                {0, 0, 0},
        };
static const struct argument WANPPPGetPPPCompressionProtocolArgs[] =
        {
                {2, 18, 0},
                {0, 0, 0},
        };
static const struct argument WANPPPGetPPPEncryptionProtocolArgs[] =
        {
                {2, 17, 0},
                {0, 0, 0},
        };
static const struct argument WANPPPGetLinkLayerMaxBitRatesArgs[] =
        {
                {2, 6, 0},
                {2, 7, 0},
                {0, 0, 0},
        };
static const struct argument WANPPPGetStatusInfoArgs[] =
        {
                {2, 3, 0},
                {2, 8, 0},
                {2, 5, 0},
                {0, 0, 0},
        };
static const struct argument WANPPPSetWarnDisconnectDelayArgs[] =
        {
                {1, 11, 0},
                {0, 0, 0},
        };
static const struct argument WANPPPSetIdleDisconnectTimeArgs[] =
        {
                {1, 10, 0},
                {0, 0, 0},
        };
static const struct argument WANPPPSetAutoDisconnectTimeArgs[] =
        {
                {1, 9, 0},
                {0, 0, 0},
        };
static const struct argument WANPPPForceTerminationArgs[] =
        {
                {0, 0, 0},
        };
static const struct argument WANPPPRequestTerminationArgs[] =
        {
                {0, 0, 0},
        };
static const struct argument WANPPPRequestConnectionArgs[] =
        {
                {0, 0, 0},
        };
static const struct argument WANPPPGetConnectionTypeInfoArgs[] =
        {
                {2, 1, 0},
                {2, 2, 0},
                {0, 0, 0},
        };
static const struct argument WANPPPSetConnectionTypeArgs[] =
        {
                {1, 1, 0},
                {0, 0, 0},
        };

static const struct argument WANPPPGetInfoArgs[] =
        {
                {2, 0, 0}, /* out */
                {2, 1, 0}, /* out */
                {2, 2, 0}, /* out */
                {2, 3, 0}, /* out */
                {2, 4, 0}, /* out */
                {2, 5, 0}, /* out */
                {2, 6, 0}, /* out */
                {2, 7, 0}, /* out */
                {2, 8, 0}, /* out */
                {2, 9, 0}, /* out */
                {2, 10, 0}, /* out */
                {2, 11, 0}, /* out */
                {2, 12, 0}, /* out */
                {2, 13, 0}, /* out */
                {2, 14, 0}, /* out */
                {2, 15, 0}, /* out */
                {2, 16, 0}, /* out */
                {2, 17, 0}, /* out */
                {2, 18, 0}, /* out */
                {2, 19, 0}, /* out */
                {2, 20, 0}, /* out */
                {2, 21, 0}, /* out */
                {2, 31, 0}, /* out */
                {2, 21, 0}, /* out */
                {2, 33, 0}, /* out */
                {2, 34, 0}, /* out */
                {2, 22, 0}, /* out */
                {2, 23, 0}, /* out */
                {2, 35, 0}, /* out */
                {2, 36, 0}, /* out */
                {2, 37, 0}, /* out */
                {2, 38, 0}, /* out */
                {2, 39, 0}, /* out */
                {2, 40, 0}, /* out */
                {2, 41, 0}, /* out */
                {2, 42, 0}, /* out */
                {2, 43, 0}, /* out */
                {0, 0, 0},
        };

static const struct argument WANPPPSetEnableArgs[] =
        {
                {1, 0, 0}, /* in */
                {0, 0, 0},
        };

static const struct action WANPPPActions[] =
        {
                {"SetEnable", WANPPPSetEnableArgs }, /* R/S */
                {"GetInfo", WANPPPGetInfoArgs }, /* Req  */
                {"SetConnectionType", WANPPPSetConnectionTypeArgs }, /* R/S  */
                {"GetConnectionTypeInfo", WANPPPGetConnectionTypeInfoArgs }, /* R  */
                {"RequestConnection", WANPPPRequestConnectionArgs }, /* R/s */
                {"RequestTermination", WANPPPRequestTerminationArgs }, /* O/S */
                {"ForceTermination", WANPPPForceTerminationArgs }, /* R/S */
                {"SetAutoDisconnectTime", WANPPPSetAutoDisconnectTimeArgs }, /*O/S*/
                {"SetIdleDisconnectTime", WANPPPSetIdleDisconnectTimeArgs }, /*O/S*/
                {"SetWarnDisconnectDelay", WANPPPSetWarnDisconnectDelayArgs }, /*O/S*/
                {"GetStatusInfo", WANPPPGetStatusInfoArgs }, /* Req  */

                {"GetLinkLayerMaxBitRates", WANPPPGetLinkLayerMaxBitRatesArgs }, /* Req  */
                {"GetPPPEncryptionProtocol", WANPPPGetPPPEncryptionProtocolArgs }, /* O  */
                {"GetPPPCompressionProtocol", WANPPPGetPPPCompressionProtocolArgs }, /* O  */
                {"GetPPPAuthenticationProtocol", WANPPPGetPPPAuthenticationProtocolArgs }, /* O  */
                {"GetUserName", WANPPPGetUserNameArgs }, /* Req  */
                {"SetUserName", WANPPPSetUserNameArgs }, /* R/S  */
                {"SetPassword", WANPPPSetPasswordArgs }, /* R/S  */

                {"GetAutoDisconnectTime", WANPPPGetAutoDisconnectTimeArgs }, /* O  */
                {"GetIdleDisconnectTime", WANPPPGetIdleDisconnectTimeArgs }, /*O*/
                {"GetWarnDisconnectDelay", WANPPPGetWarnDisconnectDelayArgs }, /*O*/
                {"GetNATRSIPStatus", WANPPPGetNATRSIPStatusArgs }, /* Req  */

                {"SetPPPoEService", WANPPPSetPPPoEServiceArgs }, /* O/S */

                {"SetConnectionTrigger", WANPPPSetConnectionTriggerArgs }, /* R/S */

                {"GetPortMappingNumberOfEntries", WANPPPGetPortMappingNumberOfEntriesArgs }, /* Req  */
                {"GetGenericPortMappingEntry", WANPPPGetGenericPortMappingEntryArgs }, /* Req  */
                {"GetSpecificPortMappingEntry", WANPPPGetSpecificPortMappingEntryArgs }, /* Req  */
                {"AddPortMapping", WANPPPAddPortMappingArgs }, /* Req  */
                {"DeletePortMapping", WANPPPDeletePortMappingArgs }, /* Req  */
                {"GetExternalIPAddress", WANPPPGetExternalIPAddressArgs }, /* Req  */
                {"SetMACAddress", WANPPPSetMACAddressArgs }, /*O/S*/

                {"SetMaxMRUSize", WANPPPSetMaxMRUSizeArgs }, /*O/S*/

                {"SetRouteProtocolRx", WANPPPSetRouteProtocolRxArgs }, /* R/S */
                {0, 0}
        };

static const struct stateVar WANPPPVars[] =
        {
/*0*/           {"Enable", 1, 0, 0, 0}, /* Required */
                {"ConnectionType", 0, 0, 0, 0}, /* Required */
                {"PossibleConnectionType", 0, 0, 0, 0}, /* Required */
                {"ConnectionStatus", 0, 0, 0, 0}, /* Required */
                {"Name", 0, 0, 0, 0}, /* Required */
/*5*/           {"Uptime", 2, 0, 0, 0}, /* Required */
                {"UpstreamMaxBitRate", 3, 0,0, 0}, /* R */
                {"DownstreamMaxBitRate", 3, 0,0, 0}, /* R */
                {"LastConnectionError", 0, 0, 0, 0}, /* Required */
                {"AutoDisconnectTime", 3, 0, 0, 0}, /* O */
/*10*/          {"IdleDisconnectTime", 3, 0, 0, 0}, /* O */
                {"WarnDisconnectDelay", 3, 0, 0, 0}, /* O */
                {"ConnectionTrigger", 0, 0, 0, 0}, /* Required */
                {"RSIPAvailable", 1, 0, 0, 0}, /* Required */
                {"NATEnabled", 1, 0, 0, 0}, /* Required */
/*15*/          {"UserName", 0, 0, 0, "Username"}, /* Required */
                {"Password", 0, 0, 0, 0}, /* Required */
                {"PPPEncryptionProtocol", 0, 0, 0, 0}, /* O */
                {"PPPCompressionProtocol", 0, 0, 0, 0}, /* O */
                {"PPPAuthenticationProtocol", 0, 0, 0, 0}, /* O */
/*20*/          {"ExternalIPAddress", 0, 0, 0, 0}, /* Required */
                {"RemoteIPAddress", 0, 0, 0, 0}, /* O */
                {"MaxMRUSize", 2, 0, 0, 0}, /* O */
                {"CurrentMRUSize", 2, 0, 0, 0}, /* O */
                {"PortMappingNumberOfEntries", 2, 0, 0, 0}, /* Required */
/*25*/          {"PortMappingEnabled", 1, 0, 0, 0}, /* Required */
                {"PortMappingLeaseDuration", 3, 0, 0, 0}, /* Required */
                {"RemoteHost", 0, 0, 0, 0}, /* Required */
                {"ExternalPort", 2, 0, 0, 0}, /* Required */
                {"InternalPort", 2, 0, 0, 0}, /* Required */
/*30*/          {"PortMappingProtocol", 0, 0, 0, 0}, /* Required */
                {"InternalClient", 0, 0, 0, 0}, /* Required */
                {"PortMappingDescription", 0, 0, 0, 0}, /* Required */
                {"MACAddress", 0, 0, 0, 0}, /* Required */
                {"MACAddressOverride", 1, 0, 0, 0}, /* O */
/*35*/          {"DNSEnabled", 1, 0, 0, 0}, /* Required */
                {"DNSOverrideAllowed", 1, 0, 0, 0}, /* Required */
                {"DNSServers", 0, 0, 0, 0}, /* Required */
                {"TransportType", 0, 0, 0, 0}, /* Required */
                {"PPPoEACName", 0, 0, 0, 0}, /* Required */
/*40*/          {"PPPoEServiceName", 0, 0, 0, 0}, /* Required */
                {"RouteProtocolRx", 0, 0, 0, 0}, /* Required */
                {"PPPLCPEcho", 2, 0, 0, 0}, /* O */
/*43*/          {"PPPLCPEchoRetry", 2, 0, 0, 0}, /* O */
                {0, 0, 0, 0, 0}
        };

static const struct serviceDesc scpdWANPPP =
        { WANPPPActions, WANPPPVars };

#endif