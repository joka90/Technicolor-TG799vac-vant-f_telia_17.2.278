# File: bcmdrivers/opensource/net/xtmrt/Makefile
#
# Makefile for the BCM63xx bcmxtmrt driver.
#

obj-$(CONFIG_BCM_XTMRT) += bcmxtmrtdrv.o

ifeq "$(BRCM_CHIP)" "6318"
bcmxtmrtdrv-objs := bcmxtmrt.o bcmxtmrtbond.o xtmrt_cfg.o xtmrt_dma.o xtmrt_bpm.o xtmrt_iq.o
endif
ifeq "$(BRCM_CHIP)" "63268"
bcmxtmrtdrv-objs := bcmxtmrt.o bcmxtmrtbond.o xtmrt_cfg.o xtmrt_dma.o xtmrt_bpm.o xtmrt_iq.o
endif
ifeq "$(BRCM_CHIP)" "63138"
ifeq ($(strip $(SUPPORT_631XX_TX_RX_IUDMA)),y)
bcmxtmrtdrv-objs := bcmxtmrt.o bcmxtmrtbond.o xtmrt_cfg.o xtmrt_dma.o xtmrt_bpm.o xtmrt_iq.o
else
bcmxtmrtdrv-objs := bcmxtmrt.o bcmxtmrtbond.o xtmrt_cfg.o xtmrt_runner.o
endif
endif
ifeq "$(BRCM_CHIP)" "63381"
bcmxtmrtdrv-objs := bcmxtmrt.o bcmxtmrtbond.o xtmrt_cfg.o xtmrt_dma.o xtmrt_bpm.o xtmrt_iq.o
endif
ifeq "$(BRCM_CHIP)" "63148"
bcmxtmrtdrv-objs := bcmxtmrt.o bcmxtmrtbond.o xtmrt_cfg.o xtmrt_runner.o
endif

EXTRA_CFLAGS += ${DSL_XTM_SAR_TX_RX_IUDMA_631XX_DEFINES}
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMBOARDPARMS_PATH)/$(BRCM_BOARD) \
-I$(INC_BRCMSHARED_PRIV_PATH)/$(BRCM_BOARD) -I$(INC_BRCMDRIVER_PRIV_PATH)/$(BRCM_BOARD) -DDPERF_MON_BONDING_US -DDPTMBOND_US_PRIO_TRAFFIC_SPLIT
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/opensource/net/enet/shared
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/opensource/net/xtmrt/impl$(CONFIG_BCM_XTMRT_IMPL)
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/opensource/char/rdpa_gpl/impl1/include
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/opensource/char/rdpa_gpl/impl1/include/dsl
EXTRA_CFLAGS += -I$(INC_BDMF_PATH)/framework
EXTRA_CFLAGS += -I$(INC_BDMF_PATH)/platform/dsl
EXTRA_CFLAGS += -I$(INC_BDMF_PATH)/system -I$(INC_BDMF_PATH)/system/linux -I$(INC_BDMF_PATH)/system/linux/ce

ifeq "$(BRCM_CHIP)" "63268"
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/broadcom/char/fap/impl$(CONFIG_BCM_FAP_IMPL)
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/broadcom/char/fap/impl$(CONFIG_BCM_FAP_IMPL)/4ke
endif

ifeq "$(BRCM_CHIP)" "63138"
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/rdp
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/rdp/base
EXTRA_CFLAGS += -DDSL_63138
endif

ifeq "$(BRCM_CHIP)" "63148"
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/rdp
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/rdp/base
EXTRA_CFLAGS += -DDSL_63148
endif

ifneq ($(strip $(BUILD_ETHWAN)),)
EXTRA_CFLAGS += -DCONFIG_ETHWAN
endif

EXTRA_CFLAGS += -Werror -Wfatal-errors
ifeq "$(BRCM_PHY_LOOPBACK)" "y"
EXTRA_CFLAGS += -DPHY_LOOPBACK
endif

EXTRA_CFLAGS += -DDDUMP_DATA -DDFRAGMENT_ALIGNMENT_32BIT_DEBUG

clean:
	rm -f core *.o *.a *.s

