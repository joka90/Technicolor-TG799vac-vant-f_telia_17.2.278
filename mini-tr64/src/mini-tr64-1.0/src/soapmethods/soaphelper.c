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

#include <string.h>
#include <stdio.h>
#include "soaphelper.h"
#include "../upnpsoap.h"
#include "../transformer/helper.h"
#include "../upnpdescgen.h"

void
BuildSendAndCloseSoapResp(struct upnphttp * h,
        const char * body, int bodylen)
{
    static const char beforebody[] =
            "<?xml version=\"1.0\"?>\r\n"
                    "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                    "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
                    "<s:Body>";

    static const char afterbody[] =
            "</s:Body>"
                    "</s:Envelope>\r\n";

    BuildHeader_upnphttp(h, 200, "OK",  sizeof(beforebody) - 1 + sizeof(afterbody) - 1 + bodylen );

    memcpy(h->res_buf + h->res_buflen, beforebody, sizeof(beforebody) - 1);
    h->res_buflen += sizeof(beforebody) - 1;

    memcpy(h->res_buf + h->res_buflen, body, bodylen);
    h->res_buflen += bodylen;

    memcpy(h->res_buf + h->res_buflen, afterbody, sizeof(afterbody) - 1);
    h->res_buflen += sizeof(afterbody) - 1;

    SendRespAndClose_upnphttp(h);
}

void GenericErrorCb(tr064_e_error errorcode, const char *errorstr, void *cookie)
{
    struct cookiestruct *ctx = cookie;

    if(!ctx) {
        syslog(LOG_ERR, "NULL context data, for error code %d, %s", errorcode, errorstr);
        return;
    }

    SoapError(ctx->h, errorcode, errorstr);
}

#define RESPONSEBUFFERSIZE 65536
#define ENTRIESNUMBER 64

static void genericGetterCb(const char *path, const char *param, const char *value, void *cookie) {

    struct cookiestruct *ctx = cookie;

    if(!ctx) {
        syslog(LOG_ERR, "NULL context data, aborting");
        return;
    }

    if(ctx->entry >= ENTRIESNUMBER - 2) { // Want to keep a last one set at 0;
        syslog(LOG_ERR, "Too many entries, cannot store %s", param);
        return;
    }

    int idx = 0;
    size_t valuelen = 0;
    char *entryval;
    int i=0;

    while(ctx->args[idx].dir) {
        if((ctx->args[idx].dir & 0x03)  == 2) { // Only "output" variables
            const char *tfvarname;
            const char *varname;
            if(ctx->args[idx].tr064name) {
                varname = ctx->args[idx].tr064name;
            } else {
                varname = ctx->varlist[ctx->args[idx].relatedVar].name;
            }
            if(ctx->varlist[ctx->args[idx].relatedVar].transformername) {
                tfvarname = ctx->varlist[ctx->args[idx].relatedVar].transformername;
            } else {
                tfvarname = varname;
            }
            if(!strcmp(tfvarname, param)) {
                syslog(LOG_DEBUG, "Found matching parameter %s", param);
                // Skip duplicates
                for (i=0; i<ctx->entry; i++)
                {
                    if (strcmp(ctx->entries[i].name, varname) == 0)
                    {
                       syslog(LOG_WARNING, "Skipping: %s.%s", path, param);
                       return;
                    }
                }
                valuelen = strlen(value);
                entryval = malloc(valuelen+1);
                if(!entryval) {
                    syslog(LOG_ERR, "Could not allocate buffer to store value for %s (len=%d)", param, (int) valuelen);
                    return;
                }

                strncpy(entryval, value, valuelen);
                entryval[valuelen] = 0; // Trailing 0 to be sure
                ctx->entries[ctx->entry].name = varname;
                ctx->entries[ctx->entry].value = entryval; // This one must be freed at the end
                if(ctx->args[idx].dir & 0x80) {
                    ctx->entries[ctx->entry].nonewprefix = 1;
                } else {
                    ctx->entries[ctx->entry].nonewprefix = 0;
                }

                ctx->entry++;
                return;
            }
        }
        idx++;
    }
}

void genericGetter(struct upnphttp * h, const char *urn, const char * action, const struct query_item tfquery[],
        const struct stateVar varlist[], const struct argument args[]) {

    char prefix[] ="<u:%sResponse xmlns:u=\"%s\">";
    char postfix[] = "</u:%sResponse>";
    char entry[] = "<New%s><![CDATA[%s]]></New%s>";
    char entrynonew[] = "<%s><![CDATA[%s]]></%s>";
    struct respentry entries[ENTRIESNUMBER] = { { 0 } };
    char response[RESPONSEBUFFERSIZE];


    struct cookiestruct cookie = {
            .h = h,
            .action = action,
            .entries = entries,
            .args = args,
            .varlist = varlist,
            .entry = 0,
            .urn = 0,
    };
    size_t left = RESPONSEBUFFERSIZE;
    int idx = 0;
    int written;
    int total = 0;
    char *posbuffer = response;

    getpv(tfquery, &genericGetterCb, &GenericErrorCb, &cookie);


    written = snprintf(posbuffer, left, prefix, action, urn);
    left -= written;
    posbuffer += written;
    total += written;

    while(entries[idx].name) {
        if(entries[idx].nonewprefix) {
            written = snprintf(posbuffer, left, entrynonew, entries[idx].name, entries[idx].value, entries[idx].name);
        } else {
            written = snprintf(posbuffer, left, entry, entries[idx].name, entries[idx].value, entries[idx].name);
        }
        left -= written;
        posbuffer += written;
        total += written;

        if(left <= 0) {
            syslog(LOG_ERR, "Response buffer too small to store xml");
            goto error;
        }
        idx++;
    }

    written = snprintf(posbuffer, left, postfix, action);
    left -= written;
    posbuffer += written;
    total += written;

    BuildSendAndCloseSoapResp(h, response, total);

error:
    // Must free allocated values
    idx = 0;
    while(entries[idx].name) {
        free(entries[idx].value);
        entries[idx].name = 0;
        entries[idx].value = 0;
        idx++;
    }
}

