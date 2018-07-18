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


#include <sys/types.h>
#include <netinet/in.h>
#include <openssl/md5.h>

#include "config.h"
#include "upnpglobalvars.h"
#include "transformer/proxy.h"
#include "upnpdescstrings.h"

/* startup time */
time_t startup_time = 0;

int runtime_flags = 0;

const char * pidfilename = "/var/run/minitr064d.pid";

char uuidvalue_igd[] = "uuid:00000000-0000-0000-0000-000000000000";
char uuidvalue_lan[] = "uuid:00000000-0000-0000-0000-000000000000";
char uuidvalue_wan[] = "uuid:00000000-0000-0000-0000-000000000000";
char uuidvalue_wcd[] = "uuid:00000000-0000-0000-0000-000000000000";
char serialnumber[SERIALNUMBER_MAX_LEN] = "00000000";

char modelnumber[MODELNUMBER_MAX_LEN] = "1";

/* presentation url :
 * http://nnn.nnn.nnn.nnn:ppppp/  => max 30 bytes including terminating 0 */
char presentationurl[PRESENTATIONURL_MAX_LEN];

#ifdef ENABLE_MANUFACTURER_INFO_CONFIGURATION
/* friendly name for root devices in XML description */
char friendly_name[FRIENDLY_NAME_MAX_LEN] = OS_NAME " router";

/* manufacturer name for root devices in XML description */
char manufacturer_name[MANUFACTURER_NAME_MAX_LEN] = ROOTDEV_MANUFACTURER;

/* manufacturer url for root devices in XML description */
char manufacturer_url[MANUFACTURER_URL_MAX_LEN] = ROOTDEV_MANUFACTURERURL;

/* model name for root devices in XML description */
char model_name[MODEL_NAME_MAX_LEN] = ROOTDEV_MODELNAME;

/* model description for root devices in XML description */
char model_description[MODEL_DESCRIPTION_MAX_LEN] = ROOTDEV_MODELDESCRIPTION;

/* model url for root devices in XML description */
char model_url[MODEL_URL_MAX_LEN] = ROOTDEV_MODELURL;
#endif

struct runtime_vars rv;
struct lan_addr_list lan_addrs;

#ifdef ENABLE_IPV6
/* ipv6 address used for HTTP */
char ipv6_addr_for_http_with_brackets[64];

/* address used to bind local services */
struct in6_addr ipv6_bind_addr;
#endif

/* Path of the Unix socket used to communicate with MiniSSDPd */
const char * minissdpdsocketpath = "/var/run/minissdpd.sock";

transformer_proxy_ctx transformer_proxy;
const char transformer_uuid[] = {0x39, 0xf9, 0x0a, 0x2d, 0x5f, 0xef, 0x42, 0x35, 0x8f, 0x90, 0x25, 0xe7, 0x5e, 0x4b, 0x5a, 0x1d};

/* HTTPS key/cert */
char sslcertfile[SSL_CERT_PATH_MAX_LEN] = "/etc/ssl/server.crt";
char sslkeyfile[SSL_KEY_PATH_MAX_LEN] = "/etc/ssl/server.key";

/* dsl-config & dsl-reset passwords */
char password_dsl_config[DSL_PASSWORD_MAXLEN] = "";
char password_dsl_reset[DSL_PASSWORD_MAXLEN] = "";

/* used to generate nonce for http-digest authent */
unsigned char digest_nonce_secret[16];

long digest_nonce_lifetime = 30*60; // 30 min lifetime for a nonce before stale

char config_session_id[37] = "";
struct timespec config_session_lastupdate;