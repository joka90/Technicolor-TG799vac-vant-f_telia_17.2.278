# File: bcmdrivers/broadcom/net/enet/bcm963xx/Makefile
#
# Makefile for the BCM63xx Enet/switch driver
#

bcm_enet-objs += bcmenet.o
bcm_enet-objs += ethsw.o
bcm_enet-objs += bcmsw.o
bcm_enet-objs += ../shared/bcmswaccess.o
bcm_enet-objs += eth_pwrmngt.o
bcm_enet-objs += ethsw_phy.o
bcm_enet-objs += bcmenet_shared_src.o 

EXTRA_CFLAGS += -I$(SHARED_DIR)/opensource

# Runner based platform specific object files
ifneq ($(strip $(RDPA_PLATFORM)),)
bcm_enet-objs +=  ethsw_phy_runner.o bcmenet_runner.o ethsw_runner.o bcmsw_runner.o
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/rdp -I$(INC_BRCMSHARED_PUB_PATH)/rdp/base
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PRIV_PATH)/bcm963xx
ifeq ("$(RDPA_PLATFORM)", "oren")
	EXTRA_CFLAGS += -DOREN
	EXTRA_CFLAGS += -DBCMENET_RUNNER_CPU_RING
endif


else
bcm_enet-objs += ../shared/bcmswaccess_dma.o
bcm_enet-objs += ../shared/bcmproc.o
bcm_enet-objs += ethsw_phy_dma.o
bcm_enet-objs += bcmenet_dma.o
bcm_enet-objs += ethsw_dma.o
bcm_enet-objs += bcmsw_dma.o
endif

EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)

ifneq ($(strip $(CONFIG_BCM_GMAC)),)
bcm_enet-objs += bcmgmac.o
endif

ifneq ($(strip $(CONFIG_BCM_TM)),)
ifeq ($(strip $(BRCM_CHIP)),63381)
EXTRA_CFLAGS += -D_CONFIG_ENET_BCM_TM
endif
endif
ifneq ($(strip $(BUILD_ETHTOOL)),)
bcm_enet-objs += ../shared/bcmenet_ethtool.o
endif

ifneq ($(strip $(CONFIG_BCM_ENET)),)
obj-$(CONFIG_BCM_ENET) = bcm_enet.o
endif
ifeq ($(strip $(BRCM_CHIP)),6838)
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/drv/phys -I$(INC_BRCMSHARED_PUB_PATH)/drv/phys/egphy -I$(INC_BRCMSHARED_PUB_PATH)/drv/phys/extphy
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/drv
else ifeq ($(findstring _$(strip $(BRCM_CHIP))_,_63138_63148_),_$(strip $(BRCM_CHIP))_)
EXTRA_CFLAGS += -DSTAR_FIGHTER2
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/drv -I$(INC_BRCMSHARED_PUB_PATH)/pmc
endif

ifeq ($(findstring _$(strip $(BRCM_CHIP))_,_63138_),_$(strip $(BRCM_CHIP))_)
EXTRA_CFLAGS += -DACB_ALGORITHM2
endif

ifeq ($(strip $(BRCM_CHIP)),63381)
EXTRA_CFLAGS += -I$(SHARED_DIR)/opensource/include/pmc
endif

#-DDUMP_DATA  -- dump data
#-DUSE_BH     -- use bottom half
#-DUSE_PROC   -- display ethernet driver internal info
#-DUSE_EMAC   -- need MDC/MDIO functions
#-DBCM_ENET_RX_LOG   -- Enable rx debug log
#-DBCM_ENET_LOG      -- Enable debug log
#-DBCM_ENET_TX_LOG   -- Enable tx debug log
#-DBCM_ENET_LINK_LOG   -- Enable link debug log
#-DBCM_ENET_DEBUG_BUILD   -- Enable Debug Build
#-DRXCHANNEL_BYTE_RATE_LIMIT -- Enable rx byte rate limiting of a channel
#-DRXCHANNEL_PKT_RATE_LIMIT -- Enable rx pkt rate limiting of a channel
#-DVLAN_TAG_FFF_STRIP -- Enable Stripping of 0xFFF tagged packets
#-DUSE_DEFAULT_EGRESS_QUEUE -- Enable use of default egress queue
#-DSEPARATE_MAC_FOR_WAN_INTERFACES -- Enable separate MAC addresses for WAN ifs

ifneq ($(strip $(BUILD_HELLO)),)
# Enable support for HELLO Packet CMF
EXTRA_CFLAGS += -DSUPPORT_HELLO
endif

ifneq ($(strip $(BUILD_SWMDK)),)
EXTRA_CFLAGS += -DSUPPORT_SWMDK
endif

ifneq ($(strip $(BUILD_ETHTOOL)),)
EXTRA_CFLAGS += -DSUPPORT_ETHTOOL
endif

