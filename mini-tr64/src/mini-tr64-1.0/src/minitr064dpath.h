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


#ifndef MINITR064DPATH_H_INCLUDED
#define MINITR064DPATH_H_INCLUDED

#include "config.h"

/* Paths and other URLs in the miniupnpd http server */

#define ROOTDESC_PATH_END 		"/rootDesc.xml"
#define BASE_CTL_PATH_END            "/ctl/"

#define DI_PATH_END			    "/DI.xml"
#define DI_CONTROLURL_END		"/ctl/DI"

#define DC_PATH_END			    "/DC.xml"
#define DC_CONTROLURL_END		"/ctl/DC"

#define LCS_PATH_END			"/LCS.xml"
#define LCS_CONTROLURL_END		"/ctl/LCS"

#define LHC_PATH_END			"/LHC.xml"
#define LHC_CONTROLURL_END		"/ctl/LHC/%d"
#define LHC_PATTERN_END		"/ctl/LHC/%d"

#define LEIC_PATH_END           "/LEIC.xml"
#define LEIC_CONTROLURL_END     "/ctl/LEIC/%d/%%d"
#define LEIC_PATTERN_END     "/ctl/LEIC/%d/%d"

#define WLANC_PATH_END           "/WLANC.xml"
#define WLANC_CONTROLURL_END     "/ctl/WLANC/%d/%%d"
#define WLANC_PATTERN_END     "/ctl/WLANC/%d/%d"

#define WCIC_PATH_END           "/WCIC.xml"
#define WCIC_CONTROLURL_END     "/ctl/WCIC/%d"
#define WCIC_PATTERN_END     "/ctl/WCIC/%d"

#define WDIC_PATH_END           "/WDIC.xml"
#define WDIC_CONTROLURL_END     "/ctl/WDIC/%d"
#define WDIC_PATTERN_END     "/ctl/WDIC/%d"

#define WEIC_PATH_END           "/WEIC.xml"
#define WEIC_CONTROLURL_END     "/ctl/WEIC/%d"
#define WEIC_PATTERN_END     "/ctl/WEIC/%d"

#define WDLC_PATH_END           "/WDLC.xml"
#define WDLC_CONTROLURL_END     "/ctl/WDLC/%d/%d"
#define WDLC_PATTERN_END     "/ctl/WDLC/%d/%d"

#define WELC_PATH_END           "/WELC.xml"
#define WELC_CONTROLURL_END     "/ctl/WELC/%d/%d"
#define WELC_PATTERN_END     "/ctl/WELC/%d/%d"

#define WANPPP_PATH_END           "/WANPPP.xml"
#define WANPPP_CONTROLURL_END     "/ctl/WANPPP/%d/%d/%%d"
#define WANPPP_PATTERN_END     "/ctl/WANPPP/%d/%d/%d"

#define WANIP_PATH_END           "/WANIP.xml"
#define WANIP_CONTROLURL_END     "/ctl/WANIP/%d/%d/%%d"
#define WANIP_PATTERN_END     "/ctl/WANIP/%d/%d/%d"

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
extern char BASE_CTL_PATH[];

extern char DI_PATH[];
extern char DI_CONTROLURL[];

extern char DC_PATH[];
extern char DC_CONTROLURL[];

extern char LCS_PATH[];
extern char LCS_CONTROLURL[];

extern char LHC_PATH[];
extern char LHC_CONTROLURL[];
extern char LHC_PATTERN[];

extern char LEIC_PATH[];
extern char LEIC_CONTROLURL[];
extern char LEIC_PATTERN[];

extern char WLANC_PATH[];
extern char WLANC_CONTROLURL[];
extern char WLANC_PATTERN[];

extern char WCIC_PATH[];
extern char WCIC_CONTROLURL[];
extern char WCIC_PATTERN[];

extern char WDIC_PATH[];
extern char WDIC_CONTROLURL[];
extern char WDIC_PATTERN[];

extern char WEIC_PATH[];
extern char WEIC_CONTROLURL[];
extern char WEIC_PATTERN[];

extern char WDLC_PATH[];
extern char WDLC_CONTROLURL[];
extern char WDLC_PATTERN[];

extern char WELC_PATH[];
extern char WELC_CONTROLURL[];
extern char WELC_PATTERN[];

extern char WANPPP_PATH[];
extern char WANPPP_CONTROLURL[];
extern char WANPPP_PATTERN[];

extern char WANIP_PATH[];
extern char WANIP_CONTROLURL[];
extern char WANIP_PATTERN[];

#endif

