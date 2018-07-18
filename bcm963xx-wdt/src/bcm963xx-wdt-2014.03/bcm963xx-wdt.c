/*
 *  Broadcom BCM963xx SoC watchdog driver
 *
 *  Copyright (C) 2007, Miguel Gaio <miguel.gaio@efixo.com>
 *  Copyright (C) 2008, Florian Fainelli <florian@openwrt.org>
 *  Copyright (C) 2013, Karl Vogel <karl.vogel@technicolor.com>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 */

#include <linux/bitops.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/watchdog.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/interrupt.h>
#include <linux/ptrace.h>
#include <linux/resource.h>
#include <linux/platform_device.h>

#if defined(CONFIG_BCM963268)
#include "63268_common.h"
#include "63268_map.h"
#elif defined(CONFIG_BCM96362)
#include "6362_common.h"
#include "6362_map.h"
#elif defined(CONFIG_BCM96318)
#include "6318_common.h"
#include "6318_map.h"
#elif defined(CONFIG_BCM96838)
#include "6838_common.h"
#include "6838_map.h"
#elif defined(CONFIG_BCM963138)
#include "63138_common.h"
#include "63138_map.h"
#elif defined(CONFIG_BCM963381)
#include "63381_common.h"
#include "63381_map.h"
#elif defined(CONFIG_BCM968500)
#include "bl_lilac_wd.h"
#else
#error "Unknown BCM963xx platform"
#endif
#include "bcm_hwdefs.h"


#define WDT_DEFAULT_TIME	30      /* seconds */
#define WDT_MAX_TIME		256     /* seconds */

static struct {
	struct timer_list timer;
	unsigned long inuse;
	atomic_t ticks;
} bcm963xx_wdt_device;

static int expect_close;

