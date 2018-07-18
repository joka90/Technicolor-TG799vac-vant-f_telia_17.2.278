/************** COPYRIGHT AND CONFIDENTIALITY INFORMATION ********************
**                                                                          **
** Copyright (c) 2014 Technicolor                                           **
** All Rights Reserved                                                      **
**                                                                          **
** This program contains proprietary information which is a trade           **
** secret of TECHNICOLOR and/or its affiliates and also is protected as     **
** an unpublished work under applicable Copyright laws. Recipient is        **
** to retain this program in confidence and is not permitted to use or      **
** make copies thereof other than as permitted in a written agreement       **
** with TECHNICOLOR, UNLESS OTHERWISE EXPRESSLY ALLOWED BY APPLICABLE LAWS. **
**                                                                          **
******************************************************************************/


#include "config.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/file.h>
#include <syslog.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/param.h>
/* for BSD's sysctl */
#include <sys/sysctl.h>

/* unix sockets */
#ifdef USE_MINITR064DCTL
#include <sys/un.h>
#include <openssl/rand.h>

#endif

#include "macros.h"
#include "upnpglobalvars.h"
#include "upnphttp.h"
#include "upnpdescgen.h"
#include "minitr064dpath.h"
#include "getifaddr.h"
#include "upnpsoap.h"
#include "options.h"
#include "minissdp.h"
#include "miniupnpdtypes.h"
#include "daemonize.h"
#include "asyncsendto.h"
#include "commonrdr.h"
#include "upnputils.h"
#ifdef USE_IFACEWATCHER
#include "ifacewatcher.h"
#endif

#ifndef DEFAULT_CONFIG
#define DEFAULT_CONFIG "/etc/minitr064d.conf"
#endif

#ifdef USE_MINITR064DCTL
struct ctlelem {
	int socket;
	LIST_ENTRY(ctlelem) entries;
};
#endif

/* variables used by signals */
static volatile sig_atomic_t quitting = 0;

/* OpenAndConfHTTPSocket() :
 * setup the socket used to handle incoming HTTP connections. */
static int
#ifdef ENABLE_IPV6
OpenAndConfHTTPSocket(unsigned short * port, int ipv6)
#else
OpenAndConfHTTPSocket(unsigned short * port)
#endif
{
    int s;
    int i = 1;
#ifdef ENABLE_IPV6
	struct sockaddr_in6 listenname6;
	struct sockaddr_in listenname4;
#else
    struct sockaddr_in listenname;
#endif
    socklen_t listenname_len;

    s = socket(
#ifdef ENABLE_IPV6
	           ipv6 ? PF_INET6 : PF_INET,
#else
            PF_INET,
#endif
            SOCK_STREAM, 0);
#ifdef ENABLE_IPV6
	if(s < 0 && ipv6 && errno == EAFNOSUPPORT)
	{
		/* the system doesn't support IPV6 */
		syslog(LOG_WARNING, "socket(PF_INET6, ...) failed with EAFNOSUPPORT, disabling IPv6");
		SETFLAG(IPV6DISABLEDMASK);
		ipv6 = 0;
		s = socket(PF_INET, SOCK_STREAM, 0);
	}
#endif
    if(s < 0)
    {
        syslog(LOG_ERR, "socket(http): %m");
        return -1;
    }

    if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i)) < 0)
    {
        syslog(LOG_WARNING, "setsockopt(http, SO_REUSEADDR): %m");
    }
#if 0
	/* enable this to force IPV6 only for IPV6 socket.
	 * see http://www.ietf.org/rfc/rfc3493.txt section 5.3 */
	if(setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, &i, sizeof(i)) < 0)
	{
		syslog(LOG_WARNING, "setsockopt(http, IPV6_V6ONLY): %m");
	}
#endif

    if(!set_non_blocking(s))
    {
        syslog(LOG_WARNING, "set_non_blocking(http): %m");
    }

#ifdef ENABLE_IPV6
	if(ipv6)
	{
		memset(&listenname6, 0, sizeof(struct sockaddr_in6));
		listenname6.sin6_family = AF_INET6;
		listenname6.sin6_port = htons(*port);
		listenname6.sin6_addr = ipv6_bind_addr;
		listenname_len =  sizeof(struct sockaddr_in6);
	} else {
		memset(&listenname4, 0, sizeof(struct sockaddr_in));
		listenname4.sin_family = AF_INET;
		listenname4.sin_port = htons(*port);
		listenname4.sin_addr.s_addr = htonl(INADDR_ANY);
		listenname_len =  sizeof(struct sockaddr_in);
	}
#else
    memset(&listenname, 0, sizeof(struct sockaddr_in));
    listenname.sin_family = AF_INET;
    listenname.sin_port = htons(*port);
    listenname.sin_addr.s_addr = htonl(INADDR_ANY);
    listenname_len =  sizeof(struct sockaddr_in);
#endif

#ifdef ENABLE_IPV6
	if(bind(s,
	        ipv6 ? (struct sockaddr *)&listenname6 : (struct sockaddr *)&listenname4,
	        listenname_len) < 0)
#else
    if(bind(s, (struct sockaddr *)&listenname, listenname_len) < 0)
#endif
    {
        syslog(LOG_ERR, "bind(http): %m");
        close(s);
        return -1;
    }

    if(listen(s, 5) < 0)
    {
        syslog(LOG_ERR, "listen(http): %m");
        close(s);
        return -1;
    }

    if(*port == 0) {
#ifdef ENABLE_IPV6
		if(ipv6) {
			struct sockaddr_in6 sockinfo;
			socklen_t len = sizeof(struct sockaddr_in6);
			if (getsockname(s, (struct sockaddr *)&sockinfo, &len) < 0) {
				syslog(LOG_ERR, "getsockname(): %m");
			} else {
				*port = ntohs(sockinfo.sin6_port);
			}
		} else {
#endif /* ENABLE_IPV6 */
        struct sockaddr_in sockinfo;
        socklen_t len = sizeof(struct sockaddr_in);
        if (getsockname(s, (struct sockaddr *)&sockinfo, &len) < 0) {
            syslog(LOG_ERR, "getsockname(): %m");
        } else {
            *port = ntohs(sockinfo.sin_port);
        }
#ifdef ENABLE_IPV6
		}
#endif /* ENABLE_IPV6 */
    }
    return s;
}

