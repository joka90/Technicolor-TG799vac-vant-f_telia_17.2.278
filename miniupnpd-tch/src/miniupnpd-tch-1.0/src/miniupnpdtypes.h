/* $Id: miniupnpdtypes.h,v 1.5 2012/09/27 15:47:15 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2012 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */
#ifndef MINIUPNPDTYPES_H_INCLUDED
#define MINIUPNPDTYPES_H_INCLUDED

#include "config.h"
#include <netinet/in.h>
#include <net/if.h>
#include <sys/queue.h>

/* structure and list for storing lan addresses
 * with ascii representation and mask */
struct lan_addr_s {
	char ifname[IFNAMSIZ];	/* example: eth0 */
#ifdef ENABLE_IPV6
	unsigned int index;		/* use if_nametoindex() */
#endif
	char str[16];	/* example: 192.168.0.1 */
	struct in_addr addr, mask;	/* ip/mask */
#ifdef MULTIPLE_EXTERNAL_IP
	char ext_ip_str[16];
	struct in_addr ext_ip_addr;
#endif
	LIST_ENTRY(lan_addr_s) list;
};
LIST_HEAD(lan_addr_list, lan_addr_s);

/* structure and list for storing extiface rules*/
/* ext_iface_rule=LAN_Clients:192.168.1.1:192.168.1.142:wan:WANIPonnection:WANIPConn1 */
struct extiface_rule_s {
    char *name; /* name of the external interface rule */
    char *startip;  /* client ip - start of range - example: 192.168.1.1 */
    char *endip;  /* client ip - end of range - example: 192.168.1.142 */
    char *ifname; /* name of external interface. example: wan */
    char *servicetype; /* upnp service type - example: WANIPConnection */
	char *serviceid; /* upnp service id - example: WANIPConn1 */
    LIST_ENTRY(extiface_rule_s) list;
};
LIST_HEAD(extiface_rule_list, extiface_rule_s);


#endif