ifneq ($(strip $(BUILD_ETHWAN)),)
EXTRA_CFLAGS += -DCONFIG_ETHWAN
endif

ifneq ($(strip $(BUILD_BRCM_FTTDP)),)
EXTRA_CFLAGS += -DBRCM_FTTDP -DG9991
EXTRA_CFLAGS += -DRDPA_VPORTS
endif

EXTRA_CFLAGS += -DUSE_BH
EXTRA_CFLAGS += -DUSE_VPORTS
EXTRA_CFLAGS += -DEXPORT_SYMTAB
EXTRA_CFLAGS += -Werror
EXTRA_CFLAGS += -g
#EXTRA_CFLAGS +=  -DBCM_ENET_DEBUG_BUILD
#EXTRA_CFLAGS +=  -DBCM_ENET_RX_LOG
#EXTRA_CFLAGS +=  -DBCM_ENET_TX_LOG
EXTRA_CFLAGS +=  -DBCM_ENET_LOG
#EXTRA_CFLAGS +=  -DRXCHANNEL_BYTE_RATE_LIMIT
EXTRA_CFLAGS +=  -DRXCHANNEL_PKT_RATE_LIMIT
EXTRA_CFLAGS +=  -DVLAN_TAG_FFF_STRIP
EXTRA_CFLAGS +=  -DUSE_DEFAULT_EGRESS_QUEUE 
#EXTRA_CFLAGS += -DSEPARATE_MAC_FOR_WAN_INTERFACES
# Stats from HW is supported only from Switch at this time; not from Runner
# This needs to be looked into again in 4.14L.02 when flows will be acceleated/switched by Runner
ifeq ($(strip $(BRCM_CHIP)),6838)
ifneq ($(strip $(CONFIG_BCM_PTP_1588)),)
bcm_enet-objs += bcmenet_ptp_1588.o
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/pmc -I$(INC_RDPA_PATH)/gpon_stack
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/broadcom/char/time_sync/impl$(CONFIG_BCM_TIME_SYNC_IMPL)
endif
endif
EXTRA_CFLAGS += -DREPORT_HARDWARE_STATS
#EXTRA_CFLAGS += -DDUMP_DATA
#EXTRA_CFLAGS += -DNON_LINUX_BUILD
ifneq ($(strip $(BRCM_CHIP)),68500)
EXTRA_CFLAGS += -DNO_CFE
endif
# BCM_ENET_CB_WAN_PORT_LNX_INTF_SUPPORT must only be enabled when customer is trying to connect
# different type of WAN interface (like Ethernet and MoCA) and need to have these two represeted 
# as different Linux interfaces to configure the WAN service
#EXTRA_CFLAGS += -DBCM_ENET_CB_WAN_PORT_LNX_INTF_SUPPORT
ifneq ($(strip $(BRCM_CONFIG_HIGH_RES_TIMERS)),)
EXTRA_CFLAGS += -DMOCA_HIGH_RES_TX
endif

ifneq ($(strip $(BRCM_SWITCH_SCHED_SP)),)
EXTRA_CFLAGS += -DBCM_SWITCH_SCHED_SP
endif
ifneq ($(strip $(BRCM_SWITCH_SCHED_WRR)),)
EXTRA_CFLAGS += -DBCM_SWITCH_SCHED_WRR
endif
ifneq ($(strip $(BUILD_ETHTOOL)),)
EXTRA_CFLAGS += -DSUPPORT_ETHTOOL
endif


EXTRA_CFLAGS += -I$(obj) -I$(obj)/../shared
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PRIV_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMBOARDPARMS_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/opensource/include/$(BRCM_BOARD)
ifeq ($(CONFIG_BCM_XTMRT_IMPL),1)
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/opensource/net/xtmrt/impl3
else
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/opensource/net/xtmrt/impl$(CONFIG_BCM_XTMRT_IMPL)
endif
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/broadcom/char/fap/impl$(CONFIG_BCM_FAP_IMPL)
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/broadcom/char/fap/impl$(CONFIG_BCM_FAP_IMPL)/4ke
EXTRA_CFLAGS += -I$(KERNEL_DIR)/net/core
# FIXME, restore following conditional to be CONFIG_BCM_RDPA when RDPA is also built.
ifneq ($(strip $(RDPA_PLATFORM)),)
EXTRA_CFLAGS += $(EXTRA_CFLAGS_RDPA)
endif

ifneq ($(strip $(CONFIG_BCM_WLAN)),)
EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/broadcom/net/wl/bcm9$(BRCM_CHIP)/include
EXTRA_CFLAGS += -DDSLCPE -DBCMDRIVER -DPKTC
endif

-include $(TOPDIR)/Rules.make

clean:
	rm -f core *.o *.a *.s

