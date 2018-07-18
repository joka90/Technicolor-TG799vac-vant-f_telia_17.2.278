# File: bcmdrivers/Makefile
#
# Makefile for the Linux kernel modules.
#
export LN_NAME=bcm9$(BRCM_CHIP)
LN_DRIVER_DIRS :=
CLEAN_DRIVER_DIRS :=

-include $(KERNEL_DIR)/.config

export BCMARCH=$(subst ",,$(CONFIG_BCM_CPU_ARCH_NAME))
#"#

# All kernel drivers are instrumented, when BOUNCE is configured for datapath.
ifdef CONFIG_BRCM_BOUNCE_DATAPATH
ifeq ($(SUBLEVEL),30)
	EXTRA_CFLAGS  += -O0 -finstrument-functions
else
	ccflags-y  += -O0 -finstrument-functions
endif
endif

EXTRA_CFLAGS += ${DSL_XTM_SAR_TX_RX_IUDMA_631XX_DEFINES}

LN_TARGS := 

#---------------------------------
define LN_RULE
# params:
#   1: config name
#   2: dir
#   3: ln name
#   4: impl number

ifneq (,$(strip $(1)))

obj-$(strip $(1)) += $(strip $(2))/$(strip $(3))/

LN_TARGS += $(strip $(2))/$(strip $(3))

$(strip $(2))/$(strip $(3)) :
	@echo "linking $$@ => impl$(strip $(4))";
	$$(Q)ln -sfn impl$(strip $(4)) $$@;

endif
endef

#$(info   )
#$(info EXAMPLE RULE: -----------------------------)
#$(info $(call LN_RULE, $(CONFIG_BCM_BOARD), opensource/char/board/$(BRCM_BOARD), $(LN_NAME), $(CONFIG_BCM_BOARD_IMPL)))
#$(info -----------------------------)
#$(info   )

#---------------------------------

#
# To create a new driver, add a line below, which has the following format:
#
# $(eval $(call LN_RULE, CONFIG_OPTION, DIRECTORY, LINK_NAME, IMPL))
#
# where:
#  CONFIG_OPTION is the config option (such as "$(CONFIG_BCM_ADSL)"), which determines
#    whether or not the driver will be compiled
#  DIRECTORY is the directory (relative to bcmdrivers) where all the implX subdirectories 
#    reside
#  LINK_NAME should be $(LN_NAME)
#  IMPL is the config option which defines the implementation number the driver should use
#    (for example "$(CONFIG_BCM_ADSL_IMPL)"
#


$(eval $(call LN_RULE, $(CONFIG_BCM_BOARD), opensource/char/board/$(BRCM_BOARD), $(LN_NAME), $(CONFIG_BCM_BOARD_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_CHIPINFO), broadcom/char/chipinfo, $(LN_NAME), $(CONFIG_BCM_CHIPINFO_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_SERIAL), opensource/char/serial, $(LN_NAME), $(CONFIG_BCM_SERIAL_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_EXT_TIMER), opensource/char/timer, $(LN_NAME), $(CONFIG_BCM_EXT_TIMER_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_PKTFLOW), broadcom/char/pktflow, $(LN_NAME), $(CONFIG_BCM_PKTFLOW_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_FBOND), broadcom/char/fbond, $(LN_NAME), $(CONFIG_BCM_FBOND_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_PKTCMF), broadcom/char/pktcmf, $(LN_NAME), $(CONFIG_BCM_PKTCMF_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_FAP), broadcom/char/fap, $(LN_NAME), $(CONFIG_BCM_FAP_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_TM), broadcom/net/tm, $(LN_NAME), $(CONFIG_BCM_TM_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_PKTDMA), opensource/net/lib/pktdma, $(LN_NAME), $(CONFIG_BCM_PKTDMA_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_PKTRUNNER), broadcom/char/pktrunner, $(LN_NAME), $(CONFIG_BCM_PKTRUNNER_IMPL)))

ifneq ($(CONFIG_BCM_XTMRT),)
$(eval $(call LN_RULE, $(CONFIG_BCM_XTMRT), opensource/net/xtmrt,$(LN_NAME), $(CONFIG_BCM_XTMRT_IMPL)))
endif
ifneq ($(CONFIG_BCM_XTMCFG),)
$(eval $(call LN_RULE, $(CONFIG_BCM_XTMCFG), broadcom/char/xtmcfg,$(LN_NAME), $(CONFIG_BCM_XTMCFG_IMPL)))
endif

$(eval $(call LN_RULE, $(CONFIG_BCM_EXT_BONDING), broadcom/char/ext_bonding, $(LN_NAME), $(CONFIG_BCM_EXT_BONDING_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_ADSL), broadcom/char/adsl, $(LN_NAME), $(CONFIG_BCM_ADSL_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_PROCFS), broadcom/char/bcmprocfs, $(LN_NAME), $(CONFIG_BCM_PROCFS_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_ENDPOINT), broadcom/char/endpoint, $(LN_NAME), $(CONFIG_BCM_ENDPOINT_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_BCMPROF), broadcom/char/profiler, $(LN_NAME), $(CONFIG_BCM_BCMPROF_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_BCMDSP), broadcom/char/dspapp, $(LN_NAME), $(CONFIG_BCM_BCMDSP_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_DECT), broadcom/char/dect, $(LN_NAME), $(CONFIG_BCM_DECT_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_DECTSHIM), opensource/char/dectshim, $(LN_NAME), $(CONFIG_BCM_DECTSHIM_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_PCMSHIM), opensource/char/pcmshim, $(LN_NAME), $(CONFIG_BCM_PCMSHIM_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_TMS), broadcom/char/tms, $(LN_NAME), $(CONFIG_BCM_TMS_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_RDPA_BRIDGE), opensource/net/bridge, $(LN_NAME), $(CONFIG_BCM_RDPA_BRIDGE_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_ENET), opensource/net/enet, $(LN_NAME), $(CONFIG_BCM_ENET_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_GPON), broadcom/char/gpon, $(LN_NAME), $(CONFIG_BCM_GPON_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_LASER), broadcom/char/laser, $(LN_NAME), $(CONFIG_BCM_LASER_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_PMD), broadcom/char/pmd, $(LN_NAME), $(CONFIG_BCM_PMD_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_MoCA), opensource/char/moca, $(LN_NAME), $(CONFIG_BCM_MoCA_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_I2C_BUS), opensource/char/i2c/busses, $(LN_NAME), $(CONFIG_BCM_I2C_BUS_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_I2C_CHIP), opensource/char/i2c/chips, $(LN_NAME), $(CONFIG_BCM_I2C_CHIP_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_VLAN), broadcom/char/vlan, $(LN_NAME), $(CONFIG_BCM_VLAN_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_EPON), broadcom/char/epon, $(LN_NAME), $(CONFIG_BCM_EPON_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_USB), broadcom/net/usb, $(LN_NAME), $(CONFIG_BCM_USB_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_ISDN), broadcom/char/isdn, $(LN_NAME), $(CONFIG_BCM_ISDN_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_SPU), opensource/char/spudd, $(LN_NAME), $(CONFIG_BCM_SPU_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_PWRMNGT), broadcom/char/pwrmngt, $(LN_NAME), $(CONFIG_BCM_PWRMNGT_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_TRNG), opensource/char/trng, $(LN_NAME), $(CONFIG_BCM_TRNG_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_INGQOS), broadcom/char/ingqos, $(LN_NAME), $(CONFIG_BCM_INGQOS_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_BPM), broadcom/char/bpm, $(LN_NAME), $(CONFIG_BCM_BPM_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_ARL), broadcom/char/arl, $(LN_NAME), $(CONFIG_BCM_ARL_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_BDMF), opensource/char/bdmf, $(LN_NAME), $(CONFIG_BCM_BDMF_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_RDPA), broadcom/char/rdpa, $(LN_NAME), $(CONFIG_BCM_RDPA_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_RDPA_GPL), opensource/char/rdpa_gpl, $(LN_NAME), $(CONFIG_BCM_RDPA_GPL_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_GPON_STACK), broadcom/char/runner_gpon_stack, $(LN_NAME), $(CONFIG_BCM_GPON_STACK_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_EPON_STACK), broadcom/char/xenu_epon_stack, $(LN_NAME), $(CONFIG_BCM_EPON_STACK_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_RDPA_MW), opensource/char/rdpa_mw, $(LN_NAME), $(CONFIG_BCM_RDPA_MW_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_RDPA_DRV), opensource/char/rdpa_drv, $(LN_NAME), $(CONFIG_BCM_RDPA_DRV_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_WIFI_FORWARDING_DRV), opensource/net/wfd, $(LN_NAME), $(CONFIG_BCM_WIFI_FORWARDING_DRV_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_SIM_CARD), opensource/char/sim_card, $(LN_NAME), $(CONFIG_BCM_SIM_CARD_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_TIME_SYNC), broadcom/char/time_sync, $(LN_NAME), $(CONFIG_BCM_TIME_SYNC_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_OTP), broadcom/char/otp, $(LN_NAME), $(CONFIG_BCM_OTP_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_DPI), broadcom/char/dpiengine, $(LN_NAME), $(CONFIG_BCM_DPI_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_DPI), opensource/char/dpi, $(LN_NAME), $(CONFIG_BCM_DPI_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_BT_SERIAL), opensource/char/bt_serial, $(LN_NAME), $(CONFIG_BCM_BT_SERIAL_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_I2S), opensource/char/i2s, $(LN_NAME), $(CONFIG_BCM_I2S_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_SATA_TEST), broadcom/char/sata_test, $(LN_NAME), $(CONFIG_BCM_SATA_TEST_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_BLUETOOTH_USB), opensource/char/btusb, $(LN_NAME), $(CONFIG_BCM_BLUETOOTH_USB_IMPL)))
$(eval $(call LN_RULE, $(CONFIG_BCM_HS_UART), opensource/char/hs_uart, $(LN_NAME), $(CONFIG_BCM_HS_UART_IMPL)))

ifneq ($(CONFIG_BCM_WLAN),)

ifeq ($(strip $(WL)),)
ifneq ($(CONFIG_BCM_WLAN_IMPL_SELECT),"")
   WL=IMPL$(patsubst "%",%,$(CONFIG_BCM_WLAN_IMPL_SELECT))
endif
endif

ifneq ($(WL),)
   CONFIG_BCM_WLAN_IMPL=$(patsubst IMPL%,%,$(WL))  
endif

$(eval $(call LN_RULE, $(CONFIG_BCM_WLAN), broadcom/net/wl,$(LN_NAME), $(CONFIG_BCM_WLAN_IMPL)))

endif


ifeq ($(CONFIG_BCM_PKTCMF_IMPL),2)
  CLEAN_DRIVER_DIRS += broadcom/char/pktcmf/impl$(CONFIG_BCM_PKTCMF_IMPL)
endif

ifneq ($(CONFIG_BCM_DECT),)
  CLEAN_DRIVER_DIRS += broadcom/char/dect/impl$(CONFIG_BCM_DECT_IMPL)
endif


.PHONY : applinks symlinks clean version_info

applinks:
ifneq ($(CONFIG_BCM_PKTCMF),)
ifeq ($(CONFIG_BCM_PKTCMF_IMPL),2)
	$(MAKE) -C $(BRCMDRIVERS_DIR)/broadcom/char/pktcmf/impl$(CONFIG_BCM_PKTCMF_IMPL) applinks
endif
endif


ifneq ($(MAKECMDGOALS),version_info)
find_opt := -print
endif

ifneq ($(strip $(BUILD_BRCM_FTTDP)),)
RDD_PLATFORM:=_G9991
endif
symlinks: applinks $(LN_TARGS)
ifneq ($(CONFIG_BCM_RDPA),)
	@mkdir -p $(BRCMDRIVERS_DIR)/broadcom/char/rdpa/impl$(CONFIG_BCM_RDPA_IMPL)/rdp/rdd
	@$(HOSTTOOLS_DIR)/make_links.sh $(RDP_PATH)/impl$(CONFIG_BCM_RDP_IMPL)/rdd $(BRCMDRIVERS_DIR)/broadcom/char/rdpa/impl$(CONFIG_BCM_RDPA_IMPL)/rdp/rdd
	@$(HOSTTOOLS_DIR)/make_links.sh $(SHARED_DIR)/opensource/rdp $(BRCMDRIVERS_DIR)/broadcom/char/rdpa/impl$(CONFIG_BCM_RDPA_IMPL)/rdp rdp_cpu_ring.c
ifeq ($(CONFIG_BCM96838),y)
	@mkdir -p $(BRCMDRIVERS_DIR)/broadcom/char/rdpa/impl$(CONFIG_BCM_RDPA_IMPL)/rdp/firmware_oren$(RDD_PLATFORM)
	@$(HOSTTOOLS_DIR)/make_links.sh $(RDP_PATH)/impl$(CONFIG_BCM_RDP_IMPL)/firmware_oren$(RDD_PLATFORM) $(BRCMDRIVERS_DIR)/broadcom/char/rdpa/impl$(CONFIG_BCM_RDPA_IMPL)/rdp/firmware_oren$(RDD_PLATFORM)
	@$(HOSTTOOLS_DIR)/make_links.sh $(SHARED_DIR)/opensource/drv/dpi $(BRCMDRIVERS_DIR)/broadcom/char/rdpa/impl$(CONFIG_BCM_RDPA_IMPL)/platform/$(RDPA_PLATFORM) oren_data_path_init.c
endif
ifeq ($(CONFIG_BCM963138),y)
	@mkdir -p $(BRCMDRIVERS_DIR)/broadcom/char/rdpa/impl$(CONFIG_BCM_RDPA_IMPL)/rdp/firmware_dsl_63138$(IS_REV_A)
	@$(HOSTTOOLS_DIR)/make_links.sh $(RDP_PATH)/impl$(CONFIG_BCM_RDP_IMPL)/firmware_dsl_63138$(IS_REV_A) $(BRCMDRIVERS_DIR)/broadcom/char/rdpa/impl$(CONFIG_BCM_RDPA_IMPL)/rdp/firmware_dsl_63138$(IS_REV_A)
	-@$(HOSTTOOLS_DIR)/make_links.sh $(SHARED_DIR)/opensource/drv/dpi $(BRCMDRIVERS_DIR)/broadcom/char/rdpa/impl$(CONFIG_BCM_RDPA_IMPL)/platform/$(RDPA_PLATFORM) bcm63138_data_path_init.c
endif
ifeq ($(CONFIG_BCM963148),y)
	@mkdir -p $(BRCMDRIVERS_DIR)/broadcom/char/rdpa/impl$(CONFIG_BCM_RDPA_IMPL)/rdp/firmware_dsl_63148
	@$(HOSTTOOLS_DIR)/make_links.sh $(RDP_PATH)/impl$(CONFIG_BCM_RDP_IMPL)/firmware_dsl_63148 $(BRCMDRIVERS_DIR)/broadcom/char/rdpa/impl$(CONFIG_BCM_RDPA_IMPL)/rdp/firmware_dsl_63148
	-@$(HOSTTOOLS_DIR)/make_links.sh $(SHARED_DIR)/opensource/drv/dpi $(BRCMDRIVERS_DIR)/broadcom/char/rdpa/impl$(CONFIG_BCM_RDPA_IMPL)/platform/$(RDPA_PLATFORM) bcm63138_data_path_init.c
endif
endif
	@echo "done bcmdriver links"

cleanlinks:
	@echo "cleaning bcmdriver links"
	@for dir in $(LN_TARGS); do \
		rm -f $$dir; \
	done
ifneq ($(CONFIG_BCM_RDPA),)
	@rm $(BRCMDRIVERS_DIR)/broadcom/char/rdpa/impl$(CONFIG_BCM_RDPA_IMPL)/rdp/rdp_cpu_ring.c
	@rm -rf $(BRCMDRIVERS_DIR)/broadcom/char/rdpa/impl$(CONFIG_BCM_RDPA_IMPL)/rdp/rdd
ifeq ($(CONFIG_BCM968500),y)
	@rm $(BRCMDRIVERS_DIR)/broadcom/char/rdpa/impl$(CONFIG_BCM_RDPA_IMPL)/rdp/firmware_$(RDPA_PLATFORM)/*
	@rmdir $(BRCMDRIVERS_DIR)/broadcom/char/rdpa/impl$(CONFIG_BCM_RDPA_IMPL)/rdp/firmware_$(RDPA_PLATFORM)
endif
ifeq ($(CONFIG_BCM96838),y)
	@rm $(BRCMDRIVERS_DIR)/broadcom/char/rdpa/impl$(CONFIG_BCM_RDPA_IMPL)/platform/$(RDPA_PLATFORM)/oren_data_path_init.c
	@rm $(BRCMDRIVERS_DIR)/broadcom/char/rdpa/impl$(CONFIG_BCM_RDPA_IMPL)/rdp/firmware_oren/*
	@rmdir $(BRCMDRIVERS_DIR)/broadcom/char/rdpa/impl$(CONFIG_BCM_RDPA_IMPL)/rdp/firmware_oren
endif
ifeq ($(CONFIG_BCM963138),y)
	-@rm $(BRCMDRIVERS_DIR)/broadcom/char/rdpa/impl$(CONFIG_BCM_RDPA_IMPL)/platform/$(RDPA_PLATFORM)/bcm63138_data_path_init.c
	@rm $(BRCMDRIVERS_DIR)/broadcom/char/rdpa/impl$(CONFIG_BCM_RDPA_IMPL)/rdp/firmware_dsl_63138$(IS_REV_A)/*
	@rmdir $(BRCMDRIVERS_DIR)/broadcom/char/rdpa/impl$(CONFIG_BCM_RDPA_IMPL)/rdp/firmware_dsl_63138$(IS_REV_A)
endif
ifeq ($(CONFIG_BCM963148),y)
	-@rm $(BRCMDRIVERS_DIR)/broadcom/char/rdpa/impl$(CONFIG_BCM_RDPA_IMPL)/platform/$(RDPA_PLATFORM)/bcm63138_data_path_init.c
	@rm $(BRCMDRIVERS_DIR)/broadcom/char/rdpa/impl$(CONFIG_BCM_RDPA_IMPL)/rdp/firmware_dsl_63148/*
	@rmdir $(BRCMDRIVERS_DIR)/broadcom/char/rdpa/impl$(CONFIG_BCM_RDPA_IMPL)/rdp/firmware_dsl_63148
endif
endif

clean:
	@echo -----------------------------------
	@echo Making Clean in bcmdrivers dir
	for dir in $(CLEAN_DRIVER_DIRS) ; do \
		$(MAKE) -C $$dir clean; \
	done

version_info: symlinks
ifneq ($(CONFIG_BCM_WLAN),)
	$(MAKE) -C $(BRCMDRIVERS_DIR)/broadcom/net/wl/$(LN_NAME) version_info
endif

