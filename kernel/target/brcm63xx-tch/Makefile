#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/tch/target-custo.mk

BOARD:=brcm63xx-tch
BOARDNAME:=Broadcom BCM63xx based Technicolor platform
FEATURES:=squashfs jffs2 usb atm pci pcmcia
DEFAULT_SUBTARGET:=VDNTO
ifdef CONFIG_PACKAGE_kmod-brcm-5.02L.02
  LINUX_VERSION:=4.1.27
else
  LINUX_VERSION:=3.4.11
endif
MAINTAINER:=

GENERATE_PROFILE=1


FULLDEPTHCLONE:=0

#override these variables if corresponding CONFIG is defined
ifdef CONFIG_KERNEL_GIT_CLONE_TCH
  ifneq ($(CONFIG_KERNEL_GIT_CLONE_TCH),"")
    BRCM_REPOSITORY:=$(CONFIG_KERNEL_GIT_CLONE_TCH)
    FULLDEPTHCLONE:=1
  endif
endif

ifdef CONFIG_KERNEL_GIT_VERSION_TCH
  ifneq ($(CONFIG_KERNEL_GIT_VERSION_TCH),"")
    BRCM_VERSION:=$(CONFIG_KERNEL_GIT_VERSION_TCH)
    FULLDEPTHCLONE:=1
  endif
endif

define Target/Description
	Build firmware images for Technicolor Broadcom based xDSL/routers
endef

include $(INCLUDE_DIR)/target.mk

ifeq ($(BRCM_CHIP),63138)
  CFLAGS:=-Os -pipe -march=armv7-a -mtune=cortex-a9 -fno-caller-saves
endif

ifeq ($(BRCM_CHIP),63148)
  CFLAGS:=-Os -pipe -march=armv7-a -mtune=cortex-a15 -fno-caller-saves
endif


# Set additional kernel make options based on the subtarget and profile.
# Earlier changes to KERNEL_MAKEOPTS are overruled by kernel-defaults.mk.
KERNEL_MAKEOPTS += SUBTARGET=$(SUBTARGET)
KERNEL_MAKEOPTS += $(SUBTARGET_KERNEL_MAKEOPTS)
KERNEL_MAKEOPTS += $(PROFILE_KERNEL_MAKEOPTS)
KERNEL_MAKEOPTS += -f Makefile.openwrt

KERNEL_MAKEOPTS += BRCM_CHIP=$(BRCM_CHIP)
KERNEL_MAKEOPTS += ADSL_PHY_MODE="file"

# ARM platforms also need the profile for kernel builds; we might extend this to all platforms
ifeq ($(GENERATE_PROFILE),1)
KERNEL_MAKEOPTS += PROFILE=$(BOARDNAME)
endif

DEFAULT_PACKAGES := $(filter-out dnsmasq,$(DEFAULT_PACKAGES))
DEFAULT_PACKAGES += dnsmasq-dhcpv6 radvd wide-dhcpv6-client

DEFAULT_PACKAGES += kmod-bcm63xx-tch-enet
DEFAULT_PACKAGES += blitools

define Kernel/PrepareBroadcomProfile
	# Starting with ARM platforms & 4.16L.03, autogenerate the profile from OpenWRT
	if [ $(GENERATE_PROFILE) -eq 1 ]; then \
		mkdir $(KERNEL_BUILD_DIR)/kernel-git/targets/$(BOARDNAME) ;\
		m4 \
		  -D_CHIPSET=$(BRCM_CHIP)\
		  -D_IQCTL=$(CONFIG_PACKAGE_iqctl) \
		  -D_SNOOPCTL=$(CONFIG_PACKAGE_libsnoopctl) \
		  -D_RDPACTL=$(CONFIG_PACKAGE_librdpactl) \
		  -D_FAP=$(CONFIG_PACKAGE_kmod-bcm63xx-tch-fap) \
		  -D_FAPCTL=$(CONFIG_PACKAGE_libfapctl) \
		  -D_ARLCTL=$(CONFIG_PACKAGE_libarlctl) \
		  -D_GMACCTL=$(CONFIG_PACKAGE_gmacctl) \
		  -D_ISDN=$(CONFIG_KERNEL_DSL_ISDN) \
		  -D_BRCM_DHD=$(CONFIG_PACKAGE_kmod-bcm63xx-tch-wireless-dhd) \
		  -D_VOICE=$(CONFIG_PACKAGE_kmod-bcm63xx-tch-endpoint) \
		  -D_BUILD_SPUCTL=$(CONFIG_PACKAGE_spuctl) \
		  -D_SELT=$(CONFIG_PACKAGE_seltctl) \
		  -D_ETHTOOL=$(CONFIG_PACKAGE_ethtool) \
		  -D_SWMDK=$(CONFIG_PACKAGE_swmdk) \
		  -D_WFD=$(CONFIG_PACKAGE_kmod-bcm63xx-tch-wfd) \
		  -D_GFAST=$(CONFIG_KERNEL_DSL_GFAST) \
		  -D_BONDING=$(CONFIG_KERNEL_DSL_BONDING) \
		  -D_HASDSL=$(CONFIG_PACKAGE_kmod-bcm63xx-tch-adsl) \
		  -D_BUILD_BMU=$(CONFIG_PACKAGE_bmud) \
		  -D_BUILD_TMS=$(CONFIG_PACKAGE_tmsctl) \
		  -D_BCMTMCTL=$(CONFIG_PACKAGE_libbcmtm) \
		  -D_BCMNFCLIBS=$(CONFIG_PACKAGE_bcmnfc) \
		  -D_MOCA=$(CONFIG_PACKAGE_kmod-bcm63xx-tch-moca) \
		  -D_DPI=$(CONFIG_PACKAGE_kmod-bcm63xx-tch-dpi) \
		  -D_WLALTBLD=$(CONFIG_KERNEL_BCM_WLALTBLD) \
		  -D_WIRELESS=$(CONFIG_PACKAGE_kmod-bcm63xx-tch-wireless) \
		  $(PLATFORM_DIR)/target_profile.$(subst bcm,,$(CONFIG_KERNEL_GIT_BRANCH)) > $(KERNEL_BUILD_DIR)/kernel-git/targets/$(BOARDNAME)/$(BOARDNAME).autogen ;\
	fi
