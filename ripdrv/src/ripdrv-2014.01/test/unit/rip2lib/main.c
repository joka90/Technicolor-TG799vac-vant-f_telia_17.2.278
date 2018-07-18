
#include <stdlib.h>
#include "unittest/unittest.h"

extern void register_riptests(Suite *s);

static testCaseInfo testcases[] =
{
  { .register_testcase = register_riptests,       .name = "RIP2 unittests" },
  { NULL }
};


static testSuiteInfo testsuite =
{
  .name = "RIP2_unit",
  .desc = "Unit tests for rip2lib",
};

static void init_tcase()
{
}

static void fini_tcase()
{
}

__attribute__((constructor))
void my_init(void)
{
  register_testsuite(&testsuite, testcases, init_tcase, fini_tcase);
}
