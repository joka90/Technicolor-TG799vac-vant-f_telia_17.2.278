#ifndef __RIP_PROC_H
#define __RIP_PROC_H

#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>

#include "rip2.h"

#define RIP_DIR_NAME "rip"
#define RIP_NAME_SZ	 16			/* includes '\0' */
#define RIP_BUF_SZ	 (RIP_NAME_SZ + 1)	/* account for extra \n that is usually added by echo */

struct rip_proc_list {
	struct list_head list;
	struct proc_dir_entry *entry;
	T_RIP2_ID id;
	uint32_t len;
	char name[RIP_NAME_SZ];
};

struct rip_cache {
	T_RIP2_ID	id;
	uint32_t	len;
	char *		data;
	struct mutex	lock;
};

int rip_proc_init(void);
int rip_proc_cleanup(void);

#endif /* __RIP_PROC_H */
