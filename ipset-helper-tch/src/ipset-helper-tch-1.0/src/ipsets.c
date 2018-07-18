/*
 * firewall3 - 3rd OpenWrt UCI firewall implementation
 *
 *   Copyright (C) 2013 Jo-Philipp Wich <jow@openwrt.org>
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

#include "libubox/utils.h"
#include "utils.h"
#include "options.h"
#include "ipsets.h"


const struct option ipset_opts[] = {
	OPT("enabled",       bool,           ipset,     enabled),

	OPT("name",          string,         ipset,     name),
	OPT("family",        family,         ipset,     family),

	OPT("storage",       ipset_method,   ipset,     method),
	LIST("match",        ipset_datatype, ipset,     datatypes),

	OPT("iprange",       address,        ipset,     iprange),
	OPT("portrange",     port,           ipset,     portrange),

	OPT("netmask",       int,            ipset,     netmask),
	OPT("maxelem",       int,            ipset,     maxelem),
	OPT("hashsize",      int,            ipset,     hashsize),
	OPT("timeout",       int,            ipset,     timeout),

	OPT("external",      string,         ipset,     external),

	{ }
};

#define T(m, t1, t2, t3, r, o) \
	{ IPSET_METHOD_##m, \
	  IPSET_TYPE_##t1 | (IPSET_TYPE_##t2 << 8) | (IPSET_TYPE_##t3 << 16), \
	  r, o }

enum ipset_optflag {
	OPT_IPRANGE   = (1 << 0),
	OPT_PORTRANGE = (1 << 1),
	OPT_NETMASK   = (1 << 2),
	OPT_HASHSIZE  = (1 << 3),
	OPT_MAXELEM   = (1 << 4),
	OPT_FAMILY    = (1 << 5),
};

struct ipset_t {
	enum ipset_method method;
	uint32_t types;
	uint8_t required;
	uint8_t optional;
};

static struct ipset_t ipset_types[] = {
	T(BITMAP, IP,   UNSPEC, UNSPEC, OPT_IPRANGE, OPT_NETMASK),
	T(BITMAP, IP,   MAC,    UNSPEC, OPT_IPRANGE, 0),
	T(BITMAP, PORT, UNSPEC, UNSPEC, OPT_PORTRANGE, 0),

	T(HASH,   IP,   UNSPEC, UNSPEC, 0,
	  OPT_FAMILY | OPT_HASHSIZE | OPT_MAXELEM | OPT_NETMASK),
	T(HASH,   NET,  UNSPEC, UNSPEC, 0,
	  OPT_FAMILY | OPT_HASHSIZE | OPT_MAXELEM),
	T(HASH,   IP,   PORT,   UNSPEC, 0,
	  OPT_FAMILY | OPT_HASHSIZE | OPT_MAXELEM),
	T(HASH,   NET,  PORT,   UNSPEC, 0,
	  OPT_FAMILY | OPT_HASHSIZE | OPT_MAXELEM),
	T(HASH,   IP,   PORT,   IP,     0,
	  OPT_FAMILY | OPT_HASHSIZE | OPT_MAXELEM),
	T(HASH,   IP,   PORT,   NET,    0,
	  OPT_FAMILY | OPT_HASHSIZE | OPT_MAXELEM),

	T(LIST,   SET,  UNSPEC, UNSPEC, 0, OPT_MAXELEM),
};


static bool
check_types(struct uci_element *e, struct ipset *ipset)
{
	int i = 0;
	uint32_t typelist = 0;
	struct ipset_datatype *type;

	list_for_each_entry(type, &ipset->datatypes, list)
	{
		if (i >= 3)
		{
			warn_elem(e, "must not have more than 3 datatypes assigned");
			return false;
		}

		typelist |= (type->type << (i++ * 8));
	}

	/* find a suitable storage method if none specified */
	if (ipset->method == IPSET_METHOD_UNSPEC)
	{
		for (i = 0; i < ARRAY_SIZE(ipset_types); i++)
		{
			/* skip type for v6 if it does not support family */
			if (ipset->family != FAMILY_V4 &&
			    !(ipset_types[i].optional & OPT_FAMILY))
				continue;

			if (ipset_types[i].types == typelist)
			{
				ipset->method = ipset_types[i].method;

				warn_elem(e, "defines no storage method, assuming '%s'",
				          ipset_method_names[ipset->method]);

				break;
			}
		}
	}

	//typelist |= ipset->method;

	for (i = 0; i < ARRAY_SIZE(ipset_types); i++)
	{
		if (ipset_types[i].method == ipset->method &&
		    ipset_types[i].types == typelist)
		{
			if (!ipset->external)
			{
				if ((ipset_types[i].required & OPT_IPRANGE) &&
					!ipset->iprange.set)
				{
					warn_elem(e, "requires an ip range");
					return false;
				}

				if ((ipset_types[i].required & OPT_PORTRANGE) &&
				    !ipset->portrange.set)
				{
					warn_elem(e, "requires a port range");
					return false;
				}

				if (!(ipset_types[i].required & OPT_IPRANGE) &&
				    ipset->iprange.set)
				{
					warn_elem(e, "iprange ignored");
					ipset->iprange.set = false;
				}

				if (!(ipset_types[i].required & OPT_PORTRANGE) &&
				    ipset->portrange.set)
				{
					warn_elem(e, "portrange ignored");
					ipset->portrange.set = false;
				}

				if (!(ipset_types[i].optional & OPT_NETMASK) &&
				    ipset->netmask > 0)
				{
					warn_elem(e, "netmask ignored");
					ipset->netmask = 0;
				}

				if (!(ipset_types[i].optional & OPT_HASHSIZE) &&
				    ipset->hashsize > 0)
				{
					warn_elem(e, "hashsize ignored");
					ipset->hashsize = 0;
				}

				if (!(ipset_types[i].optional & OPT_MAXELEM) &&
				    ipset->maxelem > 0)
				{
					warn_elem(e, "maxelem ignored");
					ipset->maxelem = 0;
				}

				if (!(ipset_types[i].optional & OPT_FAMILY) &&
				    ipset->family != FAMILY_V4)
				{
					warn_elem(e, "family ignored");
					ipset->family = FAMILY_V4;
				}
			}

			return true;
		}
	}

	warn_elem(e, "has an invalid combination of storage method and matches");
	return false;
}

