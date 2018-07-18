/*
 * Driver for /dev/ncr device (aka NCR)
 *
 * Copyright (c) 2004 Michal Ludvig <mludvig@logix.net.nz>, SuSE Labs
 * Copyright (c) 2009,2010 Nikos Mavrogiannopoulos <nmav@gnutls.org>
 *
 * This file is part of linux cryptodev.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/*
 * Device /dev/ncr provides an interface for 
 * accessing kernel CryptoAPI algorithms (ciphers,
 * hashes) from userspace programs.
 *
 * /dev/ncr interface was originally introduced in
 * OpenBSD and this module attempts to keep the API.
 *
 */

#include <linux/crypto.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/ioctl.h>
#include <linux/random.h>
#include <linux/syscalls.h>
#include <linux/pagemap.h>
#include <linux/uaccess.h>
#include <linux/scatterlist.h>
#include "cryptodev_int.h"
#include "ncr-int.h"
#include <linux/version.h>
#include "version.h"

MODULE_AUTHOR("Nikos Mavrogiannopoulos <nmav@gnutls.org>");
MODULE_DESCRIPTION("NCR driver");
MODULE_LICENSE("GPL");

#ifdef KEY_PERSISTENCE
struct ncr_lists *ncr;
struct ncr_lists *ncr_get_lists(void) { return ncr; }
EXPORT_SYMBOL(ncr_get_lists);
#endif

/* ====== Module parameters ====== */

int cryptodev_verbosity = 0;
#ifndef ENFORCE_SECURITY
module_param(cryptodev_verbosity, int, 0644);
MODULE_PARM_DESC(cryptodev_verbosity, "0: normal, 1: verbose, 2: debug");
#endif

/* ====== CryptoAPI ====== */

void release_user_pages(struct page **pg, int pagecount)
{
	while (pagecount--) {
		//The document on https://www.kernel.org/doc/Documentation/cachetlb.txt indicates that flush_dcache_page() only works for page cache pages but not anonymous page.
		//However, my tests on the current board show that flush_dcache_page() indeed has effects on the result.
		//If this function is not called, sometimes part of the encrypted/decrypted data seen by userspace contains corrupted result, which is filled with zeros.
		flush_dcache_page(pg[pagecount]);
		if (!PageReserved(pg[pagecount]))
			SetPageDirty(pg[pagecount]);
		page_cache_release(pg[pagecount]);
	}
}

/* offset of buf in it's first page */
#define PAGEOFFSET(buf) ((unsigned long)buf & ~PAGE_MASK)

/* fetch the pages addr resides in into pg and initialise sg with them */
int __get_userbuf(uint8_t __user * addr, uint32_t len, int write,
		  int pgcount, struct page **pg, struct scatterlist *sg)
{
	int ret, pglen, i = 0;
	struct scatterlist *sgp;

	down_write(&current->mm->mmap_sem);
	ret = get_user_pages(current, current->mm,
			     (unsigned long)addr, pgcount, write, 0, pg, NULL);
	up_write(&current->mm->mmap_sem);
	if (ret != pgcount)
		return -EINVAL;

	sg_init_table(sg, pgcount);

	pglen =
	    min((ptrdiff_t) (PAGE_SIZE - PAGEOFFSET(addr)), (ptrdiff_t) len);
	sg_set_page(sg, pg[i++], pglen, PAGEOFFSET(addr));

	len -= pglen;
	for (sgp = sg_next(sg); len; sgp = sg_next(sgp)) {
		pglen = min((uint32_t) PAGE_SIZE, len);
		sg_set_page(sgp, pg[i++], pglen, 0);
		len -= pglen;
	}
	sg_mark_end(sg_last(sg, pgcount));
	return 0;
}

/* ====== /dev/ncr ====== */

static int cryptodev_open(struct inode *inode, struct file *filp)
{
#ifndef KEY_PERSISTENCE
	struct ncr_lists *ncr;

	ncr = ncr_init_lists();
	if (ncr == NULL) {
		return -ENOMEM;
	}

	filp->private_data = ncr;
#endif
	return 0;
}

static int cryptodev_release(struct inode *inode, struct file *filp)
{
#ifndef KEY_PERSISTENCE
	void *ncr = filp->private_data;

	if (likely(ncr)) {
		ncr_deinit_lists(ncr);
		filp->private_data = NULL;
	}
	else
		BUG();
#endif

	return 0;
}

static long
cryptodev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
#ifndef KEY_PERSISTENCE
	void *ncr = filp->private_data;
#endif

	if (unlikely(!ncr))
		BUG();

	return ncr_ioctl(ncr, cmd, arg);
}

/* compatibility code for 32bit userlands */
#ifdef CONFIG_COMPAT

static long
cryptodev_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
#ifndef KEY_PERSISTENCE
	void *ncr = file->private_data;
#endif

	if (unlikely(!ncr))
		BUG();

	return ncr_compat_ioctl(ncr, cmd, arg);
}

#endif /* CONFIG_COMPAT */

static const struct file_operations cryptodev_fops = {
	.owner = THIS_MODULE,
	.open = cryptodev_open,
	.release = cryptodev_release,
	.unlocked_ioctl = cryptodev_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = cryptodev_compat_ioctl,
#endif /* CONFIG_COMPAT */
};

static struct miscdevice cryptodev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "ncr",
	.fops = &cryptodev_fops,
};

static int __init cryptodev_register(void)
{
	int rc;

	ncr_limits_init();
	ncr_master_key_reset();
	ncr_master_key_set_internal();

	rc = misc_register(&cryptodev);
	if (unlikely(rc)) {
		ncr_limits_deinit();
		printk(KERN_ERR PFX "registration of /dev/ncr failed\n");
		return rc;
	}

#ifdef KEY_PERSISTENCE
	ncr = ncr_init_lists();
	if (unlikely(ncr == NULL)) {
		misc_deregister(&cryptodev);
		ncr_limits_deinit();
		printk(KERN_ERR PFX "registration of /dev/ncr failed\n");
		return -ENOMEM;
	}
#endif

	return 0;
}

static void __exit cryptodev_deregister(void)
{
	misc_deregister(&cryptodev);
	ncr_limits_deinit();

#ifdef KEY_PERSISTENCE
	if (likely(ncr)) {
		ncr_deinit_lists(ncr);
	}
	else
		BUG();
#endif
}

/* ====== Module init/exit ====== */
static int __init init_ncrmod(void)
{
	int rc;

	rc = cryptodev_register();
	if (unlikely(rc))
		return rc;

	printk(KERN_INFO PFX "driver %s loaded.\n", VERSION);

	return 0;
}

static void __exit exit_ncrmod(void)
{
	cryptodev_deregister();
	printk(KERN_INFO PFX "driver unloaded.\n");
}

module_init(init_ncrmod);
module_exit(exit_ncrmod);
