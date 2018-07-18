/*
* <:copyright-BRCM:2013:GPL/GPL:standard
* 
*    Copyright (c) 2013 Broadcom Corporation
*    All Rights Reserved
* 
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License, version 2, as published by
* the Free Software Foundation (the "GPL").
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* 
* A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php, or by
* writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA 02111-1307, USA.
* 
* :> 
*/


#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>   /* printk() */
#include <linux/slab.h>     /* kmalloc() */
#include <linux/fs.h>       /* everything... */
#include <linux/errno.h>    /* error codes */
#include <linux/types.h>    /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>    /* O_ACCMODE */
#include <linux/aio.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <linux/delay.h>        /* For delay */

#include "bdmf_system.h"
#include "bdmf_session.h"
#include "bdmf_shell.h"
#include "bdmf_chrdev.h"

int bdmf_chrdev_major = 215; /* Should comply the value in targets/makeDev */
module_param(bdmf_chrdev_major, int, S_IRUSR | S_IRGRP | S_IWGRP);
MODULE_DESCRIPTION("BDMF shell module");
MODULE_AUTHOR("Broadcom");
MODULE_LICENSE("GPL");

#define BDMF_CHRDEV_MAX_SESS      16

/* #define DEBUG */

#ifdef DEBUG
#define dprintk printk
#else
#define dprintk(...)
#endif

struct bdmf_chdev_session
{
    int pid;
    bdmf_session_handle mon_session;
};

static struct bdmf_chdev_session dev_sessions[BDMF_CHRDEV_MAX_SESS];

static bdmf_mutex dev_lock;

/*
 * Session helpers
 */

/* Look for an empty slot in dev_sessions array,
 * and create a new session.
 * Returns unique pid or -EBUSY if no free skots
 */
static int bdmf_chrdev_sess_new(void)
{
    int i;
    int rc = -EBUSY;

    for(i=0; i<BDMF_CHRDEV_MAX_SESS; i++)
    {
        if (!dev_sessions[i].pid)
        {
            bdmf_session_parm_t mon_session_parm;
            memset(&mon_session_parm, 0, sizeof(mon_session_parm));
            mon_session_parm.access_right = BDMF_ACCESS_ADMIN;
            rc = bdmfmon_session_open(&mon_session_parm, &dev_sessions[i].mon_session);
            if (rc)
                return -ENOMEM;
            rc = dev_sessions[i].pid = current->pid;
            break;
        }
    }
    return rc;
}

static bdmf_session_handle bdmf_chrdev_sess_get(int session_id)
{
    int i;
    bdmf_session_handle sess=NULL;

    for(i=0; i<BDMF_CHRDEV_MAX_SESS; i++)
    {
        if (dev_sessions[i].pid == session_id)
        {
            sess = dev_sessions[i].mon_session;
            break;
        }
    }
    if (!sess)
        bdmf_print("BDMF SHELL: session %d is not found\n", session_id);
    return sess;
}

static int bdmf_chrdev_sess_free(int session_id)
{
    int i;

    for(i=0; i<BDMF_CHRDEV_MAX_SESS; i++)
    {
        if (dev_sessions[i].pid && (dev_sessions[i].pid == session_id))
        {
            bdmfmon_session_close(dev_sessions[i].mon_session);
            dev_sessions[i].pid = 0;
            return 0;
        }
    }

    bdmf_print("BDMF SHELL: session %d is not found\n", session_id);
    return -ENOENT;
}


/*
 * Open and close
 */
int bdmf_chrdev_open(struct inode *inode, struct file *filp)
{
    dprintk("%s:%d\n", __FUNCTION__, __LINE__);
    return 0;
}


int bdmf_chrdev_release(struct inode *inode, struct file *filp)
{
    dprintk("%s:%d\n", __FUNCTION__, __LINE__);
    return 0;
}

ssize_t bdmf_chrdev_write(struct file *filp, const char __user *buf,
    size_t count, loff_t *f_pos)
{
    dprintk("%s:%d\n", __FUNCTION__, __LINE__);
    return -EOPNOTSUPP;
}

