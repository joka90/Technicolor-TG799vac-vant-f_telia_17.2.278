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
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "config.h"
#include "upnpdescgen.h"
#include "minitr064dpath.h"
#include "upnpglobalvars.h"
#include "upnpdescstrings.h"
#include "upnpurns.h"
#include "transformer/helper.h"
#include "soapmethods/descriptions.h"
#include "upnputils.h"

static const char * const upnptypes[] =
{
	"string",
	"boolean",
	"ui2",
	"ui4",
	"bin.base64",
    "uuid",
    "ui1",
};

static const char * const upnpdefaultvalues[] =
{
	0,
	"IP_Routed"/*"Unconfigured"*/, /* 1 default value for ConnectionType */
	"3600", /* 2 default value for PortMappingLeaseDuration */
};

static const char * const upnpallowedvalues[] =
{
	0,		/* 0 */
	"DSL",	/* 1 */
	"POTS",
	"Cable",
	"Ethernet",
	0,
	"Up",	/* 6 */
	"Down",
	"Initializing",
	"Unavailable",
	0,
	"TCP",	/* 11 */
	"UDP",
	0,
	"Unconfigured",	/* 14 */
	"IP_Routed",
	"IP_Bridged",
	0,
	"Unconfigured",	/* 18 */
	"Connecting",
	"Connected",
	"PendingDisconnect",
	"Disconnecting",
	"Disconnected",
	0,
	"ERROR_NONE",	/* 25 */
/* Optionals values :
 * ERROR_COMMAND_ABORTED
 * ERROR_NOT_ENABLED_FOR_INTERNET
 * ERROR_USER_DISCONNECT
 * ERROR_ISP_DISCONNECT
 * ERROR_IDLE_DISCONNECT
 * ERROR_FORCED_DISCONNECT
 * ERROR_NO_CARRIER
 * ERROR_IP_CONFIGURATION
 * ERROR_UNKNOWN */
	0,
	"",		/* 27 */
	0
};

static const int upnpallowedranges[] = {
	0,
	/* 1 PortMappingLeaseDuration */
	0,
	604800,
	/* 3 InternalPort */
	1,
	65535,
    /* 5 LeaseTime */
	1,
	86400,
	/* 7 OutboundPinholeTimeout */
	100,
	200,
};

static const char xmlver[] =
	"<?xml version=\"1.0\"?>\r\n";
static const char root_service[] =
	"scpd xmlns=\"urn:dslforum-org:service-1-0\"";
static const char root_device[] =
	"root xmlns=\"urn:dslforum-org:device-1-0\"";

/* root Description of the UPnP Device
 * fixed to match UPnP_IGD_InternetGatewayDevice 1.0.pdf
 * Needs to be checked with UPnP-gw-InternetGatewayDevice-v2-Device.pdf
 * presentationURL is only "recommended" but the router doesn't appears
 * in "Network connections" in Windows XP if it is not present. */
static struct XMLElt rootDesc[512] =
{
#define SIZE_ROOTDEV 13
/* 0 */
	{root_device, INITHELPER(1,2)},
	{"specVersion", INITHELPER(3,2)},
	{"device", INITHELPER(5,SIZE_ROOTDEV)},
	{"/major", "1"},
	{"/minor", "0"},
/* 5 */
	{"/deviceType", DEVICE_TYPE_IGD},
		/* urn:dslforum-org:device:InternetGatewayDevice:1 or 2 */
#ifdef ENABLE_MANUFACTURER_INFO_CONFIGURATION
	{"/friendlyName", friendly_name/*ROOTDEV_FRIENDLYNAME*/},	/* required */
	{"/manufacturer", manufacturer_name/*ROOTDEV_MANUFACTURER*/},		/* required */
/* 8 */
	{"/manufacturerURL", manufacturer_url/*ROOTDEV_MANUFACTURERURL*/},	/* optional */
	{"/modelDescription", model_description/*ROOTDEV_MODELDESCRIPTION*/}, /* recommended */
	{"/modelName", model_name/*ROOTDEV_MODELNAME*/},	/* required */
	{"/modelNumber", modelnumber},
	{"/modelURL", model_url/*ROOTDEV_MODELURL*/},
#else
	{"/friendlyName", ROOTDEV_FRIENDLYNAME},	/* required */
	{"/manufacturer", ROOTDEV_MANUFACTURER},	/* required */
/* 8 */
	{"/manufacturerURL", ROOTDEV_MANUFACTURERURL},	/* optional */
	{"/modelDescription", ROOTDEV_MODELDESCRIPTION}, /* recommended */
	{"/modelName", ROOTDEV_MODELNAME},	/* required */
	{"/modelNumber", modelnumber},
	{"/modelURL", ROOTDEV_MODELURL},
#endif
	{"/serialNumber", serialnumber},
	{"/UDN", uuidvalue_igd},	/* required */
#define OFFSET_ROOTSERVICE 5 + SIZE_ROOTDEV
#define SIZE_SERVICE 5
#define NUM_ROOTSERVICES 3
#define OFFSET_ROOTDEVICE OFFSET_ROOTSERVICE + NUM_ROOTSERVICES * ( 1 + SIZE_SERVICE)
    {"serviceList", INITHELPER(OFFSET_ROOTSERVICE,NUM_ROOTSERVICES)},
    {"/deviceList", 0}, // Will be filled based on actual datamodel
	{"/presentationURL", presentationurl},	/* recommended */
/* 5 + SIZE_ROOTDEV */
    {"service", INITHELPER(OFFSET_ROOTSERVICE + NUM_ROOTSERVICES,SIZE_SERVICE)},
    {"service", INITHELPER(OFFSET_ROOTSERVICE + NUM_ROOTSERVICES + SIZE_SERVICE,SIZE_SERVICE)},
    {"service", INITHELPER(OFFSET_ROOTSERVICE + NUM_ROOTSERVICES + 2 * SIZE_SERVICE,SIZE_SERVICE)},
/* OFFSET_ROOTSERVICE + NUM_ROOTSERVICES */
    {"/serviceType", "urn:dslforum-org:service:DeviceInfo:1"},
    {"/serviceId", "urn:dslforum-org:serviceId:DeviceInfo"},
    {"/SCPDURL", DI_PATH},
    {"/controlURL", DI_CONTROLURL},
    {"/eventSubURL", ""},
    {"/serviceType", "urn:dslforum-org:service:DeviceConfig:1"},
    {"/serviceId", "urn:dslforum-org:serviceId:DeviceConfig"},
    {"/SCPDURL", DC_PATH},
    {"/controlURL", DC_CONTROLURL},
    {"/eventSubURL", ""},
    {"/serviceType", "urn:dslforum-org:service:LANConfigSecurity:1"},
    {"/serviceId", "urn:dslforum-org:serviceId:LANConfigSecurity"},
    {"/SCPDURL", LCS_PATH},
    {"/controlURL", LCS_CONTROLURL},
    {"/eventSubURL", ""},
    {0, 0}
};

