/********** COPYRIGHT AND CONFIDENTIALITY INFORMATION NOTICE *************
** Copyright (c) 2013-2017 - Technicolor Delivery Technologies, SAS     **
** All Rights Reserved                                                  **
*************************************************************************/

#ifndef __BANKMGR_PROC_H
#define __BANKMGR_PROC_H

#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

static void *bankmgr_proc_data(struct file *file)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
	return PDE(file->f_path.dentry->d_inode)->data;
#else
	return PDE_DATA(file_inode(file));
#endif
}

#define _BANKMGR_SEQ_FOPS(fops_name, _show, _read, _write)             \
static int fops_name##_open(struct inode *inode, struct file *file)     \
{                                                                       \
	return single_open(file, _show, bankmgr_proc_data(file));           \
}                                                                       \
static struct file_operations fops_name = {                             \
	.owner   = THIS_MODULE,                                         \
	.open    = fops_name##_open,                                    \
	.read    = _read,                                               \
	.write   = _write,                                              \
	.release = single_release,                                      \
}

#define BANKMGR_SEQ_FOPS(fops_name, _show, _write) _BANKMGR_SEQ_FOPS(fops_name, _show, seq_read, _write)

struct proc_dir_entry *bankmgr_proc_get_proc_root_dir(void);
int bankmgr_proc_init(void);
int bankmgr_proc_cleanup(void);

#endif /* __BANKMGR_PROC_H */
