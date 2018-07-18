/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
** Copyright (c) 2013-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
*************************************************************************/

#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/crc32.h>
#include <linux/string.h>
#else
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#endif

#include "storage_core.h"
#include "bankmgr.h"
#include "bankmgr_p.h"

#ifndef __KERNEL__
extern unsigned long crc32_be(unsigned long crc, unsigned char const *p, int len);
#endif

#define CRC32_INIT 0xCAFEF00D

int storage_crc32_sb(struct sfs_super_block *sb, unsigned long *crc32)
{
	unsigned long crc32_old;
	unsigned long crc32_new;
	
	crc32_old = ntohl(sb->crc32);
	
	sb->crc32 = ntohl(CRC32_INIT);
	crc32_new = crc32_be(~CRC32_INIT, (char *)sb, sizeof(struct sfs_super_block));
	
	if (crc32)
		*crc32 = ntohl(crc32_new);
		
	if (crc32_new == crc32_old)
		return 0;
	else
		return -1;
}

int storage_crc32_desc(struct sfs_desc *desc, unsigned long *crc32)
{
	unsigned long crc32_old;
	unsigned long crc32_new;
	
	crc32_old = ntohl(desc->crc32_header);

	desc->crc32_header = ntohl(CRC32_INIT);
	crc32_new = crc32_be(~CRC32_INIT, (char *)desc, SFS_DESC_SIZE);
	
	if (crc32)
		*crc32 = ntohl(crc32_new);
		
	if (crc32_new == crc32_old)
		return 0;
	else
		return -1;
}

int storage_crc32_data(void *data, int size, struct sfs_desc *desc, unsigned long *crc32)
{
	void *ptr = data;
	unsigned long crc32_new;
	
	/* ADD HERE ANY CRC MASKS IF NEEDED */
	
	/* FVP mask in BTAB */
	struct bank_table *btab = 0;
	int i;
	if (desc->state == BTAB_ID)
	{
		rs_debug("rawstorage data crc: detected btab, masking FVP's \n");
		btab = memory_alloc(desc->size);
		if (!btab)
		{
			rs_error("rawstorage: Can't allocate mem\n");
			return -1;
		}
		memcpy(btab, data, desc->size);
		/* Mask out FVP fields */
		for (i = 0; i < ntohs(btab->num_banks); i++)
				btab->banks[i].FVP = 0xffff;
		ptr = btab;
	}

	crc32_new = crc32_be(~CRC32_INIT, (char *)ptr, desc->size);

	if (crc32)
		*crc32 = crc32_new;
	
	if (btab)
		memory_free(btab);
	
	if (crc32_new == desc->crc32_data)
		return 0;
	else
		return -1;
}