endef

define Kernel/UpdateBroadcomProfile
	$(call Kernel/PrepareBroadcomProfile)

	# Starting with ARM platforms & 4.16L.03, autogenerate the profile from OpenWRT
	if [ $(GENERATE_PROFILE) -eq 1 ]; then \
		if ! diff $(KERNEL_BUILD_DIR)/kernel-git/targets/$(BOARDNAME)/$(BOARDNAME) $(KERNEL_BUILD_DIR)/kernel-git/targets/$(BOARDNAME)/$(BOARDNAME).autogen;  then \
			rm -f $(STAMP_PREPARED);\
			echo "Broadcom profile has changed; please restart the build !"; \
			echo "$(KERNEL_BUILD_DIR)/kernel-git will be removed automatically !"; \
			echo ""; \
			false; \
		fi ;\
	else \
		rm -rf $(KERNEL_BUILD_DIR)/kernel-git/.last_profile; \
	fi
    ifdef CONFIG_PACKAGE_kmod-brcm-5.02L.02
	   $(MAKE) -C "$(KERNEL_BUILD_DIR)/kernel-git/" PROFILE=$(BOARDNAME) rdp_link bcmdrivers_autogen $(KERNEL_BUILD_DIR)/kernel-git/cms_dmp_flags.h
    endif
endef

define Kernel/Patch
	echo XXXXXXXXXXXXXXXXXXXXXXX $(KERNEL_PATCHVER) XXXXXXXXXXXX
	$(Kernel/Patch/Default)
	$(call PatchDir,$(PKG_BUILD_DIR),$(PLATFORM_SUBDIR)/patches$(if $(wildcard $(PLATFORM_SUBDIR)/patches-$(KERNEL_PATCHVER)),-$(KERNEL_PATCHVER)),platform/)
endef

define Kernel/Prepare
	$(call Kernel/Prepare/Default)
	rm -rf $(KERNEL_BUILD_DIR)/broadcom-git
	ln -s $(KERNEL_BUILD_DIR)/kernel-git $(KERNEL_BUILD_DIR)/broadcom-git
	$(call Kernel/PrepareBroadcomProfile)
	if [ $(GENERATE_PROFILE) -eq 1 ]; then \
		cp $(KERNEL_BUILD_DIR)/kernel-git/targets/$(BOARDNAME)/$(BOARDNAME).autogen $(KERNEL_BUILD_DIR)/kernel-git/targets/$(BOARDNAME)/$(BOARDNAME); \
	fi
	$(call Kernel/UpdateBroadcomProfile)
endef

define Kernel/ConfigureForHeaders
	$(call Kernel/Configure/Default)
endef

define Kernel/CompileModules
	$(call Kernel/UpdateBroadcomProfile)
	$(call Kernel/CompileModules/Default)
endef

define Kernel/CompileImage
	$(call Technicolor/ApplyCusto)
	$(SCRIPT_DIR)/version.sh $(TOPDIR) $(TARGET_DIR)
	$(call Kernel/UpdateBroadcomProfile)
	$(call Kernel/CompileImage/Default)
	$(call Kernel/CompileImage/Initramfs)
endef

ifeq ($(CONFIG_PACKAGE_kmod-bcm63xx-tch-endpoint),y)
    KERNEL_MAKEOPTS += OPENWRT_BUILD_VOICE=1
endif

# Only needed when the PROFILE is not autogenerate by OpenWRT
ifneq ($(GENERATE_PROFILE),1)

# If we want to use swmdk (switch driver in user space), the enet/sw/gmac drivers
# need to compiled differently.
ifeq ($(CONFIG_PACKAGE_swmdk),y)
    KERNEL_MAKEOPTS += BUILD_SWMDK=1
endif

# If we want to use ethtool, the enet driver need to compiled differently, to 
# enable corresponding ioctl SIOCETHTOOL requet support.
ifneq ($(CONFIG_PACKAGE_ethtool),)
    KERNEL_MAKEOPTS += BUILD_ETHTOOL=1
endif

ifeq ($(CONFIG_PACKAGE_kmod-bcm63xx-tch-wireless-dhd), y)
    KERNEL_MAKEOPTS += BRCM_DRIVER_DHD=1
endif

ifeq ($(CONFIG_KERNEL_DSL_ISDN), y)
    KERNEL_MAKEOPTS += BRCM_ANNEXAB_COMBO=y
endif

#Temp for 4.16L02, remove for 4.16L02A
#Patch to support 43602 with internal 63168 WLAN. According to BRCM should only be done on such a board
ifdef CONFIG_PACKAGE_kmod-brcm-4.16L.02_voice4.16L.01
ifeq ($(SUBTARGET), GANT1)
    KERNEL_MAKEOPTS += BRCM_PATCH_KB240901=1
endif
endif
endif

$(eval $(call BuildTarget))
