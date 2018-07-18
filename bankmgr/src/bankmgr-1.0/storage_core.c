/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
** Copyright (c) 2013-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
*************************************************************************/

/*
 * Simple Flash Storage VERSION 1 :
 *
 * The format of a flash sector is as follows:
 *
 * [ header ] [ reference table ] [ data blocks ]
 *
 * header	   - generic information (e.g. magic number and block size);
 *
 * reference table - service blocks referencing corresponding data blocks.
 *	Each "data block" (db) has a corresponding "service block" (sb) which
 *	describes "what's in the data block" (empty, dirty, parameter with a given id).
 *	It's possible to calculate a position of "db" knowing a position of its "sb";
 *
 * data blocks	   - data blocks.
 */


/*
 * Simple Flash Storage VERSION 2 supporting all flash types incl nand :
 *
 * The format of a flash sector is as follows:
 *
 * [ header ] [page header + data blocks] [data blocks] ...
 *
 * Each [ ] is one page.
 *
 * header
 *	- generic information (e.g. magic number and page size);
 *
 * page header
 *	- Every 1st "data block" (db) has a "page header" (desc) which
 *	describes the data (size, id, crc over data, crc over header).
 *
 * data blocks
 *	- the data blocks.
 */

#ifdef __KERNEL__
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/err.h>
#else
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#endif

#include "storage_core.h"

#if RAWSTORAGE_VERSION > 1
#include "storage_crc32.h"
#endif

#define SFS_DUAL_BANK_SUPPORT

#if RAWSTORAGE_VERSION < 2
/* CACHE SANITY CHECKS MAKES NO SENSE FOR THE NEW VERSION */
/* BECAUSE IT MIGHT BE THAT THE DESCRIPTOR TURNS OUT OK */
/* WHILE THE DATA IS CORRUPT. IN THAT CASE THE CACHE SANITY */
/* CHECK WOULD FAIL */
#define SFS_DO_EXTRA_CHECKS
#endif

#define SFS_MAGIC_NUMBER	0xa110507f

#define BLOCK_EMPTY		0xffff
#define BLOCK_TRANSACT		0xefff
#define BLOCK_EXT		0xdfff
#define BLOCK_DIRTY		0

#define ALIGN_UP(x,a) (((x)+(a)-1)&~((a)-1))
#define ALIGN_DOWN(x,a) ((x)&~((a)-1))

static inline int is_valid_id(unsigned short id)
{
	return (id > BLOCK_DIRTY && id < BLOCK_EXT);
}

static inline int desc_is_id(struct sfs_desc *d)
{
	return is_valid_id(d->state);
}

/*
 * Get a position for the descriptor #idx in the descriptor's area
 * (reference table): 
 */
#if RAWSTORAGE_VERSION < 2
static inline unsigned long desc_addr(struct sfs_bank *bank, int idx)
{
	return ALIGN_UP(sizeof(struct sfs_super_block), bank->sb.page_size)
			+ idx * SFS_DESC_SIZE;
}
#endif

static inline int size2pages(struct sfs_bank *bank, int size)
{
#if RAWSTORAGE_VERSION < 2
	return ALIGN_UP(size, bank->sb.page_size) / bank->sb.page_size;
#else
	return ALIGN_UP(size + SFS_DESC_SIZE, bank->sb.page_size) / bank->sb.page_size;
#endif
}

static inline int check_range(struct sfs_bank *bank, int idx, int nr_blocks)
{
	return  (idx >= bank->sb.nr_blocks || (idx + nr_blocks >= bank->sb.nr_blocks));
}

/*
 * Get a position for the descriptor #idx in the 'data block' area:
 */
static inline unsigned long data_block_addr(struct sfs_bank *bank, int idx)
{
	if (!bank->cache_dblocks)
#if RAWSTORAGE_VERSION < 2
		bank->cache_dblocks = ALIGN_UP(desc_addr(bank, bank->sb.nr_blocks),
						bank->sb.page_size);
#else
		bank->cache_dblocks = ALIGN_UP(sizeof(struct sfs_super_block), 
						bank->sb.page_size);
#endif
	return bank->cache_dblocks + (idx * bank->sb.page_size);
}