struct ipset *
alloc_ipset(void)
{
	struct ipset *ipset;

	ipset = calloc(1, sizeof(*ipset));
	if (!ipset)
		return NULL;

	INIT_LIST_HEAD(&ipset->datatypes);

	ipset->enabled = true;
	ipset->family  = FAMILY_V4;

	return ipset;
}

static void
add_ipsets(struct state *state, struct uci_package *p)
{
	struct uci_section *s;
	struct uci_element *e;
	struct ipset *ipset;

	uci_foreach_element(&p->sections, e)
	{
		s = uci_to_section(e);

		if (strcmp(s->type, "ipset"))
			continue;

		ipset = alloc_ipset();

		if (!ipset)
			continue;

		parse_options(ipset, ipset_opts, s);

		if (ipset->external)
		{
			if (!*ipset->external)
				ipset->external = NULL;
			else if (!ipset->name)
				ipset->name = ipset->external;
		}
		if (!ipset->name && !s->anonymous)
		{
			ipset->name = s->e.name;
		}


		if (!ipset->name || !*ipset->name)
		{
			warn_elem(e, "must have a name assigned");
		}
		else if (lookup_ipset(state, ipset->name) != NULL)
		{
			warn_elem(e, "has duplicated set name '%s'", ipset->name);
		}
		else if (ipset->family == FAMILY_ANY)
		{
			warn_elem(e, "must not have family 'any'");
		}
		else if (ipset->iprange.set && ipset->family != ipset->iprange.family)
		{
			warn_elem(e, "has iprange of wrong address family");
		}
		else if (list_empty(&ipset->datatypes))
		{
			warn_elem(e, "has no datatypes assigned");
		}
		else if (check_types(e, ipset))
		{
			list_add_tail(&ipset->list, &state->ipsets);
			continue;
		}

		free_ipset(ipset);
	}
}

void
load_ipsets(struct state *state)
{
	if (state == NULL)
		return;

	INIT_LIST_HEAD(&state->ipsets);

	if (state->package == NULL)
		return;

	add_ipsets(state, state->package);
}


