/**************************** IDENTIFICATION *****************************
** @file config.c
** @author Alin Nastac
** @brief Parses DHCP snooping configuration
** Project :
** Module : dhcpsnooper
** Reference(s):
*************************************************************************/

/*########################################################################
 #                                                                       #
 #  HEADER (INCLUDE) SECTION                                             #
 #                                                                       #
 ####################################################################### */
#include <sys/types.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <syslog.h>
#include <net/if.h>

#include <uci.h>
#include <uci_blob.h>

#include <libubox/blob.h>
#include <libubox/list.h>

#include "dhcpsnooper.h"

/*#######################################################################
 #                                                                      #
 #  EXTERNALS                                                           #
 #                                                                      #
 ###################################################################### */
struct list_head interfaces = LIST_HEAD_INIT(interfaces);
uint16_t queue_id = 0;

/*########################################################################
#                                                                       #
#  TYPES                                                                #
#                                                                       #
####################################################################### */

/*#######################################################################
 #                                                                      #
 #  INTERNAL FUNCTIONS                                                  #
 #                                                                      #
 ###################################################################### */

/*#######################################################################
 #                                                                      #
 #  VARIABLES                                                           #
 #                                                                      #
 ###################################################################### */
enum config_iface_attr {
    IFACE_ATTR_IFNAME,
    IFACE_ATTR_ENABLE,
    IFACE_ATTR_DEL_OPTIONS,
    IFACE_ATTR_ADD_OPTIONS,
    IFACE_ATTR_OPTION_FORMAT_SCRIPT,
    IFACE_ATTR_MAX
};

static const struct blobmsg_policy iface_attrs[IFACE_ATTR_MAX] = {
    [IFACE_ATTR_IFNAME] = { .name = "ifname", .type = BLOBMSG_TYPE_STRING },
    [IFACE_ATTR_ENABLE] = { .name = "enable", .type = BLOBMSG_TYPE_BOOL },
    [IFACE_ATTR_DEL_OPTIONS] = { .name = "del_options", .type = BLOBMSG_TYPE_ARRAY },
    [IFACE_ATTR_ADD_OPTIONS] = { .name = "add_options", .type = BLOBMSG_TYPE_ARRAY },
    [IFACE_ATTR_OPTION_FORMAT_SCRIPT] = { .name = "option_format_script", .type = BLOBMSG_TYPE_STRING },
};

const struct uci_blob_param_list interface_attr_list = {
	.n_params = IFACE_ATTR_MAX,
	.params = iface_attrs,
};

static struct uci_context *config_uci_ctxt = NULL;
static struct uci_package *config_uci_pkg = NULL;

/*#######################################################################
 #                                                                      #
 #  INTERNAL FUNCTIONS                                                  #
 #                                                                      #
 ###################################################################### */
static struct blob_buf b;

static void parse_del_options(struct blob_attr *array, unsigned long *setup)
{
	struct blob_attr *cur;
	unsigned rem;

	blobmsg_for_each_attr(cur, array, rem) {
		const char *stroption = blobmsg_get_string(cur);
		unsigned long option;

		if (sscanf(stroption, "%lu", &option) == 1 && option >= MIN_OPTION && option <= MAX_OPTION)
			SET_BIT(setup, option);
		else
			syslog(LOG_WARNING, "Illegal del_option '%s', must be an integer value between %d and %d",
				stroption, MIN_OPTION, MAX_OPTION);
	}
}

static void parse_add_options(struct blob_attr *array, uint8_t *setup, int size)
{
	struct blob_attr *cur;
	unsigned rem;
	int pos = 0;

	blobmsg_for_each_attr(cur, array, rem) {
		const char *stroption = blobmsg_get_string(cur);
		unsigned long option;

		if (sscanf(stroption, "%lu", &option) == 1 && option >= MIN_OPTION && option <= MAX_OPTION) {
			if (pos >= size) {
				syslog(LOG_WARNING, "Too many add_option elements, maximum allowed number of such elements is %d",
					size);
				break;

			}
			setup[pos++] = (uint8_t)option;
		}
		else
			syslog(LOG_WARNING, "Illegal add_option '%s', must be an integer value between %d and %d",
				stroption, MIN_OPTION, MAX_OPTION);
	}
}

static struct interface *config_interface_parse(void *data, size_t len, const char *name, const char *ifname)
{
	struct interface *iface;
	struct blob_attr *attr[IFACE_ATTR_MAX], *c;

	syslog(LOG_DEBUG, "Parse interface %s ifname %s", name, ifname ? ifname : "<unknown>");
	blobmsg_parse(iface_attrs, IFACE_ATTR_MAX, attr, data, len);

	iface = config_interface_by_name(name);
	if (!iface) {
		iface = calloc(1, sizeof(*iface));
		if (!iface)
			return NULL;

		strncpy(iface->name, name, sizeof(iface->name) - 1);
		list_add(&iface->head, &interfaces);
	}

