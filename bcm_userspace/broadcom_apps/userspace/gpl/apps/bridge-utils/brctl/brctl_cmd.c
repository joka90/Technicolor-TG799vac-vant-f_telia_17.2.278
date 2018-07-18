/*
 * Copyright (C) 2000 Lennert Buytenhek
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <asm/param.h>
#include "libbridge.h"
#include "brctl.h"
#include <linux/sockios.h>

static int strtotimeval(struct timeval *tv, const char *time)
{
	double secs;
	if (sscanf(time, "%lf", &secs) != 1) 
		return -1;
	tv->tv_sec = secs;
	tv->tv_usec = 1000000 * (secs - tv->tv_sec);
	return 0;
}

static int br_cmd_addbr(int argc, char*const* argv)
{
	int err;

	switch (err = br_add_bridge(argv[1])) {
	case 0:
		return 0;

	case EEXIST:
		fprintf(stderr,	"device %s already exists; can't create "
			"bridge with the same name\n", argv[1]);
		return 1;
	default:
		fprintf(stderr, "add bridge failed: %s\n",
			strerror(err));
		return 1;
	}
}

static int br_cmd_delbr(int argc, char*const* argv)
{
	int err;

	switch (err = br_del_bridge(argv[1])){
	case 0:
		return 0;

	case ENXIO:
		fprintf(stderr, "bridge %s doesn't exist; can't delete it\n",
			argv[1]);
		return 1;

	case EBUSY:
		fprintf(stderr, "bridge %s is still up; can't delete it\n",
			argv[1]);
		return 1;

	default:
		fprintf(stderr, "can't delete bridge %s: %s\n",
			argv[1], strerror(err));
		return 1;
	}
}

static int br_cmd_addif(int argc, char *const* argv)
{
	const char *brname;
	int err;

	argc -= 2;
	brname = *++argv;

	while (argc-- > 0) {
		const char *ifname = *++argv;
		err = br_add_interface(brname, ifname);

		switch(err) {
		case 0:
			continue;

		case ENODEV:
			if (if_nametoindex(ifname) == 0)
				fprintf(stderr, "interface %s does not exist!\n", ifname);
			else
				fprintf(stderr, "bridge %s does not exist!\n", brname);
			break;

		case EBUSY:
			fprintf(stderr,	"device %s is already a member of a bridge; "
				"can't enslave it to bridge %s.\n", ifname,
				brname);
			break;

		case ELOOP:
			fprintf(stderr, "device %s is a bridge device itself; "
				"can't enslave a bridge device to a bridge device.\n",
				ifname);
			break;

		default:
			fprintf(stderr, "can't add %s to bridge %s: %s\n",
				ifname, brname, strerror(err));
		}
		return 1;
	}
	return 0;
}

static int br_cmd_delif(int argc, char *const* argv)
{
	const char *brname;
	int err;

	argc -= 2;
	brname = *++argv;

	while (argc-- > 0) {
		const char *ifname = *++argv;
		err = br_del_interface(brname, ifname);
		switch (err) {
		case 0:
			continue;

		case ENODEV:
			if (if_nametoindex(ifname) == 0)
				fprintf(stderr, "interface %s does not exist!\n", ifname);
			else
				fprintf(stderr, "bridge %s does not exist!\n", brname);
			break;

		case EINVAL:
			fprintf(stderr, "device %s is not a slave of %s\n",
				ifname, brname);
			break;

		default:
			fprintf(stderr, "can't delete %s from %s: %s\n",
				ifname, brname, strerror(err));
		}
		return 1;
	}
	return 0;
}

static int br_cmd_setageing(int argc, char *const* argv)
{
	int err;
	struct timeval tv;
	
	if (strtotimeval(&tv, argv[2])) {
		fprintf(stderr, "bad ageing time value\n");
		return 1;
	}

	err = br_set_ageing_time(argv[1], &tv);
	if (err)
		fprintf(stderr, "set ageing time failed: %s\n",
			strerror(err));

	return err != 0;
}

static int br_cmd_setbridgeprio(int argc, char *const* argv)
{
	int prio;
	int err;

	if (sscanf(argv[2], "%i", &prio) != 1) {
		fprintf(stderr,"bad priority\n");
		return 1;
	}

	err = br_set_bridge_priority(argv[1], prio);
	if (err)
		fprintf(stderr, "set bridge priority failed: %s\n",
			strerror(err));
	return err != 0;
}

static int br_cmd_setfd(int argc, char *const* argv)
{
	struct timeval tv;
	int err;

	if (strtotimeval(&tv, argv[2])) {
		fprintf(stderr, "bad forward delay value\n");
		return 1;
	}

	err = br_set_bridge_forward_delay(argv[1], &tv);
	if (err)
		fprintf(stderr, "set forward delay failed: %s\n",
			strerror(err));

	return err != 0;
}

static int br_cmd_sethello(int argc, char *const* argv)
{
	struct timeval tv;
	int err;

	if (strtotimeval(&tv, argv[2])) {
		fprintf(stderr, "bad hello timer value\n");
		return 1;
	}

	err = br_set_bridge_hello_time(argv[1], &tv);
	if (err)
		fprintf(stderr, "set hello timer failed: %s\n",
			strerror(err));

	return err != 0;
}

static int br_cmd_setmaxage(int argc, char *const* argv)
{
	struct timeval tv;
	int err;

	if (strtotimeval(&tv, argv[2])) {
		fprintf(stderr, "bad max age value\n");
		return 1;
	}
	err = br_set_bridge_max_age(argv[1], &tv);
	if (err)
		fprintf(stderr, "set max age failed: %s\n",
			strerror(err));

	return err != 0;
}

static int br_cmd_setpathcost(int argc, char *const* argv)
{
	int cost, err;

	if (sscanf(argv[3], "%i", &cost) != 1) {
		fprintf(stderr, "bad path cost value\n");
		return 1;
	}

	err = br_set_path_cost(argv[1], argv[2], cost);
	if (err)
		fprintf(stderr, "set path cost failed: %s\n",
			strerror(err));
	return err != 0;
}

static int br_cmd_setportprio(int argc, char *const* argv)
{
	int cost, err;

	if (sscanf(argv[3], "%i", &cost) != 1) {
		fprintf(stderr, "bad path priority value\n");
		return 1;
	}

	err = br_set_port_priority(argv[1], argv[2], cost);
	if (err)
		fprintf(stderr, "set port priority failed: %s\n",
			strerror(errno));

	return err != 0;
}

// brcm begin
static int br_cmd_enableportsnooping(int argc, char*const* argv)
{
	int err;
	int enable;

        sscanf(argv[2], "%i", &enable);

	if((enable < 0) || (enable > 2)) {
		fprintf(stderr, "bad value\n");
		return 1;
        }

	err = br_enable_port_snooping(argv[1], enable);
	if (err)
		fprintf(stderr, "enable port snooping failed: %s\n",
			strerror(errno));

	return err != 0;
}

static int br_cmd_enableproxymode(int argc, char *const* argv)
{
	int err;
	int enable;

        sscanf(argv[2], "%i", &enable);

	if((enable != 0) && (enable != 1)) {
		fprintf(stderr, "bad value\n");
		return 1;
	}

	err = br_enable_proxy_mode(argv[1], enable);
	if (err)
		fprintf(stderr, "enable proxy mode failed: %s\n",
			strerror(errno));

	return err != 0;
}

static int br_cmd_igmp_enableLanToLanMcast(int argc, char*const* argv)
{
	int err;
	char enable;

	sscanf(argv[2], "%hhi", &enable);

	if((enable != -1) && (enable != 0) && (enable != 1)) 
	{
		fprintf(stderr, "bad value %d\n", enable);
		return 1;
	}

	err = br_igmp_enable_lan_to_lan_mcast(argv[1], enable);
	if (err)
	{
		fprintf(stderr, "enable igmp lan to lan mcast failed: %s\n",
			strerror(errno));
	}

	return err != 0;
}

static int br_cmd_mld_enableLanToLanMcast(int argc, char*const* argv)
{
	int err;
	char enable;

	sscanf(argv[2], "%hhi", &enable);

	if((enable != -1) && (enable != 0) && (enable != 1)) 
	{
		fprintf(stderr, "bad value %d\n", enable);
		return 1;
	}

	err = br_mld_enable_lan_to_lan_mcast(argv[1], enable);
	if (err)
	{
		fprintf(stderr, "enable mld lan to lan mcast failed: %s\n",
			strerror(errno));
	}

	return err != 0;

}

static int br_cmd_mld_enableportsnooping(int argc, char*const* argv)
{
	int err;
	int enable;

        sscanf(argv[2], "%i", &enable);

	if((enable < 0) || (enable > 2)) {
		fprintf(stderr, "bad value\n");
		return 1;
	}

	err = br_mld_enable_port_snooping(argv[1], enable);
	if (err)
		fprintf(stderr, "enable mld port snooping failed: %s\n",
			strerror(errno));

	return err != 0;
}

static int br_cmd_mld_enableproxymode(int argc, char*const* argv)
{
	int err;
	int enable;

        sscanf(argv[2], "%i", &enable);

	if((enable != 0) && (enable != 1)) {
		fprintf(stderr, "bad value\n");
		return 1;
	}

	err = br_mld_enable_proxy_mode(argv[1], enable);
	if (err)
		fprintf(stderr, "enable mld proxy mode failed: %s\n",
			strerror(errno));

	return err != 0;
}

static int br_cmd_enableigmpratelimit(int argc, char*const* argv)
{
	int err;
	int limit;

	sscanf(argv[2], "%i", &limit);

	if( limit > 500 )
	{
		fprintf(stderr, "bad value\n");
		return 1;
	}

	err = br_igmp_enable_rate_limit(argv[1], limit);
	if (err)
	{
		fprintf(stderr, "enable igmp rate limit failed: %s\n",
			strerror(errno));
	}

	return err != 0;
}

static int br_cmd_addmacs(int argc, char *const* argv)
{
	const char *brName;
	const char *ifName;
	int         err;

	brName = *++argv;
	ifName = *++argv;
	argc  -= 3; /* skip past the command name and the first 2 arguments */

	while (argc-- > 0)
	{
		const char *pMac = *++argv;
		err = br_add_fdb(brName, ifName, pMac);

		switch(err) {
		case 0:
			continue;

		case ENODEV:
			if (if_nametoindex(ifName) == 0)
				fprintf(stderr, "interface %s does not exist!\n", ifName);
			else
				fprintf(stderr, "bridge %s does not exist!\n", brName);
			break;

		default:
			fprintf(stderr, "can't add mac %s to bridge %s, port %s: %s\n",
				pMac, brName, ifName, strerror(err));
		}
		return 1;
	}

	return 0;
}


