/*
 * Defines the board led defines for each Technicolor board
 *
 * Copyright (C) 2013 Technicolor <linuxgw@technicolor.com>
 *
 */

#include <linux/string.h>
#include "board_led_defines.h"


#define LED_FAMILY_WITH_SR(family, rck, clk, data, size) \
static struct gpio_led_platform_data gpled_data_##family = { \
	.leds = gpled_##family, \
	.num_leds = ARRAY_SIZE(gpled_##family), \
}; \
static struct shiftled_led_platform_data shiftled_pd_##family = { \
	.num_leds   = ARRAY_SIZE(led_inf_dsc_##family), \
	.leds	    = led_inf_dsc_##family, \
	.reg_rck    = rck, \
	.reg_clk    = clk, \
	.reg_data   = data, \
	.reg_size   = size, \
	.mask = 0, \
	.shift_work_queue = NULL \
}; \
static struct aggreg_led_platform_data aled_pd_##family = { \
	.num_leds   = ARRAY_SIZE(led_agg_dsc_##family), \
	.leds	    = led_agg_dsc_##family, \
}

#define LED_FAMILY_WITHOUT_SR(family) LED_FAMILY_WITH_SR(family, 0, 0, 0, 0)

#define LED_BOARD(family, boardname, ripname) \
static struct board board_##boardname = { \
	.name = ripname, \
	.gpioleds = &gpled_data_##family, \
	.shiftleds = &shiftled_pd_##family, \
	.aggregleds = &aled_pd_##family, \
}


#if defined(CONFIG_BCM963268)

/**
 * VBNT-R
 */

/* GPIO leds */
static struct gpio_led gpled_vbntr[] = {
	{
		.name = "mobile:red",
		.gpio = 0,
		.active_low = 1,
	},
	{
		.name = "power:green",
		.gpio = 1,
		.active_low = 1,
	},
	{
		.name = "power:red",
		.gpio = 8,
		.active_low = 1,
	},
	{
		.name = "mobile:green",
		.gpio = 9,
		.active_low = 1,
	},
	{
		.name = "internet:red",
		.gpio = 14,
		.active_low = 1,
	},
	{
		.name = "internet:green",
		.gpio = 35,
		.active_low = 1,
	},
	{
		.name = "wireless:red",
		.gpio = 36,
		.active_low = 1,
	},
	{
		.name = "wireless:green",
		.gpio = 37,
		.active_low = 1,
	}
};

static struct shiftled_led led_inf_dsc_vbntr[] = {

};
static struct aggreg_led led_agg_dsc_vbntr[] = {
    AGGREG_ORANGE("power")
    AGGREG_ORANGE("mobile")
    AGGREG_ORANGE("internet")
    AGGREG_ORANGE("wireless")
};
LED_FAMILY_WITHOUT_SR(vbntr);
LED_BOARD(vbntr, vbntr, "VBNT-R");

/**
 * VDNT-W
 */

/* GPIO leds */
static struct gpio_led gpled_vdntw[] = {
	{
		.name = "wps:green",
		.gpio = 0,
		.active_low = 1,
	},
	{
		.name = "ethernet:green",
		.gpio = 13,
		.active_low = 0,
	},
	{
		.name = "usb:green",
		.gpio = 14,
		.active_low = 1,
	},
	{
		.name = "power:green",
		.gpio = 15,
		.active_low = 1,
	},
	{
		.name = "iptv:green",
		.gpio = 16,
		.active_low = 0,
	},
	{
		.name = "internet:red",
		.gpio = 17,
		.active_low = 1,
	},
	{
		.name = "power:red",
		.gpio = 18,
		.active_low = 1,
	},
	{
		.name = "internet:green",
		.gpio = 19,
		.active_low = 1,
	},
	{
		.name = "voip:green",
		.gpio = 20,
		.active_low = 1,
	},
	{
		.name = "broadband:green",
		.gpio = 21,
		.active_low = 0,
	},
	{
		.name = "wps:red",
		.gpio = 22,
		.active_low = 1,
	},
	{
		.name = "eco:blue",
		.gpio = 23,
		.active_low = 0,
	},
	{
		.name = "wireless:red",
		.gpio = 36,
		.active_low = 1,
	},
	{
		.name = "wireless:green",
		.gpio = 37,
		.active_low = 1,
	},
	{
		.name = "upgrade:blue",
		.gpio = 38,
		.active_low = 0,
	}
};

static struct shiftled_led led_inf_dsc_vdntw[] = {

};
static struct aggreg_led led_agg_dsc_vdntw[] = {
    AGGREG_ORANGE("power")
    AGGREG_ORANGE("internet")
    AGGREG_ORANGE("wireless")
    AGGREG_ORANGE("wps")
};
LED_FAMILY_WITHOUT_SR(vdntw);
LED_BOARD(vdntw, vdntw, "VDNT-W");

/**
 * VDNT-O
 */

/* GPIO leds */
static struct gpio_led gpled_vdnto[] = {
	{
		.name = "power:red",
		.gpio = 38,
		.active_low = 1,
	},
	{
		.name = "power:green",
		.gpio = 39,
		.active_low = 1,
	},
	{
		.name = "dect:green",
		.gpio = 15,
		.active_low = 1,
	},
	{
		.name = "dect:red",
		.gpio = 19,
		.active_low = 1,
	},
	{
		.name = "iptv:green",
		.gpio = 35,
		.active_low = 1,
	},
	{
		.name = "voip:green",
		.gpio = 9,
		.active_low = 1,
	},
};


/* Shift register leds */
static struct shiftled_led led_inf_dsc_vdnto[] = {
	{
		.name = "power:blue",
		.bit = 0,
		.active_high = 1,
	},
	{
		.name = "wireless:green",
		.bit = 4,
		.active_high = 1,
	},
	{
		.name = "broadband:green",
		.bit = 1,
		.active_high = 1,
	},
	{
		.name = "internet:red",
		.bit = 3,
		.active_high = 1,
	},
	{
		.name = "internet:green",
		.bit = 2,
		.active_high = 1,
	},
	{
		.name = "wps:red",
		.bit = 6,
		.active_high = 1,
	},
	{
		.name = "wps:green",
		.bit = 5,
		.active_high = 1,
	},
	{
		.name = "ethernet:green",
		.bit = 7,
		.active_high = 1,
	},

};

/* Aggregated leds */
static struct aggreg_led led_agg_dsc_vdnto[] = {
	AGGREG_ORANGE("power")
	AGGREG_ORANGE("wps")
	AGGREG_ORANGE("internet")
	AGGREG_ORANGE("dect")
};


LED_FAMILY_WITH_SR(vdnto, 23, 0, 1, 8);
LED_BOARD(vdnto, vdnto, "VDNT-O");
LED_BOARD(vdnto, vdnt6, "VDNT-6");

/**
  * VDNT-8
 */

/* GPIO leds */
static struct gpio_led gpled_vdnt8[] = {
        {
                .name = "power:red",
                .gpio = 38,
                .active_low = 1,
        },
        {
                .name = "power:green",
                .gpio = 39,
                .active_low = 1,
        },
        {
                .name = "iptv:green",
                .gpio = 35,
                .active_low = 1,
        },
        {
                .name = "voip:green",
                .gpio = 9,
                .active_low = 1,
        },
};

