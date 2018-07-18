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

#ifndef UPNPDESCGEN_H_INCLUDED
#define UPNPDESCGEN_H_INCLUDED

#include "config.h"
#include "upnphttp.h"

/* for the root description
 * The child list reference is stored in "data" member using the
 * INITHELPER macro with index/nchild always in the
 * same order, whatever the endianness */
struct XMLElt {
    const char * eltname;	/* begin with '/' if no child */
    const char * data;	/* Value */
};

/* for service description */
struct serviceDesc {
    const struct action * actionList;
    const struct stateVar * serviceStateTable;
};

struct action {
    const char * name;
    const struct argument * args;
};

struct argument {	/* the name of the arg is obtained from the variable */
    unsigned char dir;		/* MSB : don't append "New" Flag,
	                         * 5 Medium bits : magic argument name index
	                         * 2 LSB : 1 = in, 2 = out */
    unsigned char relatedVar;	/* index of the related variable */
    const char * tr064name;
};

struct stateVar {
    const char * name;
    unsigned char itype;	/* MSB: sendEvent flag, 7 LSB: index in upnptypes */
    unsigned char idefault;	/* default value */
    unsigned char iallowedlist;	/* index in allowed values list
	                             * or in allowed range list */
    const char * transformername;
};

struct instanceList {
    int num;
    int instances[32];
};

/* little endian
 * The code has now be tested on big endian architecture */
#define INITHELPER(i, n) ((char *)(((n)<<16)|(i)))

/* char * genRootDesc(int *);
 * returns: NULL on error, string allocated on the heap */
char *
        genRootDesc(int * len, struct upnphttp * h);

char *
        genDI(int * len, struct upnphttp * h);

char *
        genDC(int * len, struct upnphttp * h);

char *
        genLCS(int * len, struct upnphttp * h);

char *
        genLHC(int * len, struct upnphttp * h);

char *
        genLEIC(int * len, struct upnphttp * h);

char *
        genWLANC(int * len, struct upnphttp * h);

char *
        genWCIC(int * len, struct upnphttp * h);

char *
        genWDIC(int * len, struct upnphttp * h);

char *
        genWEIC(int * len, struct upnphttp * h);

char *
        genWDLC(int * len, struct upnphttp * h);

char *
        genWELC(int * len, struct upnphttp * h);

char *
        genWANPPP(int * len, struct upnphttp * h);

char *
        genWANIP(int * len, struct upnphttp * h);

#endif

