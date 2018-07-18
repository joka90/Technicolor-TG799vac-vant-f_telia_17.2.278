/*
 * Generic LED driver for Broadcom platforms using the BCM LED api
 *
 * Copyright (C) 2014 Technicolor <linuxgw@technicolor.com>
 *
 */

#include "board_led_defines.h"

#if defined(BCM_LED_FW)

#include <linux/platform_device.h>
#include <linux/module.h>
#include "bcm_led.h"
#include "tch_led.h"


/* Broadcom defined LEDs */
#define BCM_MAX_LEDS 60

static struct bcm_led bcmled_dsc[BCM_MAX_LEDS] = {
	/* Will be filled in dynamically */
};

static struct bcm_led_platform_data bcmled_pd = { \
	.leds	    = bcmled_dsc
};


/* Aggregated Broadcom LEDs */
#define BCM_AGGR_MAX_LEDS 30
static struct aggreg_led led_agg_dsc[BCM_AGGR_MAX_LEDS] = {
	/* Will be filled in dynamically */
};


static struct aggreg_led_platform_data aled_pd = { \
	.leds	    = led_agg_dsc, \
};


/* Full board LED description */
static struct board board_desc = {
	.name = "Broadcom",
	.gpioleds = NULL,
	.shiftleds = NULL,
	.bcmleds = &bcmled_pd,
	.aggregleds = &aled_pd
};



/**
 * Adds a Broadcom LED if defined in boardparms
 */
static void bcm_led_add_ifavailable(const char * name, enum tch_leds ledId) {
	if (!tch_led_is_available(ledId)) {
		return;
	}

	if (bcmled_pd.num_leds >= BCM_MAX_LEDS) {
		printk("ERROR: Too many BCM LEDs defined\n");
		return;
	}

	bcmled_dsc[bcmled_pd.num_leds].name = name;
	bcmled_dsc[bcmled_pd.num_leds].id = ledId;
	bcmled_pd.num_leds++;
}

/**
 * Adds an aggregated LED if defined in boardparms
 */
static void bcm_aggr_led_add_ifavailable(const char * name, const char * led1, const char * led2, const char * led3) {
	int i;
	int led1_found = 0;
	int led2_found = 0;
	int led3_found = 0;

	for (i = 0; i < bcmled_pd.num_leds; i++) {
		if ((strcmp(led1, bcmled_dsc[i].name) == 0) || !strlen(led1)) {
			led1_found = 1;
		}
		if ((strcmp(led2, bcmled_dsc[i].name) == 0) || !strlen(led2)) {
			led2_found = 1;
		}
		if ((strcmp(led3, bcmled_dsc[i].name) == 0) || !strlen(led3)) {
			led3_found = 1;
		}
	}

	if (!led1_found || !led2_found || !led3_found) {
		return;
	}

	if (aled_pd.num_leds >= BCM_AGGR_MAX_LEDS) {
		printk("ERROR: Too many aggregated BCM LEDs defined\n");
		return;
	}

	led_agg_dsc[aled_pd.num_leds].name = name;
	led_agg_dsc[aled_pd.num_leds].led1 = led1;
	led_agg_dsc[aled_pd.num_leds].led2 = led2;
	led_agg_dsc[aled_pd.num_leds].led3 = led3;

	aled_pd.num_leds++;
}

/**
 * Returns the number of LEDs available
 */
