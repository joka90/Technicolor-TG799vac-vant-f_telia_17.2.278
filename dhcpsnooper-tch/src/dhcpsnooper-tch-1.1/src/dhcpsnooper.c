/**************************** IDENTIFICATION *****************************
** @file dhcpsnooper.c
** @author Alin Nastac
** @brief  dhcpsnooper main function
** Project :
** Module : dhcpsnooper
** with extensive renaming for readibility)
** Reference(s):
*************************************************************************/

/*########################################################################
 #                                                                       #
 #  HEADER (INCLUDE) SECTION                                             #
 #                                                                       #
 ####################################################################### */

#include <sys/types.h>
#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#include <libubox/uloop.h>
#include <linux/netfilter.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <libnetfilter_queue/libnetfilter_queue_ipv4.h>
#include <libnetfilter_queue/libnetfilter_queue_udp.h>

#include "dhcpsnooper.h"

/*########################################################################
 #                                                                       #
 #  MACROS                                                               #
 #                                                                       #
 ####################################################################### */
#define DHCPSNOOPER_MESSAGE_SIZE	4096

/*#######################################################################
 #                                                                      #
 #  EXTERNALS                                                           #
 #                                                                      #
 ###################################################################### */

/*#######################################################################
 #                                                                      #
 #  INTERNAL FUNCTIONS  (standard for multi-process support)            #
 #                                                                      #
 ###################################################################### */
static void dhcpsnooper_receive_events(struct uloop_fd *u, _unused unsigned int events);
static void dhcpsnooper_handle_dgram(char *data, int len);

/*#######################################################################
 #                                                                      #
 #  VARIABLES                                                           #
 #                                                                      #
 ###################################################################### */
static struct nfq_handle *nfq_h;
static struct nfq_q_handle *nfq_qh;
static struct dhcpsnooper_event nfq_event = { { .fd = -1, .cb = dhcpsnooper_receive_events} , .handle_dgram = dhcpsnooper_handle_dgram};

/*#######################################################################
 #                                                                      #
 #  INTERNAL FUNCTIONS  (standard for multi-process support)            #
 #                                                                      #
 ###################################################################### */

static void handle_signal(_unused int signal)
{
	uloop_end();
}

static int delete_dhcp_options(struct interface *iface, uint8_t *indata, int inlen,
		struct iphdr **outiph, struct udphdr **outudph, uint8_t *outdata, int *outlen)
{
	unsigned iphlen, iptlen, udpdlen, pos;
	struct iphdr *iph;
	struct udphdr *udph;
	struct dhcp_packet *dhcph;
	uint16_t nbolen;

	/* output buffer must be big enough to store the entire input buffer */
	if (inlen > *outlen)
		return -1;

	/* parse IP header */
	if (inlen <= (int)sizeof(*iph))
		return -1;
	iph = (struct iphdr *)indata;
	iphlen = (iph->ihl << 2);
	nbolen = iph->tot_len;
	iptlen = ntohs(nbolen);
	if (iph->version != IPVERSION || iphlen < sizeof(*iph) || iphlen >= iptlen || (int)iptlen > inlen ||
	    iph->protocol != IPPROTO_UDP)
		return -1;

	/* parse UDP header */
	if (iptlen <= iphlen + sizeof(*udph))
		return -1;
	udph = (struct udphdr *)(indata + iphlen);
	nbolen = udph->len;
	udpdlen = ntohs(nbolen);
	if (iptlen - iphlen != udpdlen ||
	    udph->dest != htons(DHCP_SERVER_PORT))
		return -1;
	udpdlen -= sizeof(*udph);

	/* parse DHCP header */
	if (udpdlen <= sizeof(*dhcph))
		return -1;
	dhcph = (struct dhcp_packet *)(indata + iphlen + sizeof(*udph));
	if (dhcph->op != BOOTREQUEST || dhcph->cookie != htonl(DHCP_MAGIC))
		return -1;

	/* copy headers to output buffer */
	*outlen = pos = iphlen + sizeof(*udph) + sizeof(*dhcph);
	memcpy(outdata, indata, pos);
	*outiph = (struct iphdr *)outdata;
	*outudph = (struct udphdr *)(outdata + iphlen);

	/* copy options not present in del_options */
	while (pos < iptlen) {
		uint8_t opt = indata[pos++];
		uint8_t optlen;

		if (opt == TAG_PAD)
			outdata[(*outlen)++] = opt;
		else if (opt == TAG_END_OPTIONS)
			return 0; /* parsed successfully */
		else if (pos == iptlen)
			return -1;
		else {
			optlen = indata[pos++];
			if (optlen + pos >= iptlen)
				return -1;
			if (!IS_BIT_SET(iface->del_options, opt)) {
				memcpy(outdata + *outlen, indata + pos - 2, optlen + 2);
				*outlen += optlen + 2;
			}
			else
				syslog(LOG_DEBUG, "Deleted option %d length %u", opt, optlen);
			pos += optlen;
		}
	}