/* Shift register leds */
static struct shiftled_led led_inf_dsc_vdnt8[] = {
        {
                .name = "power:blue",
                .bit = 0,
                .active_high = 1,
        },
        {
                .name = "wireless:green",
                .bit = 4,
                .active_high = 1,
        },
        {
                .name = "broadband:green",
                .bit = 1,
                .active_high = 1,
        },
        {
                .name = "internet:red",
                .bit = 3,
                .active_high = 1,
        },
        {
                .name = "internet:green",
                .bit = 2,
                .active_high = 1,
        },
        {
                .name = "wps:red",
                .bit = 6,
                .active_high = 1,
        },
        {
                .name = "wps:green",
                .bit = 5,
                .active_high = 1,
        },
        {
                .name = "ethernet:green",
                .bit = 7,
                .active_high = 1,
        },

};

/* Aggregated leds */
static struct aggreg_led led_agg_dsc_vdnt8[] = {
        AGGREG_ORANGE("power")
        AGGREG_ORANGE("wps")
        AGGREG_ORANGE("internet")
};

LED_FAMILY_WITH_SR(vdnt8, 23, 0, 1, 8);
LED_BOARD(vdnt8, vdnt8, "VDNT-8");

/**
 * VDNT-3
 */

/* GPIO leds */
static struct gpio_led gpled_vdnt3[] = {
	{
		.name = "power:red",
		.gpio = 38,
		.active_low = 1,
	},
	{
		.name = "power:green",
		.gpio = 39,
		.active_low = 1,
	},
	{
		.name = "iptv:green",
		.gpio = 35,
		.active_low = 1,
	},
};


/* Shift register leds */
static struct shiftled_led led_inf_dsc_vdnt3[] = {
	{
		.name = "power:blue",
		.bit = 0,
		.active_high = 1,
	},
	{
		.name = "wireless:green",
		.bit = 4,
		.active_high = 1,
	},
	{
		.name = "broadband:green",
		.bit = 1,
		.active_high = 1,
	},
	{
		.name = "internet:red",
		.bit = 3,
		.active_high = 1,
	},
	{
		.name = "internet:green",
		.bit = 2,
		.active_high = 1,
	},
	{
		.name = "wps:red",
		.bit = 6,
		.active_high = 1,
	},
	{
		.name = "wps:green",
		.bit = 5,
		.active_high = 1,
	},
	{
		.name = "ethernet:green",
		.bit = 7,
		.active_high = 1,
	},

};

/* Aggregated leds */
static struct aggreg_led led_agg_dsc_vdnt3[] = {
	AGGREG_ORANGE("power")
	AGGREG_ORANGE("wps")
	AGGREG_ORANGE("internet")
};


LED_FAMILY_WITH_SR(vdnt3, 23, 0, 1, 8);
LED_BOARD(vdnt3, vdnt3, "VDNT-3");

/**
 * VDNT-4
 */

/* GPIO leds */
static struct gpio_led gpled_vdnt4[] = {
        {
                .name = "power:green",
                .gpio = 20,
                .active_low = 0,
        },
        {
                .name = "power:red",
                .gpio = 23,
                .active_low = 0,
        },
        {
                .name = "broadband2:red",
                .gpio = 15,
                .active_low = 1,
        },
        {
                .name = "broadband2:green",
                .gpio = 16,
                .active_low = 0,
        },
        {
                .name = "wireless:red",
                .gpio = 36,
                .active_low = 0,
        },
        {
                .name = "wireless:green",
                .gpio = 37,
                .active_low = 0,
        },
        {
                .name = "wps:red",
                .gpio = 39,
                .active_low = 0,
        },
        {
                .name = "wps:green",
                .gpio = 40,
                .active_low = 0,
        },
        {
                .name = "iptv:green",
                .gpio = 9,
                .active_low = 0,
        },
        {
                .name = "ethwan:green",
                .gpio = 38,
                .active_low = 0,
        },
};

/* Shift register leds */
static struct shiftled_led led_inf_dsc_vdnt4[] = {
        {
                .name = "broadband:red",
                .bit = 2,
                .active_high = 1,
        },
        {
                .name = "broadband:green",
                .bit = 3,
                .active_high = 1,
        },
        {
                .name = "internet:red",
                .bit = 0,
                .active_high = 1,
        },
        {
                .name = "internet:green",
                .bit = 1,
                .active_high = 1,
        },
        {
                .name = "voip:red",
                .bit = 6,
                .active_high = 1,
        },
        {
                .name = "voip:green",
                .bit = 7,
                .active_high = 1,
        },
};

/* Aggregated leds */
static struct aggreg_led led_agg_dsc_vdnt4[] = {
        AGGREG_ORANGE("power")
        AGGREG_ORANGE("wireless")
        AGGREG_ORANGE("wps")
        AGGREG_ORANGE("broadband")
        AGGREG_ORANGE("broadband2")
        AGGREG_ORANGE("internet")
        AGGREG_ORANGE("voip")
};

LED_FAMILY_WITH_SR(vdnt4, 21, 0, 1, 8);
LED_BOARD(vdnt4, vdnt4, "VDNT-4");

/**
 * VANT-F, VANT-E, VANT-R, VANT-D
 */


/* GPIO leds */
static struct gpio_led gpled_vantf[] = {
};


/* Shift register leds */
static struct shiftled_led led_inf_dsc_vantf[] = {
	{
		.name = "power:green",
		.bit = 11,
		.active_high = 0,
	},
	{
		.name = "power:red",
		.bit = 8,
		.active_high = 0,
	},
	{
		.name = "power:blue",
		.bit = 9,
		.active_high = 0,
	},
	{
		.name = "wireless:green",
		.bit = 5,
		.active_high = 0,
	},
	{
		.name = "wireless_5g:green",
		.bit = 3,
		.active_high = 0,
	},
	{
		.name = "broadband:green",
		.bit = 10,
		.active_high = 0,
	},
	{
		.name = "internet:red",
		.bit = 14,
		.active_high = 0,
	},
	{
		.name = "internet:green",
		.bit = 15,
		.active_high = 0,
	},
	{
		.name = "wps:red",
		.bit = 7,
		.active_high = 0,
	},
	{
		.name = "wps:green",
		.bit = 6,
		.active_high = 0,
	},
	{
		.name = "ethernet:green",
		.bit = 12,
		.active_high = 0,
	},
	{
		.name = "dect:green",
		.bit = 4,
		.active_high = 0,
	},
	{
		.name = "dect:red",
		.bit = 13,
		.active_high = 0,
	},
	{
		.name = "iptv:green",
		.bit = 0,
		.active_high = 0,
	},
	{
		.name = "voip:green",
		.bit = 1,
		.active_high = 0,
	},

};


/* Aggregated leds */
static struct aggreg_led led_agg_dsc_vantf[] = {
	AGGREG_ORANGE("power")
	AGGREG_ORANGE("wps")
	AGGREG_ORANGE("internet")
	AGGREG_ORANGE("dect")
};

LED_FAMILY_WITH_SR(vantf, 9, 0, 1, 16);
LED_BOARD(vantf, vantf, "VANT-F");
LED_BOARD(vantf, vante, "VANT-E");
LED_BOARD(vantf, vantr, "VANT-R");
LED_BOARD(vantf, gbntg, "GBNT-G");
LED_BOARD(vantf, vantd, "VANT-D");
LED_BOARD(vantf, vant4, "VANT-4");



/**
 * VANT-G
 */

