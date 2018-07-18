/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
** Copyright (c) 2013-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
*************************************************************************/

#include <linux/module.h>

#include "bankmgr_proc.h"

static int __init bankmgr_init(void)
{
	return bankmgr_proc_init();
};

static void __exit bankmgr_exit(void)
{
	bankmgr_proc_cleanup();
}

module_init(bankmgr_init);
module_exit(bankmgr_exit);
MODULE_LICENSE("GPL");
