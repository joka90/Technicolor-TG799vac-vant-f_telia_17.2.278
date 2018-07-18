/*
 * Generic LED driver for Technicolor Linux Gateway platforms.
 *
 * Copyright (C) 2017 Technicolor <linuxgw@technicolor.com>
 *
 */

#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/time.h>
#include <linux/workqueue.h>
#include <linux/jiffies.h>

#include "leds.h"
#include "board_led_defines.h"
#include "bcm_led.h"

struct shiftled_work_struct {
	struct work_struct work;
	unsigned mask;
	struct shiftled_led_platform_data *pdata;
};

static void led_release(struct device *dev);

static struct platform_device gpled_device = {
	.name = "leds-gpio",
	.id = 0,
	.dev = {
		.release = led_release
	}
};

static struct platform_device shiftled_device = {
	.name = "shiftled-led",
	.id = 0,
	.dev = {
		.release = led_release
	}
};

static void led_release(struct device *dev)
{
	(void)dev;
	/* really nothing to be done, just avoid nagging when removing the module */
}

static void led_load_shift_reg(struct work_struct *pwork)
{
	int i;
	struct shiftled_work_struct *swork = container_of(pwork, struct shiftled_work_struct, work);
	for (i = 0; i < swork->pdata->reg_size; i++) { /* LSB first */
		gpio_set_value(swork->pdata->reg_clk, 0);
		udelay(1);
		gpio_set_value(swork->pdata->reg_data, (swork->mask >> i) & 1);
		udelay(1);
		gpio_set_value(swork->pdata->reg_clk, 1);
		udelay(2);
	}
	gpio_set_value(swork->pdata->reg_rck, 1);
	udelay(2);
	gpio_set_value(swork->pdata->reg_rck, 0);
	kfree(swork);
}

static int shiftled_update(struct shiftled_led_platform_data *pdata) {
	static unsigned long last_mask = ULONG_MAX;
	if(!pdata || !pdata->shift_work_queue) {
		return -1;
	}
	if (last_mask != pdata->mask) {
		struct shiftled_work_struct *shift_work = kmalloc(sizeof(struct shiftled_work_struct), GFP_KERNEL);
		if (!shift_work) {
			return -ENOMEM;
		}
		INIT_WORK(&shift_work->work, led_load_shift_reg);
		// Make a copy of mask so we don't need to lock things
		shift_work->mask = pdata->mask;
		shift_work->pdata = pdata;
		if(queue_work(pdata->shift_work_queue, &shift_work->work) == 0) {
			printk(KERN_INFO "LED Error adding work to workqueue\n");
			kfree(shift_work);
			return -1;
		}
		last_mask = pdata->mask;
	}
	return 0;
}

static void shiftled_led_set(struct led_classdev *led_cdev,
							 enum led_brightness value)
{
	struct shiftled_led_data *led_dat =
		container_of(led_cdev, struct shiftled_led_data, cdev);
	struct shiftled_led_platform_data *pdata = dev_get_platdata(&shiftled_device.dev);

	led_dat->brightness = value;

	if (led_dat->active_high ^ (value == LED_OFF))
		pdata->mask |= led_dat->bit;
	else
		pdata->mask &= ~led_dat->bit;

	shiftled_update(pdata);
}

static enum led_brightness shiftled_led_get(struct led_classdev *led_cdev)
{
	struct shiftled_led_data *led_dat =
		container_of(led_cdev, struct shiftled_led_data, cdev);

	return led_dat->brightness;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0)
