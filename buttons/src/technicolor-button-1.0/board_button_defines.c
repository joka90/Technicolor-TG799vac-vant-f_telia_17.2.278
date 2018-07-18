/*
 * Defines the board button defines for each Technicolor board
 *
 * Copyright (C) 2013 Technicolor <linuxgw@technicolor.com>
 *
 */

#include <linux/types.h>
#include <linux/gpio_keys.h>
#include <linux/input.h>

#define GPIO_BUTTON_FAMILY(family) \
static struct gpio_keys_platform_data gpio_buttons_pd_##family = { \
    .nbuttons = ARRAY_SIZE(gpio_buttons_##family), \
    .buttons = gpio_buttons_##family \
}

#define GPIO_BUTTON_BOARD(family, boardname, ripname) \
static struct board board_##boardname = { \
    .name = ripname, \
    .buttons = &gpio_buttons_pd_##family \
}

#include "board_button_defines.h"

#if defined(CONFIG_BCM963268)

static struct gpio_keys_button gpio_buttons_vbntr[] = {
    {
        .desc		= "reset",
        .gpio		= 32,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_0,
    },
    {
        .desc		= "wps",
        .gpio		= 34,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_4,
    }
};

GPIO_BUTTON_FAMILY(vbntr);
GPIO_BUTTON_BOARD(vbntr, vbntr, "VBNT-R");

static struct gpio_keys_button gpio_buttons_vdntw[] = {
    {
        .desc		= "reset",
        .gpio		= 32,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_0,
    },
    {
        .desc		= "wps",
        .gpio		= 33,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_4,
    },
    {
        .desc		= "wlan_eco",
        .gpio		= 35,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_1,
    },
};

GPIO_BUTTON_FAMILY(vdntw);
GPIO_BUTTON_BOARD(vdntw, vdntw, "VDNT-W");

static struct gpio_keys_button gpio_buttons_vdnto[] = {
    {
        .desc		= "reset",
        .gpio		= 32,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_0,
    },
    {
        .desc		= "wlan_eco",
        .gpio		= 36,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_1,
    },
    {
        .desc		= "dect_pair",
        .gpio		= 20,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_2,
    },
    {
        .desc		= "info",
        .gpio		= 37,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_3,
    },
    {
        .desc		= "wps",
        .gpio		= 22,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_4,
    }
};

GPIO_BUTTON_FAMILY(vdnto);
GPIO_BUTTON_BOARD(vdnto, vdnto, "VDNT-O");
GPIO_BUTTON_BOARD(vdnto, vdnt6, "VDNT-6");

static struct gpio_keys_button gpio_buttons_vdnt8[] = {
    {
        .desc           = "reset",
        .gpio           = 32,
        .active_low     = 1,
        .type           = EV_KEY,
        .code           = BTN_0,
    },
    {
        .desc           = "wlan_eco",
        .gpio           = 36,
        .active_low     = 1,
        .type           = EV_KEY,
        .code           = BTN_1,
    },
    {
        .desc           = "wps",
        .gpio           = 22,
        .active_low     = 1,
        .type           = EV_KEY,
        .code           = BTN_4,
    }
};

GPIO_BUTTON_FAMILY(vdnt8);
GPIO_BUTTON_BOARD(vdnt8, vdnt8, "VDNT-8");

static struct gpio_keys_button gpio_buttons_vdnt3[] = {
    {
        .desc		= "reset",
        .gpio		= 32,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_0,
    },
    {
        .desc		= "wlan_eco",
        .gpio		= 36,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_1,
    },
    {
        .desc		= "info",
        .gpio		= 37,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_3,
    },
    {
        .desc		= "wps",
        .gpio		= 22,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_4,
    }
};


GPIO_BUTTON_FAMILY(vdnt3);
GPIO_BUTTON_BOARD(vdnt3, vdnt3, "VDNT-3");

static struct gpio_keys_button gpio_buttons_vdnt4[] = {
    {
        .desc           = "reset",
        .gpio           = 32,
        .active_low     = 1,
        .type           = EV_KEY,
        .code           = BTN_0,
    },
    {
        .desc           = "wps",
        .gpio           = 33,
        .active_low     = 1,
        .type           = EV_KEY,
        .code           = BTN_4,
    }
};