#if RAWSTORAGE_VERSION < 2
static int set_desc_noverify(struct sfs_bank *bank, int idx, struct sfs_desc *desc)
{
	struct sfs_desc disk_desc;

	disk_desc.state = htons(desc->state);
	disk_desc.size = htons(desc->size);

	return storage_write(bank->storage, bank->sb.bank,
			desc_addr(bank, idx), SFS_DESC_SIZE, &disk_desc);
}

static int set_desc(struct sfs_bank *bank, int idx, struct sfs_desc *desc)
{
	if (check_range(bank, idx, size2pages(bank, desc->size)))
		return -1;

	return set_desc_noverify(bank, idx, desc);
}
#endif

static int get_desc(struct sfs_bank *bank, int idx, struct sfs_desc *desc)
{
	struct sfs_desc disk_desc;

	if (check_range(bank, idx, 0))
		return -1;

#if RAWSTORAGE_VERSION < 2
	if (storage_read(bank->storage, bank->sb.bank,
			desc_addr(bank, idx), SFS_DESC_SIZE, &disk_desc))
#else
	if (storage_read(bank->storage, bank->sb.bank,
			data_block_addr(bank, idx), SFS_DESC_SIZE, &disk_desc))
#endif
		return -1;

	desc->state = ntohs(disk_desc.state);
	desc->size = ntohs(disk_desc.size);
#if RAWSTORAGE_VERSION > 1
	desc->crc32_data = ntohl(disk_desc.crc32_data);
	desc->crc32_header = ntohl(disk_desc.crc32_header);
#endif

	if (!desc_is_id(desc) ||
	    check_range(bank, idx, size2pages(bank, desc->size)))
		return -1;

#if RAWSTORAGE_VERSION > 1
	if (storage_crc32_desc(&disk_desc, 0))
		return -1;
#endif

	return 0;
}

#if RAWSTORAGE_VERSION < 2
static int clear_desc(struct sfs_bank *bank, int idx)
{
	struct sfs_desc desc;

	desc.state = BLOCK_DIRTY;
	desc.size  = BLOCK_DIRTY;

	return set_desc_noverify(bank, idx, &desc);
}
#endif

#if RAWSTORAGE_VERSION < 2
static int set_data(struct sfs_bank *bank, int idx, void *data, int size)
{
	return storage_write(bank->storage, bank->sb.bank,
			data_block_addr(bank, idx), size, data);
}
#else
static int set_data(struct sfs_bank *bank, int idx, int id, void *data, int size)
{
	char *page;
	struct sfs_desc desc, disk_desc;
	int len_per_page;
	int index, ret = 0;
	
	if (check_range(bank, idx, size2pages(bank, size)))
		goto out;

	page = memory_alloc(bank->sb.page_size);
	if (!page)
	{
		ret = -1;
		goto out;
	}

	desc.state = id;
	desc.size = size;
	storage_crc32_data(data, size, &desc, &desc.crc32_data);

	disk_desc.crc32_data = htonl(desc.crc32_data);
	disk_desc.state = htons(desc.state);
	disk_desc.size  = htons(desc.size);
	storage_crc32_desc(&disk_desc, &disk_desc.crc32_header);

	len_per_page = bank->sb.page_size - SFS_DESC_SIZE;
	index = 0;
	
	if (len_per_page > size)
		len_per_page = size;
	
	while(size)
	{
		memset(page, 0xFF, bank->sb.page_size);
		if (index == 0)
		{
			memcpy(page, &disk_desc, SFS_DESC_SIZE);
			memcpy(page + SFS_DESC_SIZE, data, len_per_page);
		}
		else
			memcpy(page, &(((char *)data)[index]), len_per_page);
			

		if ((ret = storage_write(bank->storage, bank->sb.bank,
				data_block_addr(bank, idx), bank->sb.page_size, page)))
			goto out;

		idx++;
		index += len_per_page;
		size -= len_per_page;
		if(size > bank->sb.page_size)
			len_per_page = bank->sb.page_size;
		else
			len_per_page = size;

	}

out:
	if (page)
		memory_free(page);

	return ret;
	
}
#endif

