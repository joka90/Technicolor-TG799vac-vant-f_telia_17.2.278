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

#include <stdio.h>
#include <string.h>
#include <sys/syslog.h>
#include "deviceinfo.h"
#include "soaphelper.h"
#include "../upnpurns.h"
#include "../transformer/helper.h"
#include "deviceinfodesc.h"

void DIGetInfo(struct upnphttp * h, const char * action)
{
    struct query_item query[] = {
            { .name = "InternetGatewayDevice.DeviceInfo.Manufacturer" },
            { .name = "InternetGatewayDevice.DeviceInfo.ManufacturerOUI" },
            { .name = "InternetGatewayDevice.DeviceInfo.ModelName" },
            { .name = "InternetGatewayDevice.DeviceInfo.Description" },
            { .name = "InternetGatewayDevice.DeviceInfo.ProductClass" },
            { .name = "InternetGatewayDevice.DeviceInfo.SerialNumber" },
            { .name = "InternetGatewayDevice.DeviceInfo.SoftwareVersion" },
            { .name = "InternetGatewayDevice.DeviceInfo.AdditionalSoftwareVersion" },
            { .name = "InternetGatewayDevice.DeviceInfo.ModemFirmwareVersion" },
            { .name = "InternetGatewayDevice.DeviceInfo.EnabledOptions" },
            { .name = "InternetGatewayDevice.DeviceInfo.HardwareVersion" },
            { .name = "InternetGatewayDevice.DeviceInfo.AdditionalHardwareVersion" },
            { .name = "InternetGatewayDevice.DeviceInfo.SpecVersion" },
            { .name = "InternetGatewayDevice.DeviceInfo.ProvisioningCode" },
            { .name = "InternetGatewayDevice.DeviceInfo.UpTime" },
            { .name = "InternetGatewayDevice.DeviceInfo.FirstUseDate" },
            { .name = 0 }
    };
    genericGetter(h, SERVICE_TYPE_DI, action, query, DIVars, GetInfoArgs);
}

void DISetProvisioningCode(struct upnphttp * h, const char * action)
{
    static const char resp[] =
            "<u:%sResponse "
                    "xmlns:u=\"%s\">"
                    "</u:%sResponse>";

    char body[512];
    int bodylen;

    bodylen = snprintf(body, sizeof(body), resp,
            action, SERVICE_TYPE_DI
            , action);
    BuildSendAndCloseSoapResp(h, body, bodylen);
}

void DIGetDeviceLog(struct upnphttp * h, const char * action)
{
    struct query_item query[] = {
            { .name = "InternetGatewayDevice.DeviceInfo.DeviceLog" },
            { .name = 0 }
    };
    genericGetter(h, SERVICE_TYPE_DI, action, query, DIVars, GetDeviceLogArgs);
}