GPIO_BUTTON_FAMILY(vdnt4);
GPIO_BUTTON_BOARD(vdnt4, vdnt4, "VDNT-4");

static struct gpio_keys_button gpio_buttons_vantf[] = {
    {
        .desc		= "reset",
        .gpio		= 20,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_0,
    },
    {
        .desc		= "info",
        .gpio		= 18,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_3,
    },
    {
        .desc		= "dect_pair",
        .gpio		= 36,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_2,
    },
    {
        .desc		= "wlan_eco",
        .gpio		= 35,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_1,
    },
    {
        .desc		= "wps",
        .gpio		= 34,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_4,
    }
};

GPIO_BUTTON_FAMILY(vantf);
GPIO_BUTTON_BOARD(vantf, vantf, "VANT-F");
GPIO_BUTTON_BOARD(vantf, vantr, "VANT-R");
GPIO_BUTTON_BOARD(vantf, gbntg, "GBNT-G");
GPIO_BUTTON_BOARD(vantf, vant5, "VANT-5");
GPIO_BUTTON_BOARD(vantf, vbnta, "VBNT-A");

static struct gpio_keys_button gpio_buttons_vbntb[] = {
    {
        .desc           = "reset",
        .gpio           = 20,
        .active_low     = 1,
        .type           = EV_KEY,
        .code           = BTN_0,
    },
    {
        .desc           = "wps",
        .gpio           = 33,
        .active_low     = 1,
        .type           = EV_KEY,
        .code           = BTN_4,
    },
    {
        .desc           = "dect_pair",
        .gpio           = 35,
        .active_low     = 1,
        .type           = EV_KEY,
        .code           = BTN_2,
    },
};

GPIO_BUTTON_FAMILY(vbntb);
GPIO_BUTTON_BOARD(vbntb, vbntb, "VBNT-B");

static struct gpio_keys_button gpio_buttons_vantg[] = {
    {
        .desc		= "reset",
        .gpio		= 32,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_0,
    },
    {
        .desc		= "wps",
        .gpio		= 34,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_4,
    },
    {
        .desc		= "wlan_eco",
        .gpio		= 35,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_1,
    },
};

GPIO_BUTTON_FAMILY(vantg);
GPIO_BUTTON_BOARD(vantg, vantg, "VANT-G");

static struct gpio_keys_button gpio_buttons_vant9[] = {
    {
        .desc		= "reset",
        .gpio		= 32,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_0,
    },
    {
        .desc		= "wps",
        .gpio		= 34,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_4,
    }
};

GPIO_BUTTON_FAMILY(vant9);
GPIO_BUTTON_BOARD(vant9, vant9, "VANT-9");

static struct gpio_keys_button gpio_buttons_vantd[] = {
    {
        .desc		= "reset",
        .gpio		= 20,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_0,
    },
    {
        .desc		= "info",
        .gpio		= 18,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_3,
    },
    {
        .desc		= "wlan_eco",
        .gpio		= 35,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_1,
    },
    {
        .desc		= "wps",
        .gpio		= 34,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_4,
    }
};

GPIO_BUTTON_FAMILY(vantd);
GPIO_BUTTON_BOARD(vantd, vantd, "VANT-D");
GPIO_BUTTON_BOARD(vantd, vante, "VANT-E");
GPIO_BUTTON_BOARD(vantd, vant4, "VANT-4");
GPIO_BUTTON_BOARD(vantd, vant6, "VANT-6");
GPIO_BUTTON_BOARD(vantd, vant7, "VANT-7");
GPIO_BUTTON_BOARD(vantd, vant8, "VANT-8");
GPIO_BUTTON_BOARD(vantd, vbntl, "VBNT-L");
GPIO_BUTTON_BOARD(vantd, vbntt, "VBNT-T");

static struct gpio_keys_button gpio_buttons_vantc[] = {
    {
        .desc		= "reset",
        .gpio		= 32,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_0,
    },
    {
        .desc		= "wps",
        .gpio		= 35,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_4,
    },
    {
        .desc		= "wlan_eco",
        .gpio		= 33,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_1,
    }
};

GPIO_BUTTON_FAMILY(vantc);
GPIO_BUTTON_BOARD(vantc, vantc, "VANT-C");

