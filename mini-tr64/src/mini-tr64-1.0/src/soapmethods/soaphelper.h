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

#ifndef SOAPMETHODS_HELPER_H_INCLUDED
#define SOAPMETHODS_HELPER_H_INCLUDED

#include "../upnphttp.h"
#include "../transformer/error.h"
#include "../transformer/helper.h"
#include "../upnpdescgen.h"

struct respentry {
    const char *name;
    char *value;
    int nonewprefix;
};

struct cookiestruct {
    struct upnphttp *h;
    const char *urn;
    const char *action;
    struct respentry *entries;
    const struct stateVar *varlist;
    const struct argument *args;
    int entry;
};

struct ctxdata {
    struct upnphttp *h;
    const char *action;
    void *data;
};

void BuildSendAndCloseSoapResp(struct upnphttp * h, const char * body, int bodylen);
void GenericErrorCb(tr064_e_error errorcode, const char *errorstr, void *cookie);

void genericGetter(struct upnphttp * h, const char *urn, const char * action, const struct query_item tfquery[],
        const struct stateVar varlist[], const struct argument args[]);

void genericSetter(struct upnphttp * h, const char *urn, const char * action, const char* tfpathtemplate,
        const struct stateVar varlist[], const struct argument args[]);
void genericSetterErrorCb(tr064_e_error errorcode, const char *errorstr, void *cookie);
void genericSetterCb(void *cookie);

#endif