        if ((c = attr[IFACE_ATTR_ENABLE]))
		iface->state.enable = blobmsg_get_bool(c);
	else
		iface->state.enable = 1;

	if (!iface->state.enable)
		return NULL;

	if (!ifname && (c = attr[IFACE_ATTR_IFNAME]))
		ifname = blobmsg_get_string(c);

	if (!ifname && iface->ifname[0] == '\0')
		return NULL;

	if (iface->ifname[0] == '\0')
		strncpy(iface->ifname, ifname, sizeof(iface->ifname) - 1);

	if ((iface->ifindex = if_nametoindex(iface->ifname)) <= 0)
		return NULL;

	iface->state.in_use = 1;

	memset(&iface->del_options, 0, sizeof(iface->del_options));
	if ((c = attr[IFACE_ATTR_DEL_OPTIONS]))
		parse_del_options(c, iface->del_options);

	memset(&iface->add_options, 0, sizeof(iface->add_options));
	if ((c = attr[IFACE_ATTR_ADD_OPTIONS]))
		parse_add_options(c, iface->add_options, OPTS_ARRAY_SIZE);

	if (iface->format_script) {
		free(iface->format_script);
		iface->format_script = NULL;
	}
	if ((c = attr[IFACE_ATTR_OPTION_FORMAT_SCRIPT])) {
		const char *script = blobmsg_get_string(c);

		if (script[0])
			iface->format_script = strdup(script);
	}

	return iface;
}

static void config_reload_interfaces()
{
	struct uci_element *e;
	uci_foreach_element(&config_uci_pkg->sections, e) {
		struct uci_section *s = uci_to_section(e);

		if (strcmp(s->type, "interface"))
			continue;

		blob_buf_init(&b, 0);
		uci_to_blob(&b, s, &interface_attr_list);
		config_interface_parse(blob_data(b.head), blob_len(b.head), s->e.name, NULL);
	}
}

/*#######################################################################
 #                                                                      #
 #  EXTERNAL FUNCTIONS                                                  #
 #                                                                      #
 ###################################################################### */
int config_uci_load(void)
{
	if (!config_uci_ctxt) {
		config_uci_ctxt = uci_alloc_context();
		if (!config_uci_ctxt)
			return -1;
	}

	if (!uci_load(config_uci_ctxt, "dhcpsnooping", &config_uci_pkg))
		return 0;

	config_uci_unload();

	return -1;
}

void config_uci_unload(void)
{
	uci_unload(config_uci_ctxt, config_uci_pkg);
	uci_free_context(config_uci_ctxt);
	config_uci_pkg = NULL;
	config_uci_ctxt = NULL;
}

void config_load(void)
{
	if (config_uci_load() < 0)
		return;

	config_interfaces_set_unused();
	config_reload_interfaces();
	config_interfaces_flush_unused();

	config_uci_unload();
}

void config_interface_load(const char *name, const char *ifname)
{
	struct uci_element *e;

	if (!config_uci_pkg)
		return;

	uci_foreach_element(&config_uci_pkg->sections, e) {
		struct uci_section *s = uci_to_section(e);
		if (strcmp(s->type, "interface") || strcmp(s->e.name, name))
			continue;

		blob_buf_init(&b, 0);
		uci_to_blob(&b, s, &interface_attr_list);

		config_interface_parse(blob_data(b.head), blob_len(b.head), s->e.name, ifname);
		break;
	}
}

struct interface* config_interface_by_ifindex(const int ifindex)
{
	struct interface *c;
	list_for_each_entry(c, &interfaces, head)
		if (ifindex == c->ifindex)
			return c;
	return NULL;
}

struct interface* config_interface_by_name(const char *name)
{
	struct interface *c;
	list_for_each_entry(c, &interfaces, head)
		if (!strcmp(c->name, name))
			return c;
	return NULL;
}

struct interface* config_interface_by_ifname(const char *ifname)
{
	struct interface *c;
	list_for_each_entry(c, &interfaces, head)
		if (!strcmp(c->ifname, ifname))
			return c;
	return NULL;
}

void config_interface_close(struct interface *iface)
{
	if (iface->head.next)
		list_del(&iface->head);

	if (iface->format_script)
		free(iface->format_script);

	free(iface);
}

void config_interfaces_close(void)
{
	struct interface *c, *n;

	list_for_each_entry_safe(c, n, &interfaces, head)
		config_interface_close(c);
}

void config_interfaces_set_unused(void)
{
	struct interface *i;

	list_for_each_entry(i, &interfaces, head) {
		i->state.in_use = 0;
	}
}

void config_interfaces_flush_unused(void)
{
	struct interface *i, *n;

	list_for_each_entry_safe(i, n, &interfaces, head) {
		if (!i->state.in_use)
			config_interface_close(i);
	}
}