#define MAX_SERVICE_ENTRIES_TO_FREE 256
void * serviceDataToFree[MAX_SERVICE_ENTRIES_TO_FREE] = { 0 };
int serviceDataToFreeIdx = 0;

static void addToServiceDataToFree(void* ptr) {
    if(serviceDataToFreeIdx >= MAX_SERVICE_ENTRIES_TO_FREE) {
        syslog(LOG_ERR, "Reached maximum capacity of entries to free, will leak memory");
        return;
    }
    serviceDataToFree[serviceDataToFreeIdx] = ptr;
    serviceDataToFreeIdx++;
}

static void cleanServiceDataToFree(void) {
    int idx;
    for(idx = 0; idx < serviceDataToFreeIdx; idx++) {
        free(serviceDataToFree[idx]);
        serviceDataToFree[idx] = 0;
    }
    serviceDataToFreeIdx = 0;
}

/* strcat_str()
 * concatenate the string and use realloc to increase the
 * memory buffer if needed. */
static char *
strcat_str(char * str, int * len, int * tmplen, const char * s2)
{
	int s2len;
	int newlen;
	char * p;

	s2len = (int)strlen(s2);
	if(*tmplen <= (*len + s2len))
	{
		if(s2len < 256)
			newlen = *tmplen + 256;
		else
			newlen = *tmplen + s2len + 1;
		p = (char *)realloc(str, newlen);
		if(p == NULL) /* handle a failure of realloc() */
			return str;
		str = p;
		*tmplen = newlen;
	}
	/*strcpy(str + *len, s2); */
	memcpy(str + *len, s2, s2len + 1);
	*len += s2len;
	return str;
}

/* strcat_char() :
 * concatenate a character and use realloc to increase the
 * size of the memory buffer if needed */
static char *
strcat_char(char * str, int * len, int * tmplen, char c)
{
	char * p;

	if(*tmplen <= (*len + 1))
	{
		*tmplen += 256;
		p = (char *)realloc(str, *tmplen);
		if(p == NULL) /* handle a failure of realloc() */
		{
			*tmplen -= 256;
			return str;
		}
		str = p;
	}
	str[*len] = c;
	(*len)++;
	return str;
}

/* strcat_int()
 * concatenate the string representation of the integer.
 * call strcat_char() */
static char *
strcat_int(char * str, int * len, int * tmplen, int i)
{
	char buf[16];
	int j;

	if(i < 0) {
		str = strcat_char(str, len, tmplen, '-');
		i = -i;
	} else if(i == 0) {
		/* special case for 0 */
		str = strcat_char(str, len, tmplen, '0');
		return str;
	}
	j = 0;
	while(i && j < (int)sizeof(buf)) {
		buf[j++] = '0' + (i % 10);
		i = i / 10;
	}
	while(j > 0) {
		str = strcat_char(str, len, tmplen, buf[--j]);
	}
	return str;
}

/* iterative subroutine using a small stack
 * This way, the progam stack usage is kept low */
