/*
 * Button driver for Technicolor Linux based gateways.
 *
 * Copyright (C) 2012 Technicolor <linuxgw@technicolor.com>
 *
 */

#include <linux/types.h>
#include <linux/gpio_keys.h>
#include <linux/module.h>
#include <linux/leds.h>
#include <linux/spinlock.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include "board_button_defines.h"

static struct gpio_keys_platform_data bcm63xx_gpio_buttons_data = {
    .poll_interval  = 150, //150 msec
};

static struct platform_device bcm63xx_gpio_buttons_device = {
        .name           = "gpio-keys-polled",
        .id             = 0,
        .dev.platform_data = &bcm63xx_gpio_buttons_data,
};

static int __init button_init( void )
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
#elif defined (BOARD_GBNTD)
    const struct board * board_desc = get_board_description("GBNT-D");
#else
    const struct board * board_desc = get_board_description(tch_board);
#endif

    if (!board_desc) {
        printk("Could not find button description for platform: %s\n", tch_board);
        return -1;
    }

    bcm63xx_gpio_buttons_data.nbuttons = board_desc->buttons->nbuttons;
    bcm63xx_gpio_buttons_data.buttons = board_desc->buttons->buttons;

    err = platform_device_register(&bcm63xx_gpio_buttons_device);
    if (err) {
        printk("Failed to register GPIO button device\n");
    }

    return 0;
}

static void __exit button_exit( void )
{
    platform_device_unregister(&bcm63xx_gpio_buttons_device);
}

module_init(button_init);
module_exit(button_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Button support for Technicolor linux gateways.");
MODULE_AUTHOR("Technicolor <linuxgw@technicolor.com>");