const struct board * bcm_led_get_board_description(void) {
	bcmled_pd.num_leds = 0;
	bcm_led_add_ifavailable("power:green", kLedPowerGreen);
	bcm_led_add_ifavailable("power:red", kLedPowerRed);
	bcm_led_add_ifavailable("power:blue", kLedPowerBlue);
	bcm_led_add_ifavailable("broadband:green", kLedBroadbandGreen);
	bcm_led_add_ifavailable("broadband:red", kLedBroadbandRed);
	bcm_led_add_ifavailable("broadband2:green", kLedBroadband2Green);
	bcm_led_add_ifavailable("broadband2:red", kLedBroadband2Red);
	bcm_led_add_ifavailable("dect:green", kLedDectGreen);
	bcm_led_add_ifavailable("dect:red", kLedDectRed);
	bcm_led_add_ifavailable("dect:blue",kLedDectBlue);
	bcm_led_add_ifavailable("ethernet:green", kLedEthernetGreen);
	bcm_led_add_ifavailable("ethernet:red", kLedEthernetRed);
	bcm_led_add_ifavailable("ethernet:blue", kLedEthernetBlue);
	bcm_led_add_ifavailable("iptv:green", kLedIPTVGreen);
	bcm_led_add_ifavailable("wireless:green", kLedWirelessGreen);
	bcm_led_add_ifavailable("wireless:red", kLedWirelessRed);
	bcm_led_add_ifavailable("wireless:blue", kLedWirelessBlue);
	bcm_led_add_ifavailable("wireless_5g:green", kLedWireless5GHzGreen);
	bcm_led_add_ifavailable("wireless_5g:red", kLedWireless5GHzRed);
	bcm_led_add_ifavailable("internet:green", kLedInternetGreen);
	bcm_led_add_ifavailable("internet:red", kLedInternetRed);
	bcm_led_add_ifavailable("internet:blue", kLedInternetBlue);
	bcm_led_add_ifavailable("voip:green", kLedVoip1Green);
	bcm_led_add_ifavailable("voip:red", kLedVoip1Red);
	bcm_led_add_ifavailable("voip:blue", kLedVoip1Blue);
	bcm_led_add_ifavailable("voip2:green", kLedVoip2Green);
	bcm_led_add_ifavailable("voip2:red", kLedVoip2Red);
	bcm_led_add_ifavailable("wps:green", kLedWPSGreen);
	bcm_led_add_ifavailable("wps:red", kLedWPSRed);
	bcm_led_add_ifavailable("usb:green", kLedUsbGreen);
	bcm_led_add_ifavailable("upgrade:blue", kLedUpgradeBlue);
	bcm_led_add_ifavailable("upgrade:green", kLedUpgradeGreen);
	bcm_led_add_ifavailable("lte:green", kLedLteGreen);
	bcm_led_add_ifavailable("lte:red", kLedLteRed);
	bcm_led_add_ifavailable("lte:blue", kLedLteBlue);
	bcm_led_add_ifavailable("sfp:green", kLedSFPGreen);
	bcm_led_add_ifavailable("moca:green", kLedMoCAGreen);
	bcm_led_add_ifavailable("ambient1:white", kLedAmbient1White);
	bcm_led_add_ifavailable("ambient2:white", kLedAmbient2White);
	bcm_led_add_ifavailable("ambient3:white", kLedAmbient3White);
	bcm_led_add_ifavailable("ambient4:white", kLedAmbient4White);
	bcm_led_add_ifavailable("ambient5:white", kLedAmbient5White);

	aled_pd.num_leds = 0;
	bcm_aggr_led_add_ifavailable("wps:orange", "wps:green", "wps:red", "");
	bcm_aggr_led_add_ifavailable("power:orange", "power:green", "power:red", "");
	bcm_aggr_led_add_ifavailable("ethernet:orange", "ethernet:green", "ethernet:red", "");
	bcm_aggr_led_add_ifavailable("ethernet:white", "ethernet:green", "ethernet:red", "ethernet:blue");
	bcm_aggr_led_add_ifavailable("internet:orange", "internet:green", "internet:red", "");
	bcm_aggr_led_add_ifavailable("internet:magenta", "internet:blue", "internet:red", "");
	bcm_aggr_led_add_ifavailable("internet:white", "internet:green", "internet:red", "internet:blue");
	bcm_aggr_led_add_ifavailable("dect:orange", "dect:green", "dect:red", "");
	bcm_aggr_led_add_ifavailable("dect:white", "dect:green", "dect:red", "dect:blue");
	bcm_aggr_led_add_ifavailable("wireless:orange", "wireless:green", "wireless:red", "");
	bcm_aggr_led_add_ifavailable("wireless:magenta", "wireless:blue", "wireless:red", "");
	bcm_aggr_led_add_ifavailable("wireless_5g:orange", "wireless_5g:green", "wireless_5g:red", "");
	bcm_aggr_led_add_ifavailable("voip:orange", "voip:green", "voip:red", "");
	bcm_aggr_led_add_ifavailable("voip:white", "voip:green", "voip:red", "voip:blue");
	bcm_aggr_led_add_ifavailable("voip2:orange", "voip2:green", "voip2:red", "");
	bcm_aggr_led_add_ifavailable("lte:orange", "lte:green", "lte:red", "");
	bcm_aggr_led_add_ifavailable("lte:magenta", "lte:blue", "lte:red", "");

	return &board_desc;
}



