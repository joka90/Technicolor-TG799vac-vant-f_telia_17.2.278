/* $Id: getifstats.c,v 1.12 2013/04/29 10:18:20 nanard Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006-2013 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <time.h>

#include "../config.h"
#include "../getifstats.h"
#include "../upnpglobalvars.h"
#include <unistd.h>

#ifdef GET_WIRELESS_STATS
#include <unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <linux/wireless.h>
#endif /* GET_WIRELESS_STATS */

/* that is the answer */
#define BAUDRATE_DEFAULT 4200000
#define NBYTES (100)

/****************************************************************************
 Return the interface info based on a script stdout piping in
script output format is: "<WANTYPE>\t<UpstreamBandwith>\t<DownstreamBandwith>
****************************************************************************/
int
waninfoFromScript(const char *path, struct ifdata * data )
{
	int waninfo_retval = 0;
	int r = -1;
	FILE *pipe;
	int num_of_params_read;
	int nbytes = NBYTES;
	int res;
	char wantype[NBYTES], state[NBYTES];
	int us, ds;
	char *my_string;

	if( access(path, F_OK )  != 0 ) {
		syslog(LOG_ERR, "waninfo script file not found %s\n", path);
		return r;
	}

  	/* Open our pipe to read from */
  	pipe = popen (path, "r");
	/* Check that pipes are non-null, therefore open */
	if (!pipe)
	{
		syslog(LOG_ERR, "pipe failed.\n");
		return r;
    }
	my_string = (char *) malloc (nbytes + 1);
	if(my_string == NULL){
		syslog(LOG_ERR, "unable to allocate memory \n");
		return r;
	}
	res=fscanf(pipe,"%100[^\n]",my_string);
	if (res == EOF) return r;

	/* Close ps_pipe, checking for errors */
	if (pclose (pipe) != 0)
    {
		syslog(LOG_ERR, "pipe close failed.\n");
		if ((my_string) != NULL){
			free(my_string);
			my_string=NULL;
		}
		return r;
    }
    if(my_string){
		num_of_params_read = sscanf(my_string, "%s %d %d %s", wantype, &us, &ds, state);
		if (num_of_params_read != 4){
			syslog(LOG_ERR, "not all parameters found (%d !=4)\n", num_of_params_read);
			return r;
		}
		/*syslog(LOG_DEBUG, "found strings: %s - %d - %d - %s\n", wantype, us, ds, state);*/
		strncpy(data->wantype, wantype, UPNP_WAN_TYPE_LEN-1);
		data->upstream_bitrate = us;
		data->downstream_bitrate = ds;
	}else{
		syslog(LOG_ERR, "cannot read from script!\n");
		if ((my_string) != NULL){
			free(my_string);
			my_string=NULL;
		}
		return r;
	}

	free(my_string);
	my_string=NULL;

	return(waninfo_retval);
}

int
getifstats(const char * ifname, struct ifdata * data)
{
	FILE *f;
	char line[512];
	char * p;
	int i;
	int r = -1;
	char fname[64];
#ifdef ENABLE_GETIFSTATS_CACHING
	static time_t cache_timestamp = 0;
	static struct ifdata cache_data;
	time_t current_time;
#endif /* ENABLE_GETIFSTATS_CACHING */
	if(!data)
		return -1;
	data->upstream_bitrate = BAUDRATE_DEFAULT;
	data->downstream_bitrate = BAUDRATE_DEFAULT;
	data->opackets = 0;
	data->ipackets = 0;
	data->obytes = 0;
	data->ibytes = 0;
	strcpy(data->wantype,"DSL");
	if(!ifname || ifname[0]=='\0')
		return -1;
#ifdef ENABLE_GETIFSTATS_CACHING
	current_time = time(NULL);
	if(current_time == ((time_t)-1)) {
		syslog(LOG_ERR, "getifstats() : time() error : %m");
	} else {
		if(current_time < cache_timestamp + GETIFSTATS_CACHING_DURATION) {
			/* return cached data */
			memcpy(data, &cache_data, sizeof(struct ifdata));
			return 0;
		}
	}
#endif /* ENABLE_GETIFSTATS_CACHING */
	f = fopen("/proc/net/dev", "r");
	if(!f) {
		syslog(LOG_ERR, "getifstats() : cannot open /proc/net/dev : %m");
		return -1;
	}
	/* discard the two header lines */
	if(!fgets(line, sizeof(line), f) || !fgets(line, sizeof(line), f)) {
		syslog(LOG_ERR, "getifstats() : error reading /proc/net/dev : %m");
	}
	while(fgets(line, sizeof(line), f)) {
		p = line;
		while(*p==' ') p++;
		i = 0;
		while(ifname[i] == *p) {
			p++; i++;
		}
		/* TODO : how to handle aliases ? */
		if(ifname[i] || *p != ':')
			continue;
		p++;
		while(*p==' ') p++;
		data->ibytes = strtoul(p, &p, 0);
		while(*p==' ') p++;
		data->ipackets = strtoul(p, &p, 0);
		/* skip 6 columns */
		for(i=6; i>0 && *p!='\0'; i--) {
			while(*p==' ') p++;
			while(*p!=' ' && *p) p++;
		}
		while(*p==' ') p++;
		data->obytes = strtoul(p, &p, 0);
		while(*p==' ') p++;
		data->opackets = strtoul(p, &p, 0);
		r = 0;
		break;
	}
	fclose(f);
	/* get interface speed */
	snprintf(fname, sizeof(fname), "/sys/class/net/%s/speed", ifname);
	f = fopen(fname, "r");
	if(f) {
		if(fgets(line, sizeof(line), f)) {
			i = atoi(line);	/* 65535 means unknown */
			if(i > 0 && i < 65535)
				data->upstream_bitrate = 1000000*i;
				data->downstream_bitrate = 1000000*i;
		}
		fclose(f);
	} else {
		syslog(LOG_WARNING, "cannot read %s file : %m", fname);
	}
	/* TCH added, as we don't support /sys/class/net/<itf>/speed. */
	waninfoFromScript(upnp_wgclp, data );

#ifdef GET_WIRELESS_STATS
	if(data->downstream_bitrate == BAUDRATE_DEFAULT) {
		struct iwreq iwr;
		int s;
		s = socket(AF_INET, SOCK_DGRAM, 0);
		if(s >= 0) {
			strncpy(iwr.ifr_name, ifname, IFNAMSIZ);
			if(ioctl(s, SIOCGIWRATE, &iwr) >= 0) {
				data->upstream_bitrate = iwr.u.bitrate.value;
				data->downstream_bitrate = iwr.u.bitrate.value;
			}
			close(s);
		}
	}
#endif /* GET_WIRELESS_STATS */
#ifdef ENABLE_GETIFSTATS_CACHING
	if(r==0 && current_time!=((time_t)-1)) {
		/* cache the new data */
		cache_timestamp = current_time;
		memcpy(&cache_data, data, sizeof(struct ifdata));
	}
#endif /* ENABLE_GETIFSTATS_CACHING */
	return r;
}