static int br_cmd_delmacs(int argc, char *const* argv)
{
	const char *brName;
	const char *ifName;
	int         err;

	brName = *++argv;
	ifName = *++argv;
	argc  -= 3; /* skip past the command name and the first 2 arguments */

	while (argc-- > 0)
	{
		const char *pMac = *++argv;
		err = br_del_fdb(brName, ifName, pMac);

		switch(err) {
		case 0:
			continue;

		case ENODEV:
			if (if_nametoindex(ifName) == 0)
				fprintf(stderr, "interface %s does not exist!\n", ifName);
			else
				fprintf(stderr, "bridge %s does not exist!\n", brName);
			break;

		default:
			fprintf(stderr, "can't add mac %s to bridge %s, port %s: %s\n",
				pMac, brName, ifName, strerror(err));
		}
		return 1;
	}

	return 0;
}

static int br_cmd_block_stp(int argc, char *const* argv)
{
    const char *brName;
    const char *ifName;
    int         err;

    brName = *++argv;
    ifName = *++argv;

    err = br_block_stp(brName, ifName);
    return 0;
}

static int br_dedicate_stp_port(int argc, char *const* argv)
{
    const char *brName;
    const char *ifName;
    int         setting;
    int         err;

    brName = *++argv;
    ifName = *++argv;
    sscanf(*++argv, "%i", &setting);

    err = br_mark_dedicated_stp_port(brName, ifName, setting);
    return 0;
}