	return -1;
}

static int nfq_queue_callback(struct nfq_q_handle *qh, _unused struct nfgenmsg *nfmsg, struct nfq_data *nfd, _unused void *unused)
{
	uint32_t id = 0;
	struct nfqnl_msg_packet_hdr *ph;
	struct nfqnl_msg_packet_hw *hwph;
	struct interface *iface;
	int inlen, outlen = DHCPSNOOPER_MESSAGE_SIZE;
	uint8_t *indata;
	struct iphdr *iph;
	struct udphdr *udph;
	uint8_t outdata[DHCPSNOOPER_MESSAGE_SIZE];

	ph = nfq_get_msg_packet_hdr(nfd);
	if (ph) {
		uint32_t tmp = ph->packet_id;

		id = ntohl(tmp);
	}

	iface = config_interface_by_ifindex(nfq_get_indev(nfd));
	inlen = nfq_get_payload(nfd, &indata);
	if (iface && iface->state.in_use && delete_dhcp_options(iface, indata, inlen, &iph, &udph, outdata, &outlen) == 0) {
		char ifname_phys_in[IF_NAMESIZE] = "", ifname_phys_out[IF_NAMESIZE] = "";
		char src_mac[6 * 3] = "";
		uint32_t ifid;
		int i, cmdlen = 0, fatal_error = 0;
		char cmd[1024];
		FILE *fcmd;
		int datalen, dataspace;

		for (i = 0; i < OPTS_ARRAY_SIZE && iface->add_options[i]; i++) {
			if (i == 0) {
				hwph = nfq_get_packet_hw(nfd);
				if (hwph) {
					uint16_t nbolen = hwph->hw_addrlen;
					int i, pos = 0, hlen = ntohs(nbolen);

					if (hlen * 3 > (int)sizeof(src_mac))
						hlen = sizeof(src_mac) / 3;

					for (i = 0; i < hlen; i++) {
						if (pos)
							src_mac[pos++] = ':';
						pos += sprintf(src_mac + pos, "%02x", hwph->hw_addr[i]);
					}
				}

				ifid = nfq_get_physindev(nfd);
				if (ifid)
					if_indextoname(ifid, ifname_phys_in);
				ifid = nfq_get_physoutdev(nfd);
				if (ifid)
					if_indextoname(ifid, ifname_phys_out);

				cmdlen = snprintf(cmd, sizeof(cmd), "'%s' '%s' '%s' '%s' '%s' ",
						iface->format_script, iface->ifname,
						ifname_phys_in, ifname_phys_out,
						src_mac);

				if (cmdlen >= (int)sizeof(cmd)) {
					syslog(LOG_ERR, "Option format script command line too long");
					fatal_error = 1;
					break;
				}
			}

			if (DHCPSNOOPER_MESSAGE_SIZE - outlen < 4) {
				syslog(LOG_ERR, "Packet too long");
				fatal_error = 1;
				break;
			}

			/* command: script bridge inport outport dhclient_macaddr opt_num */
			if (snprintf(cmd + cmdlen, sizeof(cmd) - cmdlen, "%d", iface->add_options[i]) >= (int)(sizeof(cmd) - cmdlen)) {
				syslog(LOG_ERR, "Option format script command line too long");
				fatal_error = 1;
				break;
			}

			syslog(LOG_DEBUG, "Executing %s", cmd);
			fcmd = popen(cmd, "r");
			if (fcmd == NULL) {
				syslog(LOG_ERR, "Failed to execute option format script");
				fatal_error = 1;
				break;
			}

			/* generate the option content and append it at the end of the message */
			outdata[outlen++] = iface->add_options[i];
			dataspace = DHCPSNOOPER_MESSAGE_SIZE - outlen - 1 /* opt length */;
			if (dataspace > MAX_OPTION_LENGTH + 1)
				dataspace = MAX_OPTION_LENGTH + 1;

			datalen = fread(outdata + outlen + 1 /* opt length */, 1, dataspace, fcmd);
			if (datalen >= dataspace) {
				syslog(LOG_ERR, "Option %d too long", iface->add_options[i]);
				fatal_error = 1;
				pclose(fcmd);
				break;
			}
			outdata[outlen++] = (uint8_t)datalen;
			outlen += datalen;
			syslog(LOG_DEBUG, "Added option %d length %u", iface->add_options[i], datalen);

			pclose(fcmd);
		}

		/* add "end option" tag and some padding */
		if (!fatal_error && outlen >= DHCPSNOOPER_MESSAGE_SIZE) {
			syslog(LOG_ERR, "Packet too long");
			fatal_error = 1;
		}
		else {
			int padsize;

			outdata[outlen++] = (uint8_t)TAG_END_OPTIONS;

			padsize = MIN_DHCP_DATA_LEN - (outlen - (iph->ihl << 2) - sizeof(*udph));
			if (padsize > 0) {
				memset(outdata + outlen, TAG_PAD, padsize);
				outlen += padsize;
			}
		}

		if (!fatal_error) {
			if (inlen != outlen) {
				uint16_t udplen;

				iph->tot_len = htons(outlen);
				udplen = outlen - (iph->ihl << 2);
				udph->len = htons(udplen);
				nfq_ip_set_checksum(iph);
			}
			nfq_udp_compute_checksum_ipv4(udph, iph);
			syslog(LOG_DEBUG, "Pass mangled packet with ID %u", id);
			return nfq_set_verdict(qh, id, NF_ACCEPT, outlen, outdata);
		}
	}

	syslog(LOG_DEBUG, "Pass unmodified packet with ID %u", id);
	return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
}

