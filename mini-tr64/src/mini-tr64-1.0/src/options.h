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


#ifndef OPTIONS_H_INCLUDED
#define OPTIONS_H_INCLUDED

#include "config.h"

#ifndef DISABLE_CONFIG_FILE
/* enum of option available in the miniupnpd.conf */
enum upnpconfigoptions {
    UPNP_INVALID = 0,
    UPNPLISTENING_IP,		/* listening_ip */
#ifdef ENABLE_IPV6
    UPNPIPV6_LISTENING_IP,		/* listening address for IPv6 */
#endif /* ENABLE_IPV6 */
    UPNPPORT,				/* "port" / "http_port" */
    UPNPHTTPSPORT,			/* "https_port" */
    UPNPHTTPSCERT,			/* "https_cert" */
    UPNPHTTPSKEY,			/* "https_key" */
    UPNPPRESENTATIONURL,	/* presentation_url */
#ifdef ENABLE_MANUFACTURER_INFO_CONFIGURATION
    UPNPFRIENDLY_NAME,		/* "friendly_name" */
    UPNPMANUFACTURER_NAME,	/* "manufacturer_name" */
    UPNPMANUFACTURER_URL,	/* "manufacturer_url" */
    UPNPMODEL_NAME,	/* "model_name" */
    UPNPMODEL_DESCRIPTION,	/* "model_description" */
    UPNPMODEL_URL,	/* "model_url" */
#endif
    UPNPNOTIFY_INTERVAL,	/* notify_interval */
    UPNPSYSTEM_UPTIME,		/* "system_uptime" */
    UPNPUUID,				/* uuid */
    UPNPSERIAL,				/* serial */
    UPNPMODEL_NUMBER,		/* model_number */
    UPNPMINISSDPDSOCKET,	/* minissdpdsocket */
    UPNPENABLE,				/* enable_upnp */
    PASSWORD_DSLFCONFIG,      /* password_dslfconfig */
    PASSWORD_DSLFRESET        /* password_dslfreset */
};

/* readoptionsfile()
 * parse and store the option file values
 * returns: 0 success, -1 failure */
int
readoptionsfile(const char * fname);

/* freeoptions()
 * frees memory allocated to option values */
void
freeoptions(void);

struct option
{
	enum upnpconfigoptions id;
	const char * value;
};

extern struct option * ary_options;
extern unsigned int num_options;

#endif /* DISABLE_CONFIG_FILE */

#endif /* OPTIONS_H_INCLUDED */