/* GPIO leds */
static struct gpio_led gpled_vantg[] = {
	{
		.name = "power:blue",
		.gpio = 8,
		.active_low = 0,
	},
	{
		.name = "wps:blue",
		.gpio = 9,
		.active_low = 0,
	},
	{
		.name = "broadband:red",
		.gpio = 14,
		.active_low = 0,
	},
	{
		.name = "fxs2:red",
		.gpio = 15,
		.active_low = 1,
	},
	{
		.name = "wireless:blue",
		.gpio = 18,
		.active_low = 0,
	},
	{
		.name = "ethernet:red",
		.gpio = 22,
		.active_low = 0,
	},
	{
		.name = "internet:blue",
		.gpio = 36,
		.active_low = 0,
	},
	{
		.name = "broadband:blue",
		.gpio = 37,
		.active_low = 0,
	},
	{
		.name = "ethernet:blue",
		.gpio = 38,
		.active_low = 0,
	},
};


/* Shift register leds */
static struct shiftled_led led_inf_dsc_vantg[] = {
        {
                .name = "fxs2:blue",
                .bit = 0,
                .active_high = 0,
        },
        {
                .name = "fxs1:blue",
                .bit = 1,
                .active_high = 0,
        },
        {
                .name = "fxs1:red",
                .bit = 2,
                .active_high = 0,
        },
        {
                .name = "wireless:red",
                .bit = 3,
                .active_high = 0,
        },
        {
                .name = "internet:red",
                .bit = 4,
                .active_high = 0,
        },
        {
                .name = "ethwan:red",
                .bit = 5,
                .active_high = 0,
        },
        {
                .name = "ethwan:blue",
                .bit = 6,
                .active_high = 0,
        },
        {
                .name = "wps:red",
                .bit = 7,
                .active_high = 0,
        },
};

/* Aggregated leds */
static struct aggreg_led led_agg_dsc_vantg[] = {
	AGGREG_MAGENTA("wireless")
	AGGREG_MAGENTA("wps")
	AGGREG_MAGENTA("fxs1")
	AGGREG_MAGENTA("fxs2")
	AGGREG_MAGENTA("broadband")
	AGGREG_MAGENTA("ethernet")
	AGGREG_MAGENTA("internet")
	AGGREG_MAGENTA("ethwan")
};



LED_FAMILY_WITH_SR(vantg, 39, 0, 1, 8);
LED_BOARD(vantg, vantg, "VANT-G");


/**
 * VANT-9
 */

/* GPIO leds */
static struct gpio_led gpled_vant9[] = {
	{
		.name = "power:red",
		.gpio = 0,
		.active_low = 1,
	},
	{
		.name = "power:green",
		.gpio = 1,
		.active_low = 1,
	},
	{
		.name = "mobile:red",
		.gpio = 8,
		.active_low = 1,
	},
	{
		.name = "mobile:green",
		.gpio = 9,
		.active_low = 1,
	},
	{
		.name = "wireless:red",
		.gpio = 36,
		.active_low = 1,
	},
	{
		.name = "internet:red",
		.gpio = 14,
		.active_low = 1,
	},
	{
		.name = "mobile:blue",
		.gpio = 18,
		.active_low = 1,
	},
	{
		.name = "internet:green",
		.gpio = 35,
		.active_low = 1,
	},
	{
		.name = "wireless:green",
		.gpio = 37,
		.active_low = 1,
	},
	{
		.name = "voip:red",
		.gpio = 38,
		.active_low = 1,
	},
	{
		.name = "voip:green",
		.gpio = 39,
		.active_low = 1,
	},
};


/* Shift register leds */
static struct shiftled_led led_inf_dsc_vant9[] = {
};

/* Aggregated leds */
static struct aggreg_led led_agg_dsc_vant9[] = {
	AGGREG_ORANGE("internet")
	AGGREG_ORANGE("wireless")
	AGGREG_ORANGE("voip")
	AGGREG_ORANGE("power")
};


LED_FAMILY_WITHOUT_SR(vant9);
LED_BOARD(vant9, vant9, "VANT-9");

/**
 * VANT-5
 */


/* GPIO leds */
static struct gpio_led gpled_vant5[] = {
};


/* Shift register leds */
static struct shiftled_led led_inf_dsc_vant5[] = {
	{
		.name = "power:green",
		.bit = 11,
		.active_high = 0,
	},
	{
		.name = "power:red",
		.bit = 8,
		.active_high = 0,
	},
	{
		.name = "power:blue",
		.bit = 9,
		.active_high = 0,
	},
	{
		.name = "ethernet:green",
		.bit = 5,
		.active_high = 0,
	},
	{
		.name = "wireless:green",
		.bit = 3,
		.active_high = 0,
	},
	{
		.name = "broadband:green",
		.bit = 10,
		.active_high = 0,
	},
	{
		.name = "internet:red",
		.bit = 14,
		.active_high = 0,
	},
	{
		.name = "internet:green",
		.bit = 15,
		.active_high = 0,
	},
	{
		.name = "wps:red",
		.bit = 7,
		.active_high = 0,
	},
	{
		.name = "wps:green",
		.bit = 6,
		.active_high = 0,
	},
	{
		.name = "voip:green",
		.bit = 12,
		.active_high = 0,
	},

};


/* Aggregated leds */
static struct aggreg_led led_agg_dsc_vant5[] = {
	AGGREG_ORANGE("power")
	AGGREG_ORANGE("wps")
	AGGREG_ORANGE("internet")
};

LED_FAMILY_WITH_SR(vant5, 9, 0, 1, 16);
LED_BOARD(vant5, vant5, "VANT-5");


/**
 * VBNT-A
 */


/* GPIO leds */
static struct gpio_led gpled_vbnta[] = {
};


/* Shift register leds */
static struct shiftled_led led_inf_dsc_vbnta[] = {
	{
		.name = "power:green",
		.bit = 11,
		.active_high = 0,
	},
	{
		.name = "power:red",
		.bit = 8,
		.active_high = 0,
	},
	{
		.name = "power:blue",
		.bit = 9,
		.active_high = 0,
	},
	{
		.name = "wireless:green",
		.bit = 5,
		.active_high = 0,
	},
	{
		.name = "wireless_5g:green",
		.bit = 3,
		.active_high = 0,
	},
	{
		.name = "broadband:green",
		.bit = 10,
		.active_high = 0,
	},
	{
		.name = "internet:red",
		.bit = 14,
		.active_high = 0,
	},
	{
		.name = "internet:green",
		.bit = 15,
		.active_high = 0,
	},
	{
		.name = "wps:red",
		.bit = 7,
		.active_high = 0,
	},
	{
		.name = "wps:green",
		.bit = 6,
		.active_high = 0,
	},
	{
		.name = "ethernet:green",
		.bit = 12,
		.active_high = 0,
	},
	{
		.name = "voip:green",
		.bit = 0,
		.active_high = 0,
	},

};


/* Aggregated leds */
static struct aggreg_led led_agg_dsc_vbnta[] = {
	AGGREG_ORANGE("power")
	AGGREG_ORANGE("wps")
	AGGREG_ORANGE("internet")
};

LED_FAMILY_WITH_SR(vbnta, 9, 0, 1, 16);
LED_BOARD(vbnta, vbnta, "VBNT-A");

/**
 * VANT-6, VANT-7, VANT-8 , VBNT-L
 */


/* GPIO leds */
static struct gpio_led gpled_vant6[] = {
};


