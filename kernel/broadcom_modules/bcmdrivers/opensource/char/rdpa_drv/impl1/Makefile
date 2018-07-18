#
# <:copyright-BRCM:2013:DUAL/GPL:standard
# 
#    Copyright (c) 2013 Broadcom Corporation
#    All Rights Reserved
# 
# Unless you and Broadcom execute a separate written software license
# agreement governing use of this software, this software is licensed
# to you under the terms of the GNU General Public License version 2
# (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
# with the following added to such license:
# 
#    As a special exception, the copyright holders of this software give
#    you permission to link this software with independent modules, and
#    to copy and distribute the resulting executable under terms of your
#    choice, provided that you also meet, for each linked independent
#    module, the terms and conditions of the license of that module.
#    An independent module is a module which is not derived from this
#    software.  The special exception does not apply to any modifications
#    of the software.
# 
# Not withstanding the above, under no circumstances may you combine
# this software in any way with any other Broadcom software provided
# under a license other than the GPL, without Broadcom's express prior
# written consent.
# 
# :> 
#
# Makefile for the RDPA CMD
#
ifeq ("$(CONFIG_BCM963138)" ,"y")
obj-$(CONFIG_BCM_RDPA_DRV) := rdpa_cmd_drv.o rdpa_cmd_tm.o rdpa_cmd_ds_wan_udp_filter.o
else ifeq ("$(CONFIG_BCM963148)" ,"y")
obj-$(CONFIG_BCM_RDPA_DRV) := rdpa_cmd_drv.o rdpa_cmd_tm.o rdpa_cmd_ds_wan_udp_filter.o
else
obj-$(CONFIG_BCM_RDPA_DRV) := rdpa_cmd_drv.o rdpa_cmd_tm.o rdpa_cmd_iptv.o rdpa_cmd_ic.o rdpa_cmd_sys.o rdpa_cmd_port.o	rdpa_cmd_br.o rdpa_cmd_llid.o
endif
obj-$(CONFIG_BCM_RDPA_DRV) += rdpa_cmd_spdsvc.o

SRCS := $(subst rdpa_drv.mod.c,,$(notdir $(shell ls $(src)/*.c)))
OBJS := $(SRCS:.c=.o)
rdpa_drv-objs += $(OBJS)


ifeq ("$(CONFIG_BCM96838)" ,"y")
	CONFIG_PLATFORM=oren
	EXTRA_INCLUDES += -I$(INC_BRCMSHARED_PUB_PATH)/rdp
	EXTRA_INCLUDES += -I$(INC_BRCMSHARED_PUB_PATH)/rdp/base
endif
ifeq ("$(CONFIG_BCM963138)" ,"y")
	CONFIG_PLATFORM=dsl
	EXTRA_CFLAGS += -DDSL_63138
	EXTRA_INCLUDES += -I$(INC_BRCMSHARED_PUB_PATH)/rdp
	EXTRA_INCLUDES += -I$(INC_BRCMSHARED_PUB_PATH)/rdp/base
endif
ifeq ("$(CONFIG_BCM963148)" ,"y")
	CONFIG_PLATFORM=dsl
	EXTRA_CFLAGS += -DDSL_63148
	EXTRA_INCLUDES += -I$(INC_BRCMSHARED_PUB_PATH)/rdp
	EXTRA_INCLUDES += -I$(INC_BRCMSHARED_PUB_PATH)/rdp/base
endif

EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/opensource/net/enet/shared
EXTRA_CFLAGS += -I$(INC_BDMF_PATH)/platform/$(CONFIG_PLATFORM) -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/opensource/char/rdpa_gpl/impl1/include/autogen
EXTRA_CFLAGS += $(EXTRA_INCLUDES) $(EXTRA_CFLAGS_RDPA)
EXTRA_CFLAGS += -Werror -Wall

-include $(TOPDIR)/Rules.make

clean:
	rm -f core *.o *.a *.s .*.cmd *.ko *.mod.c modules.*