static void
create_ipset(struct ipset *ipset, struct state *state)
{
	bool first = true;

	struct ipset_datatype *type;

	info(" * Creating ipset %s", ipset->name);

	first = true;
	pr("create %s %s", ipset->name, ipset_method_names[ipset->method]);

	list_for_each_entry(type, &ipset->datatypes, list)
	{
		pr("%c%s", first ? ':' : ',', ipset_type_names[type->type]);
		first = false;
	}

	if (ipset->method == IPSET_METHOD_HASH)
		pr(" family inet%s", (ipset->family == FAMILY_V4) ? "" : "6");

	if (ipset->iprange.set)
	{
		pr(" range %s", address_to_string(&ipset->iprange, false, true));
	}
	else if (ipset->portrange.set)
	{
		pr(" range %u-%u",
		       ipset->portrange.port_min, ipset->portrange.port_max);
	}

	if (ipset->timeout > 0)
		pr(" timeout %u", ipset->timeout);

	if (ipset->maxelem > 0)
		pr(" maxelem %u", ipset->maxelem);

	if (ipset->netmask > 0)
		pr(" netmask %u", ipset->netmask);

	if (ipset->hashsize > 0)
		pr(" hashsize %u", ipset->hashsize);

	pr("\n");
}

void
create_ipsets(struct state *state)
{
	int tries;
	bool exec = false;
	struct ipset *ipset;


	/* spawn ipsets */
	list_for_each_entry(ipset, &state->ipsets, list)
	{
		if (ipset->external)
			continue;

		if (!exec)
		{
			exec = command_pipe(false, "ipset", "-exist", "-");

			if (!exec)
				return;
		}

		create_ipset(ipset, state);
	}

	if (exec)
	{
		pr("quit\n");
		command_close();
	}

	/* wait for ipsets to appear */
	list_for_each_entry(ipset, &state->ipsets, list)
	{
		if (ipset->external)
			continue;

		for (tries = 0; !check_ipset(ipset) && tries < 10; tries++)
			usleep(50000);
	}
}

void
destroy_ipsets(struct state *state)
{
	int tries;
	bool exec = false;
	struct ipset *ipset;

	/* destroy ipsets */
	list_for_each_entry(ipset, &state->ipsets, list)
	{
		if (!exec)
		{
			exec = command_pipe(false, "ipset", "-exist", "-");

			if (!exec)
				return;
		}

		info(" * Deleting ipset %s", ipset->name);

		pr("flush %s\n", ipset->name);
		pr("destroy %s\n", ipset->name);
	}

	if (exec)
	{
		pr("quit\n");
		command_close();
	}

	/* wait for ipsets to disappear */
	list_for_each_entry(ipset, &state->ipsets, list)
	{
		if (ipset->external)
			continue;

		for (tries = 0; check_ipset(ipset) && tries < 10; tries++)
			usleep(50000);
	}
}

struct ipset *
lookup_ipset(struct state *state, const char *name)
{
	struct ipset *s;

	if (list_empty(&state->ipsets))
		return NULL;

	list_for_each_entry(s, &state->ipsets, list)
	{
		if (strcmp(s->name, name))
			continue;

		return s;
	}

	return NULL;
}

bool
check_ipset(struct ipset *set)
{
	bool rv = false;

	socklen_t sz;
	int s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	struct ip_set_req_version req_ver;
	struct ip_set_req_get_set req_name;

	if (s < 0 || fcntl(s, F_SETFD, FD_CLOEXEC))
		goto out;

	sz = sizeof(req_ver);
	req_ver.op = IP_SET_OP_VERSION;

	if (getsockopt(s, SOL_IP, SO_IP_SET, &req_ver, &sz))
		goto out;

	sz = sizeof(req_name);
	req_name.op = IP_SET_OP_GET_BYNAME;
	req_name.version = req_ver.version;
	snprintf(req_name.set.name, IPSET_MAXNAMELEN - 1, "%s",
	         set->external ? set->external : set->name);

	if (getsockopt(s, SOL_IP, SO_IP_SET, &req_name, &sz))
		goto out;

	rv = ((sz == sizeof(req_name)) && (req_name.set.index != IPSET_INVALID_ID));

out:
	if (s >= 0)
		close(s);

	return rv;
}

int
update_state(struct state *state)
{
	struct ipset	*ipset;
	struct ipset	*tmp;
	int		count = 0;

	if (state == NULL)
		return count;

	list_for_each_entry_safe(ipset, tmp, &state->ipsets, list)
	{
		if (check_ipset(ipset))
		{
			count++;
		}
		else
		{
			list_del(&ipset->list);
			free_ipset(ipset);
		}
	}

	return count;
}

void
merge_state(struct state *dst, const struct state *src)
{
	if (dst == NULL || src == NULL || src->package == NULL)
		return;

	add_ipsets(dst, src->package);
}
