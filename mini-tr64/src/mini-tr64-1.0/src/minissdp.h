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

#ifndef MINISSDP_H_INCLUDED
#define MINISSDP_H_INCLUDED

#include "miniupnpdtypes.h"

int
OpenAndConfSSDPReceiveSocket(int ipv6);

int
OpenAndConfSSDPNotifySockets(int * sockets);

void
SendSSDPNotifies2(int * sockets,
                  unsigned short http_port,
                  unsigned short https_port,
                  unsigned int lifetime);

void
ProcessSSDPRequest(int s,
                   unsigned short http_port, unsigned short https_port);

void
ProcessSSDPData(int s, const char *bufr, int n,
                const struct sockaddr * sendername,
                unsigned short http_port, unsigned short https_port);

int
SendSSDPGoodbye(int * sockets, int n);

int
SubmitServicesToMiniSSDPD(const char * host, unsigned short port);

#endif

