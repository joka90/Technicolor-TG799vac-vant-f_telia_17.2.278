# File: bcmdrivers/broadcom/opensource/char/rdpa_gpl/impl1/Makefile
#
# Makefile for RDPA GPL driver
#

obj-$(CONFIG_BCM_RDPA_GPL)	 := rdpa_gpl.o
SRCS     := $(subst rdpa_gpl.mod.c,,$(notdir $(shell ls $(src)/*.c)))
OBJS 	 := $(SRCS:.c=.o)
rdpa_gpl-objs = $(OBJS)

ifeq ("$(CONFIG_BCM96838)" ,"y")
	PLATFORM=oren
	EXTRA_INCLUDES += -I$(INC_BRCMSHARED_PUB_PATH)/rdp -I$(INC_BRCMSHARED_PUB_PATH)/rdp/base
else ifeq ("$(CONFIG_BCM963138)" ,"y")
	PLATFORM=dsl
	EXTRA_INCLUDES += -I$(INC_BRCMSHARED_PUB_PATH)/rdp -I$(INC_BRCMSHARED_PUB_PATH)/rdp/base -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)
else ifeq ("$(CONFIG_BCM963148)" ,"y")
	PLATFORM=dsl
	EXTRA_INCLUDES += -I$(INC_BRCMSHARED_PUB_PATH)/rdp -I$(INC_BRCMSHARED_PUB_PATH)/rdp/base -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)
else
	PLATFORM=sim
endif

EXTRA_INCLUDES += -I$(INC_BDMF_PATH)/framework -I$(INC_BDMF_PATH)/system -I$(INC_BDMF_PATH)/system/linux -I$(INC_BDMF_PATH)/system/linux/ce
EXTRA_INCLUDES += -I$(INC_BDMF_PATH)/platform/$(PLATFORM)
EXTRA_INCLUDES += -I$(src)/include -I$(src)/include/$(PLATFORM)
EXTRA_INCLUDES += -I$(INC_BRCMSHARED_PUB_PATH)/bcm963xx
EXTRA_CFLAGS = -Werror $(EXTRA_INCLUDES) $(EXTRA_DEFINES)
EXTRA_CFLAGS += -include rdpa_api.h

-include $(TOPDIR)/Rules.make

clean:
	rm -rf `find . -name '*.[o,d]'`
	rm -rf `find . -name '*~'`
	rm -rf `find . -name '*.ko'`
	rm -rf rdpa_gpl.mod.c modules.order Module.symvers .tmp_versions
	rm -rf `find . -name '.*.cmd'`
	rm -rf rdpa
