#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdarg.h>
#include "crc.h"
#include "rip2.h"

pthread_mutex_t rip2_biglock = PTHREAD_MUTEX_INITIALIZER;
static void *rip_base = NULL;
static size_t rip_size = 0;


int rip2_flash_init(void *base, size_t size)
{
	rip_base = base;
	rip_size = size;

	return 1;
}

int rip2_flash_read(loff_t from,
                           size_t len,
                           size_t *retlen,
                           unsigned char *buf)
{
	from -= RIP2_OFFSET;

	if( (from<0) || (rip_size<from)) {
		/* outside of rip data */
		if( retlen ) *retlen = 0;
		return -1;
	}

	if( from+len>rip_size ) {
		len = rip_size-from;
	}

	memcpy(buf, rip_base+from, len);
	if( retlen ) *retlen = len;

	return len;
}

int rip2_flash_write(loff_t to,
                            size_t len,
                            size_t *retlen,
                            unsigned char *buf)
{
	to -= RIP2_OFFSET;

	if( (to<0) || (rip_size<to)) {
		/* outside of rip data */
		if( retlen ) *retlen = 0;
		return -1;
	}

	if( to+len>rip_size ) {
		len = rip_size - to;
	}

	memcpy(rip_base+to, buf, len);
	if( retlen )*retlen = len;

	return len;
}

int rip2_flash_clear( loff_t to,
                             size_t len,
                             size_t *retlen)
{
	if( (to<0) || (rip_size<to)) {
		if( retlen ) *retlen = 0;
		return -1;
	}

	if( to+len>rip_size) {
		len = rip_size - to;
	}

	memset(rip_base+to, 0, len);
	if( retlen ) *retlen = len;

	return len;
}

int otp_chipid_read()
{
	return 0;
}
