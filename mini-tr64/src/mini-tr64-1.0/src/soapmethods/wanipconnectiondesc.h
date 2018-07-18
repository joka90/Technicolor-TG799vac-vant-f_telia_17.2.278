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

#ifndef WANIPCONNECTIONDESC_H_INCLUDED
#define WANIPCONNECTIONDESC_H_INCLUDED

#include "../upnpdescgen.h"

/* Read TR064 spec - WANDSLInterfaceConfig */
static const struct argument WANIPSetRouteProtocolRxArgs[] =
        {
                {1, 32, 0},
                {0, 0, 0},
        };
static const struct argument WANIPSetConnectionTriggerArgs[] =
        {
                {1, 31, 0},
                {0, 0, 0},
        };
static const struct argument WANIPSetMaxMTUSizeArgs[] =
        {
                {1, 27, 0},
                {0, 0, 0},
        };
static const struct argument WANIPSetMACAddressArgs[] =
        {
                {1, 25, 0},
                {0, 0, 0},
        };
static const struct argument WANIPSetIPInterfaceInfoArgs[] =
        {
                {1, 23, 0},
                {1, 12, 0},
                {1, 13, 0},
                {1, 24, 0},
                {0, 0, 0},
        };
static const struct argument WANIPGetExternalIPAddressArgs[] =
        {
                {2, 12, 0},
                {0, 0, 0},
        };
static const struct argument WANIPDeletePortMappingArgs[] =
        {
                {1, 17, 0},
                {1, 18, 0},
                {1, 20, 0},
                {0, 0, 0},
        };
static const struct argument WANIPAddPortMappingArgs[] =
        {
                {1, 17, 0},
                {1, 18, 0},
                {1, 20, 0},
                {1, 19, 0},
                {1, 21, 0},
                {1, 15, 0},
                {1, 22, 0},
                {1, 16, 0},
                {0, 0, 0},
        };
static const struct argument WANIPGetSpecificPortMappingEntryArgs[] =
        {
                {1, 17, 0},
                {1, 18, 0},
                {1, 20, 0},
                {2, 19, 0},
                {2, 21, 0},
                {2, 15, 0},
                {2, 22, 0},
                {2, 16, 0},
                {0, 0, 0},
        };
static const struct argument WANIPGetGenericPortMappingEntryArgs[] =
        {
                {1, 14, 0},//todo: special name
                {2, 17, 0},
                {2, 18, 0},
                {2, 20, 0},
                {2, 19, 0},
                {2, 21, 0},
                {2, 15, 0},
                {2, 22, 0},
                {2, 16, 0},
                {0, 0, 0},
        };
static const struct argument WANIPGetPortMappingNumberOfEntriesArgs[] =
        {
                {2, 14, 0},
                {0, 0, 0},
        };
static const struct argument WANIPGetNATRSIPStatusArgs[] =
        {
                {2, 10, 0},
                {2, 11, 0},
                {0, 0, 0},
        };
static const struct argument WANIPGetWarnDisconnectDelayArgs[] =
        {
                {2, 9, 0},
                {0, 0, 0},
        };
static const struct argument WANIPGetIdleDisconnectTimeArgs[] =
        {
                {2, 8, 0},
                {0, 0, 0},
        };
static const struct argument WANIPGetAutoDisconnectTimeArgs[] =
        {
                {2, 7, 0},
                {0, 0, 0},
        };
static const struct argument WANIPGetStatusInfoArgs[] =
        {
                {2, 3, 0},
                {2, 6, 0},
                {2, 5, 0},
                {0, 0, 0},
        };
static const struct argument WANIPSetWarnDisconnectDelayArgs[] =
        {
                {1, 9, 0},
                {0, 0, 0},
        };
static const struct argument WANIPSetIdleDisconnectTimeArgs[] =
        {
                {1, 8, 0},
                {0, 0, 0},
        };
static const struct argument WANIPSetAutoDisconnectTimeArgs[] =
        {
                {1, 7, 0},
                {0, 0, 0},
        };
static const struct argument WANIPForceTerminationArgs[] =
        {
                {0, 0, 0},
        };
static const struct argument WANIPRequestTerminationArgs[] =
        {
                {0, 0, 0},
        };
static const struct argument WANIPRequestConnectionArgs[] =
        {
                {0, 0, 0},
        };
static const struct argument WANIPGetConnectionTypeInfoArgs[] =
        {
                {2, 1, 0},
                {2, 2, 0},
                {0, 0, 0},
        };
static const struct argument WANIPSetConnectionTypeArgs[] =
        {
                {1, 1, 0},
                {0, 0, 0},
        };

static const struct argument WANIPGetInfoArgs[] =
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
                {2, 23, 0}, /* out */
                {2, 24, 0}, /* out */
                {2, 25, 0}, /* out */
                {2, 26, 0}, /* out */
                {2, 27, 0}, /* out */
                {2, 28, 0}, /* out */
                {2, 29, 0}, /* out */
                {2, 30, 0}, /* out */
                {2, 31, 0}, /* out */
                {2, 32, 0}, /* out */
                {0, 0, 0},
        };

static const struct argument WANIPSetEnableArgs[] =
        {
                {1, 0, 0}, /* in */
                {0, 0, 0},
        };

