/*
 * <:copyright-BRCM:2014:DUAL/GPL:standard
 * 
 *    Copyright (c) 2014 Broadcom Corporation
 *    All Rights Reserved
 * 
 * Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed
 * to you under the terms of the GNU General Public License version 2
 * (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
 * with the following added to such license:
 * 
 *    As a special exception, the copyright holders of this software give
 *    you permission to link this software with independent modules, and
 *    to copy and distribute the resulting executable under terms of your
 *    choice, provided that you also meet, for each linked independent
 *    module, the terms and conditions of the license of that module.
 *    An independent module is a module which is not derived from this
 *    software.  The special exception does not apply to any modifications
 *    of the software.
 * 
 * Not withstanding the above, under no circumstances may you combine
 * this software in any way with any other Broadcom software provided
 * under a license other than the GPL, without Broadcom's express prior
 * written consent.
 * 
 * :>
 */

#include "boardparms.h"
#include "bcm_led.h"
#include "bcm_gpio.h"

#ifdef _CFE_
#include "lib_types.h"
#include "lib_printf.h"
#include "lib_string.h"
#include "bcm_map.h"
#define printk  printf
#else // Linux
#include <linux/kernel.h>
#include <linux/module.h>
#include <bcm_map_part.h>
#include <linux/string.h>
#endif

// #define BCM_LED_DEBUG 1

/*
  These are low level functions that can be called from CFE or from the Linux board driver
  The Linux board driver handles any necessary locking so these functions should not be called
  directly from elsewhere.
*/

/* This code manages the PowerOn LED in 60333 and the physical GPIOs in
 * functional mode 0 to drive LEDs in software. The PowerOn LED works
 * in a different way than the regular ones: it's driven by the AON and
 * it's not selectable.
 *
 * For the general purpose LEDs (GPIO-driven) functional mode 0 is
 * assumed. This is the default configuration.
 */

/* These are defined the same as in
 * cfe/cfe/board/bcm63xx_ram/include/bcm63xx_util.h
 */
enum {
    LED_OFF,
    LED_ON
};

/* Keeps the current state of the PowerOn LED: [LED_ON/LED_OFF] */
static unsigned short bcm_led_driver_state;

/************************************************************************
 * Name        : BSTIWrite(addr, data)
 *
 * Description : Writes <data> in BSTI address <addr> and waits for the
 *               transaction to complete
 *
 * Parameters  : [IN] addr     - Address to write to
 *               [IN] data     - Data to write
 *
 *************************************************************************/
static void BSTIWrite(unsigned int addr, unsigned short data)
{
    unsigned int bstictrl = 0;
    unsigned int finish = 0;

    bstictrl = (data & BSTI_SER_CTRL_WR_DATA_MASK);
    bstictrl |= ((addr << BSTI_SER_CTRL_ADDR_SHIFT) & BSTI_SER_CTRL_ADDR_MASK);
    bstictrl |= ((BSTI_WRITE_OP << BSTI_SER_CTRL_CMD_SHIFT)
                                  & BSTI_SER_CTRL_CMD_MASK);
    bstictrl |= ((BSTI_START_OP << BSTI_SER_CTRL_START_SHIFT)
                                  & BSTI_SER_CTRL_START_MASK);
    BSTI->ser_ctrl = bstictrl;

    do
    {
        finish = (BSTI->ser_ctrl & BSTI_SER_CTRL_START_MASK)
                                >> BSTI_SER_CTRL_START_SHIFT;
    } while (finish == BSTI_START_OP);
}

/**********************************************************************
 * Name        : bcm_led_driver_set
 *
 * Description : Given a GPIO number and a state, configures the GPIO
 *               for OUTPUT and sets it to the value in state [1,0]
 *
 * Parameters  : [IN] num        - GPIO number (physical GPIO)
 *               [IN] state      - state (1: LED_ON, 0: LED_OFF)
 *
 * Notes       : Uses bcm_gpio functions to configure GPIO registers.
 *               The caller function must handle the GPIO spinlock
 *               when calling this function.
 *
 *               Assumes GPIO_num is in functional mode 0 (not muxed).
 *********************************************************************/
void bcm_led_driver_set(unsigned short num, unsigned short state)
{
#ifdef BCM_LED_DEBUG
    printk("LED %x set state %d\n",num,state);
#endif
    if (num == BP_PIN_AON_POWER) {
        /* AON-driven LED */
        if (state != bcm_led_driver_state) {
            bcm_led_driver_state = state;
            if (state == LED_ON) {
                BSTIWrite(AON_REGISTERS_LED_EN, 1);
            } else {
                BSTIWrite(AON_REGISTERS_LED_EN, 0);
            }
        }
    }
    else {
        /* Regular GPIO-driven LED */
        bcm_gpio_set_dir(num, 1);
        bcm_gpio_set_data(num, state);
    }
}

/**********************************************************************
 * Name        : bcm_led_driver_toggle
 *
 * Description : Given a GPIO number toggles its value
 *
 * Parameters  : [IN] pin_num      - GPIO number (physical GPIO)
 *
 * Notes       : Uses bcm_gpio functions to configure GPIO registers.
 *               The caller function must handle the GPIO spinlock
 *               when calling this function.
 *
 *               Assumes GPIO_num is in functional mode 0 (not muxed).
 *********************************************************************/
void bcm_led_driver_toggle(unsigned short num)
{
    if (num == BP_PIN_AON_POWER) {
        if (bcm_led_driver_state == LED_ON) {
            bcm_led_driver_set(num, LED_OFF);
        } else {
            bcm_led_driver_set(num, LED_ON);
        }
    } else {
        bcm_gpio_set_data(num, 1 ^ bcm_gpio_get_data(num));
    }
}

/**********************************************************************
 * Name        : bcm_common_led_setAllSoftLedsOff
 *
 * Description : Turns off the PowerOn LED
 *********************************************************************/
void bcm_common_led_setAllSoftLedsOff(void)
{
    bcm_led_driver_set(BP_PIN_AON_POWER, LED_OFF);
}

/**********************************************************************
 * Name        : bcm_common_led_init
 *
 * Description : Configures the initial state of the PowerOn LED (OFF)
 *********************************************************************/
void bcm_common_led_init(void) {
    /* Enable Power LED */
    BSTIWrite(AON_REGISTERS_LED_EN, 0);
    bcm_led_driver_state = LED_OFF;

    /* Configure LED duty cycle: 100% of time ON when led is enabled */
    BSTIWrite(AON_REGISTERS_LED0_ON, 0xFFFF);  /* LED on (lower half-word) */
    BSTIWrite(AON_REGISTERS_LED1_ON, 0xFFFF);  /* LED on (upper half-word) */
    BSTIWrite(AON_REGISTERS_LED0_OFF, 0);      /* LED off (lower half-word) */
    BSTIWrite(AON_REGISTERS_LED1_OFF, 0);      /* LED off (upper half-word) */
}
