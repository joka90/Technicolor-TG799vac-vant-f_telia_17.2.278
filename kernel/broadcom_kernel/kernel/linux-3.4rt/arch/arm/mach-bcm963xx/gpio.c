#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/gpio.h>
#include <linux/module.h>


#include <bcm_map_part.h>

#define GPIO_DIR_OUT 0x0
#define GPIO_DIR_IN	0x1

extern spinlock_t bcm_gpio_spinlock;

static int bcm963xx_set_direction(struct gpio_chip *chip, unsigned gpio, int dir)
{
	volatile uint32 *reg;
	unsigned long flags;
	uint32_t mask = (1 << gpio % 32);

	if (gpio / 32 > sizeof(GPIO->GPIODir) / sizeof(GPIO->GPIODir[0])) {
		printk("GPIO access to %d out of range\n", gpio);
		return -1;
	}
	reg = &GPIO->GPIODir[gpio / 32];

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
	volatile uint32 *reg;

	if (gpio / 32 > sizeof(GPIO->GPIODir) / sizeof(GPIO->GPIODir[0])) {
		printk("GPIO access to %d out of range\n", gpio);
		return -1;
	}
	reg = &GPIO->GPIOio[gpio / 32];

	gpio = 1 << (gpio % 32);

	return (*reg & gpio) != 0;
}

static void bcm963xx_set(struct gpio_chip *chip, unsigned gpio, int value)
{
	volatile uint32 *reg;
	unsigned long flags;

	if (gpio / 32 > sizeof(GPIO->GPIODir) / sizeof(GPIO->GPIODir[0])) {
		printk("GPIO access to %d out of range\n", gpio);
		return;
	}
	reg = &GPIO->GPIOio[gpio / 32];

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

static int __init bcm963xx_gpio_init(void)
{
	int ret=1;

	ret = gpiochip_add(&bcm963xx_gpiochip);
	if (ret < 0) {
		printk("Failed to register BCM963xx GPIO chip\n");
	}
	return ret;
}

subsys_initcall(bcm963xx_gpio_init);

