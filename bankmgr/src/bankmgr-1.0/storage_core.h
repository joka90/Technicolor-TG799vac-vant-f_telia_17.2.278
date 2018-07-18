/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
** Copyright (c) 2013-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
*************************************************************************/

#ifndef __STORAGE_CORE_H
#define __STORAGE_CORE_H

/* 
 * Generic storage-independent implementation:
 */

#ifndef RAWSTORAGE_VERSION
#define RAWSTORAGE_VERSION CONFIG_RAWSTORAGE_VERSION
#endif

struct storage_context {
	void *opaque;
};

extern int storage_ctx_init(struct storage_context *ctx, void *storage);

extern int storage_ctx_close(struct storage_context *ctx);

extern int storage_get_param(struct storage_context *ctx, unsigned short id, 
			void *data, int count);

extern int storage_set_param(struct storage_context *ctx, unsigned short id,
			void *data, int count);

extern int storage_format_bank(void *storage, int bank_id);

/*
 * Implementation-specific details:
 */

/* Storage */

struct storage_media_config {
	unsigned int bank_size;
	unsigned short page_size;
};

extern int storage_read(void *obj, int bank, unsigned long off, int len, void *buf);
extern int storage_write(void *obj, int bank, unsigned long off, int len, void *buf);
extern int storage_format(void *obj, int bank);
extern int storage_get_config(void *obj, struct storage_media_config *cfg);

/* Memory allocator */

extern void * memory_alloc(int size);
extern void memory_free(void *obj);

/* Cache */

extern void * cache_init(void);
extern void cache_destroy(void *c);

extern void * cache_get_iterator(void *c);
extern void * cache_next_element(void *c, void *itr, unsigned long *data);

extern int cache_update(void *c, int key, unsigned long *data,
			unsigned long *old_data);
extern int cache_del(void *c, int key, unsigned long *data);
extern int cache_locate(void *c, int key, unsigned long *data);

extern int cache_list_all_items(void *obj, void (*callback)(unsigned short id, void * custom), void * custom);

/*
 * The following structures are placed in storage:
 */

#if RAWSTORAGE_VERSION < 2
#pragma pack(1)
/* 
 * NOTE: the 'sfs_desc' takes 16 bits in-flash. Our assumption is
 * that 16 bit aligned writes are seen as 'atomic', i.e. either 
 * the whole 16 bits are written into flash or none. Otherwise,
 * we may get problems, e.g. in set_desc().
 *
 * Ok, 'state' must be written after 'size'. This should imply that if
 * 'state' is valid, then 'size' should be ok (it has been written
 * before 'state'. Grr... alternatively, enable checksums.
 */

struct sfs_desc {
	unsigned short size;
	unsigned short state;
	/* add checksum? */
};

struct sfs_super_block {
	unsigned int magic;

	unsigned short page_size;
	unsigned short nr_blocks;
	unsigned short bank;
	unsigned short state;
	/* add checksum? */

	struct sfs_desc desc_table[0];
};
#pragma pack()

#define RAWSTORAGE_DEFAULT_PAGE_SIZE 64
#define RAWSTORAGE_BANK_SIZE 128*1024

#else /* RAWSTORAGE_VERSION > 1 */

#pragma pack(1)

struct sfs_desc {
	unsigned short size;
	unsigned short state;
	
	unsigned long crc32_data;
	unsigned long crc32_header;
};

struct sfs_super_block {
	unsigned int magic;

	unsigned short page_size;
	unsigned short nr_blocks;
	unsigned short bank;
	unsigned short state;
	
	unsigned long crc32;
};
#pragma pack()

#define RAWSTORAGE_DEFAULT_PAGE_SIZE 256
#define RAWSTORAGE_BANK_SIZE 128*1024

#endif /* RAWSTORAGE_VERSION */


struct sfs_bank {
	struct sfs_super_block sb;
	void *cache,
	     *storage;
	int first_free_idx;

	unsigned long cache_dblocks;
};

#define SFS_DESC_SIZE		sizeof(struct sfs_desc)

/* Debugging */

#ifdef __KERNEL__
#define print_method	printk
#else
#define print_method	printf
#endif

#define rs_error(format, args...)  print_method("%s : "format"\n", __FUNCTION__, ##args)

#ifdef RAWSTORAGE_rs_debug
#define rs_debug(format, args...)  print_method("%s : "format"\n", __FUNCTION__, ##args)
#else
#define rs_debug(format, args...)
#endif

struct storage_context * rawstorage_open(void);
void rawstorage_close(void);

#endif /* __STORAGE_CORE_H */