static int get_data(struct sfs_bank *bank, int idx, void *data, int count)
{
	struct sfs_desc desc;
	int ret = -1, size, len_per_page, index;

	if (get_desc(bank, idx, &desc))
		return -1;

	size = desc.size;

	/* Size request */
	if (!data && !count)
		return size;

	if (size && count >= size)
#if RAWSTORAGE_VERSION < 2
		ret = storage_read(bank->storage, bank->sb.bank, 
				data_block_addr(bank, idx), size, data);
#else
	{
		len_per_page = bank->sb.page_size - SFS_DESC_SIZE;
		index = 0;
	
		if (len_per_page > size)
			len_per_page = size;
	
		while(size)
		{
			ret = storage_read(bank->storage, bank->sb.bank, 
					data_block_addr(bank, idx) + (index==0?SFS_DESC_SIZE:0), len_per_page, &(((char *)data)[index]));
			
			if (ret)
				goto out;

			idx++;
			index += len_per_page;
			size -= len_per_page;
			if(size > bank->sb.page_size)
				len_per_page = bank->sb.page_size;
			else
				len_per_page = size;
		}

		if (storage_crc32_data(data, desc.size, &desc, 0))
		{
			ret = -1;
			goto out;
		}
	}
#endif

out:
	return (ret? -1 : desc.size);
}

/*
 * Reserve space for nr_blocks and allocate a descriptor:
 */
static int alloc_desc(struct sfs_bank *bank, int nr_blocks)
{
#if RAWSTORAGE_VERSION < 2
	struct sfs_desc desc;
#endif
	int w_blocks = 0,
	    idx = bank->first_free_idx;

	if (check_range(bank, idx, nr_blocks)) {
		rs_debug("alloc_desc: failed\r\n");
		return -1;
	}

#if RAWSTORAGE_VERSION < 2
	desc.state = BLOCK_TRANSACT;
	desc.size  = BLOCK_EMPTY;

	for (; w_blocks < nr_blocks; w_blocks++) {
		if (set_desc_noverify(bank, idx + w_blocks, &desc))
			break;
		if (w_blocks == 0) {
			desc.state = BLOCK_EXT;
			desc.size  = BLOCK_EXT;
		}
	}
#else
	w_blocks = nr_blocks;
#endif

	bank->first_free_idx += w_blocks;

	if (w_blocks != nr_blocks)
		idx = -1;

	rs_debug("alloc_desc: idx = %d\r\n", idx);
	return idx;
}

#if RAWSTORAGE_VERSION < 2
static int clear_desc_data(struct sfs_bank *bank, int idx)
{
	struct sfs_desc desc;
	int n_blks, i, ret = -1;
	void *buf;

	buf = memory_alloc(bank->sb.page_size);
	if (!buf)
		return -1;

	memset(buf, 0, bank->sb.page_size);

	if (get_desc(bank, idx, &desc) == 0) {
		/* to be erased */
		n_blks = size2pages(bank, desc.size);
		ret = 0;

		/* optimize: write a few blocks at once */
		for (i = 0; i < n_blks; i++)
			ret += set_data(bank, idx + i, buf, bank->sb.page_size);
	}
	memory_free(buf);

	if (ret)
		rs_debug("clear_desc_data: failed\r\n");

	return ret;
}

static inline void clear_desc_all(struct sfs_bank *bank, int idx)
{
	clear_desc_data(bank, idx);
	clear_desc(bank, idx);
}
#endif

/*
 * Generic bank management:
 */
static struct sfs_bank* bank_create(void *storage)
{
	struct sfs_bank *bank;