/* Shift register leds */
static struct shiftled_led led_inf_dsc_vant6[] = {
	{
		.name = "power:green",
		.bit = 11,
		.active_high = 0,
	},
	{
		.name = "power:red",
		.bit = 8,
		.active_high = 0,
	},
	{
		.name = "power:blue",
		.bit = 9,
		.active_high = 0,
	},
	{
		.name = "wireless:green",
		.bit = 5,
		.active_high = 0,
	},
	{
		.name = "wireless:red",
		.bit = 4,
		.active_high = 0,
	},
	{
		.name = "wireless_5g:green",
		.bit = 3,
		.active_high = 0,
	},
	{
		.name = "wireless_5g:red",
		.bit = 2,
		.active_high = 0,
	},
	{
		.name = "broadband:green",
		.bit = 10,
		.active_high = 0,
	},
	{
		.name = "internet:red",
		.bit = 14,
		.active_high = 0,
	},
	{
		.name = "internet:green",
		.bit = 15,
		.active_high = 0,
	},
	{
		.name = "wps:red",
		.bit = 7,
		.active_high = 0,
	},
	{
		.name = "wps:green",
		.bit = 6,
		.active_high = 0,
	},
	{
		.name = "ethernet:green",
		.bit = 12,
		.active_high = 0,
	},
	{
		.name = "iptv:green",
		.bit = 0,
		.active_high = 0,
	},
	{
		.name = "voip:green",
		.bit = 1,
		.active_high = 0,
	},
	{
		.name = "voip:red",
		.bit = 13,
		.active_high = 0,
	},
};


/* Aggregated leds */
static struct aggreg_led led_agg_dsc_vant6[] = {
	AGGREG_ORANGE("power")
	AGGREG_ORANGE("wireless")
	AGGREG_ORANGE("wireless_5g")
	AGGREG_ORANGE("wps")
	AGGREG_ORANGE("voip")
	AGGREG_ORANGE("internet")
};

LED_FAMILY_WITH_SR(vant6, 9, 0, 1, 16);
LED_BOARD(vant6, vant6, "VANT-6");
LED_BOARD(vant6, vant7, "VANT-7");
LED_BOARD(vant6, vant8, "VANT-8");
LED_BOARD(vant6, vbntl, "VBNT-L");

/* VBNT-B */
/* GPIO leds */
static struct gpio_led gpled_vbntb[] = {
        {
                .name = "power:green",
                .gpio = 37,
                .active_low = 1,
        },
        {
                .name = "power:blue",
                .gpio = 36,
                .active_low = 1,
        },
        {
                .name = "power:red",
                .gpio = 38,
                .active_low = 1,
        },
        {
                .name = "wireless:green",
                .gpio = 15,
                .active_low = 1,
        },
        {
                .name = "wireless:blue",
                .gpio = 17,
                .active_low = 1,
        },
        {
                .name = "wireless:red",
                .gpio = 18,
                .active_low = 1,
        },
};

static struct shiftled_led led_inf_dsc_vbntb[] = {
        {
                .name = "ethernet:blue",
                .bit = 0,
                .active_high = 0,
        },
        {
                .name = "ethernet:green",
                .bit = 1,
                .active_high = 0,
        },
        {
                .name = "ethernet:red",
                .bit = 2,
                .active_high = 0,
        },
        {
                .name = "internet:blue",
                .bit = 3,
                .active_high = 0,
        },
        {
                .name = "internet:green",
                .bit = 4,
                .active_high = 0,
        },
        {
                .name = "internet:red",
                .bit = 5,
                .active_high = 0,
        },
        {
                .name = "voip:blue",
                .bit = 8,
                .active_high = 0,
        },
        {
                .name = "voip:green",
                .bit = 9,
                .active_high = 0,
        },
        {
                .name = "voip:red",
                .bit = 10,
                .active_high = 0,
        },
        {
                .name = "dect:blue",
                .bit = 11,
                .active_high = 0,
        },
        {
                .name = "dect:green",
                .bit = 12,
                .active_high = 0,
        },
        {
                .name = "dect:red",
                .bit = 13,
                .active_high = 0,
        },
};

static struct aggreg_led led_agg_dsc_vbntb[] = {
        AGGREG_WHITE("power")
        AGGREG_WHITE("wireless")
        AGGREG_WHITE("ethernet")
        AGGREG_WHITE("internet")
        AGGREG_WHITE("voip")
        AGGREG_WHITE("dect")
};

LED_FAMILY_WITH_SR(vbntb, 22, 0, 1, 16);
LED_BOARD(vbntb, vbntb, "VBNT-B");

/**
 * VANT-C
 */

/* GPIO leds */
static struct gpio_led gpled_vantc[] = {
	{
		.name = "power:green",
		.gpio = 15,
		.active_low = 1,
	},
	{
		.name = "power:red",
		.gpio = 18,
		.active_low = 1,
	},
	{
		.name = "wireless:red",
		.gpio = 36,
		.active_low = 1,
	},
	{
		.name = "internet:green",
		.gpio = 19,
		.active_low = 1,
	},
	{
		.name = "broadband:green",
		.gpio = 21,
		.active_low = 0,
	},
	{
		.name = "internet:red",
		.gpio = 17,
		.active_low = 1,
	},
	{
		.name = "wps:green",
		.gpio = 8,
		.active_low = 1,
	},
	{
		.name = "wireless:green",
		.gpio = 37,
		.active_low = 1,
	},
	{
		.name = "wps:red",
		.gpio = 22,
		.active_low = 1,
	},
	{
		.name = "ethernet:green",
		.gpio = 13,
		.active_low = 0,
	},
};

/* Shift leds */
static struct shiftled_led led_inf_dsc_vantc[] = {
};

/* Aggregated leds */
static struct aggreg_led led_agg_dsc_vantc[] = {
	AGGREG_ORANGE("power")
	AGGREG_ORANGE("wps")
	AGGREG_ORANGE("internet")
	AGGREG_ORANGE("wireless")
};

LED_FAMILY_WITHOUT_SR(vantc);
LED_BOARD(vantc, vantc, "VANT-C");

/**
 * GANT-1
 */

/* GPIO leds */
static struct gpio_led gpled_gant1[] = {
	{
		.name = "power:green",
		.gpio = 1,
		.active_low = 1,
	},
	{
		.name = "power:red",
		.gpio = 8,
		.active_low = 1,
	},
	{
		.name = "internet:green",
		.gpio = 9,
		.active_low = 1,
	},
	{
		.name = "internet:red",
		.gpio = 10,
		.active_low = 1,
	},
	{
		.name = "wireless:green",
		.gpio = 11,
		.active_low = 1,
	},
	{
		.name = "wps:green",
		.gpio = 14,
		.active_low = 1,
	},
	{
		.name = "wps:red",
		.gpio = 18,
		.active_low = 1,
	},
	{
		.name = "wireless_5g:green",
		.gpio = 22,
		.active_low = 1,
	},
	{
		.name = "iptv:green",
		.gpio = 36,
		.active_low = 1,
	},
	{
		.name = "ethernet:green",
		.gpio = 37,
		.active_low = 1,
	},
	{
		.name = "voip:green",
		.gpio = 38,
		.active_low = 1,
	},
};

/* Shift leds */
static struct shiftled_led led_inf_dsc_gant1[] = {
};

/* Aggregated leds */
static struct aggreg_led led_agg_dsc_gant1[] = {
	AGGREG_ORANGE("power")
	AGGREG_ORANGE("wps")
	AGGREG_ORANGE("internet")
};

LED_FAMILY_WITHOUT_SR(gant1);
LED_BOARD(gant1, gant1, "GANT-1");
LED_BOARD(gant1, gant2, "GANT-2");
LED_BOARD(gant1, gant8, "GANT-8");


/**
 * VANT-T
 */

