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

#ifndef LANHOSTCONFIGDESC_H_INCLUDED
#define LANHOSTCONFIGDESC_H_INCLUDED

#include "../upnpdescgen.h"

/* Read TR064 spec - LANHostConfig */
static const struct argument LHCGetInfoArgs[] =
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
                {0, 0, 0},
        };

static const struct argument SetDHCPServerConfigurableArgs[] =
        {
                {1, 0, 0}, /* in */
                {0, 0, 0},
        };

static const struct argument GetDHCPServerConfigurableArgs[] =
        {
                {2, 0, 0}, /* out */
                {0, 0, 0},
        };

static const struct argument GetDHCPRelayArgs[] =
        {
                {2, 1, 0}, /* out */
                {0, 0, 0},
        };

static const struct argument SetSubnetMaskArgs[] =
        {
                {1, 2, 0}, /* in */
                {0, 0, 0},
        };

static const struct argument GetSubnetMaskArgs[] =
        {
                {2, 2, 0}, /* out */
                {0, 0, 0},
        };

static const struct argument SetIPRouterArgs[] =
        {
                {1, 7, 0}, /* in */
                {0, 0, 0},
        };

static const struct argument DeleteIPRouterArgs[] =
        {
                {1, 7, 0}, /* in */
                {0, 0, 0},
        };

static const struct argument GetIPRoutersListArgs[] =
        {
                {2, 7, 0}, /* out */
                {0, 0, 0},
        };

static const struct argument SetDomainNameArgs[] =
        {
                {1, 4, 0}, /* in */
                {0, 0, 0},
        };

static const struct argument GetDomainNameArgs[] =
        {
                {2, 4, 0}, /* out */
                {0, 0, 0},
        };

static const struct argument SetAddressRangeArgs[] =
        {
                {1, 5, 0}, /* in */
                {1, 6, 0}, /* in */
                {0, 0, 0},
        };

static const struct argument GetAddressRangeArgs[] =
        {
                {2, 5, 0}, /* out */
                {2, 6, 0}, /* out */
                {0, 0, 0},
        };

static const struct argument SetReservedAddressArgs[] =
        {
                {1, 8, 0}, /* in */
                {0, 0, 0},
        };

static const struct argument DeleteReservedAddressArgs[] =
        {
                {1, 8, 0}, /* in */
                {0, 0, 0},
        };

static const struct argument GetReservedAddressesArgs[] =
        {
                {2, 8, 0}, /* out */
                {0, 0, 0},
        };

static const struct argument SetDNSServerArgs[] =
        {
                {1, 3, 0}, /* in */
                {0, 0, 0},
        };

static const struct argument DeleteDNSServerArgs[] =
        {
                {1, 3, 0}, /* in */
                {0, 0, 0},
        };

static const struct argument GetDNSServersArgs[] =
        {
                {2, 3, 0}, /* out */
                {0, 0, 0},
        };

static const struct argument SetDHCPLeaseTimeArgs[] =
        {
                {1, 9, 0}, /* in */
                {0, 0, 0},
        };

static const struct argument SetDHCPServerEnableArgs[] =
        {
                {1, 10, 0}, /* in */
                {0, 0, 0},
        };

static const struct argument SetAddressAllocationArgs[] =
        {
                {1, 11, 0}, /* in */
                {1, 12, 0}, /* in */
                {1, 13, 0}, /* in */
                {1, 14, 0}, /* in */
                {0, 0, 0},
        };

static const struct argument SetAllowedMACAddressesArgs[] =
        {
                {1, 15, 0}, /* in */
                {0, 0, 0},
        };

static const struct argument DeleteAllowedMacAddressArgs[] =
        {
                {1, 15, 0}, /* in */
                {0, 0, 0},
        };

static const struct argument GetIPInterfacetNumberOfEntriesArgs[] =
        {
                {2, 20, 0}, /* out */
                {0, 0, 0},
        };

static const struct argument SetIPInterfaceArgs[] = //TODO: names are not generic
        {
                {1, 16, 0}, /* in */
                {1, 17, 0}, /* in */
                {1, 18, 0}, /* in */
                {1, 19, 0}, /* in */
                {0, 0, 0},
        };

static const struct argument AddIPInterfaceArgs[] = //TODO: names are not generic
        {
                {1, 16, 0}, /* in */
                {1, 17, 0}, /* in */
                {1, 18, 0}, /* in */
                {1, 19, 0}, /* in */
                {0, 0, 0},
        };

static const struct argument DeleteIPInterfaceArgs[] = //TODO: names are not generic
        {
                {1, 17, 0}, /* in */
                {1, 18, 0}, /* in */
                {0, 0, 0},
        };

static const struct argument GetIPInterfaceSpecificEntryArgs[] = //TODO: names are not generic
        {
                {1, 17, 0}, /* in */
                {1, 18, 0}, /* in */
                {2, 16, 0}, /* out */
                {2, 19, 0}, /* out */
                {0, 0, 0},
        };

