#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/crc32.h>
#include "rip2.h"
#include "rip2_cert_privkey.h"
#include "otp_api.h"
#include "otp.h"

EXPORT_SYMBOL(rip2_drv_read);
EXPORT_SYMBOL(rip2_get_cert_privkey_position);

DEFINE_MUTEX(rip2_biglock);

unsigned long rip2_crc32 (unsigned char *data, unsigned count);


unsigned long rip2_crc32 (unsigned char *data, unsigned count)
{
	u32 crc;
	crc = crc32_be(~0, data, count);
	return (~crc);
}

int otp_chipid_read()
{
	return otp_getchipid();
}