static void bcmled_release(struct device *dev)
{
}

static struct platform_device bcmled_device = {
       .name                   = "bcm-led",
       .id                     = 0,
       .dev = {
		.release = bcmled_release,
	}
};

static void bcm_led_set(struct led_classdev *led_cdev,
                             enum led_brightness value)
{
	struct bcm_led_data *led_dat =
		container_of(led_cdev, struct bcm_led_data, cdev);

	led_dat->brightness = value;
	if (tch_led_set(led_dat->id, value ? 1 : 0 ) != 0) {
		printk("Failed to set led %d value %d\n", led_dat->id, value);
	}
}

static enum led_brightness bcm_led_get(struct led_classdev *led_cdev)
{
	struct bcm_led_data *led_dat =
		container_of(led_cdev, struct bcm_led_data, cdev);

	return led_dat->brightness;
}

static int __devinit bcm_led_probe(struct platform_device *pdev)
{
	int i;
	int err;
	struct bcm_led_data *led;
	struct bcm_led_platform_data *pdata = dev_get_platdata(&pdev->dev);

	led = devm_kzalloc(&pdev->dev, sizeof(struct bcm_led_data) * pdata->num_leds,
	                   GFP_KERNEL);
	if (!led)
		return -ENOMEM;

	dev_set_drvdata(&pdev->dev, led);

	for(i = 0; i < pdata->num_leds; i++) {
		led[i].cdev.name = pdata->leds[i].name;
		led[i].cdev.brightness_set = bcm_led_set;
		led[i].cdev.brightness_get = bcm_led_get;
		led[i].cdev.default_trigger = pdata->leds[i].default_trigger;

		led[i].brightness = LED_OFF;
		led[i].id = pdata->leds[i].id;

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

static int __devexit bcm_led_remove(struct platform_device *pdev)
{
	struct bcm_led_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct bcm_led_data *led = dev_get_drvdata(&pdev->dev);
	int i;

	for(i = 0; i < pdata->num_leds; i++) {
		if (led[i].cdev.name)
			led_classdev_unregister(&led[i].cdev);
	}
	devm_kfree(&pdev->dev, led);
	return 0;
}

static struct platform_driver bcmled_driver = {
	.probe = bcm_led_probe,
	.remove = __devexit_p(bcm_led_remove),
	.driver = {
		.name = "bcm-led",
		.owner = THIS_MODULE,
	},
};


int bcmled_driver_init(void ) {
	int err = 0;
	struct bcm_led_platform_data *bcmdata;

	bcmled_device.dev.platform_data = board_desc.bcmleds;
	bcmdata = dev_get_platdata(&bcmled_device.dev);

	if (!bcmdata || bcmdata->num_leds == 0) {
		/* No LEDs defined, so do not load driver */
		return 0;
	}

	err = platform_driver_register(&bcmled_driver);
	if (err) {
		printk("Failed to register Broadcom led driver\n");
		return err;
	}

	err = platform_device_register(&bcmled_device);
	if (err) {
		printk("Failed to register Broadcom led device\n");
		return err;
	}

	return err;
}

void bcmled_driver_deinit(void ) {
	struct bcm_led_platform_data *bcmdata = dev_get_platdata(&bcmled_device.dev);

	if (bcmdata && bcmdata->num_leds) {
		platform_device_unregister(&bcmled_device);
		platform_driver_unregister(&bcmled_driver);
	}
}

#endif