static void dhcpsnooper_handle_dgram(char *data, int len)
{
	nfq_handle_packet(nfq_h, data, len);
}

static void dhcpsnooper_receive_events(struct uloop_fd *u, _unused unsigned int events)
{
	struct dhcpsnooper_event *e = container_of(u, struct dhcpsnooper_event, uloop);
	char buf[DHCPSNOOPER_MESSAGE_SIZE];

	while (true) {
		ssize_t len = recv(u->fd, buf, sizeof(buf), MSG_DONTWAIT);
		if (len < 0) {
			if (errno == EAGAIN)
				break;
			else
				continue;
		}

		e->handle_dgram(buf, len);
	}
}

static void dhcpsnooper_run(void)
{
	struct sigaction sigact;
 
	memset (&sigact, '\0', sizeof(sigact));

	sigact.sa_handler = SIG_IGN;
	sigaction(SIGUSR1, &sigact, NULL);

	sigact.sa_handler = &handle_signal;
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGINT, &sigact, NULL);

	while (ubus_init() < 0)
		sleep(1);

	config_load();
	uloop_run();
}

static void dhcpsnooper_done(void)
{
	config_interfaces_close();

	if (nfq_qh != NULL) {
		nfq_destroy_queue(nfq_qh);
		nfq_qh = NULL;
	}
	if (nfq_h != NULL) {
		nfq_close(nfq_h);
		nfq_h = NULL;
	}
}

static int dhcpsnooper_usage(void)
{
	const char buf[] =
	"Usage: dhcpsnooper [options]\n"
	"\nFeature options:\n"
	"	-q <nf-queue>	The NFQUEUE number used to intercept DHCP traffic\n"
	"\nInvocation options:\n"
	"	-e		Write logmessages to stderr\n"
	"	-v		Increase logging verbosity\n"
	"	-h		Show this help\n\n";
	write(STDERR_FILENO, buf, sizeof(buf));
	return 1;
}

/*#######################################################################
 #                                                                      #
 #  EXTERNAL FUNCTIONS                                                  #
 #                                                                      #
 ###################################################################### */

int main(int argc, char *const argv[])
{
	int logopt = LOG_PID;
	int c;
	int verbosity = 0;
	bool help = false;
	bool illegal_nfqueue = false;
	struct sched_param param = {
		.sched_priority = 50,
	};

	while ((c = getopt(argc, argv, "evq:")) != -1) {
		switch (c) {
		case 'q':
		        if (sscanf(optarg, "%hd", &queue_id) != 1)
				illegal_nfqueue = true;
			break;

		case 'e':
			logopt |= LOG_PERROR;
			break;

		case 'v':
			++verbosity;
			break;

		default:
			help = true;
			break;
		}
	}

	openlog("dhcpsnooper", logopt, LOG_DAEMON);
	if (!verbosity)
		setlogmask(LOG_UPTO(LOG_WARNING));

	if (help)
		return dhcpsnooper_usage();

	if (illegal_nfqueue) {
		syslog(LOG_ERR, "Illegal NFQUEUE number");
		return 1;
	}

	if (sched_setscheduler(0, SCHED_RR, &param) < 0) {
		syslog(LOG_ERR, "Failed to set scheduler params (%d <%s>)",
			errno, strerror(errno));
		return 1;
	}

	uloop_init();

	nfq_h = nfq_open();
	if ((nfq_h = nfq_open()) == NULL ||
	     nfq_unbind_pf(nfq_h, AF_INET) < 0 ||
	     nfq_bind_pf(nfq_h, AF_INET) < 0 ||
	     (nfq_qh = nfq_create_queue(nfq_h,  queue_id, &nfq_queue_callback, NULL)) == NULL ||
	     nfq_set_mode(nfq_qh, NFQNL_COPY_PACKET, DHCPSNOOPER_MESSAGE_SIZE) < 0) {
		syslog(LOG_ERR, "Failed to create netfilter queue %hu", queue_id);
		dhcpsnooper_done();
		return 1;
	}

	nfq_event.uloop.fd = nfq_fd(nfq_h);
	if (uloop_fd_add(&nfq_event.uloop, ULOOP_READ) < 0) {
		syslog(LOG_ERR, "Failed to register nfq_event");
		dhcpsnooper_done();
		return 1;
	}

	dhcpsnooper_run();

	dhcpsnooper_done();

	return 0;
}