	bank = memory_alloc(sizeof(*bank));
	if (bank) {
		memset(bank, 0, sizeof(*bank));

		bank->first_free_idx = -1;
		bank->storage = storage;

		bank->cache = cache_init();
		if (!bank->cache) {
			memory_free(bank);
			bank = NULL;
		}
	}

	return bank;
}

static void bank_destroy(struct sfs_bank *bank)
{
	cache_destroy(bank->cache);
	memory_free(bank);
}

#if RAWSTORAGE_VERSION < 2
static int bank_populate_cache(struct sfs_bank *bank)
{
	int max_blocks, idx, sb_size, fence_idx;
	struct sfs_super_block *sb;
	struct sfs_desc *dtable;
	int ret = 0;

	max_blocks = bank->sb.nr_blocks;
	sb_size = (int)desc_addr(bank, max_blocks);

	sb = memory_alloc(sb_size);
	if (!sb)
		return -1;

	if (storage_read(bank->storage, bank->sb.bank, 0, sb_size, sb)) {
		memory_free(sb);
		return -1;
	}

	dtable = (struct sfs_desc *)((char *)sb + desc_addr(bank, 0));

	for (fence_idx = 0, idx = 0; idx < max_blocks; idx++) {
		unsigned short val = ntohs(dtable[idx].state),
				size = ntohs(dtable[idx].size);
		int old_idx = -1;

		if (val == BLOCK_EMPTY && size == BLOCK_EMPTY)
			break;

		if (!is_valid_id(val))
			continue;

		if (idx < fence_idx) {
			rs_error("bank_populate_cache: corrupted desc. table!\r\n");
			break;
		}
		fence_idx = idx + size2pages(bank, size);
		if (fence_idx > max_blocks) {
			rs_error("bank_populate_cache: corrupted desc. table!\r\n");
			break;
		}

		/* If duplicate elemenets are detected, do garbage collection: */
		ret = cache_update(bank->cache, val, (unsigned long *)idx,
				(unsigned long *)&old_idx);
		if (ret)
			break;

		if (old_idx != -1)
			clear_desc_all(bank, old_idx);
	}

	memory_free(sb);
	bank->first_free_idx = idx;

	return ret;
}
#else
static int bank_populate_cache(struct sfs_bank *bank)
{
	int idx;
	struct sfs_desc desc, disk_desc;

	int ret = 0;
	void *buf;
	int def_size = 1024;
	int old_idx;
	unsigned long tmp;

	rs_debug("Populating cache...\n");

	buf = memory_alloc(def_size);
	if (!buf)
		return -1;
	
	idx = 0;
	
	while(1)
	{
		if (idx >= bank->sb.nr_blocks)
			break;
			
		ret = storage_read(bank->storage, bank->sb.bank,
			data_block_addr(bank, idx), SFS_DESC_SIZE, &disk_desc);

		if (ret)
			break;
		
		desc.state = ntohs(disk_desc.state);
		desc.size = ntohs(disk_desc.size);
		desc.crc32_data = ntohl(disk_desc.crc32_data);
		desc.crc32_header = ntohl(disk_desc.crc32_header);

		if (desc.state == BLOCK_EMPTY && desc.size == BLOCK_EMPTY && desc.crc32_header == ~0 && desc.crc32_data == ~0)
			break;

		if (storage_crc32_desc(&disk_desc, 0))
		{
			idx++;
			continue;
		}
		
		old_idx = idx;
		idx += size2pages(bank, desc.size);
		
		if (idx >= bank->sb.nr_blocks)
			break;

		if (!desc_is_id(&desc))
			continue;

		if (desc.size > def_size)
		{
			def_size = desc.size;
			memory_free(buf);
			buf = memory_alloc(def_size);
			if (!buf)
				break;
		}
		
		if (get_data(bank, old_idx, buf, desc.size) == -1)
			continue;

		ret = cache_update(bank->cache, desc.state, (unsigned long *)old_idx, &tmp);
		if (ret)
			break;
	}

	if (buf)
		memory_free(buf);
	bank->first_free_idx = idx;

	rs_debug("Populating cache done (%d)\n", ret);
	return ret;
}
#endif

