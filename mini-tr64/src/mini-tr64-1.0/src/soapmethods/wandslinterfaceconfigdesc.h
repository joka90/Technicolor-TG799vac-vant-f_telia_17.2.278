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

#ifndef WANDSLINTERFACECONFIGDESC_H_INCLUDED
#define WANDSLINTERFACECONFIGDESC_H_INCLUDED

#include "../upnpdescgen.h"

/* Read TR064 spec - WANDSLInterfaceConfig */
static const struct argument WDICGetStatisticsQuarterHourArgs[] =
        {
                {0, 0, 0},
        };
static const struct argument WDICGetStatisticsCurrentDayArgs[] =
        {
                {0, 0, 0},
        };
static const struct argument WDICGetStatisticsLastShowtimeArgs[] =
        {
                {0, 0, 0},
        };
static const struct argument WDICGetStatisticsShowtimeArgs[] =
        {
                {0, 0, 0},
        };
static const struct argument WDICGetStatisticsTotalArgs[] =
        {
                {0, 0, 0},
        };

static const struct argument WDICGetInfoArgs[] =
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
                {2, 22, 0}, /* out */
                {2, 23, 0}, /* out */
                {2, 24, 0}, /* out */
                {2, 25, 0}, /* out */
                {2, 26, 0}, /* out */
                {2, 27, 0}, /* out */
                {2, 28, 0}, /* out */
                {2, 29, 0}, /* out */
                {0, 0, 0},
        };

static const struct argument WDICSetEnableArgs[] =
        {
                {1, 0, 0}, /* in */
                {0, 0, 0},
        };

static const struct action WDICActions[] =
        {
                {"SetEnable", WDICSetEnableArgs }, /* R/S */
                {"GetInfo", WDICGetInfoArgs }, /* Req  */
                {"GetStatisticsTotal", WDICGetStatisticsTotalArgs }, /* Req  */
                {"GetStatisticsShowtime", WDICGetStatisticsShowtimeArgs }, /* D  */
                {"GetStatisticsLastShowtime", WDICGetStatisticsLastShowtimeArgs }, /* D */
                {"GetStatisticsCurrentDay", WDICGetStatisticsCurrentDayArgs }, /* D */
                {"GetStatisticsQuarterHour", WDICGetStatisticsQuarterHourArgs }, /* D */
                {0, 0}
        };

static const struct stateVar WDICVars[] =
        {
                {"Enable", 1, 0, 0, 0}, /* Required */
                {"Status", 0, 0, 0, 0}, /* Required */
                {"ModulationType", 0, 0, 0, 0}, /* O */
                {"LineEncoding", 0, 0, 0, 0}, /* O */
                {"LineNumber", 6, 0, 0, 0}, /* O */
                {"UpstreamCurrRate", 3, 0, 0, 0}, /* Required */
                {"DownstreamCurrRate", 3, 0, 0, 0}, /* Required */
                {"UpstreamMaxRate", 3, 0, 0, 0}, /* Required */
                {"DownstreamMaxRate", 3, 0, 0, 0}, /* Required */
                {"UpstreamNoiseMargin", 3, 0, 0, 0}, /* Required */
                {"DownstreamNoiseMargin", 3, 0, 0, 0}, /* Required */
                {"UpstreamAttenuation", 3, 0, 0, 0}, /* Required */
                {"DownstreamAttenuation", 3, 0, 0, 0}, /* Required */
                {"UpstreamPower", 3, 0, 0, 0}, /* Required */
                {"DownstreamPower", 3, 0, 0, 0}, /* Required */
                {"DataPath", 0, 0, 0, 0}, /* Required */
                {"InterleavedDepth", 2, 0, 0, 0}, /* Required */
                {"ATURVendor", 0, 0, 0, 0}, /* Required */
                {"ATURCountry", 3, 0, 0, 0}, /* Required */
                {"ATURANSIStd", 3, 0, 0, 0}, /* Opt */
                {"ATURANSIRev", 3, 0, 0, 0}, /* Opt */
                {"ATUCVendor", 0, 0, 0, 0}, /* Required */
                {"ATUCCountry", 3, 0, 0, 0}, /* Required */
                {"ATUCANSIStd", 3, 0, 0, 0}, /* Opt */
                {"ATUCANSIRev", 3, 0, 0, 0}, /* Opt */
                {"TotalStart", 3, 0, 0, 0}, /* D */
                {"ShowtimeStart", 3, 0, 0, 0}, /* D */
                {"LastShowtimeStart", 3, 0, 0, 0}, /* D */
                {"CurrentDayStart", 3, 0, 0, 0}, /* D */
                {"QuarterHourStart", 3, 0, 0, 0}, /* D */
                {0, 0, 0, 0, 0}
        };

static const struct serviceDesc scpdWDIC =
        { WDICActions, WDICVars };

#endif