static char *
genXML(char * str, int * len, int * tmplen,
                   const struct XMLElt * p)
{
	unsigned short i, j;
	unsigned long k;
	int top;
	const char * eltname, *s;
	char c;
	struct {
		unsigned short i;
		unsigned short j;
		const char * eltname;
	} pile[16]; /* stack */
	top = -1;
	i = 0;	/* current node */
	j = 1;	/* i + number of nodes*/
	for(;;)
	{
		eltname = p[i].eltname;
		if(!eltname)
			return str;
		if(eltname[0] == '/')
		{
			if(p[i].data)
			{
				/*printf("<%s>%s<%s>\n", eltname+1, p[i].data, eltname); */
				str = strcat_char(str, len, tmplen, '<');
				str = strcat_str(str, len, tmplen, eltname+1);
				str = strcat_char(str, len, tmplen, '>');
                if(p[i].data[0]) {
                    str = strcat_str(str, len, tmplen, p[i].data);
                }
				str = strcat_char(str, len, tmplen, '<');
				str = strcat_str(str, len, tmplen, eltname);
				str = strcat_char(str, len, tmplen, '>');
			}
			for(;;)
			{
				if(top < 0)
					return str;
				i = ++(pile[top].i);
				j = pile[top].j;
				/*printf("  pile[%d]\t%d %d\n", top, i, j); */
				if(i==j)
				{
					/*printf("</%s>\n", pile[top].eltname); */
					str = strcat_char(str, len, tmplen, '<');
					str = strcat_char(str, len, tmplen, '/');
					s = pile[top].eltname;
					for(c = *s; c > ' '; c = *(++s))
						str = strcat_char(str, len, tmplen, c);
					str = strcat_char(str, len, tmplen, '>');
					top--;
				}
				else
					break;
			}
		}
		else
		{
			/*printf("<%s>\n", eltname); */
			str = strcat_char(str, len, tmplen, '<');
			str = strcat_str(str, len, tmplen, eltname);
			str = strcat_char(str, len, tmplen, '>');
			k = (unsigned long)p[i].data;
			i = k & 0xffff;
			j = i + (k >> 16);
			top++;
			/*printf(" +pile[%d]\t%d %d\n", top, i, j); */
			pile[top].i = i;
			pile[top].j = j;
			pile[top].eltname = eltname;
		}
	}
}

void getinstances (const char* paramPath, const char* paramName, void *cookie) {
    struct instanceList* il = cookie;

    const char *path = paramPath;
    const char *instanceName;
    while(*path) {
        if(*path == '.') {
            instanceName = path;
        }
        path++;
    }

    if(il->num < 32) {
        il->instances[il->num] = atoi(instanceName+1);
        il->num++;
    } else {
        syslog(LOG_ERR, "Too many instances for %s", paramPath);
    }
}

static struct XMLElt lanDevTemplate[] = {
        {"/deviceType", DEVICE_TYPE_LAN},
        {"/friendlyName", LANDEV_FRIENDLYNAME},	/* required */
        {"/manufacturer", LANDEV_MANUFACTURER},	/* required */
        {"/manufacturerURL", LANDEV_MANUFACTURERURL},	/* optional */
        {"/modelDescription", LANDEV_MODELDESCRIPTION}, /* recommended */
        {"/modelName", LANDEV_MODELNAME},	/* required */
        {"/modelNumber", modelnumber},
        {"/modelURL", LANDEV_MODELURL},
        {"/serialNumber", serialnumber},
        // Will be completed in code
        {0, 0},
};

static struct XMLElt wanDevTemplate[] = {
        {"/deviceType", DEVICE_TYPE_WAN},
        {"/friendlyName", WANDEV_FRIENDLYNAME},	/* required */
        {"/manufacturer", WANDEV_MANUFACTURER},	/* required */
        {"/manufacturerURL", WANDEV_MANUFACTURERURL},	/* optional */
        {"/modelDescription", WANDEV_MODELDESCRIPTION}, /* recommended */
        {"/modelName", WANDEV_MODELNAME},	/* required */
        {"/modelNumber", modelnumber},
        {"/modelURL", WANDEV_MODELURL},
        {"/serialNumber", serialnumber},
        // Will be completed in code
        {0, 0},
};

static struct XMLElt wanCDevTemplate[] = {
        {"/deviceType", DEVICE_TYPE_WCD},
        {"/friendlyName", WANCDEV_FRIENDLYNAME},	/* required */
        {"/manufacturer", WANCDEV_MANUFACTURER},	/* required */
        {"/manufacturerURL", WANCDEV_MANUFACTURERURL},	/* optional */
        {"/modelDescription", WANCDEV_MODELDESCRIPTION}, /* recommended */
        {"/modelName", WANCDEV_MODELNAME},	/* required */
        {"/modelNumber", modelnumber},
        {"/modelURL", WANCDEV_MODELURL},
        {"/serialNumber", serialnumber},
        // Will be completed in code
        {0, 0},
};

void addServiceToRootDesc(struct XMLElt rootDesc[], int *serviceoffset, int *servicedataoffset, int instancenumber, const char* st, const char *si, const char *control, const char* scpd) {
    char *data;

    rootDesc[*serviceoffset].eltname = "service";
    rootDesc[*serviceoffset].data = INITHELPER(*servicedataoffset, 5);
    (*serviceoffset)++;
    rootDesc[*servicedataoffset].eltname = "/serviceType";
    rootDesc[*servicedataoffset].data = st;
    (*servicedataoffset)++;

    data = (char*) malloc(65);
    addToServiceDataToFree(data);
    if(instancenumber) {
        snprintf(data, 65, si, instancenumber);
    } else {
        snprintf(data,65, "%s", si);
    }
    rootDesc[*servicedataoffset].eltname = "/serviceId";
    rootDesc[*servicedataoffset].data = data;
    (*servicedataoffset)++;
    rootDesc[*servicedataoffset].eltname = "/SCPDURL";
    rootDesc[*servicedataoffset].data = scpd;
    (*servicedataoffset)++;

    data = (char*) malloc(65);
    addToServiceDataToFree(data);
    if(instancenumber) {
        snprintf(data, 65, control, instancenumber);
    } else {
        snprintf(data,65, "%s", control);
    }
    rootDesc[*servicedataoffset].eltname = "/controlURL";
    rootDesc[*servicedataoffset].data = data;
    (*servicedataoffset)++;
    rootDesc[*servicedataoffset].eltname = "/eventSubURL";
    rootDesc[*servicedataoffset].data = "";
    (*servicedataoffset)++;
}

