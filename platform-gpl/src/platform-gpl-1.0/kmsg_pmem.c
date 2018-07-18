
/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *********************************************
** Copyright (c) [2016] - [2016]  -  [Technicolor Connected Home India Pvt Ltd]				**
** - All Rights Reserved										**
** Technicolor hereby informs you that certain portions							**
** of this software module and/or Work are owned by Technicolor						**
** and/or its software providers.									**
** Distribution copying and modification of all such work are reserved					**
** to Technicolor and/or its affiliates, and are not permitted without					**
** express written authorization from Technicolor.							**
** Technicolor is registered trademark and trade name of Technicolor,					**
** and shall not be used in any manner without express written						**
** authorization from Technicolor									**
*********************************************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/kmsg_dump.h>
#include "kmsg_pmem.h"

#include <linux/version.h>
#include <linux/zlib.h>
#include <asm/io.h>
#include <linux/slab.h>

#define OFFSET 8
#define LOG_BUF_SIZE (64 * 1024)
#define WINDOW_BITS 12
#define MEM_LEVEL 4
static struct z_stream_s stream;

#define MAX_DISCONT_REGIONS	2

typedef struct prz_memory_region {
	char *cont_region;
	unsigned long cont_size;
	unsigned long num_parts;
	char *discont_regions[MAX_DISCONT_REGIONS];
	unsigned long discont_size[MAX_DISCONT_REGIONS];
}prz_memory_region;

prz_memory_region prz_mem;
EXPORT_SYMBOL(prz_mem);

char             *przpanicBuffer                 = NULL;
unsigned long    przpanicBufferSize              = 0;

EXPORT_SYMBOL(przpanicBuffer);
EXPORT_SYMBOL(przpanicBufferSize);

static		struct kmsg_dumper	pmem_dump;

static void flush_memory( prz_memory_region *p_prz_mem ) {
	unsigned long remaining_size, count;
	if (p_prz_mem->cont_size  && p_prz_mem->cont_region && p_prz_mem->num_parts) {
		remaining_size = p_prz_mem->cont_size;
		for (count = 0; count < p_prz_mem->num_parts; count++) {
			if (!remaining_size) {
				break;
			}
			if (remaining_size >= p_prz_mem->discont_size[count]) {
				/* copy entire region */
				memcpy_toio(p_prz_mem->discont_regions[count], &p_prz_mem->cont_region[p_prz_mem->cont_size - remaining_size], p_prz_mem->discont_size[count]);
				remaining_size = remaining_size - p_prz_mem->discont_size[count];
			} else {
				/* copy only until free region */
				memcpy_toio(p_prz_mem->discont_regions[count], &p_prz_mem->cont_region[p_prz_mem->cont_size - remaining_size], remaining_size);
				remaining_size = 0;
			}
		}
	}
}

static void do_compress(	const char *s1, unsigned int l1,
				const char *s2, unsigned int l2,
				char *out_buf, unsigned int out_len) {
	int err;
	int flush = Z_NO_FLUSH;
	unsigned long *p_compressed_len = (unsigned long *)&out_buf[0];

        err = zlib_deflateInit2(&stream, Z_BEST_COMPRESSION, Z_DEFLATED, WINDOW_BITS,
                            MEM_LEVEL, Z_DEFAULT_STRATEGY);
        if (err != Z_OK){
                printk(KERN_ALERT "Deflate Init failed : %d !!!\n",err);
		return ;
        }
	if (!s2) {
		flush = Z_FINISH;
	}
        stream.next_in = (Byte *)s1;
        stream.avail_in = l1;
        stream.total_in = 0;
        stream.next_out = (Byte *)out_buf + OFFSET;
        stream.avail_out = out_len - OFFSET ;
        stream.total_out = 0;

	err = zlib_deflate(&stream, flush);
        if ( err != Z_STREAM_END && err != Z_OK ) {
                printk(KERN_ALERT "Deflate compression failed %d !!\n",err);
        }

	if (s2) {
		stream.next_in = (Byte *)s2;
		stream.avail_in = l2;
		stream.next_out = (Byte *)out_buf + OFFSET + stream.total_out;
		stream.avail_out = out_len - OFFSET - stream.total_out;

		err = zlib_deflate(&stream, Z_FINISH);
		if ( err != Z_STREAM_END && err != Z_OK ) {
			printk(KERN_ALERT "Deflate compression failed %d !!\n",err);
		}
	}

	err = zlib_deflateEnd(&stream);
	if (err != Z_OK)
		printk(KERN_ALERT "zlib_deflateEnd failed %d !!\n",err);
	*p_compressed_len = (unsigned long) stream.total_out;

}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,10,0)
static void pmemoops_do_dump(	struct			kmsg_dumper *dumper,
				enum			kmsg_dump_reason reason,
				const char		*s1,
				unsigned long		l1,
				const char		*s2,
				unsigned long		l2)
{
	static int	avoid_cyclic_update = 0;

	if (reason != KMSG_DUMP_OOPS && reason != KMSG_DUMP_PANIC)
		return;

	if ( przpanicBufferSize < 4 || !przpanicBuffer || avoid_cyclic_update )
	{
		if (avoid_cyclic_update)
		{
			printk(KERN_ERR "Avoided the Cyclic Calling of this Function \n");
			return;
		}
		printk(KERN_ERR "pmemoops :Failed to get the PMEM buffer [%p ] size [%d]  \n",przpanicBuffer,(int)przpanicBufferSize);
		return;
	}

	do_compress(s1,l1,s2,l2,przpanicBuffer, przpanicBufferSize);
	avoid_cyclic_update = 1;
}
#else
static void pmemoops_do_dump(	struct			kmsg_dumper *dumper,
				enum			kmsg_dump_reason reason)
{
	static int	avoid_cyclic_update = 0;
	char *s1;
	unsigned long l1 = 0;

	if (reason != KMSG_DUMP_OOPS && reason != KMSG_DUMP_PANIC)
		return;

	if ( przpanicBufferSize < 4 || !przpanicBuffer || avoid_cyclic_update )
	{
		if (avoid_cyclic_update)
		{
			printk(KERN_ERR "Avoided the Cyclic Calling of this Function \n");
			return;
		}
		printk(KERN_ERR "pmemoops :Failed to get the PMEM buffer [%p ] size [%d]  \n",przpanicBuffer,(int)przpanicBufferSize);
		return;
	}

	l1 = LOG_BUF_SIZE;
        s1 = kzalloc(l1, GFP_KERNEL);
        if (!s1) {
                printk(KERN_ALERT "No memory to handle Kernel Diagnostic !!!\n");
                return ;
        }
	if (kmsg_dump_get_buffer(dumper, true, s1, l1, &l1) == true) {
		do_compress(s1,l1,NULL,0, przpanicBuffer, przpanicBufferSize);
		flush_memory(&prz_mem);
		avoid_cyclic_update = 1;
	}
	kfree(s1);
}
#endif

int pmemoops_init(void)
{
	int err;
	stream.workspace = kmalloc(zlib_deflate_workspacesize(WINDOW_BITS, MEM_LEVEL), GFP_KERNEL);
	if (!stream.workspace) {
                printk(KERN_ALERT "No memory for compression workspace; skipping compression\n");
		return -1;
        }
	pmem_dump.dump = pmemoops_do_dump;
	err = kmsg_dump_register(&pmem_dump);

	if (err)
	{
		printk(KERN_ERR "pmemoops: registering kmsg dumper failed, error %d\n", err);
		return -ENOMEM;
	}
	return 0;
}

void pmemoops_exit(void)
{
	kmsg_dump_unregister(&pmem_dump);
}