static int br_cmd_del_dyn_macs(int argc, char *const* argv)
{
	const char *brName;
	const char *ifName;
	int         err;

	brName = *++argv;
	ifName = *++argv;
  
	err = br_del_dyn_fdb(brName, ifName);

	switch(err) {
	case 0:
		break;

	case ENODEV:
		if (if_nametoindex(ifName) == 0)
			fprintf(stderr, "interface %s does not exist!\n", ifName);
		else
			fprintf(stderr, "bridge %s does not exist!\n", brName);
		return 1;

	default:
		fprintf(stderr, "can't delete dynamic addrs from bridge %s, port %s: %s\n",
			brName, ifName, strerror(err));
		return 1;
	}

	return 0;
}

static int br_cmd_flows(int argc, char *const* argv)
{
	const char *brName;
	const char *rxifName, *txifName;
	int         err;

	brName   = *++argv;
	rxifName = *++argv;
   txifName = *++argv;

   err = br_set_flows(brName, rxifName, txifName);
   if (err)
   {
		switch(err)
      {
		case ENODEV:
			if (if_nametoindex(rxifName) == 0)
				fprintf(stderr, "rx interface %s does not exist!\n", rxifName);
			else if (if_nametoindex(txifName) == 0)
				fprintf(stderr, "tx interface %s does not exist!\n", txifName);
			else
				fprintf(stderr, "bridge %s does not exist!\n", brName);
			break;

		default:
			fprintf(stderr, "%s: can't set flows for path: bridge %s, rxif %s, txif %s\n",
				     strerror(err), brName, rxifName, txifName);
		}
		return 1;
	}

	return 0;
}

