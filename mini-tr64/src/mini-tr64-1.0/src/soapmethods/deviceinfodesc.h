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

#ifndef DEVICEINFODESC_H_INCLUDED
#define DEVICEINFODESC_H_INCLUDED
#include "../upnpdescgen.h"

/* Read TR064 spec - DeviceInfo */
static const struct argument GetInfoArgs[] =
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
                {0, 0, 0},
        };

static const struct argument SetProvisioningCodeArgs[] =
        {
                {1, 13, 0}, /* in */
                {0, 0, 0},
        };

static const struct argument GetDeviceLogArgs[] =
        {
                {2, 16, 0}, /* out */
                {0, 0, 0},
        };

static const struct action DIActions[] =
        {
                {"GetInfo", GetInfoArgs}, /* Req */
                {"SetProvisioningCode", SetProvisioningCodeArgs}, /* Opt / Secure */
                {"GetDeviceLog", GetDeviceLogArgs}, /* Req / Secure */
                {0, 0},
        };

static const struct stateVar DIVars[] =
        {
                {"ManufacturerName", 0, 0, 0, "Manufacturer"}, /* Required */
                {"ManufacturerOUI", 0, 0, 0, 0}, /* Required */
                {"ModelName", 0, 0, 0, 0}, /* Required */
                {"Description", 0, 0, 0, 0}, /* Required */
                {"ProductClass", 0, 0, 0, 0}, /* Optional */
                {"SerialNumber", 0, 0, 0, 0}, /* Required */
                {"SoftwareVersion", 0, 0, 0, 0}, /* Required */
                {"AdditionalSoftwareVersions", 0, 0, 0, "AdditionalSoftwareVersion"}, /* Optional */
                {"ModemFirmwareVersion", 0, 0, 0, 0}, /* Required */
                {"EnabledOptions", 0, 0, 0, 0}, /* Optional */
                {"HardwareVersion", 0, 0, 0, 0}, /* Required */
                {"AdditionalHardwareVersions", 0, 0, 0, "AdditionalHardwareVersion"}, /* Optional */
                {"SpecVersion", 0, 0, 0, 0}, /* Required */
                {"ProvisioningCode", 0, 0, 0, 0}, /* Optional */
                {"UpTime", 3, 0, 0, 0}, /* Optional */
                {"FirstUseDate", 0, 0, 0, 0}, /* Optional */
                {"DeviceLog", 0 , 0, 0, 0}, /* Required */
                {0, 0, 0, 0, 0}
        };

static const struct serviceDesc scpdDI =
        { DIActions, DIVars };
#endif
