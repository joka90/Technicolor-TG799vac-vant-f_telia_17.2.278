/**************************** IDENTIFICATION *****************************
** @file ubus.c
** @author Alin Nastac
** @brief dhcpsnooper UBUS functionality
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
#include <syslog.h>

#include <libubus.h>
#include <libubox/blobmsg.h>

#include "dhcpsnooper.h"

/*#######################################################################
 #                                                                      #
 #  EXTERNALS                                                           #
 #                                                                      #
 ###################################################################### */

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
static void handle_event(_unused struct ubus_context *ctx, _unused struct ubus_event_handler *ev,
				_unused const char *type, struct blob_attr *msg);

/*#######################################################################
 #                                                                      #
 #  VARIABLES                                                           #
 #                                                                      #
 ###################################################################### */
enum ubus_obj_attr {
	OBJ_ATTR_ID,
	OBJ_ATTR_PATH,
	OBJ_ATTR_MAX
};

static const struct blobmsg_policy obj_attrs[OBJ_ATTR_MAX] = {
	[OBJ_ATTR_ID] = { .name = "id", .type = BLOBMSG_TYPE_INT32 },
	[OBJ_ATTR_PATH] = { .name = "path", .type = BLOBMSG_TYPE_STRING },
};

enum ubus_dump_attr {
	DUMP_ATTR_INTERFACE,
	DUMP_ATTR_MAX
};

static const struct blobmsg_policy dump_attrs[DUMP_ATTR_MAX] = {
	[DUMP_ATTR_INTERFACE] = { .name = "interface", .type = BLOBMSG_TYPE_ARRAY },
};

enum ubus_iface_attr {
	IFACE_ATTR_INTERFACE,
	IFACE_ATTR_IFNAME,
	IFACE_ATTR_UP,
	IFACE_ATTR_MAX,
};

static const struct blobmsg_policy iface_attrs[IFACE_ATTR_MAX] = {
	[IFACE_ATTR_INTERFACE] = { .name = "interface", .type = BLOBMSG_TYPE_STRING },
	[IFACE_ATTR_IFNAME] = { .name = "l3_device", .type = BLOBMSG_TYPE_STRING },
	[IFACE_ATTR_UP] = { .name = "up", .type = BLOBMSG_TYPE_BOOL },
};

static struct ubus_context *ubus = NULL;
static struct ubus_event_handler event_handler = { .cb = handle_event, };
static struct ubus_subscriber netifd;
static struct ubus_request req_dump = { .list = LIST_HEAD_INIT(req_dump.list) };
static uint32_t objid = 0;

/*#######################################################################
 #                                                                      #
 #  INTERNAL FUNCTIONS                                                  #
 #                                                                      #
 ###################################################################### */

static void handle_iface_status(void *data, size_t len)
{
	struct blob_attr *tb[IFACE_ATTR_MAX], *a;

	blobmsg_parse(iface_attrs, IFACE_ATTR_MAX, tb, data, len);

	if (!(a = tb[IFACE_ATTR_INTERFACE]))
		return;

	const char *iface = blobmsg_get_string(a);

	if (!(a = tb[IFACE_ATTR_UP]))
		return;

	bool up = blobmsg_get_bool(a);

	if (up) {
		if (!(a = tb[IFACE_ATTR_IFNAME]))
			return;

		const char *ifname = blobmsg_get_string(a);

		config_interface_load(iface, ifname);
	}
	else {
		struct interface *i = config_interface_by_name(iface);
		if (!i)
			return;

		config_interface_close(i);
	}
}

static void handle_dump(_unused struct ubus_request *req, _unused int type, struct blob_attr *msg)
{
	struct blob_attr *tb[DUMP_ATTR_MAX];
	blobmsg_parse(dump_attrs, DUMP_ATTR_MAX, tb, blob_data(msg), blob_len(msg));

	if (!tb[DUMP_ATTR_INTERFACE])
		return;

	struct blob_attr *c;
	unsigned rem;

	if (config_uci_load() < 0)
		return;

	config_interfaces_set_unused();

	blobmsg_for_each_attr(c, tb[DUMP_ATTR_INTERFACE], rem)
		handle_iface_status(blobmsg_data(c), blobmsg_data_len(c));

	config_interfaces_flush_unused();

	config_uci_unload();
}

static void subscribe_sync_netifd(void)
{
	ubus_subscribe(ubus, &netifd, objid);

	ubus_abort_request(ubus, &req_dump);
	if (!ubus_invoke_async(ubus, objid, "dump", NULL, &req_dump)) {
		req_dump.data_cb = handle_dump;
		ubus_complete_request_async(ubus, &req_dump);
	}
}

static int handle_update(_unused struct ubus_context *ctx, _unused struct ubus_object *obj,
		_unused struct ubus_request_data *req, _unused const char *method,
		struct blob_attr *msg)
{
	if (config_uci_load() < 0)
		return 0;

	handle_iface_status(blob_data(msg), blob_len(msg));

	config_uci_unload();

	return 0;
}

static void handle_event(_unused struct ubus_context *ctx, _unused struct ubus_event_handler *ev,
				_unused const char *type, struct blob_attr *msg)
{
	struct blob_attr *tb[OBJ_ATTR_MAX];
	blobmsg_parse(obj_attrs, OBJ_ATTR_MAX, tb, blob_data(msg), blob_len(msg));

	if (!tb[OBJ_ATTR_ID] || !tb[OBJ_ATTR_PATH])
		return;

	if (strcmp(blobmsg_get_string(tb[OBJ_ATTR_PATH]), "network.interface"))
		return;

	objid = blobmsg_get_u32(tb[OBJ_ATTR_ID]);
	subscribe_sync_netifd();
}

/*#######################################################################
 #                                                                      #
 #  EXTERNAL FUNCTIONS                                                  #
 #                                                                      #
 ###################################################################### */

int ubus_init(void)
{
	if (!(ubus = ubus_connect(NULL))) {
		syslog(LOG_ERR, "Unable to connect to ubus: %s", strerror(errno));
		return -1;
	}

	netifd.cb = handle_update;
	ubus_register_subscriber(ubus, &netifd);

	ubus_add_uloop(ubus);
	ubus_register_event_handler(ubus, &event_handler, "ubus.object.add");
	if (!ubus_lookup_id(ubus, "network.interface", &objid))
		subscribe_sync_netifd();

	return 0;
}
