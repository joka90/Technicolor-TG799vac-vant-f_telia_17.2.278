#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/gpio.h>
#include <linux/module.h>

#if defined(CONFIG_BCM963268)
#include <63268_map_part.h>
#elif defined(CONFIG_BCM96318)
#include <6318_map_part.h>
#elif defined(CONFIG_BCM96368)
#include <6368_map_part.h>
#elif defined(CONFIG_BCM96362)
#include <6362_map_part.h>
#elif defined(CONFIG_BCM96328)
#include <6328_map_part.h>
#elif defined(CONFIG_BCM963381)
#include <63381_map_part.h>
#elif defined(CONFIG_BCM96838)
#include <6838_map_part.h>
#else
#error Chipset not yet supported by the GPIO driver
#endif

#if !defined(CONFIG_BCM963381) && !defined(CONFIG_BCM96838)
/*
** Gpio Controller
*/
typedef struct BcmGpioControl {
	uint32_t      GPIODir_high;
	uint32_t      GPIODir;
	uint32_t      GPIOio_high;
	uint32_t      GPIOio;
#if defined(CONFIG_BCM96362) || defined(CONFIG_BCM963268)
        uint32_t      LEDCtrl;
        uint32_t      SpiSlaveCfg;
        uint32_t      GPIOMode;
        uint32_t      GPIOCtrl;
#endif
} BcmGpioControl;

#ifdef GPIO
#undef GPIO 
#endif

/* map our own struct on top of GPIO registers to avoid using 64bit regs */
#define GPIO ((volatile BcmGpioControl * const) GPIO_BASE)
#endif

#define GPIO_DIR_OUT 0x0
#define GPIO_DIR_IN  0x1

#if defined(CONFIG_BCM96838)
#define GPIO_NUM_MAX 74
#endif

extern spinlock_t bcm_gpio_spinlock;

static int bcm963xx_set_direction(struct gpio_chip *chip, unsigned gpio, int dir)
{
	unsigned long flags;
	uint32_t mask = (1 << gpio % 32);

#if defined(CONFIG_BCM963381)
        volatile uint32 *reg;
        if (gpio / 32 > sizeof(GPIO->GPIODir) / sizeof(GPIO->GPIODir[0])) {
                printk("GPIO access to %d out of range\n", gpio);
                return -1;
        }
        reg = &GPIO->GPIODir[gpio / 32];
#elif defined(CONFIG_BCM96838)
        volatile uint32 *reg;
        if(gpio < 32)
            reg = &GPIO->GPIODir_low;
        else if(gpio < 64)
            reg = &GPIO->GPIODir_mid0;
        else
            reg = &GPIO->GPIODir_mid1;
#else
        volatile uint32_t *reg;
	if (gpio >= 32)
		reg = &GPIO->GPIODir_high;
	else
		reg = &GPIO->GPIODir;
#endif

        spin_lock_irqsave(&bcm_gpio_spinlock, flags);
        if (dir == GPIO_DIR_IN)
                *reg &= ~mask;
        else
                *reg |= mask;
        spin_unlock_irqrestore(&bcm_gpio_spinlock, flags);
	return 0;
}

static int bcm963xx_get(struct gpio_chip *chip, unsigned gpio)
{
#if defined(CONFIG_BCM963381)
        volatile uint32 *reg;
        if (gpio / 32 > sizeof(GPIO->GPIODir) / sizeof(GPIO->GPIODir[0])) {
                printk("GPIO access to %d out of range\n", gpio);
                return -1;
        }
        reg = &GPIO->GPIOio[gpio / 32];
#elif defined(CONFIG_BCM96838)
        volatile uint32 *reg;
        if(gpio < 32)
            reg = &GPIO->GPIOData_low;
        else if(gpio < 64)
            reg = &GPIO->GPIOData_mid0;
        else
            reg = &GPIO->GPIOData_mid1;
#else
        volatile uint32_t *reg;
	if (gpio >= 32)
		reg = &GPIO->GPIOio_high;
	else
		reg = &GPIO->GPIOio;
#endif
	gpio = 1 << (gpio % 32);

	return (*reg & gpio) != 0;
}

static void bcm963xx_set(struct gpio_chip *chip, unsigned gpio, int value)
{
	unsigned long flags;

#if defined(CONFIG_BCM963381)
        volatile uint32 *reg;
        if (gpio / 32 > sizeof(GPIO->GPIODir) / sizeof(GPIO->GPIODir[0])) {
                printk("GPIO access to %d out of range\n", gpio);
                return;
        }
        reg = &GPIO->GPIOio[gpio / 32];
#elif defined(CONFIG_BCM96838)
        volatile uint32 *reg;
        if(gpio < 32)
            reg = &GPIO->GPIOData_low;
        else if(gpio < 64)
            reg = &GPIO->GPIOData_mid0;
        else
            reg = &GPIO->GPIOData_mid1;
#else
        volatile uint32_t *reg;
#if defined(CONFIG_BCM96362) || defined(CONFIG_BCM963268)
        /* Take over high GPIOs from WLAN block */
        if (gpio > 31)
            GPIO->GPIOCtrl |= (1 << (gpio - 32));
#endif
	if (gpio >= 32)
		reg = &GPIO->GPIOio_high;
	else
		reg = &GPIO->GPIOio;
#endif
	gpio = 1 << (gpio % 32);

	spin_lock_irqsave(&bcm_gpio_spinlock, flags);
	if (value)
		*reg |= gpio;
	else
		*reg &= ~gpio;
	spin_unlock_irqrestore(&bcm_gpio_spinlock, flags);
}

static int bcm963xx_direction_in(struct gpio_chip *chip, unsigned offset)
{
#if defined(CONFIG_BCM96362) || defined(CONFIG_BCM963268)
        /* Take over high GPIOs from WLAN block */
        if (offset > 31)
            GPIO->GPIOCtrl |= (1 << (offset- 32));
#endif
	return bcm963xx_set_direction(chip, offset, GPIO_DIR_IN);
}

static int bcm963xx_direction_out(struct gpio_chip *chip, unsigned gpio, int value)
{
	bcm963xx_set(chip, gpio, value);
	return bcm963xx_set_direction(chip, gpio, GPIO_DIR_OUT);
}

static struct gpio_chip bcm963xx_gpiochip = {
	.label			= "bcm963xx-gpio",
	.owner			= THIS_MODULE,

	.get                    = bcm963xx_get,
	.set                    = bcm963xx_set,
	.direction_input        = bcm963xx_direction_in,
	.direction_output       = bcm963xx_direction_out,
	.base                   = 0,
	.ngpio                  = GPIO_NUM_MAX,
	.can_sleep              = 0
};

int gpio_to_irq(unsigned gpio)
{
	return __gpio_to_irq(gpio);
}
EXPORT_SYMBOL_GPL(gpio_to_irq);

static int __init bcm963xx_gpio_init(void)
{
	int ret=1;

	ret = gpiochip_add(&bcm963xx_gpiochip);
	if (ret < 0) {
		printk("Failed to register BCM963268 GPIO chip\n");
	}
	return ret;
}

subsys_initcall(bcm963xx_gpio_init);

