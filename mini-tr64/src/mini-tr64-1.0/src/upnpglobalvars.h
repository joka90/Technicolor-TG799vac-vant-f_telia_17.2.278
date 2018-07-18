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

#ifndef UPNPGLOBALVARS_H_INCLUDED
#define UPNPGLOBALVARS_H_INCLUDED

#include <time.h>
#include <openssl/md5.h>
#include "miniupnpdtypes.h"
#include "config.h"
#include "transformer/proxy.h"

/* statup time */
extern time_t startup_time;

/* runtime boolean flags */
extern int runtime_flags;
#define LOGPACKETSMASK		0x0001
#define SYSUPTIMEMASK		0x0002
#define CHECKCLIENTIPMASK	0x0008
#define SECUREMODEMASK		0x0010

#define ENABLEUPNPMASK		0x0020

#ifdef ENABLE_IPV6
#define IPV6DISABLEDMASK	0x0080
#endif

#define SETFLAG(mask)	runtime_flags |= mask
#define GETFLAG(mask)	(runtime_flags & mask)
#define CLEARFLAG(mask)	runtime_flags &= ~mask

extern const char * pidfilename;

extern char uuidvalue_igd[];	/* uuid of root device (IGD) */
extern char uuidvalue_lan[];	/* uuid of LAN Device */
extern char uuidvalue_wan[];	/* uuid of WAN Device */
extern char uuidvalue_wcd[];	/* uuid of WAN Connection Device */

#define SERIALNUMBER_MAX_LEN (10)
extern char serialnumber[];

#define MODELNUMBER_MAX_LEN (48)
extern char modelnumber[];

#define PRESENTATIONURL_MAX_LEN (64)
extern char presentationurl[];

#ifdef ENABLE_MANUFACTURER_INFO_CONFIGURATION
#define FRIENDLY_NAME_MAX_LEN (64)
extern char friendly_name[];

#define MANUFACTURER_NAME_MAX_LEN (64)
extern char manufacturer_name[];

#define MANUFACTURER_URL_MAX_LEN (64)
extern char manufacturer_url[];

#define MODEL_NAME_MAX_LEN (64)
extern char model_name[];

#define MODEL_DESCRIPTION_MAX_LEN (64)
extern char model_description[];

#define MODEL_URL_MAX_LEN (64)
extern char model_url[];
#endif

/* structure containing variables used during "main loop"
 * that are filled during the init */
struct runtime_vars {
    /* LAN IP addresses for SSDP traffic and HTTP */
    /* moved to global vars */
    int port;	/* HTTP Port */
    int https_port;	/* HTTPS Port */
    int notify_interval;	/* seconds between SSDP announces */
};

extern struct runtime_vars rv;

/* lan addresses to listen to SSDP traffic */
extern struct lan_addr_list lan_addrs;

#ifdef ENABLE_IPV6
/* ipv6 address used for HTTP */
extern char ipv6_addr_for_http_with_brackets[64];

/* address used to bind local services */
extern struct in6_addr ipv6_bind_addr;

#endif

extern const char * minissdpdsocketpath;

extern transformer_proxy_ctx transformer_proxy;
extern const char transformer_uuid[];

#define SSL_CERT_PATH_MAX_LEN (256)
#define SSL_KEY_PATH_MAX_LEN (256)
extern char sslcertfile[];
extern char sslkeyfile[];

/* dsl-config & dsl-reset passwords */
#define DSL_PASSWORD_MAXLEN (2 * MD5_DIGEST_LENGTH + 1)
extern char password_dsl_config[];
extern char password_dsl_reset[];

extern unsigned char digest_nonce_secret[16];
extern long digest_nonce_lifetime;

extern char config_session_id[37];
extern struct timespec config_session_lastupdate;
#endif