long bdmf_chrdev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct io_param *shell_io_param;
    int rc = 0;

    dprintk("%s:%d cmd=0x%x\n", __FUNCTION__, __LINE__, cmd);

    /* don't even decode wrong cmds: better returning  ENOTTY than EFAULT */
    if (_IOC_TYPE(cmd) != BDMF_CHRDEV_IOC_MAGIC)
        return -ENOTTY;

    if (_IOC_NR(cmd) > BDMF_CHRDEV_MAX_NR)
        return -ENOTTY;

    /*
     * the type is a bitmask, and VERIFY_WRITE catches R/W
     * transfers. Note that the type is user-oriented, while
     * verify_area is kernel-oriented, so the concept of "read" and
     * "write" is reversed
     */
    if (_IOC_DIR(cmd) & _IOC_READ)
        rc = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        rc =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    if (rc)
        return -EFAULT;

    shell_io_param = kmalloc(sizeof(*shell_io_param), GFP_KERNEL);
    if (!shell_io_param)
        return -ENOMEM;

    bdmf_mutex_lock(&dev_lock);

    switch(cmd) {

    case BDMF_CHRDEV_SESSION_INIT:
    {
        shell_io_param->rc = 0;
        shell_io_param->session_id = bdmf_chrdev_sess_new();
        rc = copy_to_user((char *)arg, (char *)shell_io_param, sizeof(*shell_io_param)-
            sizeof(shell_io_param->command));
    }
    break;

    case BDMF_CHRDEV_SESSION_SEND: /* Tell: arg is the value */
    {
        bdmf_session_handle sess;
        rc = copy_from_user((char *)shell_io_param, (char *)arg, sizeof(*shell_io_param));
        if (rc < 0)
            return rc;
        sess = bdmf_chrdev_sess_get(shell_io_param->session_id);
        if (!sess)
            return -ENOENT;
        dprintk("%s:%d cmd=<%s>", __FUNCTION__, __LINE__, shell_io_param->command);
        shell_io_param->rc = bdmfmon_parse(sess, shell_io_param->command);
        rc = copy_to_user((char *)arg, (char *)shell_io_param, sizeof(*shell_io_param)-
            sizeof(shell_io_param->command));
        dprintk("%s:%d rc=%d io_rc=%d\n", __FUNCTION__, __LINE__, shell_io_param->rc, rc);
    }
    break;

    case BDMF_CHRDEV_SESSION_CLOSE:
    {
        rc = copy_from_user((char *)shell_io_param, (char *)arg, sizeof(*shell_io_param)-
            sizeof(shell_io_param->command));
        rc = rc ? rc : bdmf_chrdev_sess_free(shell_io_param->session_id);
    }
    break;

    default:  /* redundant, as cmd was checked against MAXNR */
        rc = -ENOTTY;
        break;
    }
    dprintk("%s:%d rc=%d\n", __FUNCTION__, __LINE__, rc);
    bdmf_mutex_unlock(&dev_lock);
    kfree(shell_io_param);

    return rc;
}


/*
 * The fops
 */
struct file_operations bdmf_chrdev_fops = {
    .owner = THIS_MODULE,
    .open = bdmf_chrdev_open,
    .release = bdmf_chrdev_release,
    .write = bdmf_chrdev_write,
    .unlocked_ioctl = bdmf_chrdev_ioctl
};

static struct cdev bdmf_chrdev_cdev;
static int is_chrdev_reg, is_cdev_add;

int bdmf_chrdev_init(void)
{
    dev_t dev = MKDEV(bdmf_chrdev_major, 0);
    int rc;

    is_chrdev_reg = 0;
    is_cdev_add = 0;
    /*
     * Register your major, and accept a dynamic number.
     */
    if (!bdmf_chrdev_major)
        return -1;
    rc = register_chrdev_region(dev, 0, "bdmf_shell");
    if (rc < 0)
        return rc;
    is_chrdev_reg = 1;

    bdmf_mutex_init(&dev_lock);
    cdev_init(&bdmf_chrdev_cdev, &bdmf_chrdev_fops);
    rc = cdev_add(&bdmf_chrdev_cdev, dev, 1);
    if (rc < 0)
        return rc;
    is_cdev_add = 1;

    return 0;
}


void bdmf_chrdev_exit(void)
{
    if (is_cdev_add)
        cdev_del(&bdmf_chrdev_cdev);
    if (is_chrdev_reg)
        unregister_chrdev_region(MKDEV(bdmf_chrdev_major, 0), 1);
}

