/*
 * firewall3 - 3rd OpenWrt UCI firewall implementation
 *
 *   Copyright (C) 2014 XiuJuan.TAN <XiuJuan.Tan@technicolor.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <string.h>
#include <netdb.h>

#include <libubox/utils.h>

#include "utils.h"
#include "options.h"
#include "ipset_entries.h"
#include "icmp_codes.h"

const struct option ipset_entry_opts[] = {
	OPT("ipset",       setmatch,         ipset_entry,     ipset),
	OPT("timeout",     int,              ipset_entry,     timeout),
	OPT("ip",          address,          ipset_entry,     ip),
	OPT("proto",       protocol,         ipset_entry,     proto),
	OPT("port",        string,           ipset_entry,     port_str),
	OPT("src_ip",      address,          ipset_entry,     ip_src),
	OPT("dest_ip",     address,          ipset_entry,     ip_dest),
	OPT("network",     address,          ipset_entry,     network),
	OPT("nomatch",     bool,             ipset_entry,     nomatch),

	{ }
};

#define TCP_PROTO_NUM          6
#define ICMPV4_PROTO_NUM       1
#define ICMPV6_PROTO_NUM       58

#define T(m, t1, t2, t3, r, o) \
	{ IPSET_METHOD_##m, \
	  IPSET_TYPE_##t1 | (IPSET_TYPE_##t2 << 8) | (IPSET_TYPE_##t3 << 16), \
	  r, o }

enum ipset_entry_optflag {
	OPT_TIMEOUT     = (1 << 0), //timeout
	OPT_IPRANGE     = (1 << 1), //ip
	OPT_PROTO       = (1 << 2), //proto
	OPT_PORTRANGE   = (1 << 3), //port
	OPT_SRC_IPRANGE = (1 << 4), //ip_src
	OPT_DST_IPRANGE = (1 << 5), //ip_dest
	OPT_NETWORK     = (1 << 6), //network
	OPT_NOMATCH     = (1 << 7), //nomatch
};

//index maps to ipset_entry_types[], DO NOT change sequences
enum ipset_entry_types_index_mapper {
	BITMAP_IP_INDEX        = 0,
	BITMAP_IP_MAC_INDEX    = 1,
	BITMAP_PORT_INDEX      = 2,
	HASH_IP_INDEX          = 3,
	HASH_NET_INDEX         = 4,
	HASH_IP_PORT_INDEX     = 5,
	HASH_NET_PORT_INDEX    = 6,
	HASH_IP_PORT_IP_INDEX  = 7,
	HASH_IP_PORT_NET_INDEX = 8,
	LIST_SET_INDEX         = 9,
};

struct ipset_entry_type {
	enum ipset_method method;
	uint32_t types;
	uint8_t required;
	uint8_t optional;
};

static struct ipset_entry_type ipset_entry_types[] = {
	T(BITMAP, IP,   UNSPEC, UNSPEC, 0, 0),                       //BITMAP_IP_INDEX
	T(BITMAP, IP,   MAC,    UNSPEC, 0, 0),                       //BITMAP_IP_MAC_INDEX
	T(BITMAP, PORT, UNSPEC, UNSPEC, 0, 0),                       //BITMAP_PORT_INDEX

	T(HASH,   IP,   UNSPEC, UNSPEC, OPT_IPRANGE,
	  OPT_TIMEOUT),                                              //HASH_IP_INDEX
	T(HASH,   NET,  UNSPEC, UNSPEC, OPT_NETWORK,
	  OPT_TIMEOUT | OPT_NOMATCH),                                //HASH_NET_INDEX
	T(HASH,   IP,   PORT,   UNSPEC, OPT_IPRANGE | OPT_PORTRANGE,
	  OPT_TIMEOUT | OPT_PROTO),                                  //HASH_IP_PORT_INDEX
	T(HASH,   NET,  PORT,   UNSPEC, OPT_NETWORK | OPT_PORTRANGE,
	  OPT_TIMEOUT | OPT_PROTO | OPT_NOMATCH),                    //HASH_NET_PORT_INDEX
	T(HASH,   IP,   PORT,   IP,     OPT_SRC_IPRANGE | OPT_DST_IPRANGE | OPT_PORTRANGE,
	  OPT_TIMEOUT | OPT_PROTO),                                  //HASH_IP_PORT_IP_INDEX
	T(HASH,   IP,   PORT,   NET,    OPT_IPRANGE | OPT_PORTRANGE | OPT_NETWORK,
	  OPT_TIMEOUT | OPT_PROTO | OPT_NOMATCH),                    //HASH_IP_PORT_NET_INDEX

	T(LIST,   SET,  UNSPEC, UNSPEC, 0, 0),                       //LIST_SET_INDEX
};

static void
ipset_entry_set_port(struct protoent *p_protoent, struct ipset_entry *p_ipset_entry)
{
	if (p_ipset_entry->port.set)
	{
		if (p_ipset_entry->port.port_min < p_ipset_entry->port.port_max)
			pr(",%s:%d-%d", p_protoent->p_name, p_ipset_entry->port.port_min, p_ipset_entry->port.port_max);
		else
			pr(",%s:%d", p_protoent->p_name, p_ipset_entry->port.port_min);
	}
	else if (p_ipset_entry->icmp.family == FAMILY_V4)
	{
		pr(",icmp:%d/%d", p_ipset_entry->icmp.type, p_ipset_entry->icmp.code_min);
	}
	else if (p_ipset_entry->icmp.family == FAMILY_V6)
	{
		pr(",icmpv6:%d/%d", p_ipset_entry->icmp.type6, p_ipset_entry->icmp.code6_min);
	}
}

static void
ipset_entry_set_ip(bool first, struct address ip)
{
	if (ip.set)
		pr("%s%s", first ? " " : ",", address_to_string(&ip, false, true));
}

static void
ipset_entry_set_ip_nocid(bool first, struct address ip)
{
	if (ip.set)
	{
		const char *ip_str = address_to_string(&ip, false, true);
		char *p = strchr(ip_str, '/');

		if (p) *p = '\0';
		pr("%s%s", first ? " " : ",", ip_str);
	}
}

static bool
ipset_entry_verify(struct uci_element *e, struct ipset_entry *ipset_entry)
{
	int i = 0;
	uint32_t typelist = 0;
	struct ipset_datatype *type;
	const char *ip_str = NULL;

	if (!(ipset_entry->ipset.ptr->enabled))
	{
		warn_elem(e, "skip entry as %s not enabled", ipset_entry->ipset.name);
		return false;
	}

	list_for_each_entry(type, &(ipset_entry->ipset.ptr->datatypes), list)
	{
		ipset_entry->ipset.dir[i] = type->dir;
		typelist |= (type->type << (i++ * 8));
		ipset_entry->typelist = typelist;
	}

	for (i = 0; i < ARRAY_SIZE(ipset_entry_types); i++)
	{
		if (ipset_entry_types[i].method == ipset_entry->ipset.ptr->method &&
		    ipset_entry_types[i].types == typelist)
		{
			//default protocol is TCP
			if (ipset_entry->proto.protocol == 0)
				ipset_entry->proto.protocol = TCP_PROTO_NUM;

			//type: ip,port,ip (2nd ip should be plain ipaddress)
			if (i == HASH_IP_PORT_IP_INDEX)
			{
				if (!strcmp(ipset_entry->ipset.dir[2],"src"))
					ip_str = address_to_string(&ipset_entry->ip_src, false, true);
				else
					ip_str = address_to_string(&ipset_entry->ip_dest, false, true);

				if ((ipset_entry->ipset.ptr->family == FAMILY_V4 && !strstr(ip_str, "/32"))
					|| (ipset_entry->ipset.ptr->family == FAMILY_V6 && !strstr(ip_str, "/128")))
				{
					if (strchr(ip_str, '/') || strchr(ip_str, '-'))
					{
						warn_elem(e, "%s is not plain ipaddress", ip_str);
						return false;
					}
				}
			}

			//required
			if (ipset_entry_types[i].required & OPT_PORTRANGE)
			{
				if (ipset_entry->port_str)
				{
					if ((ipset_entry->proto.protocol == ICMPV4_PROTO_NUM
							&& ipset_entry->ipset.ptr->family == FAMILY_V6)
						|| (ipset_entry->proto.protocol == ICMPV6_PROTO_NUM
							&& ipset_entry->ipset.ptr->family == FAMILY_V4))
					{
						warn_elem(e, "protocol & family mismatch");
						return false;
					}

					if (ipset_entry->proto.protocol == ICMPV4_PROTO_NUM
						|| ipset_entry->proto.protocol == ICMPV6_PROTO_NUM)
					{
						if (!parse_icmptype(&ipset_entry->icmp, ipset_entry->port_str, false))
						{
							warn_elem(e, "can not resolve port %s for icmp(v6)",ipset_entry->port_str);
							return false;
						}
						else
						{
							ipset_entry->icmp.family = ipset_entry->ipset.ptr->family;
						}
					}
					else
					{
						if (!parse_port(&ipset_entry->port, ipset_entry->port_str, false))
						{
							warn_elem(e, "can not resolve port %s",ipset_entry->port_str);
							return false;
						}
					}
				}
				else
				{
					warn_elem(e, "requires a port(/portrange)");
					return false;
				}
			}

			if ((ipset_entry_types[i].required & OPT_IPRANGE) &&
					!ipset_entry->ip.set)
			{
				warn_elem(e, "requires an ip(/iprange/net)");
				return false;
			}

			if ((ipset_entry_types[i].required & OPT_SRC_IPRANGE) &&
					!ipset_entry->ip_src.set)
			{
				warn_elem(e, "requires a src_ip(/iprange/net)");
				return false;
			}

			if ((ipset_entry_types[i].required & OPT_DST_IPRANGE) &&
					!ipset_entry->ip_dest.set)
			{
				warn_elem(e, "requires a dest_ip(/iprange/net)");
				return false;
			}

			if ((ipset_entry_types[i].required & OPT_NETWORK) &&
					!ipset_entry->network.set)
			{
				warn_elem(e, "requires a network(/ip/iprange)");
				return false;
			}

			//ignore
			if (!(ipset_entry_types[i].required & OPT_IPRANGE) &&
					ipset_entry->ip.set)
			{
				warn_elem(e, "ip ignored");
				ipset_entry->ip.set = false;
			}

			if (!(ipset_entry_types[i].required & OPT_SRC_IPRANGE) &&
					ipset_entry->ip_src.set)
			{
				warn_elem(e, "src_ip ignored");
				ipset_entry->ip_src.set = false;
			}

			if (!(ipset_entry_types[i].required & OPT_DST_IPRANGE) &&
					ipset_entry->ip_dest.set)
			{
				warn_elem(e, "dest_ip ignored");
				ipset_entry->ip_dest.set = false;
			}

			if (!(ipset_entry_types[i].required & OPT_NETWORK) &&
					ipset_entry->network.set)
			{
				warn_elem(e, "network ignored");
				ipset_entry->network.set = false;
			}

			if (ipset_entry->timeout != 0)
			{
				if (ipset_entry_types[i].optional & OPT_TIMEOUT) //optional
				{
					if (ipset_entry->ipset.ptr->timeout == 0)
					{
						warn_elem(e, "timeout ignored (not in ipset Header)");
						ipset_entry->timeout = 0;
					}

				}
				else //not in optional
				{
					warn_elem(e, "timeout ignored");
					ipset_entry->timeout = 0;
				}
			}

			//not in optional
			if (!(ipset_entry_types[i].optional & OPT_NOMATCH) &&
					ipset_entry->nomatch)
			{
				warn_elem(e, "nomatch ignored");
				ipset_entry->nomatch = 0;
			}

			return true;
		}
	}
	warn_elem(e, "has an invalid combination of storage method and matches");

	return false;
}

static void
create_ipset_entry(struct ipset_entry *ipset_entry, struct state *state)
{
	struct protoent *p_protoent = getprotobynumber(ipset_entry->proto.protocol);

	pr("add %s ",ipset_entry->ipset.name);

	if (ipset_entry->typelist == ipset_entry_types[HASH_IP_INDEX].types //ip
		|| ipset_entry->typelist == ipset_entry_types[HASH_NET_INDEX].types //net
		|| ipset_entry->typelist == ipset_entry_types[HASH_IP_PORT_INDEX].types //ip,port
		|| ipset_entry->typelist == ipset_entry_types[HASH_NET_PORT_INDEX].types) //net,port
	{
		if (ipset_entry->ipset.ptr->family == FAMILY_V6)
			ipset_entry_set_ip_nocid(1, ipset_entry->ip);
		else
			ipset_entry_set_ip(1, ipset_entry->ip);
		ipset_entry_set_ip(1, ipset_entry->network);
		ipset_entry_set_port(p_protoent, ipset_entry);
	}
	else if (ipset_entry->typelist == ipset_entry_types[HASH_IP_PORT_IP_INDEX].types) //ip,port,ip
	{
		if (!strcmp(ipset_entry->ipset.dir[0],"src"))
			if (ipset_entry->ipset.ptr->family == FAMILY_V6) // use plain IP addresses
				ipset_entry_set_ip_nocid(1, ipset_entry->ip_src);
			else
				ipset_entry_set_ip(1, ipset_entry->ip_src);
		else
			if (ipset_entry->ipset.ptr->family == FAMILY_V6) // use plain IP addresses
				ipset_entry_set_ip_nocid(1, ipset_entry->ip_dest);
			else
				ipset_entry_set_ip(1, ipset_entry->ip_dest);

		ipset_entry_set_port(p_protoent, ipset_entry);

		if (!strcmp(ipset_entry->ipset.dir[2],"src"))//plain ipaddress
			ipset_entry_set_ip_nocid(0, ipset_entry->ip_src);
		else
			ipset_entry_set_ip_nocid(0, ipset_entry->ip_dest);
	}
	else if (ipset_entry->typelist == ipset_entry_types[HASH_IP_PORT_NET_INDEX].types) //ip,port,net
	{
		if (ipset_entry->ipset.ptr->family == FAMILY_V6)
			ipset_entry_set_ip_nocid(1, ipset_entry->ip);
		else
			ipset_entry_set_ip(1, ipset_entry->ip);
		ipset_entry_set_port(p_protoent, ipset_entry);
		ipset_entry_set_ip(0, ipset_entry->network);
	}

	if (ipset_entry->timeout > 0)
		pr(" timeout %u", ipset_entry->timeout);

	if (ipset_entry->nomatch)
		pr(" nomatch");

	pr("\n");
}

void
create_ipset_entries(struct state *state)
{
	bool exec = false;
	struct ipset_entry *ipset_entry;

	list_for_each_entry(ipset_entry, &state->ipset_entries, list)
	{
		if (!exec)
		{
			exec = command_pipe(false, "ipset", "-exist", "-");

			if (!exec)
				return;
		}

		create_ipset_entry(ipset_entry, state);
	}

	if (exec)
	{
		pr("quit\n");
		command_close();
	}
}

void
flush_ipset_entries(struct state *state)
{
	bool exec = false;
	struct ipset_entry *ipset_entry;

	/* flush ipset entries */
	list_for_each_entry(ipset_entry, &state->ipset_entries, list)
	{
		if (!exec)
		{
			exec = command_pipe(false, "ipset", "-exist", "-");

			if (!exec)
				return;
		}

		info(" * Flushing entries of ipset %s", ipset_entry->ipset.name);

		pr("flush %s\n", ipset_entry->ipset.name);
	}

	if (exec)
	{
		pr("quit\n");
		command_close();
	}
}

void
load_ipset_entries(struct state *state)
{
	struct uci_section *s;
	struct uci_element *e;
	struct ipset_entry *ipset_entry;

	if (state == NULL)
		return;

	INIT_LIST_HEAD(&state->ipset_entries);

	if (state->package == NULL)
		return;

	uci_foreach_element(&state->package->sections, e)
	{
		s = uci_to_section(e);

		if (strcmp(s->type, "ipset_entry"))
			continue;

		ipset_entry = calloc(1, sizeof(struct ipset_entry));
		if (ipset_entry == NULL)
			continue;

		parse_options(ipset_entry, ipset_entry_opts, s);

		if (!ipset_entry->ipset.set)
		{
			warn_elem(e, "ipset_entry must reference to one ipset");
		}
		else if (!(ipset_entry->ipset.ptr = lookup_ipset(state, ipset_entry->ipset.name)))
		{
			warn_elem(e, "ipset_entry refers to unknown ipset '%s'", ipset_entry->ipset.name);
		}
		else if (ipset_entry_verify(e, ipset_entry))
		{
			list_add_tail(&ipset_entry->list, &state->ipset_entries);
			continue;
		}

		free_object(ipset_entry, ipset_entry_opts);
	}
}