static int bank_sb_format(struct sfs_super_block *sb, int bank_id, void *storage)
{
	struct storage_media_config cfg;
	unsigned short nr_blocks;

	/* Format storage. */
	if (storage_format(storage, bank_id) ||
	    storage_get_config(storage, &cfg))
		return -1;

#if RAWSTORAGE_VERSION < 2
	nr_blocks = (cfg.bank_size 
			- ALIGN_UP(sizeof(struct sfs_super_block), cfg.page_size) 
			- (SFS_DESC_SIZE + cfg.page_size)) /
			(SFS_DESC_SIZE + cfg.page_size);
#else
	nr_blocks = (cfg.bank_size 
			- ALIGN_UP(sizeof(struct sfs_super_block), cfg.page_size)) 
			/ cfg.page_size;
#endif
	sb->magic = SFS_MAGIC_NUMBER;
	sb->page_size = cfg.page_size;
	sb->nr_blocks = nr_blocks;
	sb->bank = bank_id;
	sb->state = BLOCK_EMPTY;

	return 0;
}

static int bank_format(struct sfs_bank *bank, int bank_id)
{
	int ret = bank_sb_format(&bank->sb, bank_id, bank->storage);

	if (!ret)
		bank->first_free_idx = 0;

	return ret;
}

static int bank_sb_commit(struct sfs_super_block *sb, void *storage, unsigned short state)
{
	struct sfs_super_block disk_sb;
	char *page;
	int ret;

	disk_sb.state	    = htons(state);
	disk_sb.magic	    = htonl(sb->magic);
	disk_sb.page_size   = htons(sb->page_size);
	disk_sb.nr_blocks   = htons(sb->nr_blocks);
	disk_sb.bank	    = htons(sb->bank);

#if RAWSTORAGE_VERSION < 2
	ret = storage_write(storage, sb->bank, 0, sizeof(disk_sb), &disk_sb);
#else
	storage_crc32_sb(&disk_sb, &disk_sb.crc32);
	
	page = memory_alloc(sb->page_size);
	if (!page)
		return -1;
	
	memset(page, 0xFF, sb->page_size);
	memcpy(page, &disk_sb, sizeof(struct sfs_super_block));

	ret = storage_write(storage, sb->bank, 0, sb->page_size, page);
	
	rs_debug("bank_sb_commit: magic(%x), state(%d), nr_blocks(%d), crc32(%x), ret %d\r\n",
		ntohl(disk_sb.magic), ntohs(disk_sb.state), ntohs(disk_sb.nr_blocks), ntohl(disk_sb.crc32), ret);

	memory_free(page);
#endif
	return ret;
}

static int bank_commit(struct sfs_bank *bank)
{
	return bank_sb_commit(&bank->sb, bank->storage, BLOCK_EXT);
}

static int bank_sb_read(struct sfs_super_block *sblk, int bank_id, void *storage)
{
	struct sfs_super_block sb;
	int ret = -1;

	/* Read a super block. */
	if (storage_read(storage, bank_id, 0, sizeof(sb), &sb) == 0
	    && sb.magic == ntohl(SFS_MAGIC_NUMBER)) {

#if RAWSTORAGE_VERSION > 1
		if (!storage_crc32_sb(&sb, 0))
#endif
		{
			sblk->magic	 = ntohl(sb.magic);
			sblk->page_size  = ntohs(sb.page_size);
			sblk->nr_blocks  = ntohs(sb.nr_blocks);
			sblk->bank	 = ntohs(sb.bank);
			sblk->state	 = ntohs(sb.state);

			ret = 0;
		}
	}

	rs_debug("bank_sb_read: magic(%x), state(%d), nr_blocks(%d)\r\n",
		sb.magic, ntohs(sb.state), ntohs(sb.nr_blocks));
#if RAWSTORAGE_VERSION > 1
	rs_debug("bank_sb_read: crc32(%x), ret %d\r\n", ntohl(sb.crc32), ret);
#endif

	return ret;
}

