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

#ifndef LANCONFIGSECURITYDESC_H_INCLUDED
#define LANCONFIGSECURITYDESC_H_INCLUDED
#include "../upnpdescgen.h"

/* Read TR064 spec - LANConfigSecurity */
static const struct argument SetConfigPasswordArgs[] =
        {
                {1, 0, "Password"}, /* in */
                {0, 0, 0},
        };

static const struct action LCSActions[] =
        {
                {"SetConfigPassword", SetConfigPasswordArgs}, /* Req / Secure */
                {0, 0},
        };

static const struct stateVar LCSVars[] =
        {
                {"ConfigPassword", 0, 0, 0, 0}, /* Required */
                {0, 0, 0, 0, 0}
        };

static const struct serviceDesc scpdLCS =
        { LCSActions, LCSVars };

#endif