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

#ifndef DEVICECONFIGDESC_H_INCLUDED
#define DEVICECONFIGDESC_H_INCLUDED
#include "../upnpdescgen.h"

/* Read TR064 spec - DeviceConfig */
static const struct argument GetPersistentDataArgs[] =
        {
                {2, 0, 0}, /* out */
                {0, 0, 0},
        };

static const struct argument SetPersistentdataArgs[] =
        {
                {1, 0, 0}, /* in */
                {0, 0, 0},
        };

static const struct argument GetConfigurationArgs[] =
        {
                {2, 1, 0}, /* out */
                {0, 0, 0},
        };

static const struct argument SetConfigurationArgs[] =
        {
                {1, 1, 0}, /* in */
                {0, 0, 0},
        };

static const struct argument ConfigurationStartedArgs[] =
        {
                {1, 3, "SessionID"}, /* in */
                {0, 0, 0},
        };

static const struct argument ConfigurationFinishedArgs[] =
        {
                {2, 2, "Status"}, /* out */
                {0, 0, 0},
        };

static const struct argument FactoryResetArgs[] =
        {
                {0, 0, 0},
        };

static const struct argument RebootArgs[] =
        {
                {0, 0, 0},
        };

static const struct action DCActions[] =
        {
                {"GetPersistentData", GetPersistentDataArgs}, /* Req */
                {"SetPersistentdata", SetPersistentdataArgs}, /* Req / Secure */
                {"GetConfiguration", GetConfigurationArgs}, /* Opt */
                {"SetConfiguration", SetConfigurationArgs}, /* Opt / Secure */
                {"ConfigurationStarted", ConfigurationStartedArgs}, /* Req / Secure */
                {"ConfigurationFinished", ConfigurationFinishedArgs}, /* Req / Secure */
                {"FactoryReset", FactoryResetArgs}, /* Req / Secure */
                {"Reboot", RebootArgs}, /* Req / Secure */
                {0, 0}
        };

static const struct stateVar DCVars[] =
        {
                {"PersistentData", 0, 0, 0, 0}, /* Required */
                {"ConfigFile", 0, 0, 0, 0}, /* Optional */
                {"A_ARG_TYPE_Status", 0, 0, 0, 0}, /* Required */
                {"A_ARG_TYPE_UUID", 5, 0, 0, 0}, /* Required */
                {0, 0, 0, 0, 0}
        };

static const struct serviceDesc scpdDC =
        { DCActions, DCVars };

#endif