static int __devinit shiftled_led_probe(struct platform_device *pdev)
#else
static int shiftled_led_probe(struct platform_device *pdev)
#endif
{
	int i;
	int err;
	struct shiftled_led_data *led;
	struct shiftled_led_platform_data *pdata = dev_get_platdata(&pdev->dev);

	led = devm_kzalloc(&pdev->dev, sizeof(struct shiftled_led_data) * pdata->num_leds,
					   GFP_KERNEL);
	if (!led)
		return -ENOMEM;

	err = gpio_request(pdata->reg_rck, "rck");
	if (err)
		goto free_led_data;

	err = gpio_request(pdata->reg_clk, "clk");
	if (err)
		goto free_sr_rck;

	err = gpio_request(pdata->reg_data, "data");
	if (err)
		goto free_sr_clk;

	gpio_direction_output(pdata->reg_rck, 1);
	gpio_direction_output(pdata->reg_clk, 1);
	gpio_direction_output(pdata->reg_data, 1);

	dev_set_drvdata(&pdev->dev, led);
	for(i = 0; i < pdata->num_leds; i++) {
		led[i].cdev.name = pdata->leds[i].name;
		led[i].cdev.brightness_set = shiftled_led_set;
		led[i].cdev.brightness_get = shiftled_led_get;
		led[i].cdev.default_trigger = pdata->leds[i].default_trigger;

		led[i].bit = 1 << pdata->leds[i].bit;
		led[i].active_high = pdata->leds[i].active_high;
		led[i].brightness = LED_OFF;

		if (led[i].active_high == 1)
			pdata->mask &= ~led[i].bit;
		else
			pdata->mask |= led[i].bit;
	}

	err = shiftled_update(pdata);
	if (err)
		goto free_sr_data;

	for(i = 0; i < pdata->num_leds; i++) {
		err = led_classdev_register(&pdev->dev, &led[i].cdev);
		if (err)
			goto free_leds;
	}
	return 0;

free_leds:
	while( --i >= 0)
		led_classdev_unregister(&led[i].cdev);

free_sr_data:
	gpio_free(pdata->reg_data);

free_sr_clk:
	gpio_free(pdata->reg_clk);

free_sr_rck:
	gpio_free(pdata->reg_rck);

free_led_data:
	devm_kfree(&pdev->dev, led);
	return err;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0)
static int __devexit shiftled_led_remove(struct platform_device *pdev)
#else
static int shiftled_led_remove(struct platform_device *pdev)
#endif
{
	struct shiftled_led_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct shiftled_led_data *led = dev_get_drvdata(&pdev->dev);
	int i;

	for(i = 0; i < pdata->num_leds; i++) {
		if (led[i].cdev.name)
			led_classdev_unregister(&led[i].cdev);
	}

	if (pdata->shift_work_queue) {
		flush_workqueue(pdata->shift_work_queue);
		destroy_workqueue(pdata->shift_work_queue);
		pdata->shift_work_queue = NULL;
	}

	devm_kfree(&pdev->dev, led);
	gpio_free(pdata->reg_data);
	gpio_free(pdata->reg_clk);
	gpio_free(pdata->reg_rck);
	return 0;
}

static struct platform_driver shiftled_driver = {
	.probe = shiftled_led_probe,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0)
	.remove = __devexit_p(shiftled_led_remove),
#else
	.remove = shiftled_led_remove,
#endif

	.driver = {
		.name = "shiftled-led",
		.owner = THIS_MODULE,
	},
};


static void aled_release(struct device *dev)
{
}

static struct platform_device aggregled_device = {
	.name                   = "aggreg-led",
	.id                     = 0,
	.dev = {
		.release = aled_release
	}
};

static void aggreg_led_set(struct led_classdev *led_cdev,
							 enum led_brightness value)
{
	struct aggreg_led_data *led_dat =
		container_of(led_cdev, struct aggreg_led_data, cdev);

	led_dat->brightness = value;
	if (led_dat->led1) {
		led_set_brightness(led_dat->led1, value);
	}
	if (led_dat->led2) {
		led_set_brightness(led_dat->led2, value);
	}
	if (led_dat->led3) {
		led_set_brightness(led_dat->led3, value);
	}
}

static enum led_brightness aggreg_led_get(struct led_classdev *led_cdev)
{
	struct aggreg_led_data *led_dat =
		container_of(led_cdev, struct aggreg_led_data, cdev);