/* GPIO leds */
static struct gpio_led gpled_vantt[] = {
	{
		.name = "power:red",
		.gpio = 23,
		.active_low = 0,
	},
	{
		.name = "power:green",
		.gpio = 20,
		.active_low = 0,
	},
	{
		.name = "broadband2:red",
		.gpio = 15,
		.active_low = 1,
	},
	{
		.name = "broadband2:green",
		.gpio = 13,
		.active_low = 0,
	},
	{
		.name = "usb:green",
		.gpio = 12,
		.active_low = 0,
	},
	{
		.name = "wireless:red",
		.gpio = 36,
		.active_low = 0,
	},
	{
		.name = "wireless:green",
		.gpio = 37,
		.active_low = 0,
	},
	{
		.name = "wireless_5g:red",
		.gpio = 43,
		.active_low = 1,
	},
	{
		.name = "wireless_5g:green",
		.gpio = 42,
		.active_low = 0,
	},
	{
		.name = "wps:red",
		.gpio = 39,
		.active_low = 0,
	},
	{
		.name = "wps:green",
		.gpio = 40,
		.active_low = 1,
	},
    {
        .name = "wanlan:green",
        .gpio = 38,
        .active_low = 0,
    },
};


/* Shift register leds */
static struct shiftled_led led_inf_dsc_vantt[] = {
        {
                .name = "voip:red",
                .bit = 6,
                .active_high = 1,
        },
        {
                .name = "voip:green",
                .bit = 7,
                .active_high = 1,
        },
        {
                .name = "voip2:red",
                .bit = 4,
                .active_high = 1,
        },
        {
                .name = "voip2:green",
                .bit = 5,
                .active_high = 1,
        },
        {
                .name = "broadband:red",
                .bit = 2,
                .active_high = 1,
        },
        {
                .name = "broadband:green",
                .bit = 3,
                .active_high = 1,
        },
        {
                .name = "internet:red",
                .bit = 0,
                .active_high = 1,
        },
        {
                .name = "internet:green",
                .bit = 1,
                .active_high = 1,
        },
};


/* Aggregated leds */
static struct aggreg_led led_agg_dsc_vantt[] = {
	AGGREG_ORANGE("power")
	AGGREG_ORANGE("wps")
	AGGREG_ORANGE("internet")
};

LED_FAMILY_WITH_SR(vantt, 21, 0, 1, 8);
LED_BOARD(vantt, vantt, "VANT-T");

/**
 * VBNT-N
 */

/* GPIO leds */
static struct gpio_led gpled_vbntn[] = {
        {
                .name = "internet:green",
                .gpio = 8,
                .active_low = 0,
        },
        {
                .name = "internet:red",
                .gpio = 9,
                .active_low = 0,
        },
        {
                .name = "wanlan:green",
                .gpio = 12,
                .active_low = 0,
        },
        {
                .name = "broadband:red",
                .gpio = 14,
                .active_low = 0,
        },
        {
                .name = "broadband:green",
                .gpio = 18,
                .active_low = 0,
        },
	{
		.name = "power:green",
		.gpio = 20,
		.active_low = 0,
	},
	{
		.name = "wps:green",
		.gpio = 22,
		.active_low = 1,
	},
	{
		.name = "power:red",
		.gpio = 23,
		.active_low = 0,
	},
	{
		.name = "wireless:red",
		.gpio = 36,
		.active_low = 0,
	},
	{
		.name = "wireless:green",
		.gpio = 37,
		.active_low = 0,
	},
	{
		.name = "usb:green",
		.gpio = 38,
		.active_low = 0,
	},
	{
		.name = "wps:red",
		.gpio = 39,
		.active_low = 0,
	},
};

/* Shift register leds */
static struct shiftled_led led_inf_dsc_vbntn[] = {
};


/* Aggregated leds */
static struct aggreg_led led_agg_dsc_vbntn[] = {
	AGGREG_ORANGE("power")
	AGGREG_ORANGE("wps")
	AGGREG_ORANGE("broadband")
	AGGREG_ORANGE("wireless")
	AGGREG_ORANGE("internet")
};

LED_FAMILY_WITHOUT_SR(vbntn);
LED_BOARD(vbntn, vbntn, "VBNT-N");


/**
 * VDNT-Y
 */

/* GPIO leds */
static struct gpio_led gpled_vdnty[] = {
	{
		.name = "fxs2:blue",
		.gpio = 22,
		.active_low = 1,
	},
};


/* Shift register leds */
static struct shiftled_led led_inf_dsc_vdnty[] = {
	{
		.name = "power:blue",
		.bit = 15,
		.active_high = 0,
	},
	{
		.name = "broadband:red",
		.bit = 14,
		.active_high = 0,
	},
	{
		.name = "broadband:blue",
		.bit = 13,
		.active_high = 0,
	},
	{
        .name = "ethwan:red",
        .bit = 12,
        .active_high = 0,
    },
    {
        .name = "ethwan:blue",
        .bit = 11,
        .active_high = 0,
    },
	{
		.name = "internet:red",
		.bit = 10,
		.active_high = 0,
	},
	{
		.name = "ethernet:red",
		.bit = 9,
		.active_high = 0,
	},
	{
		.name = "ethernet:blue",
		.bit = 8,
		.active_high = 0,
	},
	{
		.name = "internet:blue",
		.bit = 7,
		.active_high = 0,
	},
	{
		.name = "fxs1:red",
		.bit = 6,
		.active_high = 0,
	},
	{
		.name = "fxs1:blue",
		.bit = 5,
		.active_high = 0,
	},
	{
		.name = "fxs2:red",
		.bit = 4,
		.active_high = 0,
	},
	{
		.name = "wps:red",
		.bit = 3,
		.active_high = 0,
	},
	{
		.name = "wps:blue",
		.bit = 2,
		.active_high = 0,
	},	
	{
		.name = "wireless:red",
		.bit = 1,
		.active_high = 0,
	},
	{
		.name = "wireless:blue",
		.bit = 0,
		.active_high = 0,
	},
};


/* Aggregated leds */
static struct aggreg_led led_agg_dsc_vdnty[] = {
	AGGREG_MAGENTA("wireless")
	AGGREG_MAGENTA("wps")
	AGGREG_MAGENTA("fxs1")
	AGGREG_MAGENTA("fxs2")
	AGGREG_MAGENTA("broadband")
	AGGREG_MAGENTA("ethernet")
	AGGREG_MAGENTA("internet")
	AGGREG_MAGENTA("ethwan")
};

LED_FAMILY_WITH_SR(vdnty, 9, 0, 1, 16);
LED_BOARD(vdnty, vdnty, "VDNT-Y");

/**
 * VDNT-O
 */

/* GPIO leds */
static struct gpio_led gpled_gantk[] = {
    {
        .name = "power:red",
        .gpio = 38,
        .active_low = 1,
    },
    {
        .name = "power:green",
        .gpio = 39,
        .active_low = 1,
    },
    {
        .name = "iptv:green",
        .gpio = 35,
        .active_low = 1,
    },
    {
        .name = "voip:green",
        .gpio = 9,
        .active_low = 1,
    },
};


