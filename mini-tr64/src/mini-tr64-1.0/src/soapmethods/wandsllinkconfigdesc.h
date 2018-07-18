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

#ifndef WANDSLLINKCONFIGDESC_H_INCLUDED
#define WANDSLLINKCONFIGDESC_H_INCLUDED

#include "../upnpdescgen.h"

/* Read TR064 spec - WANDSLInterfaceConfig */

static const struct argument WDLCGetInfoArgs[] =
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
                {2, 12, 0}, /* out */
                {2, 13, 0}, /* out */
                {2, 14, 0}, /* out */
                {2, 15, 0}, /* out */
                {0, 0, 0},
        };

static const struct argument WDLCSetEnableArgs[] =
        {
                {1, 0, 0}, /* in */
                {0, 0, 0},
        };
static const struct argument WDLCSetDSLLinkTypeArgs[] =
        {
                {1, 1, 0}, /* in */
                {0, 0, 0},
        };
static const struct argument WDLCGetDSLLinkInfoArgs[] =
        {
                {2, 1, 0}, /* in */
                {0, 0, 0},
        };
static const struct argument WDLCGetAutoConfigArgs[] =
        {
                {2, 3, 0}, /* in */
                {0, 0, 0},
        };
static const struct argument WDLCGetModulationTypeArgs[] =
        {
                {2, 4, 0}, /* in */
                {0, 0, 0},
        };
static const struct argument WDLCSetDestinationAddressArgs[] =
        {
                {1, 5, 0}, /* in */
                {0, 0, 0},
        };
static const struct argument WDLCGetDestinationAddressArgs[] =
        {
                {2, 5, 0}, /* in */
                {0, 0, 0},
        };
static const struct argument WDLCSetATMEncapsulationArgs[] =
        {
                {1, 6, 0}, /* in */
                {0, 0, 0},
        };
static const struct argument WDLCGetATMEncapsulationArgs[] =
        {
                {2, 6, 0}, /* in */
                {0, 0, 0},
        };
static const struct argument WDLCSetFCSPreservedArgs[] =
        {
                {1, 7, 0}, /* in */
                {0, 0, 0},
        };
static const struct argument WDLCGetFCSPreservedArgs[] =
        {
                {2, 7, 0}, /* in */
                {0, 0, 0},
        };
static const struct argument WDLCSetVCSearchListArgs[] =
        {
                {1, 8, 0}, /* in */
                {0, 0, 0},
        };
static const struct argument WDLCSetATMQosArgs[] =
        {
                {1, 12, 0}, /* in */
                {1, 13, 0}, /* in */
                {1, 14, 0}, /* in */
                {1, 15, 0}, /* in */
                {0, 0, 0},
        };
static const struct argument WDLCGetStatisticsArgs[] =
        {
                {2, 10, 0}, /* in */
                {2, 11, 0}, /* in */
                {2, 16, 0}, /* in */
                {2, 17, 0}, /* in */
                {2, 18, 0}, /* in */
                {0, 0, 0},
        };

static const struct action WDLCActions[] =
        {
                {"SetEnable", WDLCSetEnableArgs }, /* R/S */
                {"GetInfo", WDLCGetInfoArgs }, /* Req  */
                {"SetDSLLinkType", WDLCSetDSLLinkTypeArgs }, /* R/S  */
                {"GetDSLLinkInfo", WDLCGetDSLLinkInfoArgs }, /* R  */
                {"GetAutoConfig", WDLCGetAutoConfigArgs }, /* R */
                {"GetModulationType", WDLCGetModulationTypeArgs }, /* O */
                {"SetDestinationAddress", WDLCSetDestinationAddressArgs }, /* O/S */
                {"GetDestinationAddress", WDLCGetDestinationAddressArgs }, /*R*/
                {"SetATMEncapsulation", WDLCSetATMEncapsulationArgs }, /*O/S*/
                {"GetATMEncapsulation", WDLCGetATMEncapsulationArgs }, /*O*/
                {"SetFCSPreserved", WDLCSetFCSPreservedArgs }, /* O/S*/
                {"GetFCSPreserved", WDLCGetFCSPreservedArgs }, /* O  */
                {"SetVCSearchList", WDLCSetVCSearchListArgs }, /* O/S  */
                {"SetATMQos", WDLCSetATMQosArgs }, /* O/s  */
                {"GetStatistics", WDLCGetStatisticsArgs }, /* R  */
                {0, 0}
        };

static const struct stateVar WDLCVars[] =
        {
/*0*/           {"Enable", 1, 0, 0, 0}, /* Required */
                {"LinkType", 0, 0, 0, 0}, /* Required */
                {"LinkStatus", 0, 0, 0, 0}, /* Required */
                {"AutoConfig", 1, 0, 0, 0}, /* Required */
                {"ModulationType", 0, 0, 0, 0}, /* O */
/*5*/           {"DestinationAddress", 0, 0, 0, 0}, /* Required */
                {"ATMEncapsulation", 0, 0,0, 0}, /* O */
                {"FCSPreserved", 1, 0,0, 0}, /* O */
                {"VCSearchList", 0, 0, 0, 0}, /* O */
                {"ATMAAL", 0, 0, 0, 0}, /* O */
/*10*/          {"ATMTransmittedBlocks", 3, 0, 0, 0}, /* R */
                {"ATMReceivedBlocks", 3, 0, 0, 0}, /* R */
                {"ATMQoS", 0, 0, 0, 0}, /* O */
                {"ATMPeakCellRate", 3, 0, 0, 0}, /* O */
                {"ATMMaximumBurstSize", 3, 0, 0, 0}, /* O */
/*15*/          {"ATMSustainableCellRate", 3, 0, 0, 0}, /* O */
                {"AAL5CRCErrors", 3, 0, 0, 0}, /* R */
                {"ATMCRCErrors", 3, 0, 0, 0}, /* R */
/*18*/          {"ATMHECErrors", 3, 0, 0, 0}, /* O */
                {0, 0, 0, 0, 0}
        };

static const struct serviceDesc scpdWDLC =
        { WDLCActions, WDLCVars };

#endif