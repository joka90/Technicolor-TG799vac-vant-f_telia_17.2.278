
# 3.x kernel
ifneq ($(PROFILE_KERNEL_VER),LINUX_2_6_30_RT)

# Linux
obj-y		+= shared_utils.o


EXTRA_CFLAGS += -DCONFIG_BCM9$(BRCM_CHIP) -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)  -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)

-include $(TOPDIR)/Rules.make

else
# 2.6.30 Kernel

# Linux
obj-y           += shared_utils.o


EXTRA_CFLAGS += -DCONFIG_BCM9$(BRCM_CHIP) -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)  -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)

-include $(TOPDIR)/Rules.make

endif
