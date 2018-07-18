/* $Id: getifaddr.c,v 1.17 2013/04/27 15:40:09 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2013 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#if defined(sun)
#include <sys/sockio.h>
#endif

#include "config.h"
#include "getifaddr.h"
#if defined(USE_GETIFADDRS) || defined(ENABLE_IPV6)
#include <ifaddrs.h>
#define IN_LINKLOCAL(addr)  ( ( (addr) &  0xA9FE0000 ) == 0xA9FE0000 )
#endif

int
getifaddr(const char * ifname, char * buf, int len,
          struct in_addr * addr, struct in_addr * mask)
{
#ifndef USE_GETIFADDRS
	/* use ioctl SIOCGIFADDR. Works only for ip v4 */
	/* SIOCGIFADDR struct ifreq *  */
	int s;
	struct ifreq ifr;
	int ifrlen;
	struct sockaddr_in * ifaddr;
	ifrlen = sizeof(ifr);

	if(!ifname || ifname[0]=='\0')
		return -1;
	s = socket(PF_INET, SOCK_DGRAM, 0);
	if(s < 0)
	{
		syslog(LOG_ERR, "socket(PF_INET, SOCK_DGRAM): %m");
		return -1;
	}
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	if(ioctl(s, SIOCGIFADDR, &ifr, &ifrlen) < 0)
	{
		syslog(LOG_ERR, "ioctl(s, SIOCGIFADDR, ...): %m");
		close(s);
		return -1;
	}
	ifaddr = (struct sockaddr_in *)&ifr.ifr_addr;
	if(addr) *addr = ifaddr->sin_addr;
	if(!inet_ntop(AF_INET, &ifaddr->sin_addr, buf, len))
	{
		syslog(LOG_ERR, "inet_ntop(): %m");
		close(s);
		return -1;
	}
	if(mask)
	{
		strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
		if(ioctl(s, SIOCGIFNETMASK, &ifr, &ifrlen) < 0)
		{
			syslog(LOG_ERR, "ioctl(s, SIOCGIFNETMASK, ...): %m");
			close(s);
			return -1;
		}
#ifdef ifr_netmask
		*mask = ((struct sockaddr_in *)&ifr.ifr_netmask)->sin_addr;
#else
		*mask = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
#endif
	}
	close(s);
#else /* ifndef USE_GETIFADDRS */
	/* Works for all address families (both ip v4 and ip v6) */
	struct ifaddrs * ifap;
	struct ifaddrs * ife;
	int err = -1;

	if(!ifname || ifname[0]=='\0')
		return -1;
	if(getifaddrs(&ifap)<0)
	{
		syslog(LOG_ERR, "getifaddrs: %m");
		return -1;
	}
	for(ife = ifap; ife; ife = ife->ifa_next)
	{
		/* skip other interfaces if one was specified */
		if(ifname && (0 != strcmp(ifname, ife->ifa_name)))
			continue;
		if(ife->ifa_addr == NULL)
			continue;
		switch(ife->ifa_addr->sa_family)
		{
		case AF_INET:
			if (IN_LINKLOCAL(((struct sockaddr_in *)ife->ifa_addr)->sin_addr.s_addr))
				continue;
			inet_ntop(ife->ifa_addr->sa_family,
			          &((struct sockaddr_in *)ife->ifa_addr)->sin_addr,
			          buf, len);
			if(addr) *addr = ((struct sockaddr_in *)ife->ifa_addr)->sin_addr;
			if(mask) *mask = ((struct sockaddr_in *)ife->ifa_netmask)->sin_addr;
			err = 0;
			break;
/*
		case AF_INET6:
			inet_ntop(ife->ifa_addr->sa_family,
			          &((struct sockaddr_in6 *)ife->ifa_addr)->sin6_addr,
			          buf, len);
*/
		}
	}
	freeifaddrs(ifap);
#endif
	return err;
}

#ifdef ENABLE_IPV6
int
find_ipv6_addr(const char * ifname,
               char * dst, int n)
{
	struct ifaddrs * ifap;
	struct ifaddrs * ife;
	const struct sockaddr_in6 * addr;
	char buf[64];
	int r = 0;

	if(!dst)
		return -1;

	if(getifaddrs(&ifap)<0)
	{
		syslog(LOG_ERR, "getifaddrs: %m");
		return -1;
	}
	for(ife = ifap; ife; ife = ife->ifa_next)
	{
		/* skip other interfaces if one was specified */
		if(ifname && (0 != strcmp(ifname, ife->ifa_name)))
			continue;
		if(ife->ifa_addr == NULL)
			continue;
		if(ife->ifa_addr->sa_family == AF_INET6)
		{
			addr = (const struct sockaddr_in6 *)ife->ifa_addr;
			if(!IN6_IS_ADDR_LOOPBACK(&addr->sin6_addr)
			   && !IN6_IS_ADDR_LINKLOCAL(&addr->sin6_addr))
			{
				inet_ntop(ife->ifa_addr->sa_family,
				          &addr->sin6_addr,
				          buf, sizeof(buf));
				/* add brackets */
				snprintf(dst, n, "[%s]", buf);
				r = 1;
			}
		}
	}
	freeifaddrs(ifap);
	return r;
}
#endif


int addlanaddrsfrominterface(struct lan_addr_list *addrs, const char *ifname)
{
    int err=0;

#ifndef USE_GETIFADDRS
    syslog(LOG_ERR, "getlanaddrsfrominterface only supported when USE_GETIFADDRS is enabled");
    err=-1;
#else
    /* Works for all address families (both ip v4 and ip v6) */
    struct ifaddrs * ifap = NULL;
    struct ifaddrs * ife = NULL;
    struct lan_addr_s * lan_addr = NULL;

    if(!ifname || ifname[0]=='\0')
        return -1;

    if(getifaddrs(&ifap)<0)
    {
        syslog(LOG_ERR, "getifaddrs: %m");
        return -1;
    }

    for(ife = ifap; ife; ife = ife->ifa_next)
    {
        // skip other interfaces if one was specified
        if(ifname && (0 != strcmp(ifname, ife->ifa_name)))
            continue;
        if(ife->ifa_addr == NULL)
            continue;
        switch(ife->ifa_addr->sa_family)
        {
            case AF_INET:
                if (IN_LINKLOCAL(((struct sockaddr_in *)ife->ifa_addr)->sin_addr.s_addr))
                    continue;

                lan_addr = (struct lan_addr_s *) malloc(sizeof(struct lan_addr_s));

                if (!lan_addr)
                    return -1;

                snprintf(lan_addr->ifname, IFNAMSIZ, "%s", ifname);
                inet_ntop(ife->ifa_addr->sa_family,
                          &((struct sockaddr_in *)ife->ifa_addr)->sin_addr,
                          lan_addr->str, sizeof(lan_addr->str));
                lan_addr->addr = ((struct sockaddr_in *)ife->ifa_addr)->sin_addr;
                lan_addr->mask = ((struct sockaddr_in *)ife->ifa_netmask)->sin_addr;

                if(lan_addr->mask.s_addr == 0)
                {
                    // by default, networks are /24
                    lan_addr->mask.s_addr = htonl(0xffffff00u);
                }

                syslog(LOG_DEBUG, "getlanaddrsfrominterface - ifname=%s ip=%s mask=%d", lan_addr->ifname, lan_addr->str, lan_addr->mask.s_addr);

                LIST_INSERT_HEAD(addrs, lan_addr, list);
                break;
        }
    }

    freeifaddrs(ifap);
#endif
    return err;
}

