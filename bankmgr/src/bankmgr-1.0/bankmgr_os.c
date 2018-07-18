/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
** Copyright (c) 2013-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
*************************************************************************/

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/err.h>
#include <linux/slab.h>

#include "bankmgr.h"
#include "bankmgr_p.h"

#include "storage_core.h"

#define DEBUG(fmt, args...)	printk(fmt, ##args)
#define ERROR(fmt, args...)	printk(fmt, ##args)

struct storage_context * storage_ctx;

#ifdef CONFIG_MTD_SPEEDTOUCH_MAP_RAW_FLASH
int partitions_invalidate(void);
int partitions_are_invalid(void);
int partitions_reload(void* btab);
#endif

int bankmgr_init() {
	storage_ctx = rawstorage_open();
#ifdef CONFIG_MTD_SPEEDTOUCH_MAP_RAW_FLASH
	if( partitions_are_invalid()) {
		if( storage_ctx ) {
			struct bank_table *btab = load_btab();
			if( btab ) {
				/* release the storage context, otherwise the partition can
				 * not be deleted.
				 */
				rawstorage_close();
				partitions_reload(btab);
				free_btab(btab);
				/* reopen the context now */
				storage_ctx = rawstorage_open();
			}
		} else {
			/* no rawstorage available, used as partitions reload trigger */
				partitions_reload(NULL);
		}
	}
#endif
	return storage_ctx != 0;
}

void bankmgr_fini()
{
#ifdef CONFIG_MTD_SPEEDTOUCH_MAP_RAW_FLASH
	partitions_invalidate();
#endif
	rawstorage_close();
}

struct storage_context *get_storage_ctx(void)
{
	return storage_ctx;
}

struct bank_table* load_btab(void)
{
	struct bank_table *bt = NULL;
	int size;

	size = storage_get_param(storage_ctx, BTAB_ID, NULL, 0);
	if (size == 0)
		goto out;

	bt = kmalloc(size, GFP_KERNEL);
	if (bt) {

		if (storage_get_param(storage_ctx, BTAB_ID, bt, size) != size) {
			kfree(bt);
			bt = NULL;
		}

	}

out:

	return bt;
}

int store_btab(struct bank_table *bt)
{
	int size, ret = BANKMGR_NO_ERR;
	unsigned long checksum;
	struct bank_table *bt_tmp = NULL;
	int maxversion;


	/* get version of banktable currently in rawstorage, because we cant trust the version we get in bt */
	bt_tmp = load_btab();
	if (!bt_tmp) {
		ERROR("load btab failed\n");
		ret = -1;
		goto out;
	}
	
	if (bt_tmp->magic[3]=='B')
		maxversion = 2;
	else if (bt_tmp->magic[3]=='2')
		maxversion = 29999;
	else
	{
		ERROR("no btab magic found\n");
		ret = -1;
		goto out;
	}

	if (ntohs(bt_tmp->version) < maxversion)
		bt->version = htons(ntohs(bt_tmp->version) + 1);
	else
		bt->version = 0;

	if (bt_tmp) free_btab(bt_tmp);

	if (bankmgr_checksum(bt, &checksum)) {
		ERROR("bankmgr_checksum() failed\n");
		ret = -1;
		goto out;
	}
	bt->checksum = htonl(checksum);

	size = sizeof(struct bank_table) + ntohs(bt->num_banks) * sizeof(struct bank);

	if (storage_set_param(storage_ctx, BTAB_ID, bt, size) != size) {
		ERROR("rs_set_param() failed\n");
		ret = BANKMGR_ERR;
	}


out:

	return ret;
}

void free_btab(struct bank_table *bt)
{
	kfree(bt);
}

/* ----------------------------------------------------------------------------
 * 'Public' bank table operations
 * --------------------------------------------------------------------------*/

/*
 * bankmgr_set_active_bank_id -- sets the active bank of a certain type
 */
int bankmgr_set_active_bank_id(enum bank_type type, short bank_id)
{
	struct bank_table *bt = NULL;
	short act_id;
	int ret = BANKMGR_ERR;

	if ((bt = load_btab()) == NULL)
		return BANKMGR_ERR;

	if (ntohl(bt->banks[bank_id].type) != type) {
		ERROR("bankmgr_set_active_bank_id: bank %hd is not of type %d\n", bank_id, type);
		goto out;
	}

	act_id = __bankmgr_get_active_bank_id(bt, type);

	if (bank_id < 0) {
		ERROR("invalid bank_id %d\n", bank_id);
		goto out;
	}

	if (act_id != bank_id) {
		if( act_id>=0 ) {
			bt->banks[act_id].flags &= htons(~(BANK_FLAGS_ACTIVE_MASK));
		}
		bt->banks[bank_id].flags |= htons(BANK_FLAGS_ACTIVE_MASK);
		bt->banks[bank_id].FVP = 0x0000;
		ret = store_btab(bt);
		//DEBUG("New active bank %d\n", bank_id);
	} else {
		/* If the requested bank was already active, then return success */
		ret = BANKMGR_NO_ERR;
		//DEBUG("Bank %d was already active\n", bank_id);
	}

out:
	free_btab(bt);

	return ret;
}

int bankmgr_set_bank_specific(short bank_id, void *data, int size)
{
	return 0;
}

extern unsigned int btab_bank;

/*
 * bankmgr_get_booted_bank_id -- returns the bank id of the booted bank.
 * NOTE: This is not necessarily the same as the active bank, in case of
 * software start failures.
 */
short bankmgr_get_booted_bank_id(void)
{
	return btab_bank;
}

