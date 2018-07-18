/*
 * To compile:
 *   gcc ../storage_core.c host_test.c -o rs_host_test -Wall -I/cm4/fsn/lib/rawstorage
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "storage_core.h"

/*************** Storage *****************/

#define STORAGE_PAGE	64
#define STORAGE_SIZE	64 * 1024

int storage_read(void *obj, int bank,
		unsigned long off, int len, void *buf)
{
	unsigned char *ptr = obj;

	off += bank * STORAGE_SIZE;
	ptr += off;

	rs_debug("from off(%lu), to %p, len %d\n", off, buf, len);

	memcpy(buf, ptr, len);
	return 0;
}

int storage_write(void *obj, int bank,
		unsigned long off, int len, void *buf)
{
	unsigned char *ptr = obj;

	off += bank * STORAGE_SIZE;
	ptr += off;

	rs_debug("from %p, to off(%lu), len %d\n", buf, off, len);

	memcpy(ptr, buf, len);
	return 0;
}

int storage_format(void *obj, int bank)
{
	unsigned char *ptr = obj;

	ptr += bank * STORAGE_SIZE;

	rs_debug("bank #%d\n", bank);

	memset(ptr, 0xff, STORAGE_SIZE);
	return 0;
}

int storage_get_config(void *obj, struct storage_media_config *cfg)
{
	cfg->bank_size = STORAGE_SIZE;
	cfg->page_size = STORAGE_PAGE;
	return 0;
}

/*************** Cache *****************/

struct list_head {
	struct list_head *next, *prev;
};

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

static inline void __list_add(struct list_head *new,
			      struct list_head *prev,
			      struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}

static inline void __list_del(struct list_head * prev, struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = NULL;
	entry->prev = NULL;
}

#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)


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
	struct cache_struct *cache;
	
	cache = malloc(sizeof(*cache));
	if (cache)
		INIT_LIST_HEAD(&cache->head);

	return cache;
}

void cache_destroy(void *obj)
{
	/* delete all the elements first */

	free(obj);	
}

struct cache_element * __cache_locate(struct cache_struct *c, int key)
{
	struct cache_element *e;
	struct list_head *itr;