static int wdt_time = WDT_DEFAULT_TIME;
static bool nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started (default="
	__MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

static void bcm963xx_wdt_hw_start(void)
{
	/* Timeout takes into account:
	   - Printing of kernel crash dumps (>2 seconds)
	   - Printing stack dump when wdt not kicked fast enough
	   - [GPON-10949]: Need enough time to print vlan table
	   - 63138: xtmrt_runner contains busy wait (up till 1 s/queue)
	*/
	unsigned int timeUs = 17000000 * (FPERIPH/1000000);

#if defined(CONFIG_BCM96838)
#if defined(BCMRELEASE_414L04A)
	WATCHDOG->WD0DefCount = timeUs;
	WATCHDOG->WD0Ctl = 0xFF00;
	WATCHDOG->WD0Ctl = 0x00FF;
#else
	WDTIMER->WD0DefCount = timeUs;
	WDTIMER->WD0Ctl = 0xFF00;
	WDTIMER->WD0Ctl = 0x00FF;
#endif
#elif defined(CONFIG_BCM968500)
	bl_lilac_wd_start(timeUs);
#else
	TIMER->WatchDogDefCount = timeUs;
	TIMER->WatchDogCtl = 0xFF00;
	TIMER->WatchDogCtl = 0x00FF;
#endif
}

static void bcm963xx_wdt_hw_stop(void)
{
#if defined(CONFIG_BCM96838)
#if defined(BCMRELEASE_414L04A)
	WATCHDOG->WD0Ctl = 0xEE00;
	WATCHDOG->WD0Ctl = 0x00EE;
#else
	WDTIMER->WD0Ctl = 0xEE00;
	WDTIMER->WD0Ctl = 0x00EE;
#endif
#elif defined(CONFIG_BCM968500)
	bl_lilac_wd_stop();
#else
	TIMER->WatchDogCtl = 0xEE00;
	TIMER->WatchDogCtl = 0x00EE;
#endif
}

static void bcm963xx_timer_tick(unsigned long unused)
{
	bcm963xx_wdt_hw_start();

	if (!atomic_dec_and_test(&bcm963xx_wdt_device.ticks)) {
		mod_timer(&bcm963xx_wdt_device.timer, jiffies + HZ);
	} else {
		pr_crit("watchdog will restart system\n");
		BUG();
	}
}

static void bcm963xx_wdt_pet(void)
{
	atomic_set(&bcm963xx_wdt_device.ticks, wdt_time);
}

static void bcm963xx_wdt_start(void)
{
	bcm963xx_wdt_pet();
	bcm963xx_timer_tick(0);
}

static void bcm963xx_wdt_pause(void)
{
	del_timer_sync(&bcm963xx_wdt_device.timer);
	bcm963xx_wdt_hw_stop();
}

static int bcm963xx_wdt_settimeout(int new_time)
{
	if ((new_time <= 0) || (new_time > WDT_MAX_TIME))
		return -EINVAL;

	wdt_time = new_time;
	return 0;
}

static int bcm963xx_wdt_open(struct inode *inode, struct file *file)
{
	if (test_and_set_bit(0, &bcm963xx_wdt_device.inuse))
		return -EBUSY;

	bcm963xx_wdt_start();
	return nonseekable_open(inode, file);
}

static int bcm963xx_wdt_release(struct inode *inode, struct file *file)
{
	if (expect_close == 42)
		bcm963xx_wdt_pause();
	else {
		pr_crit("Unexpected close, not stopping watchdog!\n");
		bcm963xx_wdt_start();
	}
	clear_bit(0, &bcm963xx_wdt_device.inuse);
	expect_close = 0;
	return 0;
}

static ssize_t bcm963xx_wdt_write(struct file *file, const char *data,
				size_t len, loff_t *ppos)
{
	if (len) {
		if (!nowayout) {
			size_t i;

			/* In case it was set long ago */
			expect_close = 0;

			for (i = 0; i != len; i++) {
				char c;
				if (get_user(c, data + i))
					return -EFAULT;
				if (c == 'V')
					expect_close = 42;
			}
		}
		bcm963xx_wdt_pet();
	}
	return len;
}

static struct watchdog_info bcm963xx_wdt_info = {
	.identity       = KBUILD_MODNAME,
	.options        = WDIOF_SETTIMEOUT |
				WDIOF_KEEPALIVEPING |
				WDIOF_MAGICCLOSE,
};


static long bcm963xx_wdt_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int __user *p = argp;
	int new_value, retval = -EINVAL;

	switch (cmd) {
	case WDIOC_GETSUPPORT:
		return copy_to_user(argp, &bcm963xx_wdt_info,
			sizeof(bcm963xx_wdt_info)) ? -EFAULT : 0;

	case WDIOC_GETSTATUS:
	case WDIOC_GETBOOTSTATUS:
		return put_user(0, p);

	case WDIOC_SETOPTIONS:
		if (get_user(new_value, p))
			return -EFAULT;

		if (new_value & WDIOS_DISABLECARD) {
			bcm963xx_wdt_pause();
			retval = 0;
		}
		if (new_value & WDIOS_ENABLECARD) {
			bcm963xx_wdt_start();
			retval = 0;
		}

		return retval;

	case WDIOC_KEEPALIVE:
		bcm963xx_wdt_pet();
		return 0;

	case WDIOC_SETTIMEOUT:
		if (get_user(new_value, p))
			return -EFAULT;

		if (bcm963xx_wdt_settimeout(new_value))
			return -EINVAL;

		bcm963xx_wdt_pet();

	case WDIOC_GETTIMEOUT:
		return put_user(wdt_time, p);

	default:
		return -ENOTTY;

	}
}

static const struct file_operations bcm963xx_wdt_fops = {
	.owner		= THIS_MODULE,
	.llseek	= no_llseek,
	.write		= bcm963xx_wdt_write,
	.unlocked_ioctl	= bcm963xx_wdt_ioctl,
	.open		= bcm963xx_wdt_open,
	.release	= bcm963xx_wdt_release,
};

static struct miscdevice bcm963xx_wdt_miscdev = {
	.minor	= WATCHDOG_MINOR,
	.name	= "watchdog",
	.fops	= &bcm963xx_wdt_fops,
};

static int __init bcm963xx_wdt_init(void)
{
	int ret;

	setup_timer(&bcm963xx_wdt_device.timer, bcm963xx_timer_tick, 0L);

	ret = misc_register(&bcm963xx_wdt_miscdev);
	if (ret < 0) {
		printk("failed to register watchdog device\n");
		goto out;
	}
	ret = 0;

out:
	return ret;
}

static void __exit bcm963xx_wdt_exit(void)
{
	if (!nowayout)
		bcm963xx_wdt_pause();

	misc_deregister(&bcm963xx_wdt_miscdev);
}


module_init(bcm963xx_wdt_init);
module_exit(bcm963xx_wdt_exit);

MODULE_AUTHOR("Miguel Gaio <miguel.gaio@efixo.com>");
MODULE_AUTHOR("Florian Fainelli <florian@openwrt.org>");
MODULE_AUTHOR("Karl Vogel <karl.vogel@technicolor.com>");
MODULE_DESCRIPTION("Driver for the Broadcom Bcm963xx SoC watchdog");
MODULE_LICENSE("GPL");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);
MODULE_ALIAS("platform:bcm963xx-wdt");
