/* $Id: getifaddr.h,v 1.8 2013/03/23 10:46:54 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2013 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#ifndef GETIFADDR_H_INCLUDED
#define GETIFADDR_H_INCLUDED

#include "miniupnpdtypes.h"

struct in_addr;

/* getifaddr()
 * take a network interface name and write the
 * ip v4 address as text in the buffer
 * returns: 0 success, -1 failure */
int
getifaddr(const char * ifname, char * buf, int len,
          struct in_addr * addr, struct in_addr * mask);

/* find a non link local IP v6 address for the interface.
 * if ifname is NULL, look for all interfaces */
int
find_ipv6_addr(const char * ifname,
               char * dst, int n);

/*
 * get the list of lan addresses assigned to an interface via getifaddr
 * and add it to an incoming lan_addr_list.
 */
int addlanaddrsfrominterface(struct lan_addr_list *addrs, const char *ifname);

#endif

