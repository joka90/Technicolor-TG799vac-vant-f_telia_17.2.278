/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
** Copyright (c) 2013-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
*************************************************************************/

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/mtd/mtd.h>

#include <asm/uaccess.h>

#include "storage_core.h"
#include "bankmgr_proc.h"

static struct storage_device gdevice;
static int gdevice_active;
static DEFINE_MUTEX(glock);

/* Cache */

struct cache_element {
	struct list_head list;
	int key;
	void *opaque;
};

struct cache_struct {
	struct list_head head;
};

void * cache_init(void)
{
	struct cache_struct *cache = kmalloc(sizeof(*cache), GFP_KERNEL);

	if (cache)
		INIT_LIST_HEAD(&cache->head);

	return cache;
}


void cache_destroy(void *obj)
{
	/* delete all the elements first */
	kfree(obj);
}

struct cache_element * __cache_locate(struct cache_struct *c, int key)
{
	struct cache_element *e;

	list_for_each_entry(e, &c->head, list) {
		if (e->key == key) {
			rs_debug("[ %d ] => %lu\n", key, (unsigned long)e->opaque);
			return e;
		}
	}
	rs_debug("[ %d ] => NULL\n", key);

	return NULL;
}

int cache_locate(void *obj, int key, unsigned long *data)
{
	struct cache_element *e;

	e = __cache_locate(obj, key);
	if (e)
		*data = (unsigned long)e->opaque;

	return (e? 0 : -1);
}

int cache_del(void *obj, int key, unsigned long *data)
{
	struct cache_element *e;

	e = __cache_locate(obj, key);
	if (e) {
		*data = (unsigned long)e->opaque;
		list_del(&e->list);
		kfree(e);

		rs_debug("[ %d ]\n", key);
	}

	return (e? 0 : -1);
}

int cache_update(void *obj, int key, unsigned long *data, unsigned long *old_data)
{
	struct cache_struct *c = obj;
	struct cache_element *e;

	e = __cache_locate(c, key);
	if (e) {
		*old_data = (unsigned long)e->opaque;
		e->opaque = (void *)data;

		rs_debug("[ %d ] to %lu from %lu\n", key,
				(unsigned long)data, *old_data);
	} else {
		e = kmalloc(sizeof(*e), GFP_KERNEL);
		if (e) {
			e->key = key;
			e->opaque = (void *)data;
			list_add_tail(&e->list, &c->head);

			rs_debug("[ %d ] to %lu\n", key, (unsigned long)data);
		}
	}

	return (e? 0 : -1);
}

void * cache_get_iterator(void *obj)
{
	return ((struct cache_struct *)obj)->head.next;
}

void * cache_next_element(void *obj, void *itr, unsigned long *data)
{
	struct cache_struct *c = obj;
	struct list_head *pos = itr;
	struct cache_element *e;
	void *next = NULL;

	if (pos != &c->head) {
		e = (struct cache_element *)pos;
		*data = (unsigned long)e->opaque;
		next = pos->next;

		rs_debug("[ %d ] = %lu\n", e->key, *data);
	}

	return next;
}

/* Memory */

void * memory_alloc(int size)
{
	return kmalloc(size, GFP_KERNEL);
}

void memory_free(void *obj)
{
	kfree(obj);
}

/* Storage access */

struct storage_device {
	struct storage_context ctx;
	unsigned long storage;
	struct storage_media_config cfg;
	struct mtd_info *mtd;
};

int storage_read(void *obj, int bank,
		unsigned long off, int len, void *buf)
{
	struct storage_device *dev = obj;
	struct mtd_info *mtd = dev->mtd;
	int ret, retlen;

	rs_debug("off %lu, bank %d, len %d\n", off, bank, len);

	if (off >= dev->cfg.bank_size || off + len > dev->cfg.bank_size)
		return -1;
	if ((unsigned int)bank > 1)
		return -1;

	off += dev->storage + bank * dev->cfg.bank_size;

	ret =  mtd_read(mtd, off, len, &retlen, buf);
	
	if (gdevice.mtd->type == MTD_NANDFLASH)
		if (ret == -EUCLEAN || ret == -EBADMSG)
			ret = 0;
	
	return ret;
}