void genericSetterCb(void *cookie) {
    struct cookiestruct *ctx = cookie;
    char body[1024];
    int bodylen;
    static const char resp[] =
            "<u:%sResponse "
                    "xmlns:u=\"%s\">"
                    "</u:%sResponse>";

    if (!ctx) {
        syslog(LOG_ERR, "NULL context data, aborting");
        return;
    }

    bodylen = snprintf(body, sizeof(body), resp, ctx->action, ctx->urn, ctx->action);
    BuildSendAndCloseSoapResp(ctx->h, body, bodylen);
}

void genericSetterErrorCb(tr064_e_error errorcode, const char *errorstr, void *cookie) {
    struct cookiestruct *ctx = cookie;

    if(!ctx) {
        syslog(LOG_ERR, "NULL context data - code:%d errorstr: %s", errorcode, errorstr);
        return;
    }

    SoapError(ctx->h, 501, errorstr);
}

void genericSetter(struct upnphttp * h, const char *urn, const char * action, const char* tfpathtemplate,
        const struct stateVar varlist[], const struct argument args[]) {

    char *prefixnew = "<New%s>";
    char *prefixnonew = "<%s>";
    char *postfixnew = "</New%s>";
    char *postfixnonew = "</%s>";
    char prefix[256];
    char postfix[256];
    char errormsg[512];

    struct query_item query[ENTRIESNUMBER] = {
            { 0 }
    };
    struct cookiestruct cookie = {
            .h = h,
            .action = action,
            .entries = 0,
            .args = args,
            .varlist = varlist,
            .entry = 0,
            .urn = urn,
    };

    int idx = 0;
    int entryidx = 0;
    while(args[idx].dir && entryidx < ENTRIESNUMBER - 1) {
        if ((args[idx].dir & 0x03) == 1) { // Only "input" variables
            const char *varname;
            const char* transformername;
            char *startpos;
            char *endpos;
            size_t prefixlen;
            size_t valuelen;
            size_t namelen;

            if(args[idx].tr064name) {
                varname = args[idx].tr064name;
            } else {
                varname = varlist[args[idx].relatedVar].name;
            }

            if(varlist[args[idx].relatedVar].transformername) {
                transformername = varlist[args[idx].relatedVar].transformername;
            } else {
                transformername = varlist[args[idx].relatedVar].name;
            }

            if(args[idx].dir & 0x80) {
                snprintf(prefix, sizeof(prefix), prefixnonew, varname);
                snprintf(postfix, sizeof(postfix), postfixnonew, varname);
            } else {
                snprintf(prefix, sizeof(prefix), prefixnew, varname);
                snprintf(postfix, sizeof(postfix), postfixnew, varname);
            }

            prefixlen = strlen(prefix);
            startpos = strstr(h->req_buf, prefix);
            if(!startpos) {
                snprintf(errormsg, sizeof(errormsg), "Missing parameter %s", varname);
                SoapError(h, 501, errormsg);
                return;
            }
            endpos = strstr(startpos + prefixlen, postfix);
            if(!endpos) {
                snprintf(errormsg, sizeof(errormsg), "Missing parameter %s", varname);
                SoapError(h, 501, errormsg);
                return;
            }
            valuelen = endpos - startpos - prefixlen;

            query[entryidx].value = malloc(valuelen + 1);
            if(!query[entryidx].value) {
                // TODO send to error path to free previously allocated memory
                snprintf(errormsg, sizeof(errormsg), "Unable to allocate memory for %s", varname);
                SoapError(h, 603, errormsg);
                return;
            }
            memcpy(query[entryidx].value, startpos + prefixlen, valuelen);
            query[entryidx].value[valuelen] = 0;

            namelen = strlen(tfpathtemplate) + strlen(transformername) + 1; // Should be a reasonable upper bound (actually should be shorter since %s is replaced by variable name)
            query[entryidx].name = malloc(namelen);
            if(!query[entryidx].value) {
                // TODO send to error path to free previously allocated memory
                snprintf(errormsg, sizeof(errormsg), "Unable to allocate memory for %s", varname);
                SoapError(h, 603, errormsg);
                return;
            }
            snprintf(query[entryidx].name, namelen, tfpathtemplate, transformername);

            syslog(LOG_DEBUG, "Setting %s to %s",query[entryidx].name, query[entryidx].value);

            entryidx++;
        }
        idx++;
    }
    setpv(query, genericSetterCb, &genericSetterErrorCb, &cookie);

    idx = 0;
    while(query[idx].name) {
        free(query[idx].name);
        free(query[idx].value);
        query[idx].name = 0;
        query[idx].value = 0;
        idx++;
    }
}
