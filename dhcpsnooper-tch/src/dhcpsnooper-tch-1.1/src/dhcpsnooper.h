#ifndef DHCPSNOOPER_H__
#define DHCPSNOOPER_H__

/*************************** IDENTIFICATION *****************************
** @file dhcpsnooper.h
** @author Alin Nastac
** Project: dhcpsnooper
************************************************************************/
#include <stddef.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>

#include <libubox/uloop.h>

#ifndef typeof
#define typeof __typeof
#endif

#ifndef container_of
#define container_of(ptr, type, member) (	    \
    (type *)( (char *)ptr - offsetof(type,member) ))
#endif

/*#######################################################################
#                                                                      #
#  MACROS                                                              #
#                                                                      #
###################################################################### */
#define _unused __attribute__((unused))
#define _packed __attribute__((packed))

#define MIN_OPTION		1
#define MAX_OPTION		254

#define MAX_OPTION_LENGTH	0xFF
#define TAG_PAD			0
#define TAG_END_OPTIONS		0xFF

#define MIN_DHCP_DATA_LEN	300

#define DHCP_SERVER_PORT	67
#define DHCP_MAGIC              0x63825363
#define BOOTREQUEST             1
#define BOOTREPLY               2

#define OPTS_BITMAP_SIZE	((256 + 8 * sizeof(unsigned long) - 1) / (8 * sizeof(unsigned long)))
#define OPTS_ARRAY_SIZE		8

#define SET_BIT(bitmap,pos) \
	(bitmap)[(pos)/(8*sizeof(unsigned long))] |= 1UL << ((pos) % (8*sizeof(unsigned long)))
#define CLEAR_BIT(bitmap,pos) \
	(bitmap)[(pos)/(8*sizeof(unsigned long))] &= ~(1UL << ((pos) % (8*sizeof(unsigned long))))
#define IS_BIT_SET(bitmap,pos) \
	((bitmap)[(pos)/(8*sizeof(unsigned long))] & (1UL << ((pos) % (8*sizeof(unsigned long)))))

/*#######################################################################
#                                                                      #
#  TYPES                                                               #
#                                                                      #
###################################################################### */
struct dhcpsnooper_event {
	struct uloop_fd uloop;
	void (*handle_dgram)(char *data, int len);
};

struct interface {
	struct list_head head;

	int ifindex;
	char ifname[IF_NAMESIZE];
	char name[IF_NAMESIZE];

	unsigned long del_options[OPTS_BITMAP_SIZE];
	uint8_t add_options[OPTS_ARRAY_SIZE];
	char *format_script;

	struct {
	    uint8_t enable: 1,
		    in_use: 1;
	} state;
};

struct dhcp_packet {
	uint8_t op;      /* BOOTREQUEST or BOOTREPLY */
	uint8_t htype;
	uint8_t hlen;
	uint8_t hops;
	uint32_t xid;
	uint16_t secs;
	uint16_t flags;
	uint32_t ciaddr;
	uint32_t yiaddr;
	uint32_t siaddr;
	uint32_t giaddr;
	uint8_t chaddr[16];
	uint8_t sname[64];
	uint8_t file[128];
	uint32_t cookie;
} _packed;

/*#######################################################################
#                                                                      #
#  GLOBAL VARIABLES                                                    #
#                                                                      #
###################################################################### */
extern struct list_head interfaces;
extern uint16_t queue_id;

/*#######################################################################
#                                                                      #
#  PROTOTYPES                                                          #
#                                                                      #
###################################################################### */
int config_uci_load(void);
void config_uci_unload(void);
void config_load(void);
void config_interface_load(const char *name, const char *ifname);
struct interface* config_interface_by_ifindex(const int ifindex);
struct interface* config_interface_by_name(const char *name);
struct interface* config_interface_by_ifname(const char *ifname);
void config_interface_close(struct interface *iface);
void config_interfaces_close(void);
void config_interfaces_set_unused(void);
void config_interfaces_flush_unused(void);

int ubus_init(void);

#endif