typedef enum {
    UNKNOWN,
    DSL,
    ETHERNET,
} WANTypeEnum;

void getWANTypeCB(const char *path, const char *param, const char *value, void *cookie) {
    WANTypeEnum* type = cookie;

    if(strcmp(value, "DSL") == 0) {
        *type = DSL;
    } else if(strcmp(value, "Ethernet") == 0) {
        *type = ETHERNET;
    }
}

void addWANConnectionDeviceToWANDevice(struct XMLElt rootDesc[], int *devoffset, int *devdataoffset, struct instanceList wcdil, int wandevinstance, WANTypeEnum wantype) {
    char bufferQuery[128];
    char *queryStrWANIP =  "InternetGatewayDevice.WANDevice.%d.WANConnectionDevice.%d.WANIPConnection.";
    char *queryStrWANPPP = "InternetGatewayDevice.WANDevice.%d.WANConnectionDevice.%d.WANPPPConnection.";
    struct query_item queryInstances[] = {
            { .name = bufferQuery, .level = 1 },
            { .name = 0 }
    };
    int i;

    for(i=0;i<wcdil.num;i++) {
        int j = 0;
        int k;
        int initDevDataOffset;
        int serviceListOffset;
        int serviceOffset;
        int serviceDataOffset;
        int currentServiceOffset;
        int currentServiceDataOffset;

        rootDesc[*devoffset].eltname = "device";

        initDevDataOffset = *devdataoffset;
        while(lanDevTemplate[j].eltname) {
            rootDesc[*devdataoffset] = wanCDevTemplate[j];
            j++;
            (*devdataoffset)++;
        }

        // Add UIID
        rootDesc[*devdataoffset].eltname = "/UDN";
        rootDesc[*devdataoffset].data = uuidvalue_wcd; // TODO: use libuuid to generate a unique one? or is it supposed to be the same for all devices of the same type
        (*devdataoffset)++;

        // Add serviceList
        rootDesc[*devdataoffset].eltname = "serviceList";
        // Store serviceList offset
        serviceListOffset = *devdataoffset;
        (*devdataoffset)++;

        // Store lenght of device
        rootDesc[*devoffset].data = INITHELPER(initDevDataOffset, *devdataoffset-initDevDataOffset);

        // Store first service offset
        serviceOffset = *devdataoffset;
        currentServiceOffset = serviceOffset;

        // Count number of services
        struct instanceList wanip = { 0 };
        struct instanceList wanppp = { 0 };
        int countServices;

        snprintf(bufferQuery, sizeof(bufferQuery), queryStrWANIP, wandevinstance, wcdil.instances[i]);
        getpn(queryInstances, getinstances, NULL, &wanip);
        snprintf(bufferQuery, sizeof(bufferQuery), queryStrWANPPP, wandevinstance, wcdil.instances[i]);
        getpn(queryInstances, getinstances, NULL, &wanppp);
        countServices = wanip.num + wanppp.num;
        if(wantype != UNKNOWN) {
            countServices++;
        }
        serviceDataOffset = serviceOffset + countServices;
        currentServiceDataOffset = serviceDataOffset;

        // Add the service
        char baseid[65] = {}; // 64 char + trailing 0
        char controlurl[65] = {};
        char *idtemplatewdlc = "urn:dslforum-org:serviceId:wdlc-%d-%d";
        char *idtemplatewelc = "urn:dslforum-org:serviceId:welc-%d-%d";
        char *idtemplatewip = "urn:dslforum-org:serviceId:wip-%d-%d-%%d";
        char *idtemplatewppp = "urn:dslforum-org:serviceId:wppp-%d-%d-%%d";

        if(wantype == DSL) {
            snprintf(controlurl, sizeof(controlurl), WDLC_CONTROLURL, wandevinstance, wcdil.instances[i]);
            snprintf(baseid, sizeof(baseid), idtemplatewdlc, wandevinstance, wcdil.instances[i]);
            addServiceToRootDesc(rootDesc, &currentServiceOffset, &currentServiceDataOffset, 0, SERVICE_TYPE_WDLC, baseid, controlurl, WDLC_PATH);
        } else if(wantype == ETHERNET) {
            snprintf(controlurl, sizeof(controlurl), WELC_CONTROLURL, wandevinstance, wcdil.instances[i]);
            snprintf(baseid, sizeof(baseid), idtemplatewelc, wandevinstance, wcdil.instances[i]);
            addServiceToRootDesc(rootDesc, &currentServiceOffset, &currentServiceDataOffset, 0, SERVICE_TYPE_WELC, baseid, controlurl, WELC_PATH);
        }

        for(k=0;k<wanppp.num;k++) {
            snprintf(controlurl, sizeof(controlurl), WANPPP_CONTROLURL, wandevinstance, wcdil.instances[i]);
            snprintf(baseid, sizeof(baseid), idtemplatewppp, wandevinstance, wcdil.instances[i]);
            addServiceToRootDesc(rootDesc, &currentServiceOffset, &currentServiceDataOffset, wanppp.instances[k], SERVICE_TYPE_WANPPP, baseid, controlurl, WANPPP_PATH);
        }

        for(k=0;k<wanip.num;k++) {
            snprintf(controlurl, sizeof(controlurl), WANIP_CONTROLURL, wandevinstance, wcdil.instances[i]);
            snprintf(baseid, sizeof(baseid), idtemplatewip, wandevinstance, wcdil.instances[i]);
            addServiceToRootDesc(rootDesc, &currentServiceOffset, &currentServiceDataOffset, wanip.instances[k], SERVICE_TYPE_WANIP, baseid, controlurl, WANIP_PATH);
        }
        rootDesc[serviceListOffset].data = INITHELPER(serviceOffset, countServices);
        *devdataoffset = currentServiceDataOffset;

        (*devoffset)++;
    }
}

