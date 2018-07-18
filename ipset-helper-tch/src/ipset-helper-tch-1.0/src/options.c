/*
 * options.c
 *
 *  Created on: Jun 14, 2016
 *      Author: geertsn
 */

#include <stdio.h>
#include <arpa/inet.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <netdb.h>

#include <libubox/utils.h>

#include "options.h"
#include "utils.h"
#include "icmp_codes.h"

const char *ipset_method_names[__IPSET_METHOD_MAX] = {
	"(bug)",
	"bitmap",
	"hash",
	"list",
};

const char *ipset_type_names[__IPSET_TYPE_MAX] = {
	"(bug)",
	"ip",
	"port",
	"mac",
	"net",
	"set",
};

static bool
put_value(void *ptr, void *val, int elem_size, bool is_list)
{
	void *copy;

	if (is_list)
	{
		copy = malloc(elem_size);

		if (!copy)
			return false;

		memcpy(copy, val, elem_size);
		list_add_tail((struct list_head *)copy, (struct list_head *)ptr);
		return true;
	}

	memcpy(ptr, val, elem_size);
	return false;
}

static bool
parse_enum(void *ptr, const char *val, const char **values, int min, int max)
{
	int i, l = strlen(val);

	if (l > 0)
	{
		for (i = 0; i <= (max - min); i++)
		{
			if (!strncasecmp(val, values[i], l))
			{
				*((int *)ptr) = min + i;
				return true;
			}
		}
	}

	return false;
}

bool
parse_bool(void *ptr, const char *val, bool is_list)
{
	if (!strcmp(val, "true") || !strcmp(val, "yes") || !strcmp(val, "1"))
		*((bool *)ptr) = true;
	else
		*((bool *)ptr) = false;

	return true;
}

bool
parse_int(void *ptr, const char *val, bool is_list)
{
	char *e;
	int n = strtol(val, &e, 0);

	if (e == val || *e)
		return false;

	*((int *)ptr) = n;

	return true;
}

bool
parse_string(void *ptr, const char *val, bool is_list)
{
	*((char **)ptr) = (char *)val;
	return true;
}

bool
parse_address(void *ptr, const char *val, bool is_list)
{
	struct address addr = { };
	struct in_addr v4;
	struct in6_addr v6;
	char *p = NULL, *m = NULL, *s, *e;
	int bits = -1;

	if (*val == '!')
	{
		addr.invert = true;
		while (isspace(*++val));
	}

	s = strdup(val);

	if (!s)
		return false;

	if ((m = strchr(s, '/')) != NULL)
		*m++ = 0;
	else if ((p = strchr(s, '-')) != NULL)
		*p++ = 0;

	if (inet_pton(AF_INET6, s, &v6))
	{
		addr.family = FAMILY_V6;
		addr.address.v6 = v6;

		if (m)
		{
			if (!inet_pton(AF_INET6, m, &v6))
			{
				bits = strtol(m, &e, 10);

				if ((*e != 0) || !bitlen2netmask(addr.family, bits, &v6))
					goto fail;
			}

			addr.mask.v6 = v6;
		}
		else if (p)
		{
			if (!inet_pton(AF_INET6, p, &addr.mask.v6))
				goto fail;

			addr.range = true;
		}
		else
		{
			memset(addr.mask.v6.s6_addr, 0xFF, 16);
		}
	}
	else if (inet_pton(AF_INET, s, &v4))
	{
		addr.family = FAMILY_V4;
		addr.address.v4 = v4;

		if (m)
		{
			if (!inet_pton(AF_INET, m, &v4))
			{
				bits = strtol(m, &e, 10);

				if ((*e != 0) || !bitlen2netmask(addr.family, bits, &v4))
					goto fail;
			}

			addr.mask.v4 = v4;
		}
		else if (p)
		{
			if (!inet_pton(AF_INET, p, &addr.mask.v4))
				goto fail;

			addr.range = true;
		}
		else
		{
			addr.mask.v4.s_addr = 0xFFFFFFFF;
		}
	}
	else
	{
		goto fail;
	}

	free(s);
	addr.set = true;
	put_value(ptr, &addr, sizeof(addr), is_list);
	return true;

fail:
	free(s);
	return false;
}

