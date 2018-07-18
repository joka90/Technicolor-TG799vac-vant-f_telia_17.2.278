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

# remove optimization for debug 
#EXTRA_CFLAGS += -O0 -g
obj-y	 := rdp_bbh_arrays.o       \
            rdp_drv_bbh.o          \
            rdp_drv_bpm.o          \
            rdp_drv_ih.o           \
            rdp_drv_sbpm.o         \
            rdp_runner_arrays.o    \
            rdp_dma_arrays.o       \

EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/rdp
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/rdp/base
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)
ifeq ($(strip $(BRCM_CHIP)),63138)
EXTRA_CFLAGS += -DDSL_63138
endif
ifeq ($(strip $(BRCM_CHIP)),63148)
EXTRA_CFLAGS += -DDSL_63148
endif


-include $(TOPDIR)/Rules.make

clean:
	rm -f core *.o *.a *.s
