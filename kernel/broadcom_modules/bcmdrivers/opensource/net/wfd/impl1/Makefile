#
# <:copyright-BRCM:2012:DUAL/GPL:standard
# 
#    Copyright (c) 2012 Broadcom Corporation
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

# File: bcmdrivers/opensource/net/wifi_mw/impl1/Makefile
#
# Makefile for the wifi MW
#
MODULE_NAME=wfd

$(MODULE_NAME)-objs += wfd_dev.o

obj-$(CONFIG_BCM_WIFI_FORWARDING_DRV) += $(MODULE_NAME).o

EXTRA_CFLAGS += -I$(obj)
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/broadcom/char/fap/impl$(CONFIG_BCM_FAP_IMPL)
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/broadcom/char/fap/impl$(CONFIG_BCM_FAP_IMPL)/4ke
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/opensource/net/xtmrt/impl$(CONFIG_BCM_XTMRT_IMPL) 
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PRIV_PATH)/$(BRCM_BOARD) 

EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/rdp -I$(INC_BRCMSHARED_PUB_PATH)/rdp/base

EXTRA_INCLUDES  += -I$(INC_RDPA_PATH) -I$(INC_RDPA_PATH)/autogen \
    -I$(INC_BDMF_PATH)/framework -I$(INC_BDMF_PATH)/system -I$(INC_BDMF_PATH)/system/$(CONFIG_SYSTEM) \
    -I$(INC_BDMF_PATH)/shell -I$(INC_BDMF_PATH)/platform/$(RDPA_PLATFORM) \
    -I$(INC_BRCMSHARED_PUB_PATH)/bcm963xx -I$(INC_BRCMDRIVER_PUB_PATH)/bcm963xx -I$(BRCMDRIVERS_DIR)/opensource/net/enet/shared
    
ifneq ($(strip $(CONFIG_BCM_WLAN)),)
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/include

ifneq ($(CONFIG_BCM_WFD_CHAIN_SUPPORT),)
EXTRA_CFLAGS += -DCONFIG_BCM_WFD_CHAIN_SUPPORT -DPKTC -DDSLCPE_PKTC -DDSLCPE
endif
endif

ifneq ($(strip $(CONFIG_BCM_WFD_RX_ACCELERATION)),)
EXTRA_CFLAGS += -DCONFIG_BCM_WFD_RX_ACCELERATION
endif

EXTRA_CFLAGS += $(EXTRA_CFLAGS_RDPA) $(EXTRA_INCLUDES) -Werror -Wall -g

ifeq ($(strip $(CONFIG_TCH_KF_PREALLOC_WL)),y)
KBUILD_CFLAGS_MODULE += -mno-long-calls
endif

-include $(TOPDIR)/Rules.make

clean:
	rm -f core *.o *.a *.s .*.cmd *.ko


