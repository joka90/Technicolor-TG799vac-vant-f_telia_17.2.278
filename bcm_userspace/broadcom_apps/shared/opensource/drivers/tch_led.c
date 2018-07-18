
#include "boardparms.h"
#include "bcm_led.h"
#include "bcm_pinmux.h"
#include "shared_utils.h"
#include "tch_led.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <bcm_map_part.h>
#include <linux/string.h>
#include <linux/spinlock.h>

/* Spinlock to prevent concurrent access to the GPIO registers */
static DEFINE_SPINLOCK(bcm63xx_tch_led_lock);


/**
 * API to set leds
 *
 *  led: ID of the TCH led
 *  state: 1 means on, 1 means off
 *
 *  returns: 0 if SUCCESS, -1 otherwise
 */
int tch_led_set(enum tch_leds led, unsigned short state) {
	unsigned short gpio;
	unsigned long flags;

	if (BpGetLedGpio_tch(led,  &gpio) != BP_SUCCESS ) {
		printk("Failed to set led %d value %d\n", led, state);
		return -1;
	}

	spin_lock_irqsave(&bcm63xx_tch_led_lock, flags);
	bcm_led_driver_set( gpio, state );
	spin_unlock_irqrestore(&bcm63xx_tch_led_lock, flags);
	return 0;
}
EXPORT_SYMBOL(tch_led_set);

/**
 * API to determine whether a certain LED is available on this platform
 *
 *  led: ID of the TCH led
 *
 *  returns: 1 if available, 0 otherwise
 */
int tch_led_is_available(enum tch_leds led) {
	unsigned short gpio;

	return (BpGetLedGpio_tch(led,  &gpio) == BP_SUCCESS );
}

EXPORT_SYMBOL(tch_led_is_available);

