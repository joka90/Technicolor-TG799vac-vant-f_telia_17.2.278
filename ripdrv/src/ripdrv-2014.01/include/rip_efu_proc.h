#ifndef __RIP_EFU_PROC_H
#define __RIP_EFU_PROC_H

#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>

#include "rip2.h"

int rip_efu_proc_init(void);
int rip_efu_proc_cleanup(void);

#endif /* __RIP_EFU_PROC_H */