static int br_cmd_uni_uni_ctrl(int argc, char*const* argv)
{
	int err;
	int enable;

    sscanf(argv[2], "%i", &enable);

	if((enable < 0) || (enable > 2)) {
		fprintf(stderr, "bad value\n");
		return 1;
        }

	err = br_enable_uni_uni_ctrl(argv[1], enable);
	if (err)
		fprintf(stderr, "enable uni to uni ctrl failed: %s\n",
			strerror(errno));

	return err != 0;
}

// brcm end

static int br_cmd_stp(int argc, char *const* argv)
{
	int stp, err;

	if (!strcmp(argv[2], "on") || !strcmp(argv[2], "yes") 
	    || !strcmp(argv[2], "1"))
		stp = 1;
	else if (!strcmp(argv[2], "off") || !strcmp(argv[2], "no") 
		 || !strcmp(argv[2], "0"))
		stp = 0;
	else {
		fprintf(stderr, "expect on/off for argument\n");
		return 1;
	}

	err = br_set_stp_state(argv[1], stp);
	if (err)
		fprintf(stderr, "set stp status failed: %s\n", 
			strerror(errno));
	return err != 0;
}

static int br_cmd_showstp(int argc, char *const* argv)
{
	struct bridge_info info;

	if (br_get_bridge_info(argv[1], &info)) {
		fprintf(stderr, "%s: can't get info %s\n", argv[1],
			strerror(errno));
		return 1;
	}

	br_dump_info(argv[1], &info);
	return 0;
}

static int show_bridge(const char *name, void *arg)
{
	struct bridge_info info;

	printf("%s\t\t", name);
	fflush(stdout);

	if (br_get_bridge_info(name, &info)) {
		fprintf(stderr, "can't get info %s\n",
				strerror(errno));
		return 1;
	}

	br_dump_bridge_id((unsigned char *)&info.bridge_id);
	printf("\t%s\t\t", info.stp_enabled?"yes":"no");

	br_dump_interface_list(name);
	return 0;
}

static int br_cmd_show(int argc, char *const* argv)
{
	printf("bridge name\tbridge id\t\tSTP enabled\tinterfaces\n");
	br_foreach_bridge(show_bridge, NULL);
	return 0;
}

static int compare_fdbs(const void *_f0, const void *_f1)
{
	const struct fdb_entry *f0 = _f0;
	const struct fdb_entry *f1 = _f1;

	return memcmp(f0->mac_addr, f1->mac_addr, 6);
}

static int br_cmd_showmacs(int argc, char *const* argv)
{
	const char *brname = argv[1];
#define CHUNK 128
	int i, n;
	struct fdb_entry *fdb = NULL;
	int offset = 0;

	for(;;) {
		fdb = realloc(fdb, (offset + CHUNK) * sizeof(struct fdb_entry));
		if (!fdb) {
			fprintf(stderr, "Out of memory\n");
			return 1;
		}
			
		n = br_read_fdb(brname, fdb+offset, offset, CHUNK);
		if (n == 0)
			break;

		if (n < 0) {
			fprintf(stderr, "read of forward table failed: %s\n",
				strerror(errno));
			return 1;
		}

		offset += n;
	}

	qsort(fdb, offset, sizeof(struct fdb_entry), compare_fdbs);

#if defined(CONFIG_BCM_VLAN_AGGREGATION)
	printf("port no\tvid\tmac addr\t\tis local?\tageing timer\n");
#else
	printf("port no\tmac addr\t\tis local?\tageing timer\n");
#endif
	for (i = 0; i < offset; i++) {
		const struct fdb_entry *f = fdb + i;
		printf("%3i\t", f->port_no);
#if defined(CONFIG_BCM_VLAN_AGGREGATION)
		printf("%4i\t", f->vid);
#endif
		printf("%.2x:%.2x:%.2x:%.2x:%.2x:%.2x\t",
		       f->mac_addr[0], f->mac_addr[1], f->mac_addr[2],
		       f->mac_addr[3], f->mac_addr[4], f->mac_addr[5]);
		printf("%s\t\t", f->is_local?"yes":"no");
		br_show_timer(&f->ageing_timer_value);
		printf("\n");
	}
	return 0;
}