	return led_dat->brightness;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0)
static int __devinit aggreg_led_probe(struct platform_device *pdev)
#else
static int aggreg_led_probe(struct platform_device *pdev)
#endif
{
	int i;
	int err;
	struct led_classdev *led_cdev;
	struct aggreg_led_data *led;
	struct aggreg_led_platform_data *pdata = dev_get_platdata(&pdev->dev);

	led = devm_kzalloc(&pdev->dev, sizeof(struct aggreg_led_data) * pdata->num_leds,
						GFP_KERNEL);
	if (!led)
		return -ENOMEM;

	dev_set_drvdata(&pdev->dev, led);

	for(i = 0; i < pdata->num_leds; i++) {
		led[i].cdev.name = pdata->leds[i].name;
		led[i].cdev.brightness_set = aggreg_led_set;
		led[i].cdev.brightness_get = aggreg_led_get;
		led[i].cdev.default_trigger = pdata->leds[i].default_trigger;

		led[i].brightness = LED_OFF;

		/* Find the LED with the correct name */
		down_read(&leds_list_lock);
		list_for_each_entry(led_cdev, &leds_list, node) {
			down_write(&led_cdev->trigger_lock);
			if (!strcmp(pdata->leds[i].led1, led_cdev->name)) {
				led[i].led1 = led_cdev;
			}
			if (!strcmp(pdata->leds[i].led2, led_cdev->name)) {
				led[i].led2 = led_cdev;
			}
			if (!strcmp(pdata->leds[i].led3, led_cdev->name)) {
				led[i].led3 = led_cdev;
			}
			up_write(&led_cdev->trigger_lock);
		 }
		 up_read(&leds_list_lock);

		if((!led[i].led1) && strlen(pdata->leds[i].led1)) {
			printk(KERN_WARNING "Could not find led1(%s) for aggr LED(%s)\n", pdata->leds[i].led1, pdata->leds[i].name);
		}
		if((!led[i].led2) && strlen(pdata->leds[i].led2)) {
			printk(KERN_WARNING "Could not find led2(%s) for aggr LED(%s)\n", pdata->leds[i].led2, pdata->leds[i].name);
		}
		if((!led[i].led3) && strlen(pdata->leds[i].led3)) {
			printk(KERN_WARNING "Could not find led3(%s) for aggr LED(%s)\n", pdata->leds[i].led3, pdata->leds[i].name);
		}
	}

	for(i = 0; i < pdata->num_leds; i++) {
		err = led_classdev_register(&pdev->dev, &led[i].cdev);
		if (err)
			goto free_leds;
	}
	return 0;

free_leds:
	while( --i >= 0)
		led_classdev_unregister(&led[i].cdev);

	devm_kfree(&pdev->dev, led);
	return err;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0)
static int __devexit aggreg_led_remove(struct platform_device *pdev)
#else
static int aggreg_led_remove(struct platform_device *pdev)
#endif
{
	struct aggreg_led_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct aggreg_led_data *led = dev_get_drvdata(&pdev->dev);
	int i;

	for(i = 0; i < pdata->num_leds; i++) {
		if (led[i].cdev.name)
			led_classdev_unregister(&led[i].cdev);
	}
	devm_kfree(&pdev->dev, led);
	return 0;
}

static struct platform_driver aggregled_driver = {
	.probe = aggreg_led_probe,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0)
	.remove = __devexit_p(aggreg_led_remove),
#else
	.remove = aggreg_led_remove,
#endif
	.driver = {
		.name = "aggreg-led",
		.owner = THIS_MODULE,
	},
};

/* Prototypes. */
static int __init led_init(void);
static void __exit led_exit(void);

/***************************************************************************
 * Function Name: led_init
 * Description  : Initial function that is called when the module is loaded
 * Returns      : None.
 ***************************************************************************/
