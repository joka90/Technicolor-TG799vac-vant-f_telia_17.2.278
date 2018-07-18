/* $Id: ifacewatcher.c,v 1.7 2012/05/27 22:16:10 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2012 Thomas Bernard
 *
 * ifacewatcher.c
 *
 * This file implements dynamic serving of new network interfaces
 * which weren't available during daemon start. It also takes care
 * of interfaces which become unavailable.
 *
 * Copyright (c) 2011, Alexey Osipov <simba@lerlan.ru>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    * Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *    * The name of the author may not be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE. */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include "../config.h"

#ifdef USE_IFACEWATCHER

#include "../ifacewatcher.h"
#include "../minissdp.h"
#include "../getifaddr.h"
#include "../upnpglobalvars.h"


int
OpenAndConfInterfaceWatchSocket(void)
{
	int s;
	struct sockaddr_nl addr;

	s = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (s == -1)
	{
		syslog(LOG_ERR, "socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE): %m");
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.nl_family = AF_NETLINK;
	addr.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR;
	/*addr.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;*/

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		syslog(LOG_ERR, "bind(netlink): %m");
		close(s);
		return -1;
	}

	return s;
}

void
ProcessInterfaceWatchNotify(int s)
{
	char buffer[4096];
	struct iovec iov;
	struct msghdr hdr;
	struct nlmsghdr *nlhdr;
	struct ifaddrmsg *ifa;
	int len;

	struct rtattr *rth;
	int rtl;

	iov.iov_base = buffer;
	iov.iov_len = sizeof(buffer);

	memset(&hdr, 0, sizeof(hdr));
	hdr.msg_iov = &iov;
	hdr.msg_iovlen = 1;

	len = recvmsg(s, &hdr, 0);
	if (len < 0)
	{
		syslog(LOG_ERR, "recvmsg(s, &hdr, 0): %m");
		return;
	}

	for (nlhdr = (struct nlmsghdr *) buffer;
	     NLMSG_OK (nlhdr, (unsigned int)len);
	     nlhdr = NLMSG_NEXT (nlhdr, len))
	{
		int is_del = 0;
		char address[48];
		char ifname[IFNAMSIZ];
		address[0] = '\0';
		ifname[0] = '\0';
		if (nlhdr->nlmsg_type == NLMSG_DONE)
			break;
		switch(nlhdr->nlmsg_type) {
		case RTM_DELLINK:
			is_del = 1;
		case RTM_NEWLINK:
			break;
		case RTM_DELADDR:
			is_del = 1;
		case RTM_NEWADDR:
			/* see /usr/include/linux/netlink.h
			 * and /usr/include/linux/rtnetlink.h */
			ifa = (struct ifaddrmsg *) NLMSG_DATA(nlhdr);
			syslog(LOG_DEBUG, "%s %s index=%d fam=%d", "ProcessInterfaceWatchNotify",
			       is_del ? "RTM_DELADDR" : "RTM_NEWADDR",
			       ifa->ifa_index, ifa->ifa_family);
			for(rth = IFA_RTA(ifa), rtl = IFA_PAYLOAD(nlhdr);
			    rtl && RTA_OK(rth, rtl);
			    rth = RTA_NEXT(rth, rtl)) {
				char tmp[128];
				memset(tmp, 0, sizeof(tmp));
				switch(rth->rta_type) {
				case IFA_ADDRESS:
				case IFA_LOCAL:
				case IFA_BROADCAST:
				case IFA_ANYCAST:
					inet_ntop(ifa->ifa_family, RTA_DATA(rth), tmp, sizeof(tmp));
					if(rth->rta_type == IFA_ADDRESS)
						strncpy(address, tmp, sizeof(address));
					break;
				case IFA_LABEL:
					strncpy(tmp, RTA_DATA(rth), sizeof(tmp));
					strncpy(ifname, tmp, sizeof(ifname));
					break;
				case IFA_CACHEINFO:
					{
						struct ifa_cacheinfo *cache_info;
						cache_info = RTA_DATA(rth);
						snprintf(tmp, sizeof(tmp), "valid=%u prefered=%u",
						         cache_info->ifa_valid, cache_info->ifa_prefered);
					}
					break;
				default:
					strncpy(tmp, "*unknown*", sizeof(tmp));
				}
				syslog(LOG_DEBUG, " - %u - %s type=%d",
				       ifa->ifa_index, tmp,
				       rth->rta_type);
			}
			break;
		default:
			syslog(LOG_DEBUG, "%s type %d ignored",
			       "ProcessInterfaceWatchNotify", nlhdr->nlmsg_type);
		}
	}

}

#endif
