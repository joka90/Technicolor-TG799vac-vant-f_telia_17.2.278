
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "unittest/unittest.h"

#include "rip2.h"
#include "rip2_common.h"
#include "crc.h"

uint8_t *rip = NULL;

static void setup(void)
{
    int rv;
    ck_assert_msg((RIP2_OFFSET==0x20000), "RIP2_OFFSET defined with wrong value");
    rip = malloc(RIP2_SZ);
    ck_assert_msg(rip!=NULL, "Failed to alloc rip");

    /* Initialie CRC32 */
    rip2_mk_crc32_table(CRC32, rip2_crc32_hw);

    /* Fill with 0xFF as an empty flash would be */
    memset(rip, 0xFF, RIP2_SZ);
    rip2_flash_init(rip, RIP2_SZ);

    rv = rip2_init(rip, 0, RIP2_SZ);
    ck_assert_int_eq(rv, RIP2_SUCCESS);
}

static void teardown(void)
{
    free(rip);
    rip = NULL;
}

START_TEST(test_rip2_init__init)
    uint8_t *hdr = (uint8_t*)(rip+RIP2_SZ-sizeof(uint32_t));
    ck_assert_int_eq(hdr[0], 0x02);
    ck_assert_int_eq(hdr[1], 0xFF);
    ck_assert_int_eq(hdr[2], 0xFF);
    ck_assert_int_eq(hdr[3], 0xFF);
END_TEST

START_TEST(test_rip2_drv_read)
    int rv;
    T_RIP2_ID id = 0x9999;
    char *TestData = "DATA";
    char buffer[10];
    unsigned long sz;

    rv = rip2_drv_write((uint8_t*)TestData, strlen(TestData)+1, id, 0xFFFFFFFF, 0xFFFFFFFF);
    ck_assert_int_eq(rv, RIP2_SUCCESS);

    sz = sizeof(buffer);
    rv = rip2_drv_read(&sz, id, buffer);
    ck_assert_int_eq(rv, RIP2_SUCCESS);
    ck_assert_int_eq(sz, strlen(TestData)+1);
    ck_assert(strcmp(TestData, buffer)==0);

    sz = 1;
    rv = rip2_drv_read(&sz, id, buffer);
    ck_assert_int_eq(rv, RIP2_ERR_NOMEM);

    rv = rip2_drv_read(NULL, id, buffer);
    ck_assert_int_eq(rv, RIP2_ERR_NOMEM);

    sz = sizeof(buffer);
    id = 1;
    rv = rip2_drv_read(&sz, id, buffer);
    ck_assert_int_eq(rv, RIP2_ERR_NOELEM);
END_TEST

START_TEST(test_rip2_get_item)
	int rv;
	T_RIP2_ID id = 0x9999;
	char *TestData = "DATA";

	T_RIP2_ITEM raw;
	T_RIP2_ITEM item;

	/* these attribute values are unused in rip2 but we will use these values
	 * to check the translation.
	 */
	uint32_t attr_hi = 0xFFFFFFFF;
	uint32_t attr_lo = 0xFFFFFFFF;
	rv = rip2_drv_write((uint8_t*)TestData, strlen(TestData)+1, id, attr_hi, attr_lo);
	ck_assert_int_eq(rv, RIP2_SUCCESS);

	rv = rip2_get_idx(id, &raw);
	ck_assert_int_eq(rv, RIP2_SUCCESS);

	rv = rip2_get_item(id, &item);
	ck_assert_int_eq(rv, RIP2_SUCCESS);
	ck_assert_int_eq(item.length, strlen(TestData)+1);
	ck_assert_int_eq(item.attr[0], attr_lo);
	ck_assert_int_eq(item.attr[1], attr_hi);
	ck_assert_int_eq(item.addr, BETOH32(raw.addr));
END_TEST

static singleTestCaseInfo testcases[] = {
    { .name = "rip2init, init only", .function = test_rip2_init__init},
    { .name = "rip2_drv_read", .function = test_rip2_drv_read},
    { .name = "rip2_get_item", .function = test_rip2_get_item},
    { }
};

void register_riptests(Suite *s)
{
  TCase *tc = tcase_create("proxy_unit");
  tcase_addtests(tc, testcases);
  // Must come after tcase_addtests() because that one also adds
  // a fixture that enables memleak tracing. If not, the code in
  // setup() is not traced and you can miss memleaks.
  tcase_add_checked_fixture(tc, setup, teardown);
  suite_add_tcase (s, tc);
}
