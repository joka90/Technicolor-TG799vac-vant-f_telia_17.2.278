/* $Id: miniupnpdpath.h,v 1.9 2012/09/27 15:47:15 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2011 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#ifndef MINIUPNPDPATH_H_INCLUDED
#define MINIUPNPDPATH_H_INCLUDED

#include "config.h"

#define ROOTDESC_PATH_END 		"rootDesc.xml"
#define WANIPC_CONTROLURL_END 		"/ctl/IPConn"

#define DUMMY_PATH_END                  "/dummy.xml"

#define WANCFG_PATH_END                 "/WANCfg.xml"

#define WANCFG_CONTROLURL_END     	"/ctl/CmnIfCfg"

#define WANCFG_EVENTURL_END             "/evt/CmnIfCfg"

#define WANIPC_PATH_END                 "/WANIPCn.xml"

#define WANIPC_EVENTURL_END             "/evt/IPConn"

#define L3F_PATH_END                    "/L3F.xml"
#define L3F_CONTROLURL_END          	"/ctl/L3F"
#define L3F_EVENTURL_END            	"/evt/L3F"

#define WANIP6FC_PATH_END           	"/WANIP6FC.xml"
#define WANIP6FC_CONTROLURL_END     	"/ctl/IP6FCtl"
#define WANIP6FC_EVENTURL_END       	"/evt/IP6FCtl"

#define DP_PATH_END                     "/DP.xml"
#define DP_CONTROLURL_END           	"/ctl/DP"
#define DP_EVENTURL_END                 "/evt/DP"

/**
 * Function to generate the Paths
 */
void generate_paths(void);

/**
 * Functio will get the current generated security token
 */
char *get_current_token(void);

/* Paths and other URLs in the miniupnpd http server */

extern char ROOTDESC_PATH[];

#ifdef HAS_DUMMY_SERVICE
extern char DUMMY_PATH[];
#endif

extern char WANCFG_PATH[];

extern char WANCFG_CONTROLURL[];

extern char WANCFG_EVENTURL[];


extern char WANIPC_PATH[];


extern char WANIPC_CONTROLURL[];

extern char WANIPC_EVENTURL[];

#ifdef ENABLE_L3F_SERVICE
extern char L3F_PATH[];
extern char L3F_CONTROLURL[];
extern char L3F_EVENTURL[];
#endif

#ifdef ENABLE_6FC_SERVICE
extern char WANIP6FC_PATH[];
extern char WANIP6FC_CONTROLURL[];
extern char WANIP6FC_EVENTURL[];
#endif

#ifdef ENABLE_DP_SERVICE
/* For DeviceProtection introduced in IGD v2 */
extern char DP_PATH[];
extern char DP_CONTROLURL[];
extern char DP_EVENTURL[];
#endif

#endif

