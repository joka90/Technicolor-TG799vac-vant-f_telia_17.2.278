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
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "macros.h"
#include "config.h"
#include "upnpglobalvars.h"
#include "upnphttp.h"
#include "upnpsoap.h"
#include "upnpreplyparse.h"
#include "upnpurns.h"
#include "soapmethods/methods.h"
#include "transformer/helper.h"

enum accessType {
    HTTP_AND_HTTPS,
    HTTPS_ONLY
};

enum authType {
    PUBLIC,
    AUTH,
    AUTH_NOAUTHINFACTORY
};

static const struct soapMethodDef
{
    const char * urn;
	const char * methodName;
    enum accessType httpsonly;
    enum authType access;
	void (*methodImpl)(struct upnphttp *, const char *);
}

soapMethods[] =
{
        { SERVICE_TYPE_LCS, "SetConfigPassword", HTTPS_ONLY, AUTH_NOAUTHINFACTORY, LCSSetConfigPassword },
        { SERVICE_TYPE_DI, "GetInfo", HTTP_AND_HTTPS, PUBLIC, DIGetInfo },
        { SERVICE_TYPE_DI, "GetDeviceLog", HTTP_AND_HTTPS, AUTH, DIGetDeviceLog },
        { SERVICE_TYPE_DI, "SetProvisioningCode", HTTP_AND_HTTPS, AUTH, DISetProvisioningCode },
        { SERVICE_TYPE_DC, "ConfigurationStarted", HTTP_AND_HTTPS, AUTH, DCConfigurationStarted },
        { SERVICE_TYPE_DC, "ConfigurationFinished", HTTP_AND_HTTPS, AUTH, DCConfigurationFinished },
        { SERVICE_TYPE_DC, "FactoryReset", HTTP_AND_HTTPS, AUTH, DCFactoryReset },
        { SERVICE_TYPE_DC, "Reboot", HTTP_AND_HTTPS, AUTH, DCReboot },
        { SERVICE_TYPE_LEIC, "GetInfo", HTTP_AND_HTTPS, PUBLIC, LEICGetInfo },
        { SERVICE_TYPE_WLAN, "GetInfo", HTTP_AND_HTTPS, PUBLIC, WLANCGetInfo },
        { SERVICE_TYPE_WLAN, "GetSSID", HTTP_AND_HTTPS, PUBLIC, WLANCGetSSID },
        { SERVICE_TYPE_WLAN, "SetSSID", HTTP_AND_HTTPS, AUTH, WLANCSetSSID },
        { SERVICE_TYPE_WLAN, "GetRadioMode", HTTP_AND_HTTPS, PUBLIC, WLANCGetRadioMode },
        { SERVICE_TYPE_WLAN, "SetRadioMode", HTTP_AND_HTTPS, AUTH, WLANCSetRadioMode },
        { SERVICE_TYPE_WLAN, "GetBeaconAdvertisement", HTTP_AND_HTTPS, PUBLIC, WLANCGetBeaconAdvertisement },
        { SERVICE_TYPE_WLAN, "GetChannelInfo", HTTP_AND_HTTPS, PUBLIC, WLANCGetChannelInfo },
        { SERVICE_TYPE_WLAN, "GetSecurityKeys", HTTP_AND_HTTPS, PUBLIC, WLANCGetSecurityKeys },
        { SERVICE_TYPE_WLAN, "GetBasBeaconSecurityProperties", HTTP_AND_HTTPS, PUBLIC, WLANCGetBasBeaconSecurityProperties },
        { SERVICE_TYPE_WLAN, "GetWPABeaconSecurityProperties", HTTP_AND_HTTPS, PUBLIC, WLANCGetWPABeaconSecurityProperties },
        { SERVICE_TYPE_WLAN, "Get11iBeaconSecurityProperties", HTTP_AND_HTTPS, PUBLIC, WLANCGet11iBeaconSecurityProperties },
        { SERVICE_TYPE_WLAN, "SetEnable", HTTP_AND_HTTPS, AUTH, WLANCSetEnable },
        { SERVICE_TYPE_WLAN, "SetBeaconAdvertisement", HTTP_AND_HTTPS, AUTH, WLANCSetBeaconAdvertisement },
        { SERVICE_TYPE_WLAN, "SetChannel", HTTP_AND_HTTPS, AUTH, WLANCSetChannel },
        { SERVICE_TYPE_WLAN, "SetSecurityKeys", HTTP_AND_HTTPS, AUTH, WLANCSetSecurityKeys },
        { SERVICE_TYPE_WLAN, "SetBeaconType", HTTP_AND_HTTPS, AUTH, WLANCSetBeaconType },
        { SERVICE_TYPE_WLAN, "GetBeaconType", HTTP_AND_HTTPS, PUBLIC, WLANCGetBeaconType },
        { SERVICE_TYPE_WEIC, "GetInfo", HTTP_AND_HTTPS, PUBLIC, WEICGetInfo },
        { SERVICE_TYPE_WDIC, "GetInfo", HTTP_AND_HTTPS, PUBLIC, WDICGetInfo },
        { SERVICE_TYPE_WCIC, "GetCommonLinkProperties", HTTP_AND_HTTPS, PUBLIC, WCICGetCommonLinkProperties },
        { SERVICE_TYPE_WDLC, "GetInfo", HTTP_AND_HTTPS, PUBLIC, WDLCGetInfo },
        { SERVICE_TYPE_WDLC, "GetDSLLinkInfo", HTTP_AND_HTTPS, PUBLIC, WDLCGetDSLLinkInfo },
        { SERVICE_TYPE_WDLC, "GetDestinationAddress", HTTP_AND_HTTPS, PUBLIC, WDLCGetDestinationAddress },
        { SERVICE_TYPE_WDLC, "GetATMEncapsulation", HTTP_AND_HTTPS, PUBLIC, WDLCGetATMEncapsulation },
        { SERVICE_TYPE_WDLC, "SetDestinationAddress", HTTP_AND_HTTPS, PUBLIC, WDLCSetDestinationAddress },
        { SERVICE_TYPE_WANPPP, "GetUserName", HTTP_AND_HTTPS, PUBLIC, WANPPPGetUserName },
        { SERVICE_TYPE_WANPPP, "GetInfo", HTTP_AND_HTTPS, PUBLIC, WANPPPGetInfo },
        { SERVICE_TYPE_WANPPP, "GetConnectionTypeInfo", HTTP_AND_HTTPS, PUBLIC, WANPPPGetConnectionTypeInfo },
        { SERVICE_TYPE_WANPPP, "GetExternalIPAddress", HTTP_AND_HTTPS, PUBLIC, WANPPPGetExternalIPAddress },
        { SERVICE_TYPE_WANPPP, "GetStatusInfo", HTTP_AND_HTTPS, PUBLIC, WANPPPGetStatusInfo },
        { SERVICE_TYPE_WANPPP, "SetUserName", HTTP_AND_HTTPS, AUTH, WANPPPSetUserName },
        { SERVICE_TYPE_WANPPP, "SetPassword", HTTP_AND_HTTPS, AUTH, WANPPPSetPassword },
        { SERVICE_TYPE_WANPPP, "RequestConnection", HTTP_AND_HTTPS, AUTH, WANPPPRequestConnection },
        { SERVICE_TYPE_WANPPP, "ForceTermination", HTTP_AND_HTTPS, AUTH, WANPPPForceTermination },
        { SERVICE_TYPE_WANIP, "GetInfo", HTTP_AND_HTTPS, PUBLIC, WANIPGetInfo },
        { SERVICE_TYPE_WANIP, "GetConnectionTypeInfo", HTTP_AND_HTTPS, PUBLIC, WANIPGetConnectionTypeInfo },
        { SERVICE_TYPE_WANIP, "GetExternalIPAddress", HTTP_AND_HTTPS, PUBLIC, WANIPGetExternalIPAddress },
        { SERVICE_TYPE_WANIP, "GetStatusInfo", HTTP_AND_HTTPS, PUBLIC, WANIPGetStatusInfo },
        { 0, 0, HTTP_AND_HTTPS, PUBLIC, 0 }
};