/* Shift register leds */
static struct shiftled_led led_inf_dsc_gantk[] = {
    {
        .name = "power:blue",
        .bit = 0,
        .active_high = 1,
    },
    {
        .name = "wireless:green",
        .bit = 4,
        .active_high = 1,
    },
    {
        .name = "broadband:green",
        .bit = 1,
        .active_high = 1,
    },
    {
        .name = "internet:red",
        .bit = 3,
        .active_high = 1,
    },
    {
        .name = "internet:green",
        .bit = 2,
        .active_high = 1,
    },
    {
        .name = "wps:red",
        .bit = 6,
        .active_high = 1,
    },
    {
        .name = "wps:green",
        .bit = 5,
        .active_high = 1,
    },
    {
        .name = "ethernet:green",
        .bit = 7,
        .active_high = 1,
    },

};

/* Aggregated leds */
static struct aggreg_led led_agg_dsc_gantk[] = {
    AGGREG_ORANGE("power")
    AGGREG_ORANGE("wps")
    AGGREG_ORANGE("internet")
};


LED_FAMILY_WITH_SR(gantk, 23, 0, 1, 8);
LED_BOARD(gantk, gantk, "GANT-K");

static struct board * boards[] = {&board_vantc, &board_vantd, &board_vante, &board_vantf, &board_vantg, &board_vant9, &board_vant5, &board_vbnta, &board_vant6,&board_vbntr, &board_vant7, &board_vant8, &board_vantr, &board_gbntg, &board_vant4, &board_vdnt3, &board_vdnt4, &board_vdntw, &board_vdnto, &board_vdnt6, &board_vdnt8, &board_gant1, &board_gant2,  &board_gant8, &board_vantt, &board_vdnty, &board_gantk, &board_vbntl, &board_vbntb, &board_vbntn, 0};

#elif defined(CONFIG_BCM96362)

/**
 * DANT-U
 */


/* GPIO leds  */
static struct gpio_led gpled_dantu[] = {
};


/* Shift register leds */
static struct shiftled_led led_inf_dsc_dantu[] = {
    {
        .name = "power:green",
        .bit = 2,
        .active_high = 1,
    },
    {
        .name = "power:red",
        .bit = 3,
        .active_high = 1,
    },
    {
        .name = "power:blue",
        .bit = 1,
        .active_high = 1,
    },
    {
        .name = "wireless:green",
        .bit = 11,
        .active_high = 1,
    },
    {
        .name = "wps:red",
        .bit = 5,
        .active_high = 1,
        },
    {
        .name = "wps:green",
        .bit = 4,
        .active_high = 1,
    },
    {
        .name = "broadband:green",
        .bit = 8,
        .active_high = 1,
    },
    {
        .name = "internet:red",
        .bit = 7,
        .active_high = 1,
    },
    {
        .name = "internet:green",
        .bit = 6,
        .active_high = 1,
    },
    {
        .name = "ethernet:green",
        .bit = 12,
        .active_high = 1,
    },
    {
        .name = "fxs1:green",
        .bit = 9,
        .active_high = 1,
    },
    {
        .name = "fxs2:green",
        .bit = 10,
        .active_high = 1,
    },
};

/* Aggregated leds */
static struct aggreg_led led_agg_dsc_dantu[] = {
    AGGREG_ORANGE("power")
    AGGREG_ORANGE("internet")
    AGGREG_ORANGE("wps")
};


LED_FAMILY_WITH_SR(dantu, 1, 2, 3, 16);
LED_BOARD(dantu, dantu, "DANT-U");

/**
 *  * DANT-O
 *   */

/* GPIO leds  */
static struct gpio_led gpled_danto[] = {
    {
        .name = "power:green",
        .gpio = 28,
        .active_low = 1,
    },
    {
        .name = "ethernet:green",
        .gpio = 5,
        .active_low = 1,
    },
    {
        .name = "voip:green",
        .gpio = 6,
        .active_low = 1,
    },
    {
        .name = "wps:red",
        .gpio = 31,
        .active_low = 1,
        },
    {
        .name = "wps:green",
        .gpio = 30,
        .active_low = 1,
    },
};

/* Shift register leds */
static struct shiftled_led led_inf_dsc_danto[] = {
    {
        .name = "power:red",
        .bit = 7,
        .active_high = 1,
    },
    {
        .name = "power:blue",
        .bit = 0,
        .active_high = 1,
    },
    {
        .name = "wireless:green",
        .bit = 1,
        .active_high = 1,
    },
    {
        .name = "broadband:green",
        .bit = 2,
        .active_high = 1,
    },
    {
        .name = "internet:red",
        .bit = 4,
        .active_high = 1,
    },
    {
        .name = "internet:green",
        .bit = 3,
        .active_high = 1,
    },
    {
        .name = "dect:green",
        .bit = 5,
        .active_high = 1,
    },
    {
        .name = "dect:red",
        .bit = 6,
        .active_high = 1,
    },

};

/* Aggregated leds */
static struct aggreg_led led_agg_dsc_danto[] = {
    AGGREG_ORANGE("power")
    AGGREG_ORANGE("internet")
    AGGREG_ORANGE("wps")
    AGGREG_ORANGE("dect")
};

LED_FAMILY_WITH_SR(danto, 1, 2, 3, 8);
LED_BOARD(danto, danto, "DANT-O");


/**
 *  *  * DANT-Y
 *   *   */

/* GPIO leds  */
static struct gpio_led gpled_danty[] = {
    {
        .name = "power:red",
        .gpio = 9,
        .active_low = 1,
    },
    {
        .name = "power:green",
        .gpio = 10,
        .active_low = 1,
    },
    {
        .name = "power:blue",
        .gpio = 6,
        .active_low = 1,
    },
    {
        .name = "ethernet:green",
        .gpio = 29,
        .active_low = 1,
    },
    {
        .name = "wireless:green",
        .gpio = 30,
        .active_low = 1,
    },
    {
        .name = "broadband:green",
        .gpio = 11,
        .active_low = 1,
    },
    {
        .name = "wps:red",
        .gpio = 0,
        .active_low = 1,
    },
    {
        .name = "wps:green",
        .gpio = 1,
        .active_low = 1,
    },
    {
        .name = "internet:green",
        .gpio = 4,
        .active_low = 1,
    },
    {
        .name = "internet:red",
        .gpio = 3,
        .active_low = 1,
    },
};

/* Shift register leds */
static struct shiftled_led led_inf_dsc_danty[] = {
};

/* Aggregated leds */
static struct aggreg_led led_agg_dsc_danty[] = {
    AGGREG_ORANGE("wps")
};

LED_FAMILY_WITHOUT_SR(danty);
LED_BOARD(danty, danty, "DANT-Y");

static struct board * boards[] = {&board_dantu, &board_danto, &board_danty, 0};

#elif defined(CONFIG_BCM96318)


/**
 * TG582v2 family
 */

/* GPIO leds */
static struct gpio_led gpled_dant7[] = {
	{
		.name = "power:green",
		.gpio = 0,
		.active_low = 1,
	},
	{
		.name = "power:red",
		.gpio = 1,
		.active_low = 1,
	},
	{
		.name = "wireless:red",
		.gpio = 2,
		.active_low = 1,
	},
	{
		.name = "internet:green",
		.gpio = 3,
		.active_low = 1,
	},
	{
		.name = "broadband:green",
		.gpio = 8,
		.active_low = 1,
	},
	{
		.name = "internet:red",
		.gpio = 9,
		.active_low = 1,
	},
	{
		.name = "wps:green",
		.gpio = 11,
		.active_low = 1,
	},
	{
		.name = "wireless:green",
		.gpio = 39,
		.active_low = 1,
	},
	{
		.name = "wps:red",
		.gpio = 48,
		.active_low = 0,
	},
	{
		.name = "ethernet:green",
		.gpio = 49,
		.active_low = 0,
	},
};