static int __init led_init(void)
{
	int err;
	/* The build process for these boards does not yet fill in tch_board */
#if defined(BOARD_GANTN)
	const struct board * board_desc = get_board_description("GANT-N");
#elif defined(BOARD_C2KEVM)
	const struct board * board_desc = get_board_description("C2KEVM");
#elif defined (BOARD_GANTV)
	const struct board * board_desc = get_board_description("GANT-V");
#elif defined (BOARD_GANTW)
	const struct board * board_desc = get_board_description("GANT-W");
#elif defined (BOARD_GANT5)
	const struct board * board_desc = get_board_description("GANT-5");
#elif defined (BCM_LED_FW)
	const struct board * board_desc = bcm_led_get_board_description();
#elif defined (BOARD_GBNTD)
	const struct board * board_desc = get_board_description("GBNT-D");
#else
	const struct board * board_desc = get_board_description(tch_board);
#endif
	struct gpio_led_platform_data *gpled_data;
	struct shiftled_led_platform_data *pdata;
	struct aggreg_led_platform_data *adata;

	if (!board_desc) {
		printk("Could not find LED description for platform: %s\n", tch_board);
		return -1;
	}
	gpled_device.dev.platform_data = board_desc->gpioleds;
	shiftled_device.dev.platform_data = board_desc->shiftleds;
	aggregled_device.dev.platform_data = board_desc->aggregleds;

	gpled_data = dev_get_platdata(&gpled_device.dev);
	pdata = dev_get_platdata(&shiftled_device.dev);
	if(pdata) {
		pdata->shift_work_queue = create_singlethread_workqueue("led_shift_queue");
		if (!pdata->shift_work_queue) {
			printk("Failed to create shiftled workqueue\n");
			return -1;
		}
		pdata->mask = 0;
	}
	adata = dev_get_platdata(&aggregled_device.dev);

	do {
		if(gpled_data && gpled_data->num_leds > 0) {
			err = platform_device_register(&gpled_device);
			if (err) {
				printk("Failed to register GPIO led device\n");
				break;
			}
		}

		if (pdata && pdata->num_leds) {
			err = platform_driver_register(&shiftled_driver);
			if (err) {
				printk("Failed to register shiftled driver\n");
				break;
			}

			err = platform_device_register(&shiftled_device);
			if (err) {
				printk("Failed to register shiftled device\n");
				break;
			}
		}

#if defined(BCM_LED_FW)
		err = bcmled_driver_init();
		if (err) {
			break;
		}
#endif
		if (adata && adata->num_leds) {
			err = platform_driver_register(&aggregled_driver);
			if (err) {
				printk("Failed to register aggregated led driver\n");
				break;
			}

			err = platform_device_register(&aggregled_device);
			if (err) {
				printk("Failed to register aggregated led device\n");
				break;
			}
		}
	} while (0);

	return err;
}

/***************************************************************************
 * Function Name: led_exit
 * Description  : Final function that is called when the module is unloaded
 * Returns      : None.
 ***************************************************************************/
static void __exit led_exit()
{
	struct shiftled_led_platform_data *pdata = dev_get_platdata(&shiftled_device.dev);
	struct aggreg_led_platform_data *adata = dev_get_platdata(&aggregled_device.dev);
	struct gpio_led_platform_data *gpled_data = dev_get_platdata(&gpled_device.dev);

	if (adata && adata->num_leds) {
		platform_device_unregister(&aggregled_device);
		platform_driver_unregister(&aggregled_driver);
	}

	if (gpled_data && gpled_data->num_leds) {
		platform_device_unregister(&gpled_device);
	}

	if(pdata && pdata->num_leds) {
		platform_device_unregister(&shiftled_device);
		platform_driver_unregister(&shiftled_driver);
	}

#if defined(BCM_LED_FW)
	bcmled_driver_deinit();
#endif
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Generic LED support for Technicolor Linux Gateways");
MODULE_AUTHOR("Technicolor <linuxgw@technicolor.com");