void
ExecuteSoapAction(struct upnphttp * h, const char * action, int n)
{
	const char sid_pattern[] = "SessionID%*[^>]>%36[^<]</";
	char session_id[37] = "";
	char * sid_offset;
	char * p;
	char * p2;
	int i, len, ulen, methodlen, urnlen;
	struct timespec current_time;

	i = 0;
	p = strchr(action, '#');

	if(p)
	{
        urnlen = p - action;
		p++;
		p2 = strchr(p, '"');
		if(p2) {
            methodlen = p2 - p;
        } else {
            methodlen = n - (p - action);
        }
		/*syslog(LOG_DEBUG, "SoapMethod: %.*s", methodlen, p);*/
		while(soapMethods[i].methodName)
		{
			len = strlen(soapMethods[i].methodName);
            ulen = strlen(soapMethods[i].urn);

			if((ulen == urnlen) && memcmp(action, soapMethods[i].urn, ulen) == 0 && (len == methodlen) && memcmp(p, soapMethods[i].methodName, len) == 0)
			{
#ifdef DEBUG
				syslog(LOG_DEBUG, "Remote Call of SoapMethod '%s'\n",
				       soapMethods[i].methodName);
#endif

                if(soapMethods[i].httpsonly == HTTPS_ONLY && h->ssl == NULL) {
                    syslog(LOG_ERR, "Trying to access %s method over plain HTTP while it's HTTPS only", soapMethods[i].methodName);
                    SoapError(h, 501, "Action only authorized over HTTPS");
                    return;
                }
                if(soapMethods[i].access == AUTH) {
                    if(password_dsl_config[0] == '\0') {
                        syslog(LOG_ERR, "Trying to access %s method while in factory mode", soapMethods[i].methodName);
                        SoapError(h, 501, "Action only authorized in normal mode, not in factory mode");
                        return;
                    }
                    if(h->authState != DSLConfig) {
                        syslog(LOG_DEBUG, "Incorrect user for %s, 401 returned", soapMethods[i].methodName);
                        Send401(h);
                        return;
                    }
                }
                if(soapMethods[i].access == AUTH_NOAUTHINFACTORY && password_dsl_config[0] != '\0') {
                    if(h->authState != DSLConfig && h->authState != DSLReset) {
                        syslog(LOG_DEBUG, "Incorrect user for %s, 401 returned", soapMethods[i].methodName);
                        Send401(h);
                        return;
                    }
                }

                sid_offset = strstr(h->req_buf + h->req_contentoff, "SessionID");
                if(sid_offset) {
                    sscanf(sid_offset, sid_pattern, session_id);
                }

                if(strcmp(config_session_id, session_id)) {
                    SoapError(h, 899, "Session ID is invalid or has expired");
                    return;
                }

                clock_gettime(CLOCK_MONOTONIC, &config_session_lastupdate);

				soapMethods[i].methodImpl(h, soapMethods[i].methodName);

				// If current session id not set and action name starts with Set Then
				//   Transformer apply (for SessionId set, apply triggered by ConfigurationFinished)
				// EndIf
				if(config_session_id[0] == '\0' && (!memcmp(soapMethods[i].methodName, "Set", 3)
					|| !memcmp(soapMethods[i].methodName, "Reboot", 6)
					|| !memcmp(soapMethods[i].methodName, "FactoryReset", 12))) {
					apply();
				}
				return;
			}
			i++;
		}

		syslog(LOG_NOTICE, "SoapMethod: Unknown: %.*s%.*s", urnlen, action, methodlen, p);
	}

	SoapError(h, 401, "Invalid Action");
}