static struct gpio_keys_button gpio_buttons_gant1[] = {
    {
        .desc		= "reset",
        .gpio		= 33,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_0,
    },
    {
        .desc		= "wps",
        .gpio		= 35,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_4,
    },
    {
        .desc		= "wlan_eco",
        .gpio		= 34,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_1,
    }
};

GPIO_BUTTON_FAMILY(gant1);
GPIO_BUTTON_BOARD(gant1, gant1, "GANT-1");
GPIO_BUTTON_BOARD(gant1, gant2, "GANT-2");
GPIO_BUTTON_BOARD(gant1, gant8, "GANT-8");

static struct gpio_keys_button gpio_buttons_vantt[] = {
    {
        .desc		= "reset",
        .gpio		= 32,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_0,
    },
    {
        .desc		= "wps",
        .gpio		= 33,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_4,
    }
};

GPIO_BUTTON_FAMILY(vantt);
GPIO_BUTTON_BOARD(vantt, vantt, "VANT-T");
GPIO_BUTTON_BOARD(vantt, vbntn, "VBNT-N");


/*wps button and wifi button are switched on board plane comparing with the HW schematic*/
static struct gpio_keys_button gpio_buttons_vdnty[] = {
    {
        .desc		= "reset",
        .gpio		= 20,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_0,
    },
    {
        .desc		= "wps",
        .gpio		= 35,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_4,
    },
    {
        .desc		= "wlan_eco",
        .gpio		= 34,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_1,
    }
};

GPIO_BUTTON_FAMILY(vdnty);
GPIO_BUTTON_BOARD(vdnty, vdnty, "VDNT-Y");

static struct gpio_keys_button gpio_buttons_gantk[] = {
    {
        .desc           = "reset",
        .gpio           = 32,
        .active_low     = 1,
        .type           = EV_KEY,
        .code           = BTN_0,
    },
    {
        .desc           = "wlan_eco",
        .gpio           = 36,
        .active_low     = 1,
        .type           = EV_KEY,
        .code           = BTN_1,
    },
    {
        .desc           = "info",
        .gpio           = 37,
        .active_low     = 1,
        .type           = EV_KEY,
        .code           = BTN_3,
    },
    {
        .desc           = "wps",
        .gpio           = 22,
        .active_low     = 1,
        .type           = EV_KEY,
        .code           = BTN_4,
    }
};

GPIO_BUTTON_FAMILY(gantk);
GPIO_BUTTON_BOARD(gantk, gantk, "GANT-K");

static struct board * boards[] = {&board_vantc, &board_vantd, &board_vante, &board_vant4, &board_vantf, &board_vantg, &board_vant9,&board_vantr, &board_gbntg, &board_vant5, &board_vbnta, &board_vant6, &board_vbntr, &board_vant7, &board_vant8, &board_vdnt3, &board_vdnt4, &board_vdntw, &board_vdnto, &board_vdnt6, &board_vdnt8, &board_gant1, &board_gant2,  &board_gant8, &board_vantt, &board_vdnty, &board_gantk, &board_vbntl, &board_vbntb, &board_vbntn, &board_vbntt,0};

#elif defined(CONFIG_BCM96362)

static struct gpio_keys_button gpio_buttons_dantu[] = {
 {
 .desc = "reset",
 .gpio = 24,
 .active_low = 1,
 .type = EV_KEY,
 .code = BTN_0,
 },
 {
 .desc = "wlan_eco",
 .gpio = 10,
 .active_low = 1,
 .type = EV_KEY,
 .code = BTN_1,
 },
 {
 .desc = "wps",
 .gpio = 25,
 .active_low = 1,
 .type = EV_KEY,
 .code = BTN_4,
 },
 {
 .desc = "info",
 .gpio = 11,
 .active_low = 1,
 .type = EV_KEY,
 .code = BTN_3,
 }
};

GPIO_BUTTON_FAMILY(dantu);
GPIO_BUTTON_BOARD(dantu, dantu, "DANT-U");

static struct gpio_keys_button gpio_buttons_danto[] = {
 {
 .desc = "reset",
 .gpio = 24,
 .active_low = 1,
 .type = EV_KEY,
 .code = BTN_0,
 },
 {
 .desc = "wlan_eco",
 .gpio = 29,
 .active_low = 1,
 .type = EV_KEY,
 .code = BTN_1,
 },
 {
 .desc = "wps",
 .gpio = 25,
 .active_low = 1,
 .type = EV_KEY,
 .code = BTN_4,
 },
 {
 .desc = "dect_pair",
 .gpio = 33,
 .active_low = 1,
 .type = EV_KEY,
 .code = BTN_2,
 }
};