/* TODO: simplify/rework all bank_format variants */

int bank_make_empty(int bank_id, void *storage)
{
	struct sfs_super_block sb;

	rs_debug("bank_make_empty(%d)\n", bank_id);

#if RAWSTORAGE_VERSION < 2
	memset(&sb, 0, sizeof(sb));
	sb.magic = SFS_MAGIC_NUMBER;
	sb.bank  = bank_id;

	if (storage_format(storage, bank_id))
		return -1;
#else
	if (bank_sb_format(&sb, bank_id, storage))
		return -1;
#endif

	return bank_sb_commit(&sb, storage, BLOCK_EMPTY);
}

/*
 * The generic media-independent (ooh, supposedly :-) interface:
 */
int storage_ctx_init(struct storage_context *ctx, void *storage)
{
	struct sfs_bank *bank;
	int ret = -1, bank_id;

	bank = bank_create(storage);
	if (bank) {
		for (bank_id = 0; bank_id < 2; bank_id++) {
			if (bank_sb_read(&bank->sb, bank_id, bank->storage) != 0 ||
			    bank->sb.state != BLOCK_EXT)
				continue;

			ret = bank_populate_cache(bank);
			if (!ret)
				break;
		}

		if (ret) {
			bank_destroy(bank);
			bank = NULL;
		} else {
			int inactive_bank = (bank->sb.bank? 0 : 1);
			struct sfs_super_block sb;

			/* 
			 * Always keep only a single bank with data.
			 * 
			 * We may have a situation when we end up
			 * having both banks with the same data
			 * (when switch_banks() is interrupted by
			 * a power-off).
			 */
			if (bank_sb_read(&sb, inactive_bank, storage) != 0 ||
			    sb.state != BLOCK_EMPTY)
				bank_make_empty(inactive_bank, storage);
		}
	}
	ctx->opaque = bank;

	if (ret)
		rs_error("raw_storage: no valid banks have been detected!\r\n");

	return ret;
}

int storage_ctx_close(struct storage_context *ctx)
{
	struct sfs_bank *bank = ctx->opaque;

	if (!bank)
		return -1;

	bank_destroy(bank);
	ctx->opaque = NULL;

	return 0;
}

#ifdef SFS_DO_EXTRA_CHECKS
static int check_cache_sanity(struct sfs_bank *bank, unsigned short id, int idx)
{
	struct sfs_desc desc;

	if (get_desc(bank, idx, &desc) ||
	    desc.state != id) {
		rs_error("check_cache_sanity: cache is out of sync for id (%d)\r\n", id);
		return -1;
	}

	return 0;
}
#else
static inline int check_cache_sanity(struct sfs_bank *bank, unsigned short id, int idx)
{
	return 0;
}
#endif

int storage_get_param(struct storage_context *ctx, unsigned short id,
		      void *data, int count)
{
	struct sfs_bank *bank = ctx->opaque;
	int bytes = -1, idx;

	/* 
	 * data == NULL and count == 0 is valid and a size
	 * of the element is given back:
	 */
	rs_debug("-> id: %d, size: %d\r\n", id, count);

	if (!bank || !is_valid_id(id))
		return -1;

	if (cache_locate(bank->cache, id, (unsigned long *)&idx) == 0 &&
	    check_cache_sanity(bank, id, idx) == 0)
		bytes = get_data(bank, idx, data, count);

	rs_debug("<- id: %d, bytes: %d\r\n", id, bytes);

	return bytes;
}

/* 
 * Return values: 0 upon success, 1 - out of space, (-1) - internal problem. 
 */
