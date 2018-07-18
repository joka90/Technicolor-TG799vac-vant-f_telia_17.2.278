#ifndef BCM_LED_DEFINES_H__
#define BCM_LED_DEFINES_H__

#include "board_led_defines.h"

const struct board * bcm_led_get_board_description(void);
int bcmled_driver_init(void);
void bcmled_driver_deinit(void);

#endif