static const struct argument GetIPInterfaceGenericEntryArgs[] = //TODO: names are not generic
        {
                {1, 20, 0}, /* in */
                {2, 16, 0}, /* out */
                {2, 17, 0}, /* out */
                {2, 18, 0}, /* out */
                {2, 19, 0}, /* out */
                {2, 20, 0}, /* out */
                {0, 0, 0},
        };

static const struct action LHCActions[] =
        {
                {"GetInfo", LHCGetInfoArgs}, /* Req  */
                {"SetDHCPServerConfigurable", SetDHCPServerConfigurableArgs}, /* R/S  */
                {"GetDHCPServerConfigurable", GetDHCPServerConfigurableArgs}, /* R  */
                {"GetDHCPRelay", GetDHCPRelayArgs}, /* Req  */
                {"SetSubnetMask", SetSubnetMaskArgs}, /* R/S */
                {"GetSubnetMask", GetSubnetMaskArgs}, /* Req  */
                {"SetIPRouter", SetIPRouterArgs}, /* R/S */
                {"DeleteIPRouter", DeleteIPRouterArgs}, /* O/S  */
                {"GetIPRoutersList", GetIPRoutersListArgs}, /* Req  */
                {"SetDomainName", SetDomainNameArgs}, /* R/S  */
                {"GetDomainName", GetDomainNameArgs}, /* Req  */
                {"SetAddressRange", SetAddressRangeArgs}, /* R/S  */
                {"GetAddressRange", GetAddressRangeArgs}, /* Req  */
                {"SetReservedAddress", SetReservedAddressArgs}, /* R/S  */
                {"DeleteReservedAddress", DeleteReservedAddressArgs}, /* R/S */
                {"GetReservedAddresses", GetReservedAddressesArgs}, /* Req  */
                {"SetDNSServer", SetDNSServerArgs}, /* R/S  */
                {"DeleteDNSServer", DeleteDNSServerArgs}, /* R/S */
                {"GetDNSServers", GetDNSServersArgs}, /* Req  */
                {"SetDHCPLeaseTime", SetDHCPLeaseTimeArgs}, /* O/S  */
                {"SetDHCPServerEnable", SetDHCPServerEnableArgs}, /* R/S */
                {"SetAddressAllocation", SetAddressAllocationArgs}, /* O/S */
                {"SetAllowedMACAddresses", SetAllowedMACAddressesArgs}, /* O/S */
                {"DeleteAllowedMacAddress", DeleteAllowedMacAddressArgs}, /* O/S */
                {"GetIPInterfacetNumberOfEntries", GetIPInterfacetNumberOfEntriesArgs}, /* Req  */
                {"SetIPInterface", SetIPInterfaceArgs}, /* R/S */
                {"AddIPInterface", AddIPInterfaceArgs}, /* O/S */
                {"DeleteIPInterface", DeleteIPInterfaceArgs}, /* O/S */
                {"GetIPInterfaceSpecificEntry", GetIPInterfaceSpecificEntryArgs}, /* Req  */
                {"GetIPInterfaceGenericEntry", GetIPInterfaceGenericEntryArgs}, /* Req  */
                {0, 0}
        };

static const struct stateVar LHCVars[] =
        {
                {"DHCPServerConfigurable", 1, 0, 0, 0}, /* Required */
                {"DHCPRelay", 1, 0, 0, 0}, /* Required */
                {"SubnetMask", 0, 0, 0, 0}, /* Required */
                {"DNSServers", 0, 0, 0, 0}, /* Required */
                {"DomainName", 0, 0, 0, 0}, /* Required */
                {"MinAddress", 0, 0, 0, 0}, /* Required */
                {"MaxAddress", 0, 0, 0, 0}, /* Required */
                {"IPRouters", 0, 0, 0, 0}, /* Required */
                {"ReservedAddresses", 0, 0, 0, 0}, /* R */
                {"DHCPLeaseTime", 0, 0, 0, 0}, /* Optional */
                {"DHCPServerEnable", 1, 0, 0, 0}, /* Required */
                {"UseAllocatedWAN", 0, 0, 0, 0}, /* Required */
                {"AssociatedConnection", 0, 0, 0, 0}, /* Optional */
                {"PassthroughLease", 3, 0, 0, 0}, /* Optional */
                {"PassthroughMACAddress", 0, 0, 0, 0}, /* Optional */
                {"AllowedMACAddress", 0, 0, 0, 0}, /* Optional */
                {"Enable", 1, 0, 0, 0}, /* Required */
                {"IPInterfaceIPAddress", 0, 0, 0, 0},
                {"IPInterfaceSubnetMask", 0, 0, 0, 0},
                {"IPInterfaceAddressingType", 0, 0, 0, 0},
                {"IPInterfaceNumberOfEntries", 0, 0, 0, 0},
                {0, 0, 0, 0, 0}
        };

static const struct serviceDesc scpdLHC = { LHCActions, LHCVars };

#endif