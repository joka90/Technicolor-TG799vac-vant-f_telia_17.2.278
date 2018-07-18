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

#ifndef WANETHERNETLINKCONFIGDESC_H_INCLUDED
#define WANETHERNETLINKCONFIGDESC_H_INCLUDED

#include "../upnpdescgen.h"

/* Read TR064 spec - WANEthernetLinkConfig */
static const struct argument GetEthernetLinkStatusArgs[] =
        {
                {2, 0, 0}, /* out */
                {0, 0, 0},
        };

static const struct action WELCActions[] =
        {
                {"GetEthernetLinkStatus", GetEthernetLinkStatusArgs }, /* Req  */
                {0, 0},
        };

static const struct stateVar WELCVars[] =
        {
                {"EthernetLinkStatus", 0, 0, 0, 0}, /* Required */
                {0, 0, 0, 0, 0},
        };

static const struct serviceDesc scpdWELC =
        { WELCActions, WELCVars };

#endif