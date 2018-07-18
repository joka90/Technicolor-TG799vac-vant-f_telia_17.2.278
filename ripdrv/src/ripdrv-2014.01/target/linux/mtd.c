#include <linux/mtd/mtd.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/crc32.h>
#include <linux/module.h>
#include <linux/version.h>
#include "rip2.h"


static struct mtd_info *mtd = NULL;


int rip2_flash_init(void *base, size_t size)
{
	(void)base;
	(void)size;

	if (mtd == NULL) {
		mtd = get_mtd_device_nm("eripv2");
		if (mtd != ERR_PTR(-ENODEV))
			return 0;
	}
	return -1;
}

int rip2_flash_get_size(void){
	if( mtd )
		return (int) mtd->size;
	return -1;
}

void rip2_flash_release(void)
{
	if (mtd)
		put_mtd_device(mtd);
}

int rip2_flash_read(loff_t from,
			   size_t len,
			   size_t              *retlen,
			   unsigned char       *buf)
{
	int ret;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0)
	ret = mtd_read(mtd, from - RIP2_OFFSET, len, retlen, buf);
#else
	ret = mtd->read(mtd, from - RIP2_OFFSET, len, retlen, buf);
#endif
	*retlen = ret;
	return ret;
}

int rip2_flash_write(loff_t to,
			    size_t len,
			    size_t              *retlen,
			    unsigned char       *buf)
{
	int ret;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0)
	ret = mtd_write(mtd, to - RIP2_OFFSET, len, retlen, buf);
#else
	ret = mtd->write(mtd, to - RIP2_OFFSET, len, retlen, buf);
#endif
	*retlen = ret;
	return ret;
}

int rip2_flash_clear(loff_t to,
			    size_t len,
			    size_t  *retlen)
{
	unsigned char *buf = kmalloc(len, GFP_KERNEL);
	int ret = 0;

	if (buf == NULL)
		return -1;
	memset(buf, 0, len);
	ret = rip2_flash_write(to, len, retlen, buf);
	kfree(buf);
	return ret;
}