void addWANDevicesToRootDesc(struct XMLElt rootDesc[], int *devoffset, int *devdataoffset, struct instanceList wanil) {
    char bufferQuery[128];
    char *queryStrWANType = "InternetGatewayDevice.WANDevice.%d.WANCommonInterfaceConfig.WANAccessType";
    char *queryStrWCD = "InternetGatewayDevice.WANDevice.%d.WANConnectionDevice.";
    struct query_item queryWANType[] = {
            { .name = bufferQuery },
            { .name = 0 }
    };
    struct query_item queryWCD[] = {
            { .name = bufferQuery, .level = 1 },
            { .name = 0 }
    };
    int i;
    for(i=0;i<wanil.num;i++) {

        int j = 0;
        int k;
        int initDevDataOffset;
        int serviceListOffset;
        int serviceOffset;
        int serviceDataOffset;
        int currentServiceOffset;
        int currentServiceDataOffset;
        int subDeviceListOffset;
        int subDeviceOffset;
        int subDeviceDataOffset;
        int currentSubDeviceOffset;
        int currentSubDeviceDataOffset;

        rootDesc[*devoffset].eltname = "device";

        initDevDataOffset = *devdataoffset;
        while(wanDevTemplate[j].eltname) {
            rootDesc[*devdataoffset] = wanDevTemplate[j];
            j++;
            (*devdataoffset)++;
        }

        // Add UIID
        rootDesc[*devdataoffset].eltname = "/UDN";
        rootDesc[*devdataoffset].data = uuidvalue_wan; // TODO: use libuuid to generate a unique one? or is it supposed to be the same for all devices of the same type
        (*devdataoffset)++;

        // Add serviceList
        rootDesc[*devdataoffset].eltname = "serviceList";
        // Store serviceList offset
        serviceListOffset = *devdataoffset;
        (*devdataoffset)++;

        // Get number of WANConnectionDevice
        struct instanceList wcd = { 0 };
        snprintf(bufferQuery, sizeof(bufferQuery), queryStrWCD, wanil.instances[i]);
        getpn(queryWCD, getinstances, NULL, &wcd);

        // Add deviceList
        if(wcd.num > 0) {
            rootDesc[*devdataoffset].eltname = "deviceList";
            // Store deviceList offset
            subDeviceListOffset = *devdataoffset;
            (*devdataoffset)++;
        }

        // Store lenght of WANDevice
        rootDesc[*devoffset].data = INITHELPER(initDevDataOffset, *devdataoffset-initDevDataOffset);

        // Store first service offset
        serviceOffset = *devdataoffset;
        currentServiceOffset = serviceOffset;

        // Count number of services
        WANTypeEnum wantype = UNKNOWN;
        int countServices;
        countServices = 1;

        snprintf(bufferQuery, sizeof(bufferQuery), queryStrWANType, wanil.instances[i]);
        getpv(queryWANType, &getWANTypeCB, 0, &wantype);
        if(wantype != UNKNOWN) {
            countServices++;
        }

        serviceDataOffset = serviceOffset + countServices;
        currentServiceDataOffset = serviceDataOffset;

        // Add the service
        char baseid[65] = {}; // 64 char + trailing 0
        char controlurl[65] = {};
        char *idtemplatewcic = "urn:dslforum-org:serviceId:wcic-%d";
        char *idtemplatewdic = "urn:dslforum-org:serviceId:wdic-%d";
        char *idtemplateweic = "urn:dslforum-org:serviceId:weic-%d";


        snprintf(controlurl, sizeof(controlurl), WCIC_CONTROLURL, wanil.instances[i]);
        snprintf(baseid, sizeof(baseid), idtemplatewcic, wanil.instances[i]);
        addServiceToRootDesc(rootDesc, &currentServiceOffset, &currentServiceDataOffset, 0, SERVICE_TYPE_WCIC, baseid, controlurl, WCIC_PATH);

        if(wantype == DSL) {
            snprintf(controlurl, sizeof(controlurl), WDIC_CONTROLURL, wanil.instances[i]);
            snprintf(baseid, sizeof(baseid), idtemplatewdic, wanil.instances[i]);
            addServiceToRootDesc(rootDesc, &currentServiceOffset, &currentServiceDataOffset, 0, SERVICE_TYPE_WDIC, baseid, controlurl, WDIC_PATH);
        } else if(wantype == ETHERNET) {
            snprintf(controlurl, sizeof(controlurl), WEIC_CONTROLURL, wanil.instances[i]);
            snprintf(baseid, sizeof(baseid), idtemplateweic, wanil.instances[i]);
            addServiceToRootDesc(rootDesc, &currentServiceOffset, &currentServiceDataOffset, 0, SERVICE_TYPE_WEIC, baseid, controlurl, WEIC_PATH);
        }

        rootDesc[serviceListOffset].data = INITHELPER(serviceOffset, countServices);
        *devdataoffset = currentServiceDataOffset;

        // Add the WANConnectionDevices
        if(wcd.num > 0) {
            // Store first service offset
            subDeviceOffset = *devdataoffset;
            currentSubDeviceOffset = subDeviceOffset;
            subDeviceDataOffset = subDeviceOffset + wcd.num;
            currentSubDeviceDataOffset = subDeviceDataOffset;

            addWANConnectionDeviceToWANDevice(rootDesc, &currentSubDeviceOffset, &currentSubDeviceDataOffset, wcd, wanil.instances[i], wantype);

            rootDesc[subDeviceListOffset].data = INITHELPER(subDeviceOffset, wcd.num);
            *devdataoffset = currentSubDeviceDataOffset;

        }

        (*devoffset)++;
    }
}

