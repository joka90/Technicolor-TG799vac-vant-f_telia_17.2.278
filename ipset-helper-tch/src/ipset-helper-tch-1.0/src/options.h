/*
 * options.h
 *
 *  Created on: Jun 13, 2016
 *      Author: geertsn
 */

#include <stdbool.h>
#include <netinet/ip.h>
#include <netinet/ether.h>
#include <sys/queue.h>


#include <libubox/list.h>
#include <uci.h>

#ifndef SRC_OPTIONS_H_
#define SRC_OPTIONS_H_

#include <stdint.h>

enum family
{
	FAMILY_ANY = 0,
	FAMILY_V4  = 4,
	FAMILY_V6  = 5,
};

enum ipset_method
{
	IPSET_METHOD_UNSPEC = 0,
	IPSET_METHOD_BITMAP = 1,
	IPSET_METHOD_HASH   = 2,
	IPSET_METHOD_LIST   = 3,

	__IPSET_METHOD_MAX
};

enum ipset_type
{
	IPSET_TYPE_UNSPEC = 0,
	IPSET_TYPE_IP     = 1,
	IPSET_TYPE_PORT   = 2,
	IPSET_TYPE_MAC    = 3,
	IPSET_TYPE_NET    = 4,
	IPSET_TYPE_SET    = 5,

	__IPSET_TYPE_MAX
};

extern const char *ipset_method_names[__IPSET_METHOD_MAX];
extern const char *ipset_type_names[__IPSET_TYPE_MAX];

struct ipset_datatype
{
	struct list_head list;
	enum ipset_type type;
	const char *dir;
};

struct setmatch
{
	bool set;
	bool invert;
	char name[32];
	const char *dir[3];
	struct ipset *ptr;
};

struct address
{
	struct list_head list;

	bool set;
	bool range;
	bool invert;
	bool resolved;
	enum family family;
	union {
		struct in_addr v4;
		struct in6_addr v6;
		struct ether_addr mac;
	} address;
	union {
		struct in_addr v4;
		struct in6_addr v6;
		struct ether_addr mac;
	} mask;
};


struct mac
{
	struct list_head list;

	bool set;
	bool invert;
	struct ether_addr mac;
};

struct protocol
{
	struct list_head list;

	bool any;
	bool invert;
	uint32_t protocol;
};

struct port
{
	struct list_head list;

	bool set;
	bool invert;
	uint16_t port_min;
	uint16_t port_max;
};

struct icmptype
{
	struct list_head list;

	bool invert;
	enum family family;
	uint8_t type;
	uint8_t code_min;
	uint8_t code_max;
	uint8_t type6;
	uint8_t code6_min;
	uint8_t code6_max;
};

struct ipset
{
	struct list_head list;

	bool enabled;
	const char *name;
	enum family family;

	enum ipset_method method;
	struct list_head datatypes;

	struct address iprange;
	struct port portrange;

	int netmask;
	int maxelem;
	int hashsize;

	int timeout;

	const char *external;

	uint32_t flags[2];
};

struct ipset_entry
{
	struct list_head list;

	struct setmatch ipset;
	int timeout;

	struct address ip;

	struct protocol proto;

	const char *port_str;
	struct port port;
	struct icmptype icmp;

	struct address ip_src;
	struct address ip_dest;

	struct address network;
	bool nomatch;

	uint32_t typelist;
};

struct state
{
	struct uci_context *uci;
	struct uci_package *package;
	struct list_head ipsets;
	struct list_head ipset_entries;

	bool statefile;
};

struct option
{
	const char *name;
	bool (*parse)(void *, const char *, bool);
	uintptr_t offset;
	size_t elem_size;
};

#define OPT(name, parse, structure, member) \
	{ name, parse_##parse, offsetof(struct structure, member) }
#define LIST(name, parse, structure, member) \
	{ name, parse_##parse, offsetof(struct structure, member), \
	  sizeof(struct structure) }

bool parse_bool(void *ptr, const char *val, bool is_list);
bool parse_int(void *ptr, const char *val, bool is_list);
bool parse_string(void *ptr, const char *val, bool is_list);
bool parse_address(void *ptr, const char *val, bool is_list);
bool parse_port(void *ptr, const char *val, bool is_list);
bool parse_family(void *ptr, const char *val, bool is_list);
bool parse_icmptype(void *ptr, const char *val, bool is_list);
bool parse_protocol(void *ptr, const char *val, bool is_list);

bool parse_ipset_method(void *ptr, const char *val, bool is_list);
bool parse_ipset_datatype(void *ptr, const char *val, bool is_list);

bool parse_setmatch(void *ptr, const char *val, bool is_list);

bool parse_options(void *s, const struct option *opts,
                   struct uci_section *section);


const char * address_to_string(struct address *address,
                               bool allow_invert, bool as_cidr);

#endif /* SRC_OPTIONS_H_ */