static const struct action WANIPActions[] =
        {
                {"SetEnable", WANIPSetEnableArgs }, /* R/S */
                {"GetInfo", WANIPGetInfoArgs }, /* Req  */
                {"SetConnectionType", WANIPSetConnectionTypeArgs }, /* R/S  */
                {"GetConnectionTypeInfo", WANIPGetConnectionTypeInfoArgs }, /* R  */
                {"RequestConnection", WANIPRequestConnectionArgs }, /* R/s */
                {"RequestTermination", WANIPRequestTerminationArgs }, /* O/S */
                {"ForceTermination", WANIPForceTerminationArgs }, /* R/S */
                {"SetAutoDisconnectTime", WANIPSetAutoDisconnectTimeArgs }, /*O/S*/
                {"SetIdleDisconnectTime", WANIPSetIdleDisconnectTimeArgs }, /*O/S*/
                {"SetWarnDisconnectDelay", WANIPSetWarnDisconnectDelayArgs }, /*O/S*/
                {"GetStatusInfo", WANIPGetStatusInfoArgs }, /* Req  */
                {"GetAutoDisconnectTime", WANIPGetAutoDisconnectTimeArgs }, /* O  */
                {"GetIdleDisconnectTime", WANIPGetIdleDisconnectTimeArgs }, /*O*/
                {"GetWarnDisconnectDelay", WANIPGetWarnDisconnectDelayArgs }, /*O*/
                {"GetNATRSIPStatus", WANIPGetNATRSIPStatusArgs }, /* Req  */
                {"GetPortMappingNumberOfEntries", WANIPGetPortMappingNumberOfEntriesArgs }, /* Req  */
                {"GetGenericPortMappingEntry", WANIPGetGenericPortMappingEntryArgs }, /* Req  */
                {"GetSpecificPortMappingEntry", WANIPGetSpecificPortMappingEntryArgs }, /* Req  */
                {"AddPortMapping", WANIPAddPortMappingArgs }, /* Req  */
                {"DeletePortMapping", WANIPDeletePortMappingArgs }, /* Req  */
                {"GetExternalIPAddress", WANIPGetExternalIPAddressArgs }, /* Req  */
                {"SetIPInterfaceInfo", WANIPSetIPInterfaceInfoArgs }, /*O/S*/
                {"SetMACAddress", WANIPSetMACAddressArgs }, /*O/S*/
                {"SetMaxMTUSize", WANIPSetMaxMTUSizeArgs }, /*O/S*/
                {"SetConnectionTrigger", WANIPSetConnectionTriggerArgs }, /* R/S */
                {"SetRouteProtocolRx", WANIPSetRouteProtocolRxArgs }, /* R/S */
                {0, 0}
        };

static const struct stateVar WANIPVars[] =
        {
                {"Enable", 1, 0, 0, 0}, /* Required */
                {"ConnectionType", 0, 0, 0, 0}, /* Required */
                {"PossibleConnectionType", 0, 0, 0, 0}, /* Required */
                {"ConnectionStatus", 0, 0, 0, 0}, /* Required */
                {"Name", 0, 0, 0, 0}, /* Required */
                {"Uptime", 2, 0, 0, 0}, /* Required */
                {"LastConnectionError", 0, 0, 0, 0}, /* Required */
                {"AutoDisconnectTime", 3, 0, 0, 0}, /* O */
                {"IdleDisconnectTime", 3, 0, 0, 0}, /* O */
                {"WarnDisconnectDelay", 3, 0, 0, 0}, /* O */
                {"RSIPAvailable", 1, 0, 0, 0}, /* Required */
                {"NATEnabled", 1, 0, 0, 0}, /* Required */
                {"ExternalIPAddress", 0, 0, 0, 0}, /* Required */
                {"SubnetMask", 0, 0, 0, 0}, /* Required */
                {"PortMappingNumberOfEntries", 2, 0, 0, 0}, /* Required */
                {"PortMappingEnabled", 1, 0, 0, 0}, /* Required */
                {"PortMappingLeaseDuration", 3, 0, 0, 0}, /* Required */
                {"RemoteHost", 0, 0, 0, 0}, /* Required */
                {"ExternalPort", 2, 0, 0, 0}, /* Required */
                {"InternalPort", 2, 0, 0, 0}, /* Required */
                {"PortMappingProtocol", 0, 0, 0, 0}, /* Required */
                {"InternalClient", 0, 0, 0, 0}, /* Required */
                {"PortMappingDescription", 0, 0, 0, 0}, /* Required */
                {"AddressingType", 0, 0, 0, 0}, /* Required */
                {"DefaultGateway", 0, 0, 0, 0}, /* Required */
                {"MACAddress", 0, 0, 0, 0}, /* Required */
                {"MACAddressOverride", 1, 0, 0, 0}, /* O */
                {"MaxMTUSize", 2, 0, 0, 0}, /* O */
                {"DNSEnabled", 1, 0, 0, 0}, /* Required */
                {"DNSOverrideAllowed", 1, 0, 0, 0}, /* Required */
                {"DNSServers", 0, 0, 0, 0}, /* Required */
                {"ConnectionTrigger", 0, 0, 0, 0}, /* Required */
                {"RouteProtocolRx", 0, 0, 0, 0}, /* Required */
                {0, 0, 0, 0, 0}
        };

static const struct serviceDesc scpdWANIP =
        { WANIPActions, WANIPVars };

#endif