void addLANDevicesToRootDesc(struct XMLElt rootDesc[], int *devoffset, int *devdataoffset, struct instanceList lanil) {
    char bufferQuery[64];
    char *queryStrEth = "InternetGatewayDevice.LANDevice.%d.LANEthernetInterfaceConfig.";
    char *queryStrWLAN = "InternetGatewayDevice.LANDevice.%d.WLANConfiguration.";
    struct query_item queryInstances[] = {
            { .name = bufferQuery, .level = 1 },
            { .name = 0 }
    };
    int i;

    for(i=0;i<lanil.num;i++) {
        int j = 0;
        int k;
        int initDevDataOffset;
        int serviceListOffset;
        int serviceOffset;
        int serviceDataOffset;
        int currentServiceOffset;
        int currentServiceDataOffset;

        rootDesc[*devoffset].eltname = "device";

        initDevDataOffset = *devdataoffset;
        while(lanDevTemplate[j].eltname) {
            rootDesc[*devdataoffset] = lanDevTemplate[j];
            j++;
            (*devdataoffset)++;
        }

        // Add UIID
        rootDesc[*devdataoffset].eltname = "/UDN";
        rootDesc[*devdataoffset].data = uuidvalue_lan; // TODO: use libuuid to generate a unique one? or is it supposed to be the same for all devices of the same type
        (*devdataoffset)++;

        // Add serviceList
        rootDesc[*devdataoffset].eltname = "serviceList";
        // Store serviceList offset
        serviceListOffset = *devdataoffset;
        (*devdataoffset)++;

        // Store lenght of device
        rootDesc[*devoffset].data = INITHELPER(initDevDataOffset, *devdataoffset-initDevDataOffset);

        // Store first service offset
        serviceOffset = *devdataoffset;
        currentServiceOffset = serviceOffset;

        // Count number of services
        struct instanceList eth = { 0 };
        struct instanceList wlan = { 0 };
        int countServices;

        snprintf(bufferQuery, sizeof(bufferQuery), queryStrEth, lanil.instances[i]);
        getpn(queryInstances, getinstances, NULL, &eth);
        snprintf(bufferQuery, sizeof(bufferQuery), queryStrWLAN, lanil.instances[i]);
        getpn(queryInstances, getinstances, NULL, &wlan);
        countServices = eth.num + wlan.num + 1;
        serviceDataOffset = serviceOffset + countServices;
        currentServiceDataOffset = serviceDataOffset;

        // Add the service
        char baseid[65] = {}; // 64 char + trailing 0
        char controlurl[65] = {};
        char *idtemplatelhc = "urn:dslforum-org:serviceId:lhc-%d";
        char *idtemplateeth = "urn:dslforum-org:serviceId:leic-%d-%%d";
        char *idtemplatewlan = "urn:dslforum-org:serviceId:wlanc-%d-%%d";


        snprintf(controlurl, sizeof(controlurl), LHC_CONTROLURL, lanil.instances[i]);
        snprintf(baseid, sizeof(baseid), idtemplatelhc, lanil.instances[i]);
        addServiceToRootDesc(rootDesc, &currentServiceOffset, &currentServiceDataOffset, 0, SERVICE_TYPE_LHC, baseid, controlurl, LHC_PATH);

        for(k=0;k<eth.num;k++) {
            snprintf(controlurl, sizeof(controlurl), LEIC_CONTROLURL, lanil.instances[i]);
            snprintf(baseid, sizeof(baseid), idtemplateeth, lanil.instances[i]);
            addServiceToRootDesc(rootDesc, &currentServiceOffset, &currentServiceDataOffset, eth.instances[k], SERVICE_TYPE_LEIC, baseid, controlurl, LEIC_PATH);
        }

        for(k=0;k<wlan.num;k++) {
            snprintf(controlurl, sizeof(controlurl), WLANC_CONTROLURL, lanil.instances[i]);
            snprintf(baseid, sizeof(baseid), idtemplatewlan, lanil.instances[i]);
            addServiceToRootDesc(rootDesc, &currentServiceOffset, &currentServiceDataOffset, wlan.instances[k], SERVICE_TYPE_WLAN, baseid, controlurl, WLANC_PATH);
        }
        rootDesc[serviceListOffset].data = INITHELPER(serviceOffset, countServices);
        (*devoffset)++;
        *devdataoffset = currentServiceDataOffset;
    }
}