/* Shift register leds */
static struct shiftled_led led_inf_dsc_dant7[] = {
};

static struct aggreg_led led_agg_dsc_dant7[] = {
	AGGREG_ORANGE("power")
	AGGREG_ORANGE("wps")
	AGGREG_ORANGE("internet")
	AGGREG_ORANGE("wireless")
};

LED_FAMILY_WITHOUT_SR(dant7);
LED_BOARD(dant7, dant7, "DANT-7");
LED_BOARD(dant7, dant8, "DANT-8");
LED_BOARD(dant7, dant8_isdn, "DANT-8_ISDN");
LED_BOARD(dant7, dant9, "DANT-9");
LED_BOARD(dant7, vantm, "VANT-M");
LED_BOARD(dant7, vantu, "VANT-U");

static struct board * boards[] = {&board_dant7, &board_dant8, &board_dant8_isdn, &board_dant9, &board_vantm, &board_vantu, 0};

#elif defined(CONFIG_BCM968500)



/**
 * GANT-J
 */


static struct gpio_led gpled_gantj[] = {
	{
		.name			= "alarm:red",
		.gpio			= 3,
		.active_low		= false,
	},
	{
		.name			= "power:red",
		.gpio			= 5,
		.active_low		= true,
	},
	{
		.name			= "power:green",
		.gpio			= 6,
		.active_low		= true,
		.default_state  = 1,
	},
	{
		.name			= "wireless:green",
		.gpio			= 7,
		.active_low		= true,
	},
	{
		.name			= "pon:green",
		.gpio			= 38,
		.active_low		= true,
	},
	{
		.name			= "wps:red",
		.gpio			= 57,
		.active_low		= true,
	},
	{
		.name			= "wps:green",
		.gpio			= 58,
		.active_low		= true,
	},
	{
		.name			= "fxs1:green",
		.gpio			= 59,
		.active_low		= true,
	},
	{
		.name			= "fxs2:green",
		.gpio			= 60,
		.active_low		= true,
	},
	{
		.name			= "usb:green",
		.gpio			= 61,
		.active_low		= true,
	},
	{
		.name			= "broadband:green",
		.gpio			= 62,
		.active_low		= true,
	},

};

/* Shift register leds */
static struct shiftled_led led_inf_dsc_gantj[] = {
};

/* Aggregated leds */
static struct aggreg_led led_agg_dsc_gantj[] = {
	AGGREG_ORANGE("power")
	AGGREG_ORANGE("wps")
};

LED_FAMILY_WITHOUT_SR(gantj);
LED_BOARD(gantj, gantj, "GANT-J");
LED_BOARD(gantj, gant3, "GANT-3");

static struct board * boards[] = {&board_gantj, &board_gant3, 0};

#elif defined(CONFIG_BCM96838)

/**
 * GANT-U, GANT-H
 */

/* GPIO leds */
static struct gpio_led gpled_gantu[] = {
	{
		.name			= "alarm:red",
		.gpio			= 14,
		.active_low		= true,
	},
	{
		.name			= "power:red",
		.gpio			= 37,
		.active_low		= true,
	},
	{
		.name			= "power:green",
		.gpio			= 36,
		.active_low		= true,
		.default_state  = 1,
	},
	{
		.name			= "upgrade:green",
		.gpio			= 38,
		.active_low		= true,
	},
	{
		.name			= "wireless:green",
		.gpio			= 33,
		.active_low		= true,
	},
	{
		.name			= "wireless_5g:green",
		.gpio			= 34,
		.active_low		= true,
	},
	{
		.name			= "pon:green",
		.gpio			= 39,
		.active_low		= true,
	},
	{
		.name			= "wps:red",
		.gpio			= 17,
		.active_low		= true,
	},
	{
		.name			= "wps:green",
		.gpio			= 12,
		.active_low		= true,
	},
	{
		.name			= "voip:green",
		.gpio			= 13,
		.active_low		= true,
	},
	{
		.name			= "usb:green",
		.gpio			= 40,
		.active_low		= true,
	},
	{
		.name			= "broadband:green",
		.gpio			= 6,
		.active_low		= true,
	},
    {
        .name 			= "ethernet:green",
        .gpio 			= 53,
        .active_low 	= true,
    },

};

/* Shift register leds */
static struct shiftled_led led_inf_dsc_gantu[] = {
};

/* Aggregated leds */
static struct aggreg_led led_agg_dsc_gantu[] = {
	AGGREG_ORANGE("power")
	AGGREG_ORANGE("wps")
};

LED_FAMILY_WITHOUT_SR(gantu);
LED_BOARD(gantu, gantu, "GANT-U");
LED_BOARD(gantu, ganth, "GANT-H");



static struct board * boards[] = {&board_ganth, &board_gantu, 0};

#elif defined(CONFIG_BCM963138)

static struct board * boards[] = {0};

#elif defined(CONFIG_BCM963381)

static struct board * boards[] = {0};

#elif defined(CONFIG_ARCH_COMCERTO)


/**
 * GANT-N, C2KEVM, GANT-5
 */

/* GPIO leds */
static struct gpio_led gpled_gantn[] = {
    {
        .gpio = 10,
        .active_low = true,
        .name = "rssi_1:green"
    },
    {
        .gpio = 11,
        .active_low = true,
        .name = "rssi_2:green"
    },
    {
        .gpio = 12,
        .active_low = true,
        .name = "rssi_3:green"
    },
    {
        .gpio = 13,
        .active_low = true,
        .name = "rssi_4:green"
    },
    {
        .gpio = 14,
        .active_low = true,
        .name = "rssi_5:green"
    },
    {
        .gpio = 44,
        .active_low = true,
        .name = "power:green"
    },
    {
        .gpio = 45,
        .active_low = true,
        .name = "power:red"
    },
    {
        .gpio = 46,
        .active_low = true,
        .name = "3g:green"
    },
    {
        .gpio = 47,
        .active_low = true,
        .name = "voip2:red"
    },
    {
        .gpio = 48,
        .active_low = true,
        .name = "voip2:green"
    },
    {
        .gpio = 49,
        .active_low = true,
        .name = "wireless:green"
    },
    {
        .gpio = 50,
        .active_low = true,
        .name = "ethernet:green"
    },
    {
        .gpio = 51,
        .active_low = true,
        .name = "ethernet:red"
    },
    {
        .gpio = 52,
        .active_low = true,
        .name = "internet:green"
    },
    {
        .gpio = 53,
        .active_low = true,
        .name = "internet:red"
    },
    {
        .gpio = 54,
        .active_low = true,
        .name = "voip:red"
    },
    {
        .gpio = 55,
        .active_low = true,
        .name = "voip:green"
    },
    {
        .gpio = 56,
        .active_low = true,
        .name = "wps:red"
    },
    {
        .gpio = 57,
        .active_low = true,
        .name = "wps:green"
    },
    {
        .gpio = 59,
        .active_low = true,
        .name = "wireless2:green"
    }
};

/* Shift register leds */
struct shiftled_led led_inf_dsc_gantn[] = {
};

/* Aggregated leds */
static struct aggreg_led led_agg_dsc_gantn[] = {
	AGGREG_ORANGE("power")
	AGGREG_ORANGE("wps")
	AGGREG_ORANGE("internet")
	AGGREG_ORANGE("ethernet")
	AGGREG_ORANGE("voip")
	AGGREG_ORANGE("voip2")
};