GPIO_BUTTON_FAMILY(danto);
GPIO_BUTTON_BOARD(danto, danto, "DANT-O");

static struct gpio_keys_button gpio_buttons_danty[] = {
    {
        .desc           = "reset",
        .gpio           = 24,
        .active_low     = 1,
        .type           = EV_KEY,
        .code           = BTN_0,
    },
    {
        .desc           = "wps",
        .gpio           = 31,
        .active_low     = 1,
        .type           = EV_KEY,
        .code           = BTN_4,
    },
    {
        .desc           = "wlan_eco",
        .gpio           = 25,
        .active_low     = 1,
        .type           = EV_KEY,
        .code           = BTN_1,
    },
    {
        .desc           = "info",
        .gpio           = 2,
        .active_low     = 1,
        .type           = EV_KEY,
        .code           = BTN_3,
    }
};

GPIO_BUTTON_FAMILY(danty);
GPIO_BUTTON_BOARD(danty, danty, "DANT-Y");

static struct board * boards[] = {&board_dantu, &board_danto, &board_danty, 0};

#elif defined(CONFIG_BCM96318)

static struct gpio_keys_button gpio_buttons_dant7[] = {
    {
        .desc		= "reset",
        .gpio		= 34,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_0,
    },
    {
        .desc		= "wps",
        .gpio		= 47,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_4,
    },
    {
        .desc		= "wlan_eco",
        .gpio		= 33,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_1,
    }
};

GPIO_BUTTON_FAMILY(dant7);
GPIO_BUTTON_BOARD(dant7, dant7, "DANT-7");
GPIO_BUTTON_BOARD(dant7, dant8, "DANT-8");
GPIO_BUTTON_BOARD(dant7, dant8_isdn, "DANT-8_ISDN");
GPIO_BUTTON_BOARD(dant7, dant9, "DANT-9");
GPIO_BUTTON_BOARD(dant7, vantm, "VANT-M");
GPIO_BUTTON_BOARD(dant7, vantu, "VANT-U");

static struct board * boards[] = {&board_dant7, &board_dant8, &board_dant8_isdn, &board_dant9, &board_vantm, &board_vantu, 0};

#elif defined(CONFIG_BCM968500)

static struct gpio_keys_button gpio_buttons_gantj[] = {
    {
        .desc		= "reset",
        .gpio		= 2,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_0,
    },
    {
        .desc		= "wlan_eco",
        .gpio		= 36,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_1,
    },
    {
        .desc		= "wps",
        .gpio		= 41,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_2,
    },
    {
        .desc		= "info",
        .gpio		= 1,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_3,
    }
};

GPIO_BUTTON_FAMILY(gantj);
GPIO_BUTTON_BOARD(gantj, gantj, "GANT-J");
GPIO_BUTTON_BOARD(gantj, gant3, "GANT-3");


static struct board * boards[] = {&board_gantj, &board_gant3, 0};

#elif defined(CONFIG_BCM96838)

static struct gpio_keys_button gpio_buttons_gantu[] = {
    {
        .desc		= "reset",
        .gpio		= 48,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_0,
    },
    {
        .desc		= "wlan_eco",
        .gpio		= 1,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_1,
    },
    {
        .desc		= "wps",
        .gpio		= 2,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_2,
    },	
    {
        .desc		= "info",
        .gpio		= 3,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_3,
    }
};

GPIO_BUTTON_FAMILY(gantu);
GPIO_BUTTON_BOARD(gantu, gantu, "GANT-U");
GPIO_BUTTON_BOARD(gantu, ganth, "GANT-H");

static struct board * boards[] = {&board_ganth, &board_gantu, 0};

#elif defined(CONFIG_BCM963138)

static struct gpio_keys_button gpio_buttons_gbnta[] = {
    {
        .desc		= "reset",
        .gpio		= 32,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_0,
    },
    {
        .desc		= "enter",
        .gpio		= 34,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_1,
    },
    {
        .desc		= "left",
        .gpio		= 35,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_2,
    },
    {
        .desc		= "right",
        .gpio		= 36,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_3,
    }
};