int storage_write(void *obj, int bank,
		unsigned long off, int len, void *buf)
{
	struct storage_device *dev = obj;
	struct mtd_info *mtd = dev->mtd;
	int ret, retlen;

	rs_debug("off %lu, bank %d, len %d\n", off, bank, len);
	
	if (gdevice.mtd->type == MTD_NANDFLASH)
		if (len != dev->cfg.page_size)
			return -1;

	if (off >= dev->cfg.bank_size || off + len > dev->cfg.bank_size)
		return -1;
	if ((unsigned int)bank > 1)
		return -1;

	off += dev->storage + bank * dev->cfg.bank_size;

	ret = mtd_write(mtd, off, len, &retlen, buf);
	
	if (retlen != len)
	{
		rs_debug("unexpected retlen during write\n");
		return -1;
	}
		
	return ret;
}

static void erase_callback(struct erase_info *done)
{
	wait_queue_head_t *wait_q = (wait_queue_head_t *)done->priv;
	wake_up(wait_q);
}

int storage_format(void *obj, int bank)
{
	struct storage_device *dev = obj;
	struct mtd_info *mtd = dev->mtd;
	DECLARE_WAITQUEUE(wait, current);
	wait_queue_head_t wait_q;
	struct erase_info erase;
	int ret;

	rs_debug("bank %d\n", bank);

	if ((unsigned int)bank > 1)
		return -1;

	init_waitqueue_head(&wait_q);
	erase.mtd = mtd;
	erase.priv = (u_long)&wait_q;
	erase.callback = erase_callback;

	erase.addr = dev->storage + bank * dev->cfg.bank_size;
	erase.len = dev->cfg.bank_size;

	set_current_state(TASK_INTERRUPTIBLE);
	add_wait_queue(&wait_q, &wait);

	ret = mtd_erase(mtd, &erase);
	if (!ret)
		schedule();

	set_current_state(TASK_RUNNING);
	remove_wait_queue(&wait_q, &wait);

	return ret;
}

int storage_get_config(void *obj, struct storage_media_config *cfg)
{
	struct storage_device *dev = obj;

	cfg->bank_size = dev->cfg.bank_size;
	cfg->page_size = dev->cfg.page_size;

	return 0;
}

struct storage_context * rawstorage_open(void)
{
	mutex_lock(&glock);

	if (!gdevice_active) {
		gdevice.mtd = get_mtd_device_nm("rawstorage");

		if ( IS_ERR(gdevice.mtd) ) {
			gdevice.mtd = 0;
			mutex_unlock(&glock);
			rs_debug("No device!\n");
			return 0;
		}
		gdevice.storage = 0;
		gdevice.cfg.bank_size = RAWSTORAGE_BANK_SIZE;
		
		if (gdevice.mtd->type == MTD_NANDFLASH)
			gdevice.cfg.page_size = gdevice.mtd->writesize;
		else
			gdevice.cfg.page_size = RAWSTORAGE_DEFAULT_PAGE_SIZE;

		rs_debug("bank size = %d...\n", gdevice.cfg.bank_size);
		rs_debug("page size = %d...\n", gdevice.cfg.page_size);

		if (storage_ctx_init(&gdevice.ctx, &gdevice)) {
#ifndef CONFIG_MTD_SPEEDTOUCH_MAP_RAW_FLASH
			/* TEMP */
			storage_format_bank(&gdevice, 0);
			if (storage_ctx_init(&gdevice.ctx, &gdevice)) {
#else
			/* in case MAP_RAW_FLASH is active, the location of rawstorage may
			 * be incorrect. In order to make sure we do not accidently overwrite
			 * the eRIP or bootloader we do not create an empty rawstorage.
			 */
			{
#endif
				put_mtd_device(gdevice.mtd);
				gdevice.mtd = 0;
				mutex_unlock(&glock);
				return 0;
			}
		}
		gdevice_active = 1;
	}

	mutex_unlock(&glock);

	return (&gdevice.ctx);
}

void rawstorage_close(void)
{
	mutex_lock(&glock);
	if( gdevice_active ) {
		if( gdevice.mtd ) {
			put_mtd_device(gdevice.mtd);
			gdevice.mtd = 0;
		}
		gdevice_active = 0;
	}
	mutex_unlock(&glock);
}