LED_FAMILY_WITHOUT_SR(gantn);
LED_BOARD(gantn, gantn, "GANT-N");
LED_BOARD(gantn, c2kevm, "C2KEVM");
LED_BOARD(gantn, gant5, "GANT-5");

static struct board * boards[] = {&board_gant5, &board_c2kevm, &board_gantn, 0};

#elif defined(CONFIG_AG71XX)

/**
 * GANT-V
 */

/* GANT-V GPIO leds using leds-gpio driver */
static struct gpio_led gpled_gantv[] = {
};

/* Shift register leds using shiftled driver */
static struct shiftled_led led_inf_dsc_gantv[] = {
	{
		.name = "power:green",
		.bit = 15,
		.active_high = false,
	},
	{
		.name = "rssi_5:green",
		.bit = 14,
		.active_high = false,
	},
	{
		.name = "rssi_4:green",
		.bit = 13,
		.active_high = false,
	},
	{
		.name = "rssi_3:green",
		.bit = 12,
		.active_high = false,
	},
	{
		.name = "rssi_2:green",
		.bit = 11,
		.active_high = false,
	},
	{
		.name = "rssi_1:green",
		.bit = 10,
		.active_high = false,
	},
	{
		.name = "3g:green",
		.bit = 9,
		.active_high = false,
	},
	{
		.name = "wps:green",
		.bit = 8,
		.active_high = false,
	},
	{
		.name = "lan_4:green",
		.bit = 7,
		.active_high = false,
	},
	{
		.name = "internet:green",
		.bit = 6,
		.active_high = false,
	},
	{
		.name = "power:red",
		.bit = 5,
		.active_high = false,
	},
	{
		.name = "wireless:green",
		.bit = 4,
		.active_high = false,
	},
	{
		.name = "wps:red",
		.bit = 3,
		.active_high = false,
	},
	{
		.name = "lan_1:green",
		.bit = 2,
		.active_high = false,
	},
	{
		.name = "lan_2:green",
		.bit = 1,
		.active_high = false,
	},
	{
		.name = "lan_3:green",
		.bit = 0,
		.active_high = false,
	}
};

static struct aggreg_led led_agg_dsc_gantv[] = {
	AGGREG_ORANGE("power")
	AGGREG_ORANGE("wps")
};



LED_FAMILY_WITH_SR(gantv, 20, 19, 21, 16);
LED_BOARD(gantv, gantv, "GANT-V");

/**
 * GANT-W
 */

/* GPIO leds */
static struct gpio_led gpled_gantw[] = {
};

/* Shift register leds */
static struct shiftled_led led_inf_dsc_gantw[] = {
	{
		.name = "power:green",
		.bit = 23,
		.active_high = false,
	},
	{
		.name = "rssi_5:green",
		.bit = 22,
		.active_high = false,
	},
	{
		.name = "rssi_4:green",
		.bit = 21,
		.active_high = false,
	},
	{
		.name = "rssi_3:green",
		.bit = 20,
		.active_high = false,
	},
	{
		.name = "rssi_2:green",
		.bit = 19,
		.active_high = false,
	},
	{
		.name = "rssi_1:green",
		.bit = 18,
		.active_high = false,
	},
	{
		.name = "3g:green",
		.bit = 17,
		.active_high = false,
	},
	{
		.name = "wps:green",
		.bit = 16,
		.active_high = false,
	},
	{
		.name = "lan_4:green",
		.bit = 15,
		.active_high = false,
	},
	{
		.name = "internet:green",
		.bit = 14,
		.active_high = false,
	},
	{
		.name = "power:red",
		.bit = 13,
		.active_high = false,
	},
	{
		.name = "wireless:green",
		.bit = 12,
		.active_high = false,
	},
	{
		.name = "wps:red",
		.bit = 11,
		.active_high = false,
	},
	{
		.name = "lan_1:green",
		.bit = 10,
		.active_high = false,
	},
	{
		.name = "lan_2:green",
		.bit = 9,
		.active_high = false,
	},
	{
		.name = "lan_3:green",
		.bit = 8,
		.active_high = false,
	},
	{
		.name = "usb_hub_reset",
		.bit = 7,
		.active_high = true,
	},
	{
		.name = "ethernet:green",
		.bit = 6,
		.active_high = false,
	},
	{
		.name = "ethernet:red",
		.bit = 5,
		.active_high = false,
	},
	{
		.name = "voip:green",
		.bit = 4,
		.active_high = false,
	},
	{
		.name = "perst",
		.bit = 3,
		.active_high = true,
	},
	{
		.name = "lte_pwr_on_off",
		.bit = 2,
		.active_high = true,
	},
	{
		.name = "lte_ant_ctrl1",
		.bit = 1,
		.active_high = true,
	},
	{
		.name = "slic_reset",
		.bit = 0,
		.active_high = true,
	}
};

/* Aggregated leds */
static struct aggreg_led led_agg_dsc_gantw[] = {
	AGGREG_ORANGE("power")
	AGGREG_ORANGE("wps")
	AGGREG_ORANGE("ethernet")
};

LED_FAMILY_WITH_SR(gantw, 20, 19, 21, 24);
LED_BOARD(gantw, gantw, "GANT-W");


static struct board * boards[] = {&board_gantv, &board_gantw, 0};

#elif defined(CONFIG_ARCH_MDM9607)
/**
 * GBNT-D
 */

/* GPIO leds */
static struct gpio_led gpled_gbntd[] = {
};

/* Shift register leds */
static struct shiftled_led led_inf_dsc_gbntd[] = {
	{
		.name = "power:green",
		.bit = 15,
		.active_high = true,
	},
	{
		.name = "lte_ant_ctrl2_aux",
		.bit = 14,
		.active_high = true,
	},
	{
		.name = "lte_ant_ctrl1_aux",
		.bit = 13,
		.active_high = true,
	},
	{
		.name = "power:red",
		.bit = 12,
		.active_high = true,
	},
	{
		.name = "wps:red",
		.bit = 11,
		.active_high = true,
	},
	{
		.name = "wps:green",
		.bit = 10,
		.active_high = true,
	},
	{
		.name = "wireless:green",
		.bit = 9,
		.active_high = true,
	},
	{
		.name = "lte_ant_ctrl1_main",
		.bit = 8,
		.active_high = true,
	},
	{
		.name = "lte:red",
		.bit = 7,
		.active_high = true,
	},
	{
		.name = "4g_4:green",
		.bit = 6,
		.active_high = true,
	},
	{
		.name = "4g_3:green",
		.bit = 5,
		.active_high = true,
	},
	{
		.name = "4g_5:green",
		.bit = 4,
		.active_high = true,
	},
	{
		.name = "4g_2:green",
		.bit = 3,
		.active_high = true,
	},
	{
		.name = "lte:blue",
		.bit = 2,
		.active_high = true,
	},
	{
		.name = "4g_1:green",
		.bit = 1,
		.active_high = true,
	}
};

/* Aggregated leds */
static struct aggreg_led led_agg_dsc_gbntd[] = {
	AGGREG_ORANGE("power")
	AGGREG_ORANGE("wps")
};

LED_FAMILY_WITH_SR(gbntd, 59, 75, 22, 16);
LED_BOARD(gbntd, gbntd, "GBNT-D");


static struct board * boards[] = {&board_gbntd, 0};


#else

#error "Unsupported architecture"

#endif


struct board * get_board_description(const char * current_board) {
	struct board ** result = boards;

	while (*result) {
		if (strcmp((*result)->name, current_board) == 0) {
		    break;
		}
		result++;
	}

	return *result;
};