void addDevicesToRootDesc(struct XMLElt rootDesc[])
{
    struct query_item querylan[] = {
            { .name = "InternetGatewayDevice.LANDevice.", .level = 1 },
            { .name = 0 }
    };
    struct query_item querywan[] = {
            { .name = "InternetGatewayDevice.WANDevice.", .level = 1 },
            { .name = 0 }
    };
    int count = 0;
    int offsetdev;
    int offsetdevdata;

    struct instanceList lan = { .num = 0, .instances = { 0 }};
    struct instanceList wan = { .num = 0, .instances = { 0 }};

    getpn(querylan, getinstances, NULL, &lan);
    getpn(querywan, getinstances, NULL, &wan);
    count = lan.num + wan.num;

    if(count != 0) {
        rootDesc[16].eltname = "deviceList";
        rootDesc[16].data = INITHELPER(OFFSET_ROOTDEVICE,count);
    }
    // Initialize the deviceList node
    offsetdev = OFFSET_ROOTDEVICE;
    offsetdevdata = OFFSET_ROOTDEVICE + count;
    addLANDevicesToRootDesc(rootDesc, &offsetdev, &offsetdevdata, lan);
    addWANDevicesToRootDesc(rootDesc, &offsetdev, &offsetdevdata, wan);
}

/* genRootDesc() :
 * - Generate the root description of the UPnP device.
 * - the len argument is used to return the length of
 *   the returned string.
 * - tmp_uuid argument is used to build the uuid string */
char *
genRootDesc(int * len, struct upnphttp * h)
{
	char * str;
	int tmplen;
	tmplen = 32768;
	str = (char *)malloc(tmplen);
	if(str == NULL)
		return NULL;
	* len = strlen(xmlver);
	/*strcpy(str, xmlver); */
	memcpy(str, xmlver, *len + 1);

	/* Set LCS control URL to https server on GW IP used by client */
	char *lcsControl = malloc(128);
	struct sockaddr *peer;
	struct lan_addr_s * server;
	struct sockaddr_in6 clientv6;
	struct sockaddr_in client;
	addToServiceDataToFree(lcsControl);

	/* Find IP that the client is connnected to */
#ifdef ENABLE_IPV6
	if(h->ipv6) {
		clientv6.sin6_family = AF_INET6;
		clientv6.sin6_addr = h->clientaddr_v6;
		peer = (struct sockaddr*) &clientv6;
	} else {
#endif
		client.sin_family = AF_INET;
		client.sin_addr = h->clientaddr;
		peer = (struct sockaddr*) &client;
#ifdef ENABLE_IPV6
	}
#endif
	server = get_lan_for_peer(peer);

	if(server) {
		snprintf(lcsControl, 128, "https://%s:%hu%s", server->str, rv.https_port, LCS_CONTROLURL);
		lcsControl[127] = '\0';
		rootDesc[OFFSET_ROOTSERVICE + NUM_ROOTSERVICES + 13].data = lcsControl;
	} else {
		rootDesc[OFFSET_ROOTSERVICE + NUM_ROOTSERVICES + 13].data = LCS_CONTROLURL; // Fallback to just relative url over http
	}
    addDevicesToRootDesc(rootDesc);

	str = genXML(str, len, &tmplen, rootDesc);

    cleanServiceDataToFree();

	str[*len] = '\0';
	return str;
}

/* genServiceDesc() :
 * Generate service description with allowed methods and
 * related variables. */