static int set_param(struct sfs_bank *bank, unsigned short id,
		     void *data, int count)
{
	int new_idx, old_idx = -1,
	    ret = -1;

	if (data) {
		new_idx = alloc_desc(bank, size2pages(bank, count));

		if (new_idx != -1) {
#if RAWSTORAGE_VERSION < 2
			struct sfs_desc desc;

			desc.state = id;
			desc.size = count;
#endif

#if RAWSTORAGE_VERSION < 2
			/*
			 * Write (1) data and then (2) descriptor,
			 * (3) update the cache and (4) delete the old version:
			 */
			if (set_data(bank, new_idx, data, count) == 0)
				if (set_desc(bank, new_idx, &desc) == 0)
#else
			/*
			 * Write (1) data and descriptor,
			 * (2) update the cache
			 */
			if (set_data(bank, new_idx, id, data, count) == 0)
#endif
					ret = cache_update(bank->cache, id,
							(unsigned long *)new_idx,
							(unsigned long *)&old_idx);
		} else
			ret = 1; /* Out of space */
		
	} else
		ret = cache_del(bank->cache, id, (unsigned long *)&old_idx);

#if RAWSTORAGE_VERSION < 2
	if (!ret && old_idx != -1) {
		if (check_cache_sanity(bank, id, old_idx) == 0)
			clear_desc_all(bank, old_idx);
	}
#endif

	return ret;
}

#ifdef SFS_DUAL_BANK_SUPPORT
static struct sfs_bank* switch_banks(struct sfs_bank *old_bank)
{
	struct sfs_bank *bank;
	int idx, def_size = 1024, bank_id;
	void *buf, *itr;

	rs_debug("switch_banks: starting...\r\n");

	bank_id = (old_bank->sb.bank? 0 : 1);
	bank = bank_create(old_bank->storage);
	buf = memory_alloc(def_size);

	if (!bank || !buf || bank_format(bank, bank_id)) {
		bank_destroy(bank);
		if (buf)
			memory_free(buf);
		return NULL;
	}

	itr = cache_get_iterator(old_bank->cache);

	while ((itr = cache_next_element(old_bank->cache, itr, (unsigned long *)&idx))) {
		struct sfs_desc desc;
		unsigned short id, size;

		if (get_desc(old_bank, idx, &desc))
			break;

		id = desc.state;
		size = desc.size;

		if (size > def_size) {
			def_size = size;
			memory_free(buf);
			buf = memory_alloc(def_size);
			if (!buf)
				break;
		}

		size = get_data(old_bank, idx, buf, def_size);
		if (size <= 0)
			break;
		if (set_param(bank, id, buf, size) != 0)
			break;
	}
	memory_free(buf);

	if (itr || bank_commit(bank)) {
		old_bank = bank;
		bank = NULL;
	}

	storage_format(old_bank->storage, old_bank->sb.bank);
	bank_destroy(old_bank);

	if (bank)
		rs_debug("switch_banks: Ok!\r\n");

	return bank;
}
#endif /* SFS_DUAL_BANK */

int storage_set_param(struct storage_context *ctx, unsigned short id,
		      void *data, int count)
{
	struct sfs_bank *bank = ctx->opaque;
	int bytes = -1;
	int ret;

	rs_debug("-> id: %d, size: %d\r\n", id, count);

	if (!bank || !is_valid_id(id))
		return -1;

	ret = set_param(bank, id, data, count);

	if (ret == 0)
		bytes = count;
#ifdef SFS_DUAL_BANK_SUPPORT
	else if (ret == 1 /* Out of space */) {
		struct sfs_bank *new_bank = switch_banks(bank);

		if (new_bank) {
			ret = set_param(new_bank, id, data, count);
			if (!ret)
				bytes = count;

			/* Update user's context: */
			ctx->opaque = new_bank;
		}
	}
#endif
	rs_debug("<- id: %d, bytes: %d\r\n", id, bytes);

	return bytes;
}

int storage_format_bank(void *storage, int bank_id)
{
	struct sfs_super_block sb;

	if ((unsigned int)bank_id > 1)
		return -1;

	if (bank_sb_format(&sb, bank_id, storage))
		return -1;

	return bank_sb_commit(&sb, storage, BLOCK_EXT);
}

