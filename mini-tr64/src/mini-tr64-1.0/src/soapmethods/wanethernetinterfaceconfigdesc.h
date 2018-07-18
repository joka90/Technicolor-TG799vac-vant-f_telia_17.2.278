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

#ifndef WANETHERNETINTERFACECONFIGDESC_H_INCLUDED
#define WANETHERNETINTERFACECONFIGDESC_H_INCLUDED

#include "../upnpdescgen.h"

/* Read TR064 spec - LANEthernetInterfaceConfig */
static const struct argument WEICGetInfoArgs[] =
        {
                {2, 0, 0}, /* out */
                {2, 1, 0}, /* out */
                {2, 2, 0}, /* out */
                {2, 3, 0}, /* out */
                {2, 4, 0}, /* out */
                {2, 5, 0}, /* out */
                {0, 0, 0},
        };

static const struct argument WEICGetStatisticsArgs[] =
        {
                {2, 6, 0}, /* out */
                {2, 7, 0}, /* out */
                {2, 8, 0}, /* out */
                {2, 9, 0}, /* out */
                {0, 0, 0},
        };

static const struct action WEICActions[] =
        {
                {"GetInfo", WEICGetInfoArgs}, /* Req  */
                {"GetStatistics", WEICGetStatisticsArgs}, /* Req  */
                {0, 0},
        };

static const struct stateVar WEICVars[] =
        {
                {"Enable", 1, 0, 0, 0}, /* Required */
                {"Status", 0, 0, 0, 0}, /* Required */  // ENUM
                {"MACAddress", 0, 0, 0, 0}, /* Required */
                {"MACAddressControlEnabled", 1, 0, 0, 0}, /* Required */
                {"MaxBitRate", 0, 0, 0, 0}, /* Required */
                {"DuplexMode", 0, 0, 0, 0}, /* Required */
                {"Stats.BytesSent", 3, 0, 0, 0}, /* Required */
                {"Stats.BytesReceived", 3, 0, 0, 0}, /* Required */
                {"Stats.PacketsSent", 3, 0, 0, 0}, /* Optional */
                {"Stats.PacketsReceived", 3, 0, 0, 0}, /* Required */
                {0, 0, 0, 0, 0}
        };

static const struct serviceDesc scpdWEIC =
        { WEICActions, WEICVars };

#endif