static char *
genServiceDesc(int * len, const struct serviceDesc * s)
{
	int i, j;
	const struct action * acts;
	const struct stateVar * vars;
	const struct argument * args;
	const char * p;
	char * str;
	int tmplen;
	tmplen = 2048;
	str = (char *)malloc(tmplen);
	if(str == NULL)
		return NULL;
	/*strcpy(str, xmlver); */
	*len = strlen(xmlver);
	memcpy(str, xmlver, *len + 1);

	acts = s->actionList;
	vars = s->serviceStateTable;

	str = strcat_char(str, len, &tmplen, '<');
	str = strcat_str(str, len, &tmplen, root_service);
	str = strcat_char(str, len, &tmplen, '>');

	str = strcat_str(str, len, &tmplen,
		"<specVersion><major>1</major><minor>0</minor></specVersion>");

	i = 0;
	str = strcat_str(str, len, &tmplen, "<actionList>");
	while(acts[i].name)
	{
		str = strcat_str(str, len, &tmplen, "<action><name>");
		str = strcat_str(str, len, &tmplen, acts[i].name);
		str = strcat_str(str, len, &tmplen, "</name>");
		/* argument List */
		args = acts[i].args;
		if(args)
		{
			str = strcat_str(str, len, &tmplen, "<argumentList>");
			j = 0;
			while(args[j].dir)
			{
				str = strcat_str(str, len, &tmplen, "<argument><name>");
				if((args[j].dir & 0x80) == 0) {
					str = strcat_str(str, len, &tmplen, "New");
				}
				p = vars[args[j].relatedVar].name;
				if(args[j].tr064name) {
					/* use magic values ... */
					str = strcat_str(str, len, &tmplen, args[j].tr064name);
				} else {
					str = strcat_str(str, len, &tmplen, p);
				}
				str = strcat_str(str, len, &tmplen, "</name><direction>");
				str = strcat_str(str, len, &tmplen, (args[j].dir&0x03)==1?"in":"out");
				str = strcat_str(str, len, &tmplen,
						"</direction><relatedStateVariable>");
				str = strcat_str(str, len, &tmplen, p);
				str = strcat_str(str, len, &tmplen,
						"</relatedStateVariable></argument>");
				j++;
			}
			str = strcat_str(str, len, &tmplen,"</argumentList>");
		}
		str = strcat_str(str, len, &tmplen, "</action>");
		/*str = strcat_char(str, len, &tmplen, '\n'); // TEMP ! */
		i++;
	}
	str = strcat_str(str, len, &tmplen, "</actionList><serviceStateTable>");
	i = 0;
	while(vars[i].name)
	{
		str = strcat_str(str, len, &tmplen,
				"<stateVariable sendEvents=\"");
		/* for the moment always send no. Wait for SUBSCRIBE implementation
		 * before setting it to yes */
		str = strcat_str(str, len, &tmplen, "no");
		str = strcat_str(str, len, &tmplen, "\"><name>");
		str = strcat_str(str, len, &tmplen, vars[i].name);
		str = strcat_str(str, len, &tmplen, "</name><dataType>");
		str = strcat_str(str, len, &tmplen, upnptypes[vars[i].itype & 0x0f]);
		str = strcat_str(str, len, &tmplen, "</dataType>");
		if(vars[i].iallowedlist)
		{
		  if((vars[i].itype & 0x0f) == 0)
		  {
		    /* string */
		    str = strcat_str(str, len, &tmplen, "<allowedValueList>");
		    for(j=vars[i].iallowedlist; upnpallowedvalues[j]; j++)
		    {
		      str = strcat_str(str, len, &tmplen, "<allowedValue>");
		      str = strcat_str(str, len, &tmplen, upnpallowedvalues[j]);
		      str = strcat_str(str, len, &tmplen, "</allowedValue>");
		    }
		    str = strcat_str(str, len, &tmplen, "</allowedValueList>");
		  } else {
		    /* ui2 and ui4 */
		    str = strcat_str(str, len, &tmplen, "<allowedValueRange><minimum>");
			str = strcat_int(str, len, &tmplen, upnpallowedranges[vars[i].iallowedlist]);
		    str = strcat_str(str, len, &tmplen, "</minimum><maximum>");
			str = strcat_int(str, len, &tmplen, upnpallowedranges[vars[i].iallowedlist+1]);
		    str = strcat_str(str, len, &tmplen, "</maximum></allowedValueRange>");
		  }
		}
		/*if(vars[i].defaultValue) */
		if(vars[i].idefault)
		{
		  str = strcat_str(str, len, &tmplen, "<defaultValue>");
		  /*str = strcat_str(str, len, &tmplen, vars[i].defaultValue); */
		  str = strcat_str(str, len, &tmplen, upnpdefaultvalues[vars[i].idefault]);
		  str = strcat_str(str, len, &tmplen, "</defaultValue>");
		}
		str = strcat_str(str, len, &tmplen, "</stateVariable>");
		/*str = strcat_char(str, len, &tmplen, '\n'); // TEMP ! */
		i++;
	}
	str = strcat_str(str, len, &tmplen, "</serviceStateTable></scpd>");
	str[*len] = '\0';
	return str;
}

char * genDI(int * len, struct upnphttp * h)
{
    return genServiceDesc(len, &scpdDI);
}

char * genDC(int * len, struct upnphttp * h)
{
    return genServiceDesc(len, &scpdDC);
}

char * genLCS(int * len, struct upnphttp * h)
{
    return genServiceDesc(len, &scpdLCS);
}

char * genLHC(int * len, struct upnphttp * h)
{
    return genServiceDesc(len, &scpdLHC);
}

char * genLEIC(int * len, struct upnphttp * h)
{
    return genServiceDesc(len, &scpdLEIC);
}

char * genWLANC(int * len, struct upnphttp * h)
{
    return genServiceDesc(len, &scpdWLANC);
}

char * genWCIC(int * len, struct upnphttp * h)
{
    return genServiceDesc(len, &scpdWCIC);
}

char * genWDIC(int * len, struct upnphttp * h)
{
    return genServiceDesc(len, &scpdWDIC);
}

char * genWEIC(int * len, struct upnphttp * h)
{
    return genServiceDesc(len, &scpdWEIC);
}

char * genWDLC(int * len, struct upnphttp * h)
{
    return genServiceDesc(len, &scpdWDLC);
}

char * genWELC(int * len, struct upnphttp * h)
{
    return genServiceDesc(len, &scpdWELC);
}

char * genWANPPP(int * len, struct upnphttp * h)
{
    return genServiceDesc(len, &scpdWANPPP);
}

char * genWANIP(int * len, struct upnphttp * h)
{
    return genServiceDesc(len, &scpdWANIP);
}