bool
parse_port(void *ptr, const char *val, bool is_list)
{
	struct port range = { };
	uint16_t n;
	uint16_t m;
	char *p;

	if (*val == '!')
	{
		range.invert = true;
		while (isspace(*++val));
	}

	n = strtoul(val, &p, 10);

	if (errno == ERANGE || errno == EINVAL)
		return false;

	if (*p && *p != '-' && *p != ':')
		return false;

	if (*p)
	{
		m = strtoul(++p, NULL, 10);

		if (errno == ERANGE || errno == EINVAL || m < n)
			return false;

		range.port_min = n;
		range.port_max = m;
	}
	else
	{
		range.port_min = n;
		range.port_max = n;
	}

	range.set = true;
	put_value(ptr, &range, sizeof(range), is_list);
	return true;
}

bool
parse_family(void *ptr, const char *val, bool is_list)
{
	if (!strcmp(val, "any") || !strcmp(val, "*"))
		*((enum family *)ptr) = FAMILY_ANY;
	else if (!strcmp(val, "inet") || strrchr(val, '4'))
		*((enum family *)ptr) = FAMILY_V4;
	else if (!strcmp(val, "inet6") || strrchr(val, '6'))
		*((enum family *)ptr) = FAMILY_V6;
	else
		return false;

	return true;
}

bool
parse_icmptype(void *ptr, const char *val, bool is_list)
{
	struct icmptype icmp = { };
	bool v4 = false;
	bool v6 = false;
	char *p;
	int i;

	for (i = 0; i < ARRAY_SIZE(icmptype_list_v4); i++)
	{
		if (!strcmp(val, icmptype_list_v4[i].name))
		{
			icmp.type     = icmptype_list_v4[i].type;
			icmp.code_min = icmptype_list_v4[i].code_min;
			icmp.code_max = icmptype_list_v4[i].code_max;

			v4 = true;
			break;
		}
	}

	for (i = 0; i < ARRAY_SIZE(icmptype_list_v6); i++)
	{
		if (!strcmp(val, icmptype_list_v6[i].name))
		{
			icmp.type6     = icmptype_list_v6[i].type;
			icmp.code6_min = icmptype_list_v6[i].code_min;
			icmp.code6_max = icmptype_list_v6[i].code_max;

			v6 = true;
			break;
		}
	}

	if (!v4 && !v6)
	{
		i = strtoul(val, &p, 10);

		if ((p == val) || (*p != '/' && *p != 0) || (i > 0xFF))
			return false;

		icmp.type = i;

		if (*p == '/')
		{
			val = ++p;
			i = strtoul(val, &p, 10);

			if ((p == val) || (*p != 0) || (i > 0xFF))
				return false;

			icmp.code_min = i;
			icmp.code_max = i;
		}
		else
		{
			icmp.code_min = 0;
			icmp.code_max = 0xFF;
		}

		icmp.type6     = icmp.type;
		icmp.code6_min = icmp.code_max;
		icmp.code6_max = icmp.code_max;

		v4 = true;
		v6 = true;
	}

	icmp.family = (v4 && v6) ? FAMILY_ANY
	                         : (v6 ? FAMILY_V6 : FAMILY_V4);

	put_value(ptr, &icmp, sizeof(icmp), is_list);
	return true;
}

bool
parse_protocol(void *ptr, const char *val, bool is_list)
{
	struct protocol proto = { };
	struct protoent *ent;
	char *e;

	if (*val == '!')
	{
		proto.invert = true;
		while (isspace(*++val));
	}

	if (!strcmp(val, "all") || !strcmp(val, "any") || !strcmp(val, "*"))
	{
		proto.any = true;
		put_value(ptr, &proto, sizeof(proto), is_list);
		return true;
	}
	else if (!strcmp(val, "icmpv6"))
	{
		val = "ipv6-icmp";
	}
	else if (!strcmp(val, "tcpudp"))
	{
		proto.protocol = 6;
		if (put_value(ptr, &proto, sizeof(proto), is_list))
		{
			proto.protocol = 17;
			put_value(ptr, &proto, sizeof(proto), is_list);
		}

		return true;
	}

	ent = getprotobyname(val);

	if (ent)
	{
		proto.protocol = ent->p_proto;
		put_value(ptr, &proto, sizeof(proto), is_list);
		return true;
	}

	proto.protocol = strtoul(val, &e, 10);

	if ((e == val) || (*e != 0))
		return false;

	put_value(ptr, &proto, sizeof(proto), is_list);
	return true;
}

bool
parse_ipset_method(void *ptr, const char *val, bool is_list)
{
	return parse_enum(ptr, val, &ipset_method_names[IPSET_METHOD_BITMAP],
	                  IPSET_METHOD_BITMAP, IPSET_METHOD_LIST);
}