static int br_get_port_list(const char *brname, int *ifindices, int num)
{
	unsigned long args[4] = { BRCTL_GET_PORT_LIST,
				  (unsigned long)ifindices, num, 0 };
	struct ifreq ifr;
    int br_socket_fd;   

	memset(ifindices, 0, sizeof(*ifindices)*num);    
    if ((br_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Could not open socket. err %d, %s\n", errno, strerror(errno));
        return -1;
    }    

	strncpy(ifr.ifr_name, brname, IFNAMSIZ);
	ifr.ifr_data = (char *) &args;

	if (ioctl(br_socket_fd, SIOCDEVPRIVATE, &ifr) < 0) {
		printf("br_get_port_list: get %s ports list failed: %s\n", 
			brname, strerror(errno));
		goto error;
	}
    else {
        close(br_socket_fd);
        return 0;
    }

 error:
    close(br_socket_fd);
	return -1;
}

static int br_cmd_show_mac_lmt(int argc, char *const* argv)
{
    const char *brname = argv[1];
    int            fdb_lmt_min, fdb_lmt_max;
    int            err;
    int            i;    
	int            ifindices[1024];
    struct ifreq   ifr;
    unsigned long  args[5] = { BRCTL_GET_BR_FDB_LIMIT, &fdb_lmt_min, 0, 0, 1};
    int br_socket_fd;   
    char ifname[10]={0};

    if (br_get_port_list(brname, ifindices, 1024) != 0)
        return -1;
    
    if ((br_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Could not open socket. err %d, %s\n", errno, strerror(errno));
        return -1;
    }    


    strncpy(ifr.ifr_name, brname, IFNAMSIZ);
    ifr.ifr_data = (char *) args;

    /* read min */
    err = ioctl(br_socket_fd, SIOCDEVPRIVATE, &ifr);

    if (err < 0) {
        printf("Read bridge mac limit fail. err %d, %s\n", errno, strerror(errno));    
        close(br_socket_fd);
        return -1;
    }

    /* read max */
    args[4] = 0;
    args[1] = &fdb_lmt_max;
    err = ioctl(br_socket_fd, SIOCDEVPRIVATE, &ifr);

    if (err < 0) {
        printf("Read bridge mac limit fail. err %d, %s\n", errno, strerror(errno));    
        close(br_socket_fd);
        return -1;
    }
    
	printf("bridge name\t used\t  max\n");

    printf("%11s\t%5d\t%5d\n\n", brname, fdb_lmt_min, fdb_lmt_max);
    
    printf("interfaces\t\t  min\t  max\n");
    /* read port limit */
    args[2] = 1;
    for (i = 0; i < 1024; i++) {
        fdb_lmt_min = 0;
        fdb_lmt_max = 0;
        if (!ifindices[i])
            continue;

        if (if_indextoname(ifindices[i], ifname) == NULL)
            continue;
        
        args[3] = if_nametoindex(ifname);
        if (args[3] == 0) {
            printf("Interface %s doesn't exist\n", ifname);    
            close(br_socket_fd);
            return -1;
        }
        /* read min */
        args[4] = 1;
        args[1] = &fdb_lmt_min;
        err = ioctl(br_socket_fd, SIOCDEVPRIVATE, &ifr);
        if (err < 0) {
            printf("Read port mac min limit fail. err %d, %s\n", errno, strerror(errno));    
            close(br_socket_fd);
            return -1;
        }

        /* read max */
        args[4] = 0;
        args[1] = &fdb_lmt_max;
        err = ioctl(br_socket_fd, SIOCDEVPRIVATE, &ifr);
        if (err < 0) {
            printf("Read port mac max limit fail. err %d, %s\n", errno, strerror(errno));    
            close(br_socket_fd);
            return -1;
        }
        printf("%10s\t\t%5d\t%5d", ifname, fdb_lmt_min, fdb_lmt_max);
        printf("\n");
    }
    close(br_socket_fd);    
    return 0;
}

static const struct command commands[] = {
	{ 1, "addbr", br_cmd_addbr, "<bridge>\t\tadd bridge" },
	{ 1, "delbr", br_cmd_delbr, "<bridge>\t\tdelete bridge" },
	{ 2, "addif", br_cmd_addif, 
	  "<bridge> <device>\tadd interface to bridge" },
	{ 2, "delif", br_cmd_delif,
	  "<bridge> <device>\tdelete interface from bridge" },
	{ 2, "setageing", br_cmd_setageing,
	  "<bridge> <time>\t\tset ageing time" },
	{ 2, "setbridgeprio", br_cmd_setbridgeprio,
	  "<bridge> <prio>\t\tset bridge priority" },
	{ 2, "setfd", br_cmd_setfd,
	  "<bridge> <time>\t\tset bridge forward delay" },
	{ 2, "sethello", br_cmd_sethello,
	  "<bridge> <time>\t\tset hello time" },
	{ 2, "setmaxage", br_cmd_setmaxage,
	  "<bridge> <time>\t\tset max message age" },
	{ 3, "setpathcost", br_cmd_setpathcost, 
	  "<bridge> <port> <cost>\tset path cost" },
	{ 3, "setportprio", br_cmd_setportprio,
	  "<bridge> <port> <prio>\tset port priority" },
// brcm begin
	{ 2, "enableportsnooping", br_cmd_enableportsnooping,
	  "<bridge> <value>\t0-disable 1-standard 2-blocking" },
	{ 2, "enableproxymode", br_cmd_enableproxymode,
	  "<bridge> <value> \tTo enable 1 or disable 0" },
// brcm end
	{ 0, "show", br_cmd_show, "\t\t\tshow a list of bridges" },
	{ 1, "showmacs", br_cmd_showmacs, 
	  "<bridge>\t\tshow a list of mac addrs"},
// brcm begin
	{ 3, "addmacs", br_cmd_addmacs, 
	  "<bridge> <ifname> <mac>\t\tadd mac addresses to the bridge table"},
	{ 3, "delmacs", br_cmd_delmacs, 
	  "<bridge> <ifname> <mac>\t\tremove mac addresses from the bridge table"},
	{ 2, "deldynmacs", br_cmd_del_dyn_macs,
	  "<bridge> <ifname>\t\tremove all dynamic mac address from the bridge table"},
// brcm end
	{ 1, "showstp", br_cmd_showstp, 
	  "<bridge>\t\tshow bridge stp info"},
	{ 2, "stp", br_cmd_stp,
	  "<bridge> {on|off}\tturn stp on/off" },
// brcm begin
	{ 2, "mldenableportsnooping", br_cmd_mld_enableportsnooping,
	  "<bridge> <value>\t0-disable 1-standard 2-blocking" },
	{ 2, "mldenableproxymode", br_cmd_mld_enableproxymode,
	  "<bridge> <value>\tTo enable 1 or disable 0" },
	{ 2, "enableigmplan2lan", br_cmd_igmp_enableLanToLanMcast,
		"<bridge> <value> \t0-disable 1-enable" },
	{ 2, "enablemldlan2lan", br_cmd_mld_enableLanToLanMcast,
		"<bridge> <value> \t0-disable 1-enable" },
	{ 3, "flows", br_cmd_flows,
	  "<bridge> <rxif> <txif>\tTo setup layer 2 flows to the path (rxif->txif)" },
	{ 2, "enableigmpratelimit", br_cmd_enableigmpratelimit,
	  "<bridge> <value>\t0-disable, 1..500-packet rate" },
	{ 2, "eponuniunictrl", br_cmd_uni_uni_ctrl,
	  "<bridge> <value>\t0-disable 1-enable" },
	{ 1, "showmaclmt", br_cmd_show_mac_lmt, 
	  "<bridge>\t\tshow a mac limit of bridge"},
	{ 3, "dedicatestp", br_dedicate_stp_port, 
	  "<bridge> <ifname> <value>\tmark port as dedicated for stp"},
	{ 2, "_teststp", br_cmd_block_stp, 
	  "<bridge> <ifname>\ttoggle bpdu generation (hidden)"},
// brcm end
};

const struct command *command_lookup(const char *cmd)
{
	int i;

	for (i = 0; i < sizeof(commands)/sizeof(commands[0]); i++) {
		if (!strcmp(cmd, commands[i].name))
			return &commands[i];
	}

	return NULL;
}

void command_helpall(void)
{
	int i;

	for (i = 0; i < sizeof(commands)/sizeof(commands[0]); i++) {
			if (commands[i].name[0] != '_')
				printf("\t%-10s\t%s\n", commands[i].name, commands[i].help);
	}
}