static struct gpio_keys_button gpio_buttons_vanth[] = {
    {
        .desc		= "reset",
        .gpio		= 32,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_0,
    },
    {
        .desc		= "wlan_eco",
        .gpio		= 34,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_1,
    },
    {
        .desc		= "dect_pair",
        .gpio		= 37,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_2,
    },
    {
        .desc		= "info",
        .gpio		= 35,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_3,
    },
    {
        .desc		= "wps",
        .gpio		= 33,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_4,
    }
};

static struct gpio_keys_button gpio_buttons_vanty[] = {
    {
        .desc		= "reset",
        .gpio		= 32,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_0,
    },
    {
        .desc		= "wlan_eco",
        .gpio		= 35,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_1,
    },
    {
        .desc		= "dect_pair",
        .gpio		= 37,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_2,
    },
    {
        .desc		= "wps",
        .gpio		= 33,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_4,
    }
};

static struct gpio_keys_button gpio_buttons_vbntg[] = {
    {
        .desc		= "reset",
        .gpio		= 32,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_0,
    },
    {
        .desc		= "wps",
        .gpio		= 33,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_4,
    },
    {
        .desc		= "wlan_eco",
        .gpio		= 35,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_1,
    }
};

static struct gpio_keys_button gpio_buttons_vbnto[] = {
    {
        .desc		= "reset",
        .gpio		= 32,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_0,
    },
    {
        .desc		= "wlan",
        .gpio		= 37,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_1,
    },
    {
        .desc		= "wps",
        .gpio		= 35,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_2,
    },
    {
        .desc		= "line",
        .gpio		= 34,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_3,
    }
};

GPIO_BUTTON_FAMILY(gbnta);
GPIO_BUTTON_BOARD(gbnta, gbnta, "GBNT-A");

GPIO_BUTTON_FAMILY(vbntg);
GPIO_BUTTON_BOARD(vbntg, vbntg, "VBNT-G");
GPIO_BUTTON_BOARD(vbntg, vbntm, "VBNT-M");

GPIO_BUTTON_FAMILY(vanth);
GPIO_BUTTON_BOARD(vanth, vanth, "VANT-H");
GPIO_BUTTON_BOARD(vanth, vantw, "VANT-W");
GPIO_BUTTON_BOARD(vanth, vantv, "VANT-V");
GPIO_BUTTON_BOARD(vanth, vbntf, "VBNT-F");
GPIO_BUTTON_BOARD(vanth, vbntk, "VBNT-K");
GPIO_BUTTON_BOARD(vanth, vbnts, "VBNT-S");
GPIO_BUTTON_BOARD(vanth, vbntk_sfp, "VBNT-K_SFP");
GPIO_BUTTON_BOARD(vanth, vbnts_sfp, "VBNT-S_SFP");
GPIO_BUTTON_BOARD(vanth, vbnth, "VBNT-H");

GPIO_BUTTON_FAMILY(vanty);
GPIO_BUTTON_BOARD(vanty, vanty, "VANT-Y");
GPIO_BUTTON_BOARD(vanty, vbntj, "VBNT-J");
GPIO_BUTTON_BOARD(vanty, vbntv, "VBNT-V");

GPIO_BUTTON_FAMILY(vbnto);
GPIO_BUTTON_BOARD(vbnto, vbnto, "VBNT-O");
GPIO_BUTTON_BOARD(vbnto, vbnto_sfp, "VBNT-O_SFP");

static struct board * boards[] = {&board_vanth, &board_vantw, &board_vantv, &board_vbntf, &board_vbntk, &board_vbnts, &board_vbntk, &board_vbnts, &board_vbntk_sfp, &board_vbnts_sfp, &board_vbnth, &board_vanty,&board_vbntj, &board_gbnta, &board_vbntg, &board_vbntm, &board_vbnto,&board_vbnto_sfp, &board_vbntv,0};

#elif defined(CONFIG_BCM963381)

static struct gpio_keys_button gpio_buttons_vanto[] = {
    {
        .desc           = "reset",
        .gpio           = 20,
        .active_low     = 1,
        .type           = EV_KEY,
        .code           = BTN_0,
    },
    {
        .desc           = "wlan_eco",
        .gpio           = 18,
        .active_low     = 1,
        .type           = EV_KEY,
        .code           = BTN_1,
    },
    {
        .desc           = "wps",
        .gpio           = 21,
        .active_low     = 1,
        .type           = EV_KEY,
        .code           = BTN_4,
    }
};