bool
parse_ipset_datatype(void *ptr, const char *val, bool is_list)
{
	struct ipset_datatype type = { };

	type.dir = "src";

	if (!strncmp(val, "dest_", 5))
	{
		val += 5;
		type.dir = "dst";
	}
	else if (!strncmp(val, "dst_", 4))
	{
		val += 4;
		type.dir = "dst";
	}
	else if (!strncmp(val, "src_", 4))
	{
		val += 4;
		type.dir = "src";
	}

	if (parse_enum(&type.type, val, &ipset_type_names[IPSET_TYPE_IP],
	               IPSET_TYPE_IP, IPSET_TYPE_SET))
	{
		put_value(ptr, &type, sizeof(type), is_list);
		return true;
	}

	return false;
}

bool
parse_setmatch(void *ptr, const char *val, bool is_list)
{
	struct setmatch *m = ptr;
	char *p, *s;
	int i;

	if (*val == '!')
	{
		m->invert = true;
		while (isspace(*++val));
	}

	if (!(s = strdup(val)))
		return false;

	if (!(p = strtok(s, " \t")))
	{
		free(s);
		return false;
	}

	strncpy(m->name, p, sizeof(m->name));

	for (i = 0, p = strtok(NULL, " \t,");
	     i < 3 && p != NULL;
	     i++, p = strtok(NULL, " \t,"))
	{
		if (!strncmp(p, "dest", 4) || !strncmp(p, "dst", 3))
			m->dir[i] = "dst";
		else if (!strncmp(p, "src", 3))
			m->dir[i] = "src";
	}

	free(s);

	m->set = true;
	return true;
}

bool
parse_options(void *s, const struct option *opts,
                  struct uci_section *section)
{
	char *p, *v;
	bool known;
	struct uci_element *e, *l;
	struct uci_option *o;
	const struct option *opt;
	struct list_head *dest;
	bool valid = true;

	uci_foreach_element(&section->options, e)
	{
		o = uci_to_option(e);
		known = false;

		for (opt = opts; opt->name; opt++)
		{
			if (!opt->parse)
				continue;

			if (strcmp(opt->name, e->name))
				continue;

			if (o->type == UCI_TYPE_LIST)
			{
				if (!opt->elem_size)
				{
					warn_elem(e, "must not be a list");
					valid = false;
				}
				else
				{
					dest = (struct list_head *)((char *)s + opt->offset);

					uci_foreach_element(&o->v.list, l)
					{
						if (!l->name)
							continue;

						if (!opt->parse(dest, l->name, true))
						{
							warn_elem(e, "has invalid value '%s'", l->name);
							valid = false;
							continue;
						}
					}
				}
			}
			else
			{
				v = o->v.string;

				if (!v)
					continue;

				if (!opt->elem_size)
				{
					if (!opt->parse((char *)s + opt->offset, o->v.string, false))
					{
						warn_elem(e, "has invalid value '%s'", o->v.string);
						valid = false;
					}
				}
				else
				{
					dest = (struct list_head *)((char *)s + opt->offset);

					for (p = strtok(v, " \t"); p != NULL; p = strtok(NULL, " \t"))
					{
						if (!opt->parse(dest, p, true))
						{
							warn_elem(e, "has invalid value '%s'", p);
							valid = false;
							continue;
						}
					}
				}
			}

			known = true;
			break;
		}

		if (!known)
			warn_elem(e, "is unknown");
	}

	return valid;
}


const char *
address_to_string(struct address *address, bool allow_invert, bool as_cidr)
{
	char *p, ip[INET6_ADDRSTRLEN];
	static char buf[INET6_ADDRSTRLEN * 2 + 2];

	p = buf;

	if (address->invert && allow_invert)
		p += sprintf(p, "!");

	inet_ntop(address->family == FAMILY_V4 ? AF_INET : AF_INET6,
	          &address->address.v4, ip, sizeof(ip));

	p += sprintf(p, "%s", ip);

	if (address->range)
	{
		inet_ntop(address->family == FAMILY_V4 ? AF_INET : AF_INET6,
		          &address->mask.v4, ip, sizeof(ip));

		p += sprintf(p, "-%s", ip);
	}
	else if (!as_cidr)
	{
		inet_ntop(address->family == FAMILY_V4 ? AF_INET : AF_INET6,
		          &address->mask.v4, ip, sizeof(ip));

		p += sprintf(p, "/%s", ip);
	}
	else
	{
		p += sprintf(p, "/%u", netmask2bitlen(address->family,
		                                      &address->mask.v6));
	}

	return buf;
}
