/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
** Copyright (c) 2013-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
*************************************************************************/

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/crc32.h>
#include <linux/string.h>

#include "bankmgr_p.h"
#include "bankmgr.h"


int bankmgr_checksum(struct bank_table *bt, unsigned long *checksum)
{
	int i;
	int ret = 0;
	struct bank_table *bt_masked = NULL;
	size_t bt_size;

	/* Create a copy of the bank_table */
	/* Note: to create a copy, we need to know the size. The size depends
	   on the number of banks. If the given bank table is invalid (this
	   can happen when verifying a bank table), the num_banks field may
	   possibly contain rubbish. In this case, the following code could
	   allocate a lot of memory. To avoid this, we check for the sanity of
	   num_banks before making a copy. The value of 100 is 'arbitrarily'
	   chosen.
	   Another way of handling this, is to have two checksums in the bank
	   table: one for the bank table without the banks, and one for the
	   complete bank table. The first checksum would guarantee the validity
	   of num_banks. */
	if (ntohs(bt->num_banks) < 0 || ntohs(bt->num_banks) > 100) {
		ret = -1;
		goto out;
	}

	bt_size = bank_table_size(ntohs(bt->num_banks));
	if ((bt_masked = kmalloc(bt_size, GFP_KERNEL)) == NULL) {
		ret = -1;
		goto out;
	}
	memcpy(bt_masked, bt, bt_size);

	/* Mask out checksum field */
	bt_masked->checksum = 0xffffffff;

	/* Mask out FVP fields */
	for (i = 0; i < ntohs(bt_masked->num_banks); i++) {
		bt_masked->banks[i].FVP = 0xffff;
	}

	/* Calculate CRC32 */
	*checksum = ~crc32_be(-1, (unsigned char *)bt_masked, bt_size);

out:
	if (bt_masked)
		kfree(bt_masked);

	return ret;

}