GPIO_BUTTON_FAMILY(vanto);
GPIO_BUTTON_BOARD(vanto, vanto, "VANT-O");
GPIO_BUTTON_BOARD(vanto, vantn, "VANT-N");
GPIO_BUTTON_BOARD(vanto, vantp, "VANT-P");
GPIO_BUTTON_BOARD(vanto, vantq, "VANT-Q");

static struct gpio_keys_button gpio_buttons_vantz[] = {
    {
        .desc           = "reset",
        .gpio           = 20,
        .active_low     = 1,
        .type           = EV_KEY,
        .code           = BTN_0,
    },
    {
        .desc           = "wlan_eco",
        .gpio           = 34,
        .active_low     = 1,
        .type           = EV_KEY,
        .code           = BTN_1,
    },
    {
        .desc           = "wps",
        .gpio           = 21,
        .active_low     = 1,
        .type           = EV_KEY,
        .code           = BTN_4,
    }
};

GPIO_BUTTON_FAMILY(vantz);
GPIO_BUTTON_BOARD(vantz, vantz, "VANT-Z");
GPIO_BUTTON_BOARD(vantz, vant1, "VANT-1");
GPIO_BUTTON_BOARD(vantz, vant2, "VANT-2");
GPIO_BUTTON_BOARD(vantz, vant3, "VANT-3");

static struct gpio_keys_button gpio_buttons_vbnti[] = {
    {
        .desc           = "reset",
        .gpio           = 20,
        .active_low     = 1,
        .type           = EV_KEY,
        .code           = BTN_0,
    },
    {
        .desc           = "wlan_eco",
        .gpio           = 19,
        .active_low     = 1,
        .type           = EV_KEY,
        .code           = BTN_1,
    },
    {
        .desc           = "wps",
        .gpio           = 21,
        .active_low     = 1,
        .type           = EV_KEY,
        .code           = BTN_4,
    }
};

GPIO_BUTTON_FAMILY(vbnti);
GPIO_BUTTON_BOARD(vbnti, vbnti, "VBNT-I");

static struct board * boards[] = {&board_vanto, &board_vantn, &board_vantp, &board_vantq, &board_vant2, &board_vant3, &board_vantz, &board_vant1, &board_vbnti, 0};

#elif defined(CONFIG_ARCH_COMCERTO)

static struct gpio_keys_button gpio_buttons_gantn[] = {
    {
        .desc		= "reset",
        .gpio		= 1,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_0,
    },
    {
        .desc		= "wlan_eco",
        .gpio		= 5,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_1,
    },
    {
        .desc		= "wps",
        .gpio		= 0,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_2,
    }
};

GPIO_BUTTON_FAMILY(gantn);
GPIO_BUTTON_BOARD(gantn, gantn, "GANT-N");
GPIO_BUTTON_BOARD(gantn, c2kevm, "C2KEVM");
GPIO_BUTTON_BOARD(gantn, gant5, "GANT-5");


static struct board * boards[] = {&board_gant5, &board_c2kevm, &board_gantn, 0};


#elif defined(CONFIG_AG71XX)


static struct gpio_keys_button gpio_buttons_gantv[] = {
    {
        .desc		= "reset",
        .gpio		= 16,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_0,
    },
    {
        .desc		= "wlan_eco",
        .gpio		= 11,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_1,
    },
    {
        .desc		= "wps",
        .gpio		= 18,
        .active_low	= 1,
        .type		= EV_KEY,
        .code		= BTN_2,
    },	
};


GPIO_BUTTON_FAMILY(gantv);
GPIO_BUTTON_BOARD(gantv, gantv, "GANT-V");
GPIO_BUTTON_BOARD(gantv, gantw, "GANT-W");

static struct board * boards[] = {&board_gantv, &board_gantw, 0};

#elif defined(CONFIG_ARCH_MDM9607)

static struct gpio_keys_button gpio_buttons_gbntd[] = {
    {
        .desc		= "wps",
        .type		= EV_KEY,
        .code		= BTN_0,
        .gpio		= 26,
        .active_low	= 1
    }
};


GPIO_BUTTON_FAMILY(gbntd);
GPIO_BUTTON_BOARD(gbntd, gbntd, "GBNT-D");

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