static struct upnphttp *
ProcessIncomingHTTP(int shttpl, const char * protocol)
{
    int shttp;
    socklen_t clientnamelen;
#ifdef ENABLE_IPV6
	struct sockaddr_storage clientname;
	clientnamelen = sizeof(struct sockaddr_storage);
#else
    struct sockaddr_in clientname;
    clientnamelen = sizeof(struct sockaddr_in);
#endif
    shttp = accept(shttpl, (struct sockaddr *)&clientname, &clientnamelen);
    if(shttp<0)
    {
        /* ignore EAGAIN, EWOULDBLOCK, EINTR, we just try again later */
        if(errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
            syslog(LOG_ERR, "accept(http): %m");
    }
    else
    {
        struct upnphttp * tmp = 0;
        char addr_str[64];

        sockaddr_to_string((struct sockaddr *)&clientname, addr_str, sizeof(addr_str));
        syslog(LOG_INFO, "%s connection from %s", protocol, addr_str);
        if(get_lan_for_peer((struct sockaddr *)&clientname) == NULL)
        {
            /* The peer is not a LAN ! */
            syslog(LOG_WARNING,
                    "%s peer %s is not from a LAN, closing the connection",
                    protocol, addr_str);
            close(shttp);
        }
        else
        {
            /* Create a new upnphttp object and add it to
             * the active upnphttp object list */
            tmp = New_upnphttp(shttp);
            if(tmp)
            {
#ifdef ENABLE_IPV6
				if(clientname.ss_family == AF_INET)
				{
					tmp->clientaddr = ((struct sockaddr_in *)&clientname)->sin_addr;
				}
				else if(clientname.ss_family == AF_INET6)
				{
					struct sockaddr_in6 * addr = (struct sockaddr_in6 *)&clientname;
					if(IN6_IS_ADDR_V4MAPPED(&addr->sin6_addr))
					{
						memcpy(&tmp->clientaddr,
						       &addr->sin6_addr.s6_addr[12],
						       4);
					}
					else
					{
						tmp->ipv6 = 1;
						memcpy(&tmp->clientaddr_v6,
						       &addr->sin6_addr,
						       sizeof(struct in6_addr));
					}
				}
#else
                tmp->clientaddr = clientname.sin_addr;
#endif
                return tmp;
            }
            else
            {
                syslog(LOG_ERR, "New_upnphttp() failed");
                close(shttp);
            }
        }
    }
    return NULL;
}

/* Functions used to communicate with minitr064dctl */
#ifdef USE_MINITR064DCTL
static int
OpenAndConfCtlUnixSocket(const char * path)
{
	struct sockaddr_un localun;
	int s;
	s = socket(AF_UNIX, SOCK_STREAM, 0);
	localun.sun_family = AF_UNIX;
	strncpy(localun.sun_path, path,
	          sizeof(localun.sun_path));
	if(bind(s, (struct sockaddr *)&localun,
	        sizeof(struct sockaddr_un)) < 0)
	{
		syslog(LOG_ERR, "bind(sctl): %m");
		close(s);
		s = -1;
	}
	else if(listen(s, 5) < 0)
	{
		syslog(LOG_ERR, "listen(sctl): %m");
		close(s);
		s = -1;
	}
	return s;
}

static void
write_upnphttp_details(int fd, struct upnphttp * e)
{
	char buffer[256];
	int len;
	write(fd, "HTTP :\n", 7);
	while(e)
	{
		len = snprintf(buffer, sizeof(buffer),
		               "%d %d %s req_buf=%p(%dbytes) res_buf=%p(%dbytes alloc)\n",
		               e->socket, e->state, e->HttpVer,
		               e->req_buf, e->req_buflen,
		               e->res_buf, e->res_buf_alloclen);
		write(fd, buffer, len);
		e = e->entries.le_next;
	}
}

static void
write_ctlsockets_list(int fd, struct ctlelem * e)
{
	char buffer[256];
	int len;
	write(fd, "CTL :\n", 6);
	while(e)
	{
		len = snprintf(buffer, sizeof(buffer),
		               "struct ctlelem: socket=%d\n", e->socket);
		write(fd, buffer, len);
		e = e->entries.le_next;
	}
}

#ifndef DISABLE_CONFIG_FILE
static void
write_option_list(int fd)
{
	char buffer[256];
	int len;
	unsigned int i;
	write(fd, "Options :\n", 10);
	for(i=0; i<num_options; i++)
	{
		len = snprintf(buffer, sizeof(buffer),
		               "opt=%02d %s\n",
		               ary_options[i].id, ary_options[i].value);
		write(fd, buffer, len);
	}
}
#endif

static void
write_command_line(int fd, int argc, char * * argv)
{
	char buffer[256];
	int len;
	int i;
	write(fd, "Command Line :\n", 15);
	for(i=0; i<argc; i++)
	{
		len = snprintf(buffer, sizeof(buffer),
		               "argv[%02d]='%s'\n",
		                i, argv[i]);
		write(fd, buffer, len);
	}
}

#endif

/* Handler for the SIGTERM signal (kill)
 * SIGINT is also handled */
static void
sigterm(int sig)
{
    UNUSED(sig);
    /*int save_errno = errno; */
    /*signal(sig, SIG_IGN);*/	/* Ignore this signal while we are quitting */
    /* Note : isn't it useless ? */

#if 0
	/* calling syslog() is forbidden in signal handler according to
	 * signal(3) */
	syslog(LOG_NOTICE, "received signal %d, good-bye", sig);
#endif

    quitting = 1;
    /*errno = save_errno;*/
}

/* Handler for the SIGUSR1 signal indicating public IP address change. */
static void
sigusr1(int sig)
{
    UNUSED(sig);
#if 0
	/* calling syslog() is forbidden in signal handler according to
	 * signal(3) */
	syslog(LOG_INFO, "received signal %d, public ip address change", sig);
#endif

}

/* record the startup time, for returning uptime */
static void
set_startup_time(int sysuptime)
{
    startup_time = time(NULL);
    if(sysuptime)
    {
        /* use system uptime instead of daemon uptime */
        char buff[64];
        int uptime = 0, fd;
        fd = open("/proc/uptime", O_RDONLY);
        if(fd < 0)
        {
            syslog(LOG_ERR, "open(\"/proc/uptime\" : %m");
        }
        else
        {
            memset(buff, 0, sizeof(buff));
            if(read(fd, buff, sizeof(buff) - 1) < 0)
            {
                syslog(LOG_ERR, "read(\"/proc/uptime\" : %m");
            }
            else
            {
                uptime = atoi(buff);
                syslog(LOG_INFO, "system uptime is %d seconds", uptime);
            }
            close(fd);
            startup_time -= uptime;
        }
    }
}

/* parselanaddr()
 * parse address with mask
 * ex: 192.168.1.1/24 or 192.168.1.1/255.255.255.0
 * When MULTIPLE_EXTERNAL_IP is enabled, the ip address of the
 * external interface associated with the lan subnet follows.
 * ex : 192.168.1.1/24 81.21.41.11
 *
 * Can also use the interface name (ie eth0)
 *
 * return value :
 *    0 : ok
 *   -1 : error */
static int
parselanaddr(struct lan_addr_s * lan_addr, const char * str)
{
    const char * p;
    int n;
    char tmp[16];

    memset(lan_addr, 0, sizeof(struct lan_addr_s));
    p = str;
    while(*p && *p != '/' && !isspace(*p))
        p++;
    n = p - str;
    if(!isdigit(str[0]) && n < (int)sizeof(lan_addr->ifname))
    {
        /* not starting with a digit : suppose it is an interface name */
        memcpy(lan_addr->ifname, str, n);
        lan_addr->ifname[n] = '\0';
        if(getifaddr(lan_addr->ifname, lan_addr->str, sizeof(lan_addr->str),
                &lan_addr->addr, &lan_addr->mask) < 0)
            goto parselan_error;
        /*printf("%s => %s\n", lan_addr->ifname, lan_addr->str);*/
    }
    else
    {
        if(n>15)
            goto parselan_error;
        memcpy(lan_addr->str, str, n);
        lan_addr->str[n] = '\0';
        if(!inet_aton(lan_addr->str, &lan_addr->addr))
            goto parselan_error;
    }
    if(*p == '/')
    {
        const char * q = ++p;
        while(*p && isdigit(*p))
            p++;
        if(*p=='.')
        {
            /* parse mask in /255.255.255.0 format */
            while(*p && (*p=='.' || isdigit(*p)))
                p++;
            n = p - q;
            if(n>15)
                goto parselan_error;
            memcpy(tmp, q, n);
            tmp[n] = '\0';
            if(!inet_aton(tmp, &lan_addr->mask))
                goto parselan_error;
        }
        else
        {
            /* it is a /24 format */
            int nbits = atoi(q);
            if(nbits > 32 || nbits < 0)
                goto parselan_error;
            lan_addr->mask.s_addr = htonl(nbits ? (0xffffffffu << (32 - nbits)) : 0);
        }
    }
    else if(lan_addr->mask.s_addr == 0)
    {
        /* by default, networks are /24 */
        lan_addr->mask.s_addr = htonl(0xffffff00u);
    }
#ifdef ENABLE_IPV6
	if(lan_addr->ifname[0] != '\0')
	{
		lan_addr->index = if_nametoindex(lan_addr->ifname);
		if(lan_addr->index == 0)
			fprintf(stderr, "Cannot get index for network interface %s",
			        lan_addr->ifname);
	}
	else
	{
		fprintf(stderr,
		        "Error: please specify LAN network interface by name instead of IPv4 address : %s\n",
		        str);
		return -1;
	}
#endif
    return 0;
    parselan_error:
    fprintf(stderr, "Error parsing address/mask (or interface name) : %s\n",
            str);
    return -1;
}

/* fill uuidvalue_wan and uuidvalue_wcd based on uuidvalue_igd */
void complete_uuidvalues(void)
{
    size_t len;
    len = strlen(uuidvalue_igd);
    memcpy(uuidvalue_wan, uuidvalue_igd, len+1);
    switch(uuidvalue_wan[len-1]) {
        case '9':
            uuidvalue_wan[len-1] = 'a';
            break;
        case 'f':
            uuidvalue_wan[len-1] = '0';
            break;
        default:
            uuidvalue_wan[len-1]++;
    }
    memcpy(uuidvalue_wcd, uuidvalue_wan, len+1);
    switch(uuidvalue_wcd[len-1]) {
        case '9':
            uuidvalue_wcd[len-1] = 'a';
            break;
        case 'f':
            uuidvalue_wcd[len-1] = '0';
            break;
        default:
            uuidvalue_wcd[len-1]++;
    }
    memcpy(uuidvalue_lan, uuidvalue_wcd, len+1);
    switch(uuidvalue_lan[len-1]) {
        case '9':
            uuidvalue_lan[len-1] = 'a';
            break;
        case 'f':
            uuidvalue_lan[len-1] = '0';
            break;
        default:
            uuidvalue_lan[len-1]++;
    }
}

/* init phase :
 * 1) read configuration file
 * 2) read command line arguments
 * 3) daemonize
 * 4) open syslog
 * 5) check and write pid file
 * 6) set startup time stamp
 * 7) compute presentation URL
 * 8) set signal handlers
 * 9) init random generator (srandom())
 * 10) init redirection engine
 * 11) reload mapping from leasefile */
static int
init(int argc, char * * argv, struct runtime_vars * v)
{
    int i;
    int pid;
    int debug_flag = 0;
    int openlog_option;
    struct sigaction sa;
    /*const char * logfilename = 0;*/
    const char * presurl = 0;
#ifndef DISABLE_CONFIG_FILE
    int options_flag = 0;
    const char * optionsfile = DEFAULT_CONFIG;
#endif /* DISABLE_CONFIG_FILE */
    struct lan_addr_s * lan_addr;
    struct lan_addr_s * lan_addr2;

    /* only print usage if -h is used */
    for(i=1; i<argc; i++)
    {
        if(0 == strcmp(argv[i], "-h"))
            goto print_usage;
    }
#ifndef DISABLE_CONFIG_FILE
    /* first check if "-f" option is used */
    for(i=2; i<argc; i++)
    {
        if(0 == strcmp(argv[i-1], "-f"))
        {
            optionsfile = argv[i];
            options_flag = 1;
            break;
        }
    }
#endif /* DISABLE_CONFIG_FILE */

    /* set initial values */
    SETFLAG(ENABLEUPNPMASK);	/* UPnP is enabled by default */
#ifdef ENABLE_IPV6
	ipv6_bind_addr = in6addr_any;
#endif /* ENABLE_IPV6 */

    LIST_INIT(&lan_addrs);
    v->port = -1;
	v->https_port = -1;
    v->notify_interval = 30;	/* seconds between SSDP announces */
#ifndef DISABLE_CONFIG_FILE
    /* read options file first since
     * command line arguments have final say */
    if(readoptionsfile(optionsfile) < 0)
    {
        /* only error if file exists or using -f */
        if(access(optionsfile, F_OK) == 0 || options_flag)
            fprintf(stderr, "Error reading configuration file %s\n", optionsfile);
    }
    else
    {
        for(i=0; i<(int)num_options; i++)
        {
            switch(ary_options[i].id)
            {
                case UPNPLISTENING_IP:
                    lan_addr = (struct lan_addr_s *) malloc(sizeof(struct lan_addr_s));
                    if (lan_addr == NULL)
                    {
                        fprintf(stderr, "malloc(sizeof(struct lan_addr_s)): %m");
                        break;
                    }
                    if(parselanaddr(lan_addr, ary_options[i].value) != 0)
                    {
                        fprintf(stderr, "can't parse \"%s\" as a valid "
#ifndef ENABLE_IPV6
                                "LAN address or "
#endif
                                "interface name\n", ary_options[i].value);
                        free(lan_addr);
                        break;
                    }
                    LIST_INSERT_HEAD(&lan_addrs, lan_addr, list);
                    break;
#ifdef ENABLE_IPV6
                case UPNPIPV6_LISTENING_IP:
                    if (inet_pton(AF_INET6, ary_options[i].value, &ipv6_bind_addr) < 1)
                    {
                        fprintf(stderr, "can't parse \"%s\" as valid IPv6 listening address", ary_options[i].value);
                    }
                    break;
#endif /* ENABLE_IPV6 */
                case UPNPPORT:
                    v->port = atoi(ary_options[i].value);
                    break;
                case UPNPHTTPSPORT:
                    v->https_port = atoi(ary_options[i].value);
                    break;
                case UPNPHTTPSCERT:
                    strncpy(sslcertfile, ary_options[i].value, SSL_CERT_PATH_MAX_LEN);
                    sslcertfile[SSL_CERT_PATH_MAX_LEN-1] = '\0';
                    break;
                case UPNPHTTPSKEY:
                    strncpy(sslkeyfile, ary_options[i].value, SSL_KEY_PATH_MAX_LEN);
                    sslkeyfile[SSL_KEY_PATH_MAX_LEN-1] = '\0';
                    break;
                case UPNPPRESENTATIONURL:
                    presurl = ary_options[i].value;
                    break;
#ifdef ENABLE_MANUFACTURER_INFO_CONFIGURATION
                case UPNPFRIENDLY_NAME:
                    strncpy(friendly_name, ary_options[i].value, FRIENDLY_NAME_MAX_LEN);
                    friendly_name[FRIENDLY_NAME_MAX_LEN-1] = '\0';
                    break;
                case UPNPMANUFACTURER_NAME:
                    strncpy(manufacturer_name, ary_options[i].value, MANUFACTURER_NAME_MAX_LEN);
                    manufacturer_name[MANUFACTURER_NAME_MAX_LEN-1] = '\0';
                    break;
                case UPNPMANUFACTURER_URL:
                    strncpy(manufacturer_url, ary_options[i].value, MANUFACTURER_URL_MAX_LEN);
                    manufacturer_url[MANUFACTURER_URL_MAX_LEN-1] = '\0';
                    break;
                case UPNPMODEL_NAME:
                    strncpy(model_name, ary_options[i].value, MODEL_NAME_MAX_LEN);
                    model_name[MODEL_NAME_MAX_LEN-1] = '\0';
                    break;
                case UPNPMODEL_DESCRIPTION:
                    strncpy(model_description, ary_options[i].value, MODEL_DESCRIPTION_MAX_LEN);
                    model_description[MODEL_DESCRIPTION_MAX_LEN-1] = '\0';
                    break;
                case UPNPMODEL_URL:
                    strncpy(model_url, ary_options[i].value, MODEL_URL_MAX_LEN);
                    model_url[MODEL_URL_MAX_LEN-1] = '\0';
                    break;
#endif
                case UPNPNOTIFY_INTERVAL:
                    v->notify_interval = atoi(ary_options[i].value);
                    break;
                case UPNPSYSTEM_UPTIME:
                    if(strcmp(ary_options[i].value, "yes") == 0)
                        SETFLAG(SYSUPTIMEMASK);	/*sysuptime = 1;*/
                    break;
                case UPNPUUID:
                    strncpy(uuidvalue_igd+5, ary_options[i].value,
                            strlen(uuidvalue_igd+5) + 1);
                    complete_uuidvalues();
                    break;
                case UPNPSERIAL:
                    strncpy(serialnumber, ary_options[i].value, SERIALNUMBER_MAX_LEN);
                    serialnumber[SERIALNUMBER_MAX_LEN-1] = '\0';
                    break;
                case UPNPMODEL_NUMBER:
                    strncpy(modelnumber, ary_options[i].value, MODELNUMBER_MAX_LEN);
                    modelnumber[MODELNUMBER_MAX_LEN-1] = '\0';
                    break;
                case UPNPENABLE:
                    if(strcmp(ary_options[i].value, "yes") != 0)
                        CLEARFLAG(ENABLEUPNPMASK);
                    break;
                case UPNPMINISSDPDSOCKET:
                    minissdpdsocketpath = ary_options[i].value;
                    break;
                case PASSWORD_DSLFCONFIG:
                    strncpy(password_dsl_config, ary_options[i].value, DSL_PASSWORD_MAXLEN);
                    password_dsl_config[DSL_PASSWORD_MAXLEN-1] = '\0';
                    break;
                case PASSWORD_DSLFRESET:
                    strncpy(password_dsl_reset, ary_options[i].value, DSL_PASSWORD_MAXLEN);
                    password_dsl_reset[DSL_PASSWORD_MAXLEN-1] = '\0';
                    break;
                default:
                    fprintf(stderr, "Unknown option in file %s\n",
                            optionsfile);
            }
        }
    }
#endif /* DISABLE_CONFIG_FILE */

    /* command line arguments processing */
    for(i=1; i<argc; i++)
    {
        if(argv[i][0]!='-')
        {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
        }
        else switch(argv[i][1])
            {
                case 't':
                    if(i+1 < argc)
                        v->notify_interval = atoi(argv[++i]);
                    else
                        fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
                    break;
                case 'u':
                    if(i+1 < argc) {
                        strncpy(uuidvalue_igd+5, argv[++i], strlen(uuidvalue_igd+5) + 1);
                        complete_uuidvalues();
                    } else
                        fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
                    break;
#ifdef ENABLE_MANUFACTURER_INFO_CONFIGURATION
                case 'z':
                    if(i+1 < argc)
                        strncpy(friendly_name, argv[++i], FRIENDLY_NAME_MAX_LEN);
                    else
                        fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
                    friendly_name[FRIENDLY_NAME_MAX_LEN-1] = '\0';
                    break;
#endif
                case 's':
                    if(i+1 < argc)
                        strncpy(serialnumber, argv[++i], SERIALNUMBER_MAX_LEN);
                    else
                        fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
                    serialnumber[SERIALNUMBER_MAX_LEN-1] = '\0';
                    break;
                case 'm':
                    if(i+1 < argc)
                        strncpy(modelnumber, argv[++i], MODELNUMBER_MAX_LEN);
                    else
                        fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
                    modelnumber[MODELNUMBER_MAX_LEN-1] = '\0';
                    break;
                case 'U':
                    /*sysuptime = 1;*/
                    SETFLAG(SYSUPTIMEMASK);
                    break;
                case 'S':
                    SETFLAG(SECUREMODEMASK);
                    break;
                case 'p':
                    if(i+1 < argc)
                        v->port = atoi(argv[++i]);
                    else
                        fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
                    break;
                case 'H':
                    if(i+1 < argc)
                        v->https_port = atoi(argv[++i]);
                    else
                        fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
                    break;
                case 'P':
                    if(i+1 < argc)
                        pidfilename = argv[++i];
                    else
                        fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
                    break;
                case 'd':
                    debug_flag = 1;
                    break;
                case 'w':
                    if(i+1 < argc)
                        presurl = argv[++i];
                    else
                        fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
                    break;
                case 'a':
                    if(i+1 < argc)
                    {
                        i++;
                        lan_addr = (struct lan_addr_s *) malloc(sizeof(struct lan_addr_s));
                        if (lan_addr == NULL)
                        {
                            fprintf(stderr, "malloc(sizeof(struct lan_addr_s)): %m");
                            break;
                        }
                        if(parselanaddr(lan_addr, argv[i]) != 0)
                        {
                            fprintf(stderr, "can't parse \"%s\" as a valid "
#ifndef ENABLE_IPV6
                                    "LAN address or "
#endif
                                    "interface name\n", argv[i]);
                            free(lan_addr);
                            break;
                        }
                        /* check if we already have this address */
                        for(lan_addr2 = lan_addrs.lh_first; lan_addr2 != NULL; lan_addr2 = lan_addr2->list.le_next)
                        {
                            if (0 == strncmp(lan_addr2->str, lan_addr->str, 15))
                                break;
                        }
                        if (lan_addr2 == NULL)
                            LIST_INSERT_HEAD(&lan_addrs, lan_addr, list);
                    }
                    else
                        fprintf(stderr, "Option -%c takes one argument.\n", argv[i][1]);
                    break;
                case 'f':
                    i++;	/* discarding, the config file is already read */
                    break;
                default:
                    fprintf(stderr, "Unknown option: %s\n", argv[i]);
            }
    }
    if(!lan_addrs.lh_first)
    {
        /* bad configuration */
        goto print_usage;
    }

    if(debug_flag)
    {
        pid = getpid();
    }
    else
    {
#ifdef USE_DAEMON
		if(daemon(0, 0)<0) {
			perror("daemon()");
		}
		pid = getpid();
#else
        pid = daemonize();
#endif
    }

    openlog_option = LOG_PID|LOG_CONS;
    if(debug_flag)
    {
        openlog_option |= LOG_PERROR;	/* also log on stderr */
    }

    openlog("minitr064d", openlog_option, LOG_MINITR064D);

    if(!debug_flag)
    {
        /* speed things up and ignore LOG_INFO and LOG_DEBUG */
        setlogmask(LOG_UPTO(LOG_NOTICE));
    }

    if(checkforrunning(pidfilename) < 0)
    {
        syslog(LOG_ERR, "MiniTR064d is already running. EXITING");
        return 1;
    }

    set_startup_time(GETFLAG(SYSUPTIMEMASK));

    /* presentation url */
    if(presurl)
    {
        strncpy(presentationurl, presurl, PRESENTATIONURL_MAX_LEN);
        presentationurl[PRESENTATIONURL_MAX_LEN-1] = '\0';
    }
    else
    {
        snprintf(presentationurl, PRESENTATIONURL_MAX_LEN,
                "http://%s/", lan_addrs.lh_first->str);
        /*"http://%s:%d/", lan_addrs.lh_first->str, 80);*/
    }

    /* set signal handler */
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = sigterm;

    if(sigaction(SIGTERM, &sa, NULL) < 0)
    {
        syslog(LOG_ERR, "Failed to set %s handler. EXITING", "SIGTERM");
        return 1;
    }
    if(sigaction(SIGINT, &sa, NULL) < 0)
    {
        syslog(LOG_ERR, "Failed to set %s handler. EXITING", "SIGINT");
        return 1;
    }
    sa.sa_handler = SIG_IGN;
    if(sigaction(SIGPIPE, &sa, NULL) < 0)
    {
        syslog(LOG_ERR, "Failed to ignore SIGPIPE signals");
    }
    sa.sa_handler = sigusr1;
    if(sigaction(SIGUSR1, &sa, NULL) < 0)
    {
        syslog(LOG_NOTICE, "Failed to set %s handler", "SIGUSR1");
    }

    /* initialize random number generator */
    srandom((unsigned int)time(NULL));

    if(writepidfile(pidfilename, pid) < 0)
        pidfilename = NULL;

    /* Init transformer access */
    if(!transformer_proxy_init(&transformer_proxy)) {
        syslog(LOG_ERR, "Failure to initialize Transformer proxy");
        return 1;
    }

    /* Init HTTP Digest secret */
    if (!RAND_bytes(digest_nonce_secret, sizeof digest_nonce_secret)) {
        syslog(LOG_ERR, "Was not able to generate HTTP Digest Nonce random key");
        return 1;
    }

    return 0;
    print_usage:
    fprintf(stderr, "Usage:\n\t"
            "%s "
#ifndef DISABLE_CONFIG_FILE
            "[-f config_file] "
#endif
            "[-i ext_ifname] [-o ext_ip]\n"
            "\t\t[-a listening_ip]"
			" [-H https_port]"
            " [-p port] [-d]"
            " [-U] [-S]"
            "\n"
            /*"[-l logfile] " not functionnal */
            "\t\t[-u uuid] [-s serial] [-m model_number] \n"
            "\t\t[-t notify_interval] [-P pid_filename] "
#ifdef ENABLE_MANUFACTURER_INFO_CONFIGURATION
			"[-z fiendly_name]\n"
#endif
            "\t\t[-w url]\n"
            "\nNotes:\n\tThere can be one or several listening_ips.\n"
            "\tNotify interval is in seconds. Default is 30 seconds.\n"
            "\tDefault pid file is '%s'.\n"
            "\tDefault config file is '%s'.\n"
            "\tWith -d minitr064d will run as a standard program.\n"
            "\t-U causes minitr064d to report system uptime instead "
            "of daemon uptime.\n"
            "\t-w sets the presentation url. Default is http address on port 80\n"
            "\t-h prints this help and quits.\n"
            "", argv[0], pidfilename, DEFAULT_CONFIG);
    return 1;
}

/* === main === */
/* process HTTP or SSDP requests */
int
main(int argc, char * * argv)
{
    int i;
    int shttpl = -1;	/* socket for HTTP */
#if defined(V6SOCKETS_ARE_V6ONLY) && defined(ENABLE_IPV6)
	int shttpl_v4 = -1;	/* socket for HTTP (ipv4 only) */
#endif
	int shttpsl = -1;	/* socket for HTTPS */
#if defined(V6SOCKETS_ARE_V6ONLY) && defined(ENABLE_IPV6)
	int shttpsl_v4 = -1;	/* socket for HTTPS (ipv4 only) */
#endif
    int sudp = -1;		/* IP v4 socket for receiving SSDP */
#ifdef ENABLE_IPV6
	int sudpv6 = -1;	/* IP v6 socket for receiving SSDP */
#endif
#ifdef USE_IFACEWATCHER
	int sifacewatcher = -1;
#endif

    int * snotify = NULL;
    int addr_count;
    LIST_HEAD(httplisthead, upnphttp) upnphttphead;
    struct upnphttp * e = 0;
    struct upnphttp * next;
    fd_set readset;	/* for select() */
    fd_set writeset;
    struct timeval timeout, timeofday, lasttimeofday = {0, 0};
    int max_fd = -1;
#ifdef USE_MINITR064DCTL
	int sctl = -1;
	LIST_HEAD(ctlstructhead, ctlelem) ctllisthead;
	struct ctlelem * ectl;
	struct ctlelem * ectlnext;
#endif
    /* variables used for the unused-rule cleanup process */
    struct lan_addr_s * lan_addr;

    generate_paths();

    if(init(argc, argv, &rv) != 0)
        return 1;
	if(init_ssl() < 0)
		return 1;
    /* count lan addrs */
    addr_count = 0;
    for(lan_addr = lan_addrs.lh_first; lan_addr != NULL; lan_addr = lan_addr->list.le_next)
        addr_count++;
    if(addr_count > 0) {
#ifndef ENABLE_IPV6
        snotify = calloc(addr_count, sizeof(int));
#else
		/* one for IPv4, one for IPv6 */
		snotify = calloc(addr_count * 2, sizeof(int));
#endif
    }

    LIST_INIT(&upnphttphead);
#ifdef USE_MINITR064DCTL
	LIST_INIT(&ctllisthead);
#endif

    if(
            !GETFLAG(ENABLEUPNPMASK) ) {
        syslog(LOG_ERR, "Why did you run me anyway?");
        return 0;
    }

    syslog(LOG_INFO, "Starting%s",
            GETFLAG(ENABLEUPNPMASK) ? "UPnP-IGD " : "");

    if(GETFLAG(ENABLEUPNPMASK))
    {
        unsigned short listen_port;
        listen_port = (rv.port > 0) ? rv.port : 0;
        /* open socket for HTTP connections. Listen on the 1st LAN address */
#ifdef ENABLE_IPV6
		shttpl = OpenAndConfHTTPSocket(&listen_port, 1);
#else /* ENABLE_IPV6 */
        shttpl = OpenAndConfHTTPSocket(&listen_port);
#endif /* ENABLE_IPV6 */
        if(shttpl < 0)
        {
            syslog(LOG_ERR, "Failed to open socket for HTTP. EXITING");
            return 1;
        }
        rv.port = listen_port;
        syslog(LOG_NOTICE, "HTTP listening on port %d", rv.port);
#if defined(V6SOCKETS_ARE_V6ONLY) && defined(ENABLE_IPV6)
		if(!GETFLAG(IPV6DISABLEDMASK))
		{
			shttpl_v4 =  OpenAndConfHTTPSocket(&listen_port, 0);
			if(shttpl_v4 < 0)
			{
				syslog(LOG_ERR, "Failed to open socket for HTTP on port %hu (IPv4). EXITING", v.port);
				return 1;
			}
		}
#endif /* V6SOCKETS_ARE_V6ONLY */
		/* https */
		listen_port = (rv.https_port > 0) ? rv.https_port : 0;
#ifdef ENABLE_IPV6
		shttpsl = OpenAndConfHTTPSocket(&listen_port, 1);
#else /* ENABLE_IPV6 */
		shttpsl = OpenAndConfHTTPSocket(&listen_port);
#endif /* ENABLE_IPV6 */
		if(shttpl < 0)
		{
			syslog(LOG_ERR, "Failed to open socket for HTTPS. EXITING");
			return 1;
		}
		rv.https_port = listen_port;
		syslog(LOG_NOTICE, "HTTPS listening on port %d", rv.https_port);
#if defined(V6SOCKETS_ARE_V6ONLY) && defined(ENABLE_IPV6)
		shttpsl_v4 =  OpenAndConfHTTPSocket(&listen_port, 0);
		if(shttpsl_v4 < 0)
		{
			syslog(LOG_ERR, "Failed to open socket for HTTPS on port %hu (IPv4). EXITING", v.https_port);
			return 1;
		}
#endif /* V6SOCKETS_ARE_V6ONLY */
#ifdef ENABLE_IPV6
		if(find_ipv6_addr(NULL, ipv6_addr_for_http_with_brackets, sizeof(ipv6_addr_for_http_with_brackets)) > 0) {
			syslog(LOG_NOTICE, "HTTP IPv6 address given to control points : %s",
			       ipv6_addr_for_http_with_brackets);
		} else {
			memcpy(ipv6_addr_for_http_with_brackets, "[::1]", 6);
			syslog(LOG_WARNING, "no HTTP IPv6 address, disabling IPv6");
			SETFLAG(IPV6DISABLEDMASK);
		}
#endif

        /* open socket for SSDP connections */
        sudp = OpenAndConfSSDPReceiveSocket(0);
        if(sudp < 0)
        {
            syslog(LOG_NOTICE, "Failed to open socket for receiving SSDP. Trying to use MiniSSDPd");
            if(SubmitServicesToMiniSSDPD(lan_addrs.lh_first->str, rv.port) < 0) {
                syslog(LOG_ERR, "Failed to connect to MiniSSDPd. EXITING");
                return 1;
            }
        }
#ifdef ENABLE_IPV6
		if(!GETFLAG(IPV6DISABLEDMASK))
		{
			sudpv6 = OpenAndConfSSDPReceiveSocket(1);
			if(sudpv6 < 0)
			{
				syslog(LOG_WARNING, "Failed to open socket for receiving SSDP (IP v6).");
			}
		}
#endif

        /* open socket for sending notifications */
        if(OpenAndConfSSDPNotifySockets(snotify) < 0)
        {
            syslog(LOG_ERR, "Failed to open sockets for sending SSDP notify "
                    "messages. EXITING");
            return 1;
        }

#ifdef USE_IFACEWATCHER
		/* open socket for kernel notifications about new network interfaces */
		if (sudp >= 0)
		{
			sifacewatcher = OpenAndConfInterfaceWatchSocket();
			if (sifacewatcher < 0)
			{
				syslog(LOG_ERR, "Failed to open socket for receiving network interface notifications");
			}
		}
#endif
    }

    /* for minitr064dctl */
#ifdef USE_MINITR064DCTL
	sctl = OpenAndConfCtlUnixSocket("/var/run/minitr064d.ctl");
#endif

    /* main loop */
    while(!quitting)
    {
        /* Correct startup_time if it was set with a RTC close to 0 */
        if((startup_time<60*60*24) && (time(NULL)>60*60*24))
        {
            set_startup_time(GETFLAG(SYSUPTIMEMASK));
        }
        /* Check if we need to send SSDP NOTIFY messages and do it if
         * needed */
        if(gettimeofday(&timeofday, 0) < 0)
        {
            syslog(LOG_ERR, "gettimeofday(): %m");
            timeout.tv_sec = rv.notify_interval;
            timeout.tv_usec = 0;
        }
        else
        {
            /* the comparaison is not very precise but who cares ? */
            if(timeofday.tv_sec >= (lasttimeofday.tv_sec + rv.notify_interval))
            {
                if (GETFLAG(ENABLEUPNPMASK))
                    SendSSDPNotifies2(snotify,
                            (unsigned short)rv.port,
					              (unsigned short)rv.https_port,
                            rv.notify_interval << 1);
                memcpy(&lasttimeofday, &timeofday, sizeof(struct timeval));
                timeout.tv_sec = rv.notify_interval;
                timeout.tv_usec = 0;
            }
            else
            {
                timeout.tv_sec = lasttimeofday.tv_sec + rv.notify_interval
                        - timeofday.tv_sec;
                if(timeofday.tv_usec > lasttimeofday.tv_usec)
                {
                    timeout.tv_usec = 1000000 + lasttimeofday.tv_usec
                            - timeofday.tv_usec;
                    timeout.tv_sec--;
                }
                else
                {
                    timeout.tv_usec = lasttimeofday.tv_usec - timeofday.tv_usec;
                }
            }
        }
            /* select open sockets (SSDP, HTTP listen, and all HTTP soap sockets) */
        FD_ZERO(&readset);
        FD_ZERO(&writeset);

        if (sudp >= 0)
        {
            FD_SET(sudp, &readset);
            max_fd = MAX( max_fd, sudp);
#ifdef USE_IFACEWATCHER
			if (sifacewatcher >= 0)
			{
				FD_SET(sifacewatcher, &readset);
				max_fd = MAX(max_fd, sifacewatcher);
			}
#endif
        }
        if (shttpl >= 0)
        {
            FD_SET(shttpl, &readset);
            max_fd = MAX( max_fd, shttpl);
        }
#if defined(V6SOCKETS_ARE_V6ONLY) && defined(ENABLE_IPV6)
		if (shttpl_v4 >= 0)
		{
			FD_SET(shttpl_v4, &readset);
			max_fd = MAX( max_fd, shttpl_v4);
		}
#endif
		if (shttpsl >= 0)
		{
			FD_SET(shttpsl, &readset);
			max_fd = MAX( max_fd, shttpsl);
		}
#if defined(V6SOCKETS_ARE_V6ONLY) && defined(ENABLE_IPV6)
		if (shttpsl_v4 >= 0)
		{
			FD_SET(shttpsl_v4, &readset);
			max_fd = MAX( max_fd, shttpsl_v4);
		}
#endif
#ifdef ENABLE_IPV6
		if (sudpv6 >= 0)
		{
			FD_SET(sudpv6, &readset);
			max_fd = MAX( max_fd, sudpv6);
		}
#endif

        i = 0;	/* active HTTP connections count */
        for(e = upnphttphead.lh_first; e != NULL; e = e->entries.le_next)
        {
            if(e->socket >= 0)
            {
                if(e->state <= EWaitingForHttpContent)
                    FD_SET(e->socket, &readset);
                else if(e->state == ESendingAndClosing)
                    FD_SET(e->socket, &writeset);
                else
                    continue;
                max_fd = MAX(max_fd, e->socket);
                i++;
            }
        }
        /* for debug */
#ifdef DEBUG
		if(i > 1)
		{
			syslog(LOG_DEBUG, "%d active incoming HTTP connections", i);
		}
#endif
#ifdef USE_MINITR064DCTL
		if(sctl >= 0) {
			FD_SET(sctl, &readset);
			max_fd = MAX( max_fd, sctl);
		}

		for(ectl = ctllisthead.lh_first; ectl; ectl = ectl->entries.le_next)
		{
			if(ectl->socket >= 0) {
				FD_SET(ectl->socket, &readset);
				max_fd = MAX( max_fd, ectl->socket);
			}
		}
#endif

        /* queued "sendto" */
        {
            struct timeval next_send;
            i = get_next_scheduled_send(&next_send);
            if(i > 0) {
#ifdef DEBUG
				syslog(LOG_DEBUG, "%d queued sendto", i);
#endif
                i = get_sendto_fds(&writeset, &max_fd, &timeofday);
                if(timeofday.tv_sec > next_send.tv_sec ||
                        (timeofday.tv_sec == next_send.tv_sec && timeofday.tv_usec >= next_send.tv_usec)) {
                    if(i > 0) {
                        timeout.tv_sec = 0;
                        timeout.tv_usec = 0;
                    }
                } else {
                    struct timeval tmp_timeout;
                    tmp_timeout.tv_sec = (next_send.tv_sec - timeofday.tv_sec);
                    tmp_timeout.tv_usec = (next_send.tv_usec - timeofday.tv_usec);
                    if(tmp_timeout.tv_usec < 0) {
                        tmp_timeout.tv_usec += 1000000;
                        tmp_timeout.tv_sec--;
                    }
                    if(timeout.tv_sec > tmp_timeout.tv_sec
                            || (timeout.tv_sec == tmp_timeout.tv_sec && timeout.tv_usec > tmp_timeout.tv_usec)) {
                        timeout.tv_sec = tmp_timeout.tv_sec;
                        timeout.tv_usec = tmp_timeout.tv_usec;
                    }
                }
            }
        }

        if(select(max_fd+1, &readset, &writeset, 0, &timeout) < 0)
        {
            if(quitting) goto shutdown;
            if(errno == EINTR) continue; /* interrupted by a signal, start again */
            syslog(LOG_ERR, "select(all): %m");
            syslog(LOG_ERR, "Failed to select open sockets. EXITING");
            return 1;	/* very serious cause of error */
        }
       
        /* check if a configuration session is ongoing, if so check if it hasn't timeout yet */
        if (config_session_id[0] != '\0')
        {
            struct timespec current_time;
            clock_gettime(CLOCK_MONOTONIC, &current_time);

            syslog(LOG_DEBUG, "Configuration session id %s - time since last update %ds", config_session_id, current_time.tv_sec - config_session_lastupdate.tv_sec);

            if (current_time.tv_sec - config_session_lastupdate.tv_sec > 30)
            {
                // Reset session
                syslog(LOG_NOTICE, "Configuration session id %s has expired. Applying changes and closing session.", config_session_id);
                config_session_id[0] = '\0';
                apply();
            }
        }

        i = try_sendto(&writeset);
        if(i < 0) {
            syslog(LOG_ERR, "try_sendto failed to send %d packets", -i);
        }
#ifdef USE_MINITR064DCTL
		for(ectl = ctllisthead.lh_first; ectl;)
		{
			ectlnext =  ectl->entries.le_next;
			if((ectl->socket >= 0) && FD_ISSET(ectl->socket, &readset))
			{
				char buf[256];
				int l;
				l = read(ectl->socket, buf, sizeof(buf));
				if(l > 0)
				{
					/*write(ectl->socket, buf, l);*/
					write_command_line(ectl->socket, argc, argv);
#ifndef DISABLE_CONFIG_FILE
					write_option_list(ectl->socket);
#endif
					write_upnphttp_details(ectl->socket, upnphttphead.lh_first);
					write_ctlsockets_list(ectl->socket, ctllisthead.lh_first);
					/* close the socket */
					close(ectl->socket);
					ectl->socket = -1;
				}
				else
				{
					close(ectl->socket);
					ectl->socket = -1;
				}
			}
			if(ectl->socket < 0)
			{
				LIST_REMOVE(ectl, entries);
				free(ectl);
			}
			ectl = ectlnext;
		}
		if((sctl >= 0) && FD_ISSET(sctl, &readset))
		{
			int s;
			struct sockaddr_un clientname;
			struct ctlelem * tmp;
			socklen_t clientnamelen = sizeof(struct sockaddr_un);
			/*syslog(LOG_DEBUG, "sctl!");*/
			s = accept(sctl, (struct sockaddr *)&clientname,
			           &clientnamelen);
			syslog(LOG_DEBUG, "sctl! : '%s'", clientname.sun_path);
			tmp = malloc(sizeof(struct ctlelem));
			if (tmp == NULL)
			{
				syslog(LOG_ERR, "Unable to allocate memory for ctlelem in main()");
				close(s);
			}
			else
			{
				tmp->socket = s;
				LIST_INSERT_HEAD(&ctllisthead, tmp, entries);
			}
		}
#endif
        /* process SSDP packets */
        if(sudp >= 0 && FD_ISSET(sudp, &readset))
        {
            /*syslog(LOG_INFO, "Received UDP Packet");*/
			ProcessSSDPRequest(sudp, (unsigned short)rv.port, (unsigned short)rv.https_port);
        }
#ifdef ENABLE_IPV6
		if(sudpv6 >= 0 && FD_ISSET(sudpv6, &readset))
		{
			syslog(LOG_INFO, "Received UDP Packet (IPv6)");
			ProcessSSDPRequest(sudpv6, (unsigned short)rv.port, (unsigned short)rv.https_port);
		}
#endif
#ifdef USE_IFACEWATCHER
		/* process kernel notifications */
		if (sifacewatcher >= 0 && FD_ISSET(sifacewatcher, &readset))
			ProcessInterfaceWatchNotify(sifacewatcher);
#endif

        /* process active HTTP connections */
        /* LIST_FOREACH macro is not available under linux */
        for(e = upnphttphead.lh_first; e != NULL; e = e->entries.le_next)
        {
            if(e->socket >= 0)
            {
                if(FD_ISSET(e->socket, &readset) ||
                        FD_ISSET(e->socket, &writeset))
                {
                    Process_upnphttp(e);
                }
            }
        }
        /* process incoming HTTP connections */
        if(shttpl >= 0 && FD_ISSET(shttpl, &readset))
        {
            struct upnphttp * tmp;
            tmp = ProcessIncomingHTTP(shttpl, "HTTP");
            if(tmp)
            {
                LIST_INSERT_HEAD(&upnphttphead, tmp, entries);
            }
        }
#if defined(V6SOCKETS_ARE_V6ONLY) && defined(ENABLE_IPV6)
		if(shttpl_v4 >= 0 && FD_ISSET(shttpl_v4, &readset))
		{
			struct upnphttp * tmp;
			tmp = ProcessIncomingHTTP(shttpl_v4, "HTTP");
			if(tmp)
			{
				LIST_INSERT_HEAD(&upnphttphead, tmp, entries);
			}
		}
#endif
		if(shttpsl >= 0 && FD_ISSET(shttpsl, &readset))
		{
			struct upnphttp * tmp;
			tmp = ProcessIncomingHTTP(shttpsl, "HTTPS");
			if(tmp)
			{
				InitSSL_upnphttp(tmp);
				LIST_INSERT_HEAD(&upnphttphead, tmp, entries);
			}
		}
#if defined(V6SOCKETS_ARE_V6ONLY) && defined(ENABLE_IPV6)
		if(shttpsl_v4 >= 0 && FD_ISSET(shttpsl_v4, &readset))
		{
			struct upnphttp * tmp;
			tmp = ProcessIncomingHTTP(shttpsl_v4, "HTTPS");
			if(tmp)
			{
				InitSSL_upnphttp(tmp);
				LIST_INSERT_HEAD(&upnphttphead, tmp, entries);
			}
		}
#endif
        /* delete finished HTTP connections */
        for(e = upnphttphead.lh_first; e != NULL; )
        {
            next = e->entries.le_next;
            if(e->state >= EToDelete)
            {
                LIST_REMOVE(e, entries);
                Delete_upnphttp(e);
            }
            e = next;
        }

    }	/* end of main loop */

    shutdown:
    syslog(LOG_NOTICE, "shutting down MiniTR064d");
    /* send good-bye */
    if (GETFLAG(ENABLEUPNPMASK))
    {
#ifndef ENABLE_IPV6
        if(SendSSDPGoodbye(snotify, addr_count) < 0)
#else
		if(SendSSDPGoodbye(snotify, addr_count * 2) < 0)
#endif
        {
            syslog(LOG_ERR, "Failed to broadcast good-bye notifications");
        }
    }
    /* try to send pending packets */
    finalize_sendto();

    /* close out open sockets */
    while(upnphttphead.lh_first != NULL)
    {
        e = upnphttphead.lh_first;
        LIST_REMOVE(e, entries);
        Delete_upnphttp(e);
    }

    if (sudp >= 0) close(sudp);
    if (shttpl >= 0) close(shttpl);
#if defined(V6SOCKETS_ARE_V6ONLY) && defined(ENABLE_IPV6)
	if (shttpl_v4 >= 0) close(shttpl_v4);
#endif
#ifdef ENABLE_IPV6
	if (sudpv6 >= 0) close(sudpv6);
#endif
#ifdef USE_IFACEWATCHER
	if(sifacewatcher >= 0) close(sifacewatcher);
#endif
#ifdef USE_MINITR064DCTL
	if(sctl>=0)
	{
		close(sctl);
		sctl = -1;
		if(unlink("/var/run/minitr064d.ctl") < 0)
		{
			syslog(LOG_ERR, "unlink() %m");
		}
	}
#endif

    if (GETFLAG(ENABLEUPNPMASK))
    {
#ifndef ENABLE_IPV6
        for(i = 0; i < addr_count; i++)
#else
		for(i = 0; i < addr_count * 2; i++)
#endif
                close(snotify[i]);
    }

    /* remove pidfile */
    if(pidfilename && (unlink(pidfilename) < 0))
    {
        syslog(LOG_ERR, "Failed to remove pidfile %s: %m", pidfilename);
    }

    /* delete lists */
    while(lan_addrs.lh_first != NULL)
    {
        lan_addr = lan_addrs.lh_first;
        LIST_REMOVE(lan_addrs.lh_first, list);
        free(lan_addr);
    }

    transformer_proxy_fini(&transformer_proxy);

	free_ssl();
    free(snotify);
    closelog();
#ifndef DISABLE_CONFIG_FILE
    freeoptions();
#endif

    return 0;
}

