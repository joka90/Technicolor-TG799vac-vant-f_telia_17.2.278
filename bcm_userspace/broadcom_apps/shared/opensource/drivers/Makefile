

ifeq ($(strip $(CONFIG_BRCM_IKOS)),)

obj-$(CONFIG_BCM963138) += bcm_pinmux_impl1.o
obj-$(CONFIG_BCM963138) += bcm_led_impl1.o
obj-$(CONFIG_BCM963138) += bcm_gpio_impl1.o
obj-$(CONFIG_BCM963138) += bcm_misc_hw_init_impl1.o
obj-$(CONFIG_BCM963138) += tch_led.o

obj-$(CONFIG_BCM963148) += bcm_pinmux_impl1.o
obj-$(CONFIG_BCM963148) += bcm_led_impl3.o
obj-$(CONFIG_BCM963148) += bcm_gpio_impl1.o
obj-$(CONFIG_BCM963148) += bcm_misc_hw_init_impl1.o

obj-$(CONFIG_BCM963381) += bcm_gpio_impl1.o
obj-$(CONFIG_BCM963381) += bcm_pinmux_impl1.o
obj-$(CONFIG_BCM963381) += bcm_led_impl1.o
obj-$(CONFIG_BCM963381) += bcm_misc_hw_init_impl2.o
obj-$(CONFIG_BCM963381) += tch_led.o

obj-$(CONFIG_BCM960333) += bcm_led_impl2.o
obj-$(CONFIG_BCM960333) += bcm_pinmux_impl2.o
obj-$(CONFIG_BCM960333) += bcm_gpio_impl2.o
obj-$(CONFIG_BCM960333) += bcm_misc_hw_init_impl4.o

obj-$(CONFIG_BCM96838) += bcm_misc_hw_init_impl3.o
obj-$(CONFIG_BCM96838) += wan_drv_impl1.o

EXTRA_CFLAGS += -DCONFIG_BCM9$(BRCM_CHIP) -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)  -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)

ifeq ($(strip $(BRCM_CHIP)),6838)
	EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/rdp
	EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/rdp/base
	EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/drv/mdio
	EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/drv/unimac
	EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/drv/phys -I$(INC_BRCMSHARED_PUB_PATH)/drv/phys/egphy -I$(INC_BRCMSHARED_PUB_PATH)/drv/phys/extphy
	EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/pmc  -I$(INC_RDPA_PATH) -I$(INC_BRCMDRIVER_PRIV_PATH)/bcm963xx -I$(INC_BRCMSHARED_PRIV_PATH)/bcm963xx
	EXTRA_CFLAGS += -I$(INC_BDMF_PATH)/framework -I$(INC_BDMF_PATH)/system -I$(INC_BDMF_PATH)/system/linux -I$(INC_BDMF_PATH)/system/linux/ce
endif 

-include $(TOPDIR)/Rules.make

endif