/* Standard Errors:
 *
 * errorCode errorDescription Description
 * --------	---------------- -----------
 * 401 		Invalid Action 	No action by that name at this service.
 * 402 		Invalid Args 	Could be any of the following: not enough in args,
 * 							too many in args, no in arg by that name,
 * 							one or more in args are of the wrong data type.
 * 403 		Out of Sync 	Out of synchronization.
 * 501 		Action Failed 	May be returned in current state of service
 * 							prevents invoking that action.
 * 600-699 	TBD 			Common action errors. Defined by UPnP Forum
 * 							Technical Committee.
 * 700-799 	TBD 			Action-specific errors for standard actions.
 * 							Defined by UPnP Forum working committee.
 * 800-899 	TBD 			Action-specific errors for non-standard actions.
 * 							Defined by UPnP vendor.
*/
void
SoapError(struct upnphttp * h, int errCode, const char * errDesc)
{
	static const char resp[] =
		"<s:Envelope "
		"xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
		"s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
		"<s:Body>"
		"<s:Fault>"
		"<faultcode>s:Client</faultcode>"
		"<faultstring>UPnPError</faultstring>"
		"<detail>"
		"<UPnPError xmlns=\"urn:dslforum-org:control-1-0\">"
		"<errorCode>%d</errorCode>"
		"<errorDescription>%s</errorDescription>"
		"</UPnPError>"
		"</detail>"
		"</s:Fault>"
		"</s:Body>"
		"</s:Envelope>";

	char body[2048];
	int bodylen;

	syslog(LOG_INFO, "Returning UPnPError %d: %s", errCode, errDesc);
	bodylen = snprintf(body, sizeof(body), resp, errCode, errDesc);
	BuildResp2_upnphttp(h, 500, "Internal Server Error", body, bodylen);
	SendRespAndClose_upnphttp(h);
}

