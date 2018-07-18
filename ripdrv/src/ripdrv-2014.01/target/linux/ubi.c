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
#include <linux/mtd/ubi.h>

#define UBI_MAX_DEVICES 32

#define READ_SIZE (4*1024)
#define RIP_SIZE (128*1024)

struct rip2_ubi_device {
	struct ubi_volume_desc* desc;
	unsigned char * buffer;
	int total_volume_size;
	int leb_size;
	int no_of_leb;
	int min_io_size;
};
int rip2_ubi_read_once(void);
static struct rip2_ubi_device* device = NULL;

unsigned long rip2_crc32 (unsigned char *data, unsigned count);

int rip2_flash_init(void *base, size_t size)
{
	int ubi_num = 0, i;
	struct ubi_volume_info vi = {0};
	struct ubi_device_info di = {0};

	(void)base;
	(void)size;

	if(device)
		return EPERM;

	device = kmalloc(sizeof(struct rip2_ubi_device), GFP_KERNEL);

	if(device == NULL)
		return ENOMEM;

	for (i=0; i<UBI_MAX_DEVICES; i++)
	{
		device->desc = ubi_open_volume_nm(i,"eripv2",UBI_READWRITE);
		if (IS_ERR(device->desc))
		{
			continue;
		}
		else
		{
			ubi_num = i;
			break;
		}
	}
	
	if (i == UBI_MAX_DEVICES)
		return ENODEV;

	ubi_get_device_info(ubi_num, &di);
	device->leb_size = di.leb_size;

	ubi_get_volume_info(device->desc, &vi);
	device->no_of_leb = vi.size;
	device->total_volume_size = vi.size * device->leb_size;
	device->min_io_size = di.min_io_size;
	device->buffer = (unsigned char *) kmalloc(RIP_SIZE, GFP_KERNEL);
	if(device->buffer == NULL)
		return ENOMEM;
	rip2_ubi_read_once();
	return 0;
}

int rip2_flash_get_size(void)
{
	int size;

	if( device ) {
		size = device->total_volume_size;

		if(size > 131072)  /* should only be used for the GBNT-D */
			size = 131072;

		return size;
	}
	return -1;
}

void rip2_flash_release(void)
{
	ubi_close_volume(device->desc);
	kfree(device->buffer);
	kfree(device);
	device = NULL;
}

int rip2_flash_read(loff_t from, size_t len, size_t *retlen, unsigned char *buf)
{
	int err = 0, lnum, offs, bytes_left;

	lnum = div_u64_rem(from-RIP2_OFFSET, device->leb_size, &offs);
	bytes_left = len;
	while (bytes_left) {
		size_t to_read = device->leb_size - offs;

		if (to_read > bytes_left)
			to_read = bytes_left;

		err = ubi_read(device->desc, lnum, buf, offs, to_read);
		if (err)
			break;

		lnum += 1;
		offs = 0;
		bytes_left -= to_read;
		buf += to_read;
	}

	*retlen = len - bytes_left;
	return err;
}

int rip2_ubi_read_once(void)
{
   unsigned char *read_buffer = NULL;
   int err;
   size_t retlen = 0;
   int index = 0;
   read_buffer= device->buffer;
   memset(read_buffer, 0,RIP_SIZE);
   for(index  = 0; index < RIP_SIZE; index+=READ_SIZE)
   {
      err = rip2_flash_read(index+RIP2_OFFSET, READ_SIZE, &retlen ,read_buffer+index);
   }
   return err;
}

int rip2_flash_write(loff_t to, size_t len, size_t *retlen, unsigned char *buf)
{
	int err = -1, lnum, offs, bytes_left;
	unsigned char *read_buffer = device->buffer;
	int to_write = 0;
	lnum = div_u64_rem(to-RIP2_OFFSET, device->leb_size, &offs);

	if ((offs + len) > RIP_SIZE)
		return -EINVAL;

        memcpy(read_buffer+offs, buf, len);

	bytes_left = RIP_SIZE;
        while (bytes_left) {
		to_write = (bytes_left < device->leb_size) ? bytes_left : device->leb_size;
		err = ubi_leb_change(device->desc, lnum, read_buffer, to_write);
		bytes_left -= to_write;
		read_buffer += to_write;
		lnum +=1;
	}
	*retlen = len;
	return err;
}

int rip2_flash_clear(loff_t to,
			    size_t len,
			    size_t  *retlen)
{
	unsigned char *buf = kmalloc(len, GFP_KERNEL);
	int err = 0;

	if (buf == NULL)
		return ENOMEM;
	memset(buf, 0, len);
	err = rip2_flash_write(to, len, retlen, buf);
	kfree(buf);
	return err;
}