	list_for_each(itr, &c->head) {
		e = (struct cache_element *)itr;
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
		free(e);

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
		e = malloc(sizeof(*e));
		if (e) {
			e->key = key;
			e->opaque = (void *)data;
			list_add_tail(&e->list, &c->head);

			rs_debug("[ %d ] to %lu\n", key,
				(unsigned long)data);
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

/* CRC STUFF: calculate a BE crc on a LE machine. Arghhh... */

#define CRCPOLY_BE 0x04c11db7

uint32_t crc32table_be[1<<8];

static void crc32init_be(void)
{
	unsigned i, j;
	uint32_t crc = 0x80000000;

	crc32table_be[0] = 0;

	for (i = 1; i < 1<<8; i <<= 1) {
		crc = (crc << 1) ^ ((crc & 0x80000000) ? CRCPOLY_BE : 0);
		for (j = 0; j < i; j++)
			crc32table_be[i + j] = (crc ^ crc32table_be[j]);
	}
}

unsigned long crc32_be(uint32_t crc, unsigned char const *p, int len)
{
	const uint32_t *b = (uint32_t *)p;
	const uint32_t *tab = crc32table_be;

#  define DO_CRC(x) crc = ntohl(tab[ (crc ^ (x)) & 255 ]) ^ (crc>>8)

	crc = ntohl(crc);
	/* Align it */
	if((((unsigned long)b)&3 && len)){
		do {
			char *p = (char *)b;
			DO_CRC(*p++);
			b = (unsigned long *)p;
		} while ((--len) && ((unsigned long)b)&3 );
	}
	if((len >= 4)){
		/* load data 32 bits wide, xor data 32 bits wide. */
		int save_len = len & 3;
		len = len >> 2;
		--b; /* use pre increment below(*++b) for speed */
		do {
			crc ^= *++b;
			DO_CRC(0);
			DO_CRC(0);
			DO_CRC(0);
			DO_CRC(0);
		} while (--len);
		b++; /* point to next byte(s) */
		len = save_len;
	}
	/* And the last few bytes */
	if(len){
		do {
			char *p = (char *)b;
			DO_CRC(*p++);
			b = (void *)p;
		} while (--len);
	}
	return ntohl(crc);
}


/**************** Memory allocator ******************/

void * memory_alloc(int size)
{
	return malloc(size);
}

void memory_free(void *obj)
{
	free(obj);
}

/******************** Test *********************/

struct storage_context ctx;

struct foo {
	int a, b, c;
};
static struct foo test_foo = { 234, 567, 890 };
static int test_int = 111;
static char test_str[] = "Hello, World! This string is longer than 64 bytes. Really longer?"
			 "Oh yeah baby.. ooh, I mean, really-really! And by the way,"
			 "it's kind of a cool string! Heh, it's quite hard to be"
			 "cool nowadays...";

enum {
	ID_TEST_INT = 1,
	ID_TEST_STR,
	ID_TEST_FOO,
	ID_FREE_SLOT,
};

static void error(const char *err_str)
{
	printf("rs_error: %s\n", err_str);
	exit(1);
}

static void simple_read_test(void)
{
	struct foo f;
	int a, ret;
	char str[256];

	printf("*** Simple READ test\n");

	if (storage_get_param(&ctx, ID_TEST_INT, &a, sizeof(a)) != sizeof(a))
		error("get_param for test_int");
	if (a != test_int)
		error("test_int is not correct\n");

	if (storage_get_param(&ctx, ID_TEST_FOO, &f, sizeof(f)) != sizeof(f))
		error("get_param for test_foo");
	if (memcmp(&f, &test_foo, sizeof(f)))
		error("test_foo is not correct\n");

	if ((ret = storage_get_param(&ctx, ID_TEST_STR, str, sizeof(str))) != strlen(test_str) + 1)
		error("get_param for test_str");
	if (memcmp(str, test_str, ret))
		error("test_str is not correct\n");

	printf("Ok.\n");
}

static void simple_write_test(void)
{
	printf("*** Simple WRITE test\n");

	if (storage_set_param(&ctx, ID_TEST_INT, &test_int, sizeof(test_int)) <= 0)
		error("set_param for test_int");
	if (storage_set_param(&ctx, ID_TEST_STR, test_str, strlen(test_str) + 1) <= 0)
		error("set_param for test_str");
	if (storage_set_param(&ctx, ID_TEST_FOO, &test_foo, sizeof(test_foo)) <= 0)
		error("set_param for test_foo");

	printf("Ok.\n");
}

static void switch_banks_test(void)
{
	int i, test;

	printf("*** Switch Banks test\n");

	for (i = 0; i < 10; i++) {
		if (storage_set_param(&ctx, ID_FREE_SLOT, &i, sizeof(i)) != sizeof(i))
			error("set_param for ID_FREE_SLOT");

		if (storage_get_param(&ctx, ID_FREE_SLOT, &test, sizeof(test)) != sizeof(test))
			error("get_param for ID_FREE_SLOT");

		if (test != i)
			error("expected != received\n");
	}
	printf("Ok.");
}

int main()
{
	crc32init_be();

	void *storage = malloc(2 * STORAGE_SIZE);
	if (!storage)
		return 1;

	if (storage_format_bank(storage, 0))
		error("storage_format");

	printf("Mounting... #1\n");
	if (storage_ctx_init(&ctx, storage))
		error("storage_ctx_init failed");
	printf("Ok.\n");

	simple_write_test();
	simple_read_test();

	printf("Unmounting...\n");
	if (storage_ctx_close(&ctx))
		error("storage_ctx_close failed\n");
	printf("Ok.\n");

	printf("Mounting... #2\n");
	if (storage_ctx_init(&ctx, storage))
		error("storage_ctx_init failed");
	printf("Ok.\n");

	simple_read_test();

	switch_banks_test();
	simple_read_test();

	printf("Unmounting...\n");
	if (storage_ctx_close(&ctx))
		error("storage_ctx_close failed\n");
	printf("Ok.\n");

	printf("Mounting... #3\n");
	if (storage_ctx_init(&ctx, storage))
		error("storage_ctx_init failed");
	printf("Ok.\n");

	simple_read_test();
	
	printf("Unmounting...\n");
	if (storage_ctx_close(&ctx))
		error("storage_ctx_close failed\n");
	printf("Ok.\n");

	free(storage);

	return 0;
}

