#
# Copyright (c) 2012 Technicolor
# All Rights Reserved
#
# This program contains proprietary information which is a trade
# secret of TECHNICOLOR and/or its affiliates and also is protected as
# an unpublished work under applicable Copyright laws. Recipient is
# to retain this program in confidence and is not permitted to use or
# make copies thereof other than as permitted in a written agreement
# with TECHNICOLOR, UNLESS OTHERWISE EXPRESSLY ALLOWED BY APPLICABLE LAWS.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=bcm_userspace

# Default release
PKG_VERSION:=4.16L.03
PATH_OBJ_ARCH:=
PATH_BOARDCTL:=cms
PATH_TMS:=

ifdef CONFIG_PACKAGE_kmod-brcm-4.16L.04
  PKG_VERSION:=4.16L.04
endif

ifdef CONFIG_PACKAGE_kmod-brcm-4.16L.05
  PKG_VERSION:=4.16L.05
endif

ifdef CONFIG_PACKAGE_kmod-brcm-5.02L.02
  PKG_VERSION:=5.02L.02
  PATH_OBJ_ARCH:=objs/$(CONFIG_ARCH)/
  PATH_BOARDCTL:=bcm
  PATH_TMS:=tmsctl/
endif

PKG_PROFILE:=
PKG_SOURCE_URL:=git@gitolite-openwrt:/openwrt/pkg/bcm_userspace-tch.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=da0f66e0bfad9989ac97057350b978c8d9317cdb
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

include $(INCLUDE_DIR)/kernel.mk
PKG_FILE_DEPENDS:=$(LINUX_DIR)/.config
include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/bcm_userspace/Default
  SECTION:=utils
  CATEGORY:=Broadcom user space
  DEPENDS:=@TARGET_brcm63xx_tch||TARGET_brcm68xx_tch||TARGET_brcm63xx_arm_tch
  MAINTAINER:=Technicolor <linuxgw@technicolor.com>
endef

define Package/bcm_userspace/Default/Application
$(call Package/bcm_userspace/Default)
  SUBMENU:=Applications
endef

define Package/bcm_userspace/Default/GPON
$(call Package/bcm_userspace/Default)
  SUBMENU:=GPON
endef

define Package/bcm_userspace/Default/Library
$(call Package/bcm_userspace/Default)
  SUBMENU:=Libraries
endef

define Package/bcm_userspace/Default/Technicolor
$(call Package/bcm_userspace/Default)
  SUBMENU:=Technicolor
endef

define Package/bcm_userspace/Default/Utility
$(call Package/bcm_userspace/Default)
  SUBMENU:=Utilities
endef

define Build/Compile
	$(MAKE) -C $(KERNEL_BUILD_DIR)/broadcom-git \
		PROFILE=$(BOARDNAME) \
		TOOLCHAIN=$(TOOLCHAIN_DIR) \
		userspace \
		CC="$(TARGET_CC)" \
		BRCM_MAX_JOBS=1
	PKG_PROFILE=$(PROFILE)

	$(call Build/Compile/Default)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/bcm963xx
	$(CP) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/include/* $(1)/usr/include/bcm963xx
	$(CP) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/include/linux/* $(1)/usr/include/bcm963xx
	$(CP) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/public/include/* $(1)/usr/include/bcm963xx
	$(CP) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/public/include/linux/* $(1)/usr/include/bcm963xx
	$(CP) $(KERNEL_BUILD_DIR)/broadcom-git/bcmdrivers/broadcom/include/bcm963xx/* $(1)/usr/include/bcm963xx
	$(CP) $(KERNEL_BUILD_DIR)/broadcom-git/bcmdrivers/opensource/include/bcm963xx/* $(1)/usr/include/bcm963xx
	$(CP) $(KERNEL_BUILD_DIR)/broadcom-git/shared/broadcom/include/bcm963xx/* $(1)/usr/include/bcm963xx
	$(CP) $(KERNEL_BUILD_DIR)/broadcom-git/shared/opensource/include/bcm963xx/* $(1)/usr/include/bcm963xx

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/public/libs/${PATH_BOARDCTL}_boardctl/lib${PATH_BOARDCTL}_boardctl.so  $(1)/usr/lib
	if [ -e $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/xdslctl/libxdslctl.so ]; then \
		 $(CP) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/xdslctl/libxdslctl.so  $(1)/usr/lib ; \
	fi
	if [ -e $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/atmctl/libatmctl.so ]; then \
		 $(CP) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/atmctl/libatmctl.so $(1)/usr/lib ; \
	fi
	if [ -e $(KERNEL_BUILD_DIR)/broadcom-git/userspace/public/libs/cms_util/libcms_util.so ]; then \
		 $(CP) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/public/libs/cms_util/libcms_util.so  $(1)/usr/lib; \
	fi
	if [ -e $(KERNEL_BUILD_DIR)/broadcom-git/userspace/public/libs/cms_msg/$(PATH_OBJ_ARCH)libcms_msg.so ]; then \
		 $(CP) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/public/libs/cms_msg/$(PATH_OBJ_ARCH)libcms_msg.so  $(1)/usr/lib; \
	fi
	if [ -e $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/snoopctl/libsnoopctl.so ]; then \
		 $(CP) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/snoopctl/libsnoopctl.so $(1)/usr/lib ; \
	fi
	if [ -e $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/libtmctl/libtmctl.so ]; then \
		 $(CP) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/snoopctl/libtmctl.so $(1)/usr/lib ; \
	fi
	if [ -e $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/seltctl/libseltctl.so ]; then \
                 $(CP) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/seltctl/libseltctl.so  $(1)/usr/lib ; \
        fi
	if [ -e $(KERNEL_BUILD_DIR)/broadcom-git/userspace/public/libs/bcm_util/libbcm_crc.so ]; then \
		 $(CP) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/public/libs/bcm_util/libbcm_crc.so $(1)/usr/lib ; \
	fi
	if [ -e $(KERNEL_BUILD_DIR)/broadcom-git/userspace/public/libs/bcm_flashutil/libbcm_flashutil.so ]; then \
		 $(CP) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/public/libs/bcm_flashutil/libbcm_flashutil.so $(1)/usr/lib ; \
	fi
	if [ -e $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/mocactl/libmocactl.so ]; then \
		 $(CP) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/mocactl/libmocactl.so $(1)/usr/lib ; \
	fi
	if [ -e $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/ethswctl/libethswctl.so ]; then \
		 $(CP) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/ethswctl/libethswctl.so $(1)/usr/lib ; \
	fi
	if [ -e $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/fcctl/libfcctl.so ]; then \
		 $(CP) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/fcctl/libfcctl.so $(1)/usr/lib ; \
	fi

endef

###
# Applications
###

define Package/dsldiagd
$(call Package/bcm_userspace/Default/Application)
  TITLE:=Broadcom DSL diagnostics daemon
  DEPENDS+=+libcmshelper +libxdslctl
endef

define Package/bmud
$(call Package/bcm_userspace/Default/Application)
  TITLE:=Broadcom Battery Management Unit daemon
  DEPENDS+=+libcmshelper +libstlport +librt
endef

define Package/mcpd
$(call Package/bcm_userspace/Default/Application)
  TITLE:=Broadcom multicast snooping and proxy daemon
  DEPENDS+=+brctl +libcmshelper +libethswctl +libfcctl +libpwrctl +libsnoopctl +libspuctl +libvlanctl +libxdslctl +libcdk +iptables +kmod-ipt-core +kmod-nfnetlink +kmod-nfnetlink-queue
endef

define Package/mdkshell
$(call Package/bcm_userspace/Default/Application)
  TITLE:=Broadcom MDK shell
endef

define Package/mroute
$(call Package/bcm_userspace/Default/Application)
  TITLE:=Broadcom static multicast routing
  DEPENDS+=+libcms
endef

define Package/swmdk
$(call Package/bcm_userspace/Default/Application)
  TITLE:=Broadcom switch driver
  DEPENDS+=+libpthread libcmshelper +libethswctl +TARGET_brcm63xx_tch:libpwrctl +TARGET_brcm63xx_tch_CANT4:libphy +TARGET_brcm63xx_tch_CANT4:libcdk +TARGET_brcm63xx_tch_CANT4:libbmd
endef

define Package/dsldiagd/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/dsldiagd/dsldiagd $(1)/usr/bin/
endef

define Package/bmud/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/bmud/bmud $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/bmud $(1)/etc/init.d
endef

define Package/mcpd/conffiles
	/etc/config/igmpsnooping
endef

define Package/mcpd/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/snooping $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/mcpd/mcpd $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/config
	touch $(1)/etc/config/igmpsnooping
endef

define Package/mdkshell/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/mdkshell/mdkshell $(1)/usr/bin/
endef

define Package/mroute/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/mroute/mroute $(1)/usr/bin/
endef

define Package/swmdk/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/swmdk $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/swmdk/swmdk $(1)/usr/bin/
endef

define Package/swmdk/postinst
#!/bin/sh
# check if we are on real system
if [ -z "$${IPKG_INSTROOT}" ]; then
        echo "Enabling swmdk at start-up"
        /etc/init.d/swmdk enable
fi
exit 0
endef

define Package/swmdk/prerm
#!/bin/sh
# check if we are on real system
if [ -z "$${IPKG_INSTROOT}" ]; then
        echo "Disabling swmdk at start-up"
        /etc/init.d/swmdk disable
fi
exit 0
endef

$(eval $(call BuildPackage,dsldiagd))
$(eval $(call BuildPackage,bmud))
$(eval $(call BuildPackage,mcpd))
$(eval $(call BuildPackage,mdkshell))
$(eval $(call BuildPackage,mroute))
$(eval $(call BuildPackage,swmdk))

###
# Libraries
###

define Package/libatmctl
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom ATM control library
  DEPENDS+=+TARGET_brcm63xx_tch:kmod-bcm63xx-tch-xtm +TARGET_brcm63xx_arm_tch:kmod-bcm63xx-arm-tch-xtmcfg +libcmshelper
endef

define Package/libbmd
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom BMD library
  DEPENDS+=
endef

define Package/libcdk
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom CDK library
  DEPENDS+=
endef

define Package/libcms
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom CMS library
  DEPENDS+=+TARGET_brcm63xx_tch:libcmshelper +TARGET_brcm63xx_arm_tch:libcmshelper +TARGET_brcm68xx_tch:libcmshelper +TARGET_brcm68xx_tch:libomci +libpthread
endef

define Package/libcmshelper
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom CMS related helper libraries
  DEPENDS+=
endef

define Package/libethswctl
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom ethernet switch control library
  DEPENDS+=
endef

define Package/libfapctl
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom FAP control library
  DEPENDS+=@TARGET_brcm63xx_tch +kmod-bcm63xx-tch-fap
endef

define Package/libfcctl
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom Flow Cache control library
  DEPENDS+=+TARGET_brcm63xx_tch:kmod-bcm63xx-tch-pktflow +TARGET_brcm68xx_tch:kmod-bcm68xx-tch-pktflow
endef

define Package/libgmacctl
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom GMAC control library
  DEPENDS+=
endef

define Package/libiqctl
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom Ingress QoS control library
  DEPENDS+=+kmod-bcm63xx-tch-ingqos
endef

define Package/libmdm
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom MDM library
  DEPENDS+=
endef

define Package/libnanoxml
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom Nano XML library
  DEPENDS+=
endef

define Package/libphy
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom PHY library
  DEPENDS+=
endef

define Package/libpwrctl
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom power control library
  DEPENDS+=+TARGET_brcm63xx_tch:kmod-bcm63xx-tch-pwrmngt +TARGET_brcm68xx_tch_GANTU:kmod-bcm68xx-tch-pwrmngt +TARGET_brcm68xx_tch_GANTH:kmod-bcm68xx-tch-pwrmngt
endef

define Package/libsnoopctl
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom IGMP snooping control library
  DEPENDS+=@PACKAGE_kmod-brcm-4.16L.03||@PACKAGE_kmod-brcm-4.16L.04||@PACKAGE_kmod-brcm-4.16L.05
endef

define Package/libspuctl
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom Secure Processing Unit control library
  DEPENDS+=@TARGET_brcm63xx_tch
endef

define Package/libvlanctl
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom VLAN control library
  DEPENDS+=+TARGET_brcm63xx_tch:kmod-bcm63xx-tch-vlan
  DEPENDS+=+TARGET_brcm68xx_tch:kmod-bcm68xx-tch-vlan
endef

define Package/libwlctl
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom wireless control library
  DEPENDS+=TARGET_brcm63xx_tch:kmod-bcm63xx-tch-wireless
  DEPENDS+=TARGET_brcm68xx_tch:kmod-bcm68xx-tch-wlan
endef

define Package/libxdslctl
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom DSL control library
  DEPENDS+=+TARGET_brcm63xx_tch:kmod-bcm63xx-tch-adsl +TARGET_brcm63xx_arm_tch:kmod-bcm63xx-arm-tch-adsl
endef
define Package/libseltctl
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom SELT control librar
  DEPENDS+=+TARGET_brcm63xx_tch:kmod-bcm63xx-tch-adsl +TARGET_brcm63xx_arm_tch:kmod-bcm63xx-arm-tch-adsl
endef
define Package/libgponctl
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom lib gponctl
  DEPENDS+=@TARGET_brcm68xx_tch +libuci
endef
define Package/libgponif
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom lib gponif
  DEPENDS+=@TARGET_brcm68xx_tch
endef

define Package/libomci
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom libomci
  DEPENDS+=@TARGET_brcm68xx_tch
endef

define Package/libomcipm_drv
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom libomcipm_drv
  DEPENDS+=@TARGET_brcm68xx_tch
endef

define Package/libstlport
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom libstlport
  DEPENDS+=@(TARGET_brcm68xx_tch||TARGET_brcm63xx_tch) +libpthread
endef

define Package/librdpactl
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom librdpactl
  DEPENDS+=
endef

define Package/libbcmebtables
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom libebtables
  DEPENDS+=@TARGET_brcm68xx_tch
endef

define Package/libmocactl
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom MoCA Control library
  DEPENDS+=@TARGET_brcm63xx_tch +libethswctl +libpthread
endef

define Package/libtmctl
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom Traffic Manager Control Library
  DEPENDS+=+libethswctl +librdpactl
endef

define Package/libbcmtm
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom 63xx Traffic Manager Control Library 
  DEPENDS+=+TARGET_brcm63xx_tch:kmod-bcm63xx-tch-bcmtm
endef

define Package/libdpictl
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom 63xx DPI Control Library
  DEPENDS+=+kmod-bcm63xx-tch-dpi
endef

define Package/libethctl
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom ETH control library
  DEPENDS+=kmod-brcm-5.02L.02
endef

define Package/libbcmmcast
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom multicast library
  DEPENDS+=kmod-brcm-5.02L.02
endef

define Package/libbridgeutil
$(call Package/bcm_userspace/Default/Library)
  TITLE:=Broadcom bridge utility library
  DEPENDS+=kmod-brcm-5.02L.02
endef

define Package/libatmctl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/atmctl/libatmctl.so $(1)/usr/lib
endef

define Package/libbmd/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/mdk*/examples/linux-user/bmd/libbmdapi.so $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/mdk*/examples/linux-user/bmd/libbmdpkgsrc.so $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/mdk*/examples/linux-user/bmd/libbmdshared.so $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/mdk*/examples/linux-user/bmd/libbmdshell.so $(1)/usr/lib
endef

define Package/libcdk/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/mdk*/examples/linux-user/cdk/libcdklibc.so $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/mdk*/examples/linux-user/cdk/libcdkmain.so $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/mdk*/examples/linux-user/cdk/libcdkpkgsrc.so $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/mdk*/examples/linux-user/cdk/libcdkshared.so $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/mdk*/examples/linux-user/cdk/libcdkshell.so $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/mdk*/examples/linux-user/cdk/libcdksym.so $(1)/usr/lib
endef

define Package/libcmshelper/install
	$(INSTALL_DIR) $(1)/usr/lib
	if [ -e $(KERNEL_BUILD_DIR)/broadcom-git/userspace/public/libs/cms_msg/$(PATH_OBJ_ARCH)libcms_msg.so ]; then \
		 $(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/public/libs/cms_msg/$(PATH_OBJ_ARCH)libcms_msg.so $(1)/usr/lib; \
	fi
	if [ -e $(KERNEL_BUILD_DIR)/broadcom-git/userspace/public/libs/cms_util/libcms_util.so ]; then \
		 $(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/public/libs/cms_util/libcms_util.so $(1)/usr/lib; \
	fi
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/public/libs/${PATH_BOARDCTL}_boardctl/lib${PATH_BOARDCTL}_boardctl.so $(1)/usr/lib
	if [ -e $(KERNEL_BUILD_DIR)/broadcom-git/userspace/public/libs/bcm_util/libbcm_crc.so ]; then \
		 $(CP) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/public/libs/bcm_util/libbcm_crc.so $(1)/usr/lib ; \
	fi
	if [ -e $(KERNEL_BUILD_DIR)/broadcom-git/userspace/public/libs/bcm_flashutil/libbcm_flashutil.so ]; then \
		 $(CP) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/public/libs/bcm_flashutil/libbcm_flashutil.so $(1)/usr/lib ; \
	fi
endef

define Package/libcms/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/cms_cli/$(PATH_OBJ_ARCH)libcms_cli.so $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/cms_core/libcms_core.so $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/cms_dal/libcms_dal.so $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/cms_qdm/libcms_qdm.so $(1)/usr/lib
endef

define Package/libethswctl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/ethswctl/libethswctl.so $(1)/usr/lib
endef

define Package/libfapctl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/fapctl/libfapctl.so $(1)/usr/lib
endef

define Package/libfcctl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/fcctl/libfcctl.so $(1)/usr/lib
endef

define Package/libgmacctl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/gmacctl/libgmacctl.so $(1)/usr/lib
endef

define Package/libiqctl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/iqctl/libiqctl.so $(1)/usr/lib
endef

define Package/libmdm/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/mdm/libmdm.so $(1)/usr/lib
endef

define Package/libnanoxml/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/nanoxml/libnanoxml.so $(1)/usr/lib
endef

define Package/libphy/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/mdk*/examples/linux-user/phy/libphygeneric.so $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/mdk*/examples/linux-user/phy/libphypkgsrc.so $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/mdk*/examples/linux-user/phy/libphysym.so $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/mdk*/examples/linux-user/phy/libphyutil.so $(1)/usr/lib
endef

define Package/libpwrctl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/pwrctl/libpwrctl.so $(1)/usr/lib
endef

define Package/libsnoopctl/install
	$(INSTALL_DIR) $(1)/usr/lib
	if [ -e $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/snoopctl/libsnoopctl.so ]; then \
		$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/snoopctl/libsnoopctl.so $(1)/usr/lib; \
	fi
endef

define Package/libspuctl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/spuctl/libspuctl.so $(1)/usr/lib
endef

define Package/libvlanctl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/vlanctl/libvlanctl.so $(1)/usr/lib
endef

define Package/libwlctl/install
	$(INSTALL_DIR) $(1)/usr/lib  
	#Location moved to dhd dir in 4.16L04
	if [ -f $(KERNEL_BUILD_DIR)/broadcom-git/bcmdrivers/broadcom/net/wl/impl$(CONFIG_BCM_WLAN_IMPL)/dhd/src/wl/exe/libwlctl.so ]; then \
		$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/bcmdrivers/broadcom/net/wl/impl$(CONFIG_BCM_WLAN_IMPL)/dhd/src/wl/exe/libwlctl.so $(1)/usr/lib; \
	else \
		$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/bcmdrivers/broadcom/net/wl/impl$(CONFIG_BCM_WLAN_IMPL)/wl/exe/libwlctl.so $(1)/usr/lib; \
	fi
endef

define Package/libxdslctl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/xdslctl/libxdslctl.so $(1)/usr/lib
endef

define Package/libseltctl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/seltctl/libseltctl.so $(1)/usr/lib
endef

define Package/libgponctl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/gponctl/libgponctl.so $(1)/usr/lib/
	if [ -e $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/gponctl/libtrsv*.so ]; then \
		$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/gponctl/libtrsv*.so  $(1)/usr/lib/; \
	fi
	if [ -e $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/gponctl/script/trsv ]; then \
		$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/gponctl/script/trsv $(1)/etc/config/; \
	fi
endef
define Package/libgponif/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/gponif/libgponif.so $(1)/usr/lib/
endef

define Package/libomci/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/omci/libomci.so $(1)/usr/lib/
endef

define Package/libomcipm_drv/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/omcipm_drv/libomcipm_drv.so $(1)/usr/lib/
endef

define Package/libstlport/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/public/libs/stlport/libstlport.so.5.2 $(1)/usr/lib/
endef

define Package/librdpactl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/rdpactl/librdpactl.so $(1)/usr/lib/
endef

define Package/libbcmebtables/install
	$(INSTALL_DIR) $(1)/usr/lib/ebtables
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/gpl/apps/ebtables/ebtables-v2.0.10-4/extensions/libebt*.so $(1)/usr/lib/ebtables/
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/gpl/apps/ebtables/ebtables-v2.0.10-4/libebtc.so $(1)/usr/lib/ebtables/
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/gpl/apps/ebtables/ebtables-v2.0.10-4/ethertypes $(1)/etc/
endef

define Package/libmocactl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/mocactl/libmocactl.so $(1)/usr/lib
endef

define Package/libtmctl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/tmctl/libtmctl.so $(1)/usr/lib
endef

define Package/libbcmtm/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/bcmtm/libbcmtm.so $(1)/usr/lib
endef

define Package/libdpictl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/dpictl/libdpictl.so $(1)/usr/lib
endef

define Package/libethctl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/ethctl/libethctl.so $(1)/usr/lib
endef

define Package/libbcmmcast/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/bcmmcast/libbcmmcast.so $(1)/usr/lib
endef

define Package/libbridgeutil/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/bridgeutil/libbridgeutil.so $(1)/usr/lib
endef

$(eval $(call BuildPackage,libatmctl))
$(eval $(call BuildPackage,libbmd))
$(eval $(call BuildPackage,libcdk))
$(eval $(call BuildPackage,libcmshelper))
$(eval $(call BuildPackage,libcms))
$(eval $(call BuildPackage,libethswctl))
$(eval $(call BuildPackage,libfapctl))
$(eval $(call BuildPackage,libfcctl))
$(eval $(call BuildPackage,libgmacctl))
$(eval $(call BuildPackage,libiqctl))
$(eval $(call BuildPackage,libmdm))
$(eval $(call BuildPackage,libnanoxml))
$(eval $(call BuildPackage,libphy))
$(eval $(call BuildPackage,libpwrctl))
$(eval $(call BuildPackage,libsnoopctl))
$(eval $(call BuildPackage,libspuctl))
$(eval $(call BuildPackage,libvlanctl))
$(eval $(call BuildPackage,libwlctl))
$(eval $(call BuildPackage,libxdslctl))
$(eval $(call BuildPackage,libgponctl))
$(eval $(call BuildPackage,libgponif))
$(eval $(call BuildPackage,libomci))
$(eval $(call BuildPackage,libomcipm_drv))
$(eval $(call BuildPackage,libstlport))
$(eval $(call BuildPackage,librdpactl))
$(eval $(call BuildPackage,libbcmebtables))
$(eval $(call BuildPackage,libmocactl))
$(eval $(call BuildPackage,libtmctl))
$(eval $(call BuildPackage,libseltctl))
$(eval $(call BuildPackage,libbcmtm))
$(eval $(call BuildPackage,libdpictl))
$(eval $(call BuildPackage,libethctl))
$(eval $(call BuildPackage,libbcmmcast))
$(eval $(call BuildPackage,libbridgeutil))

###
# Utilities
###

define Package/arlctl
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom ARL control
  DEPENDS+=+libcdk +kmod-bcm63xx-tch-arl
endef

define Package/bpmctl
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom Buffer Pool Manager control
  DEPENDS+=@TARGET_brcm63xx_tch
endef

define Package/brctl
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom bridge control
  DEPENDS+=
endef

define Package/ethctl
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom ethernet control
  DEPENDS+=+(PACKAGE_kmod-brcm-5.02L.02):libethctl
endef

define Package/ethswctl
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom ethernet switch control
  DEPENDS+=+libethswctl +libcmshelper
endef

define Package/fapctl
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom FAP control
  DEPENDS+=+libfapctl
endef

define Package/fcctl
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom Flow Cache control
  DEPENDS+=+libfcctl
endef

define Package/gmacctl
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom GMAC control
  DEPENDS+=+libgmacctl 
endef

define Package/iqctl
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom Ingress QoS control
  DEPENDS+=+libiqctl
endef

define Package/mcpctl
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom multicast proxy control
  DEPENDS+=+TARGET_brcm63xx_tch:libcdk +TARGET_brcm63xx_tch:libcms +TARGET_brcm68xx_tch:libcms
endef

define Package/pwrctl
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom power control
  DEPENDS+=+libcmshelper +libpwrctl +libethswctl
endef

define Package/spuctl
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom Secure Processing Unit control
  DEPENDS+=+libcmshelper +libspuctl
endef

define Package/vlanctl
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom VLAN control
  DEPENDS+=+libvlanctl
  DEPENDS+=+(PACKAGE_kmod-brcm-4.16L.05):libcmshelper +(PACKAGE_kmod-brcm-5.02L.02):libcmshelper
endef

define Package/wlctl
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom wireless control
  DEPENDS+=+libwlctl
endef

define Package/dhdctl
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom wireless control (offloaded)
  DEPENDS+=+TARGET_brcm63xx_tch:kmod-bcm63xx-tch-wireless-dhd
endef
      
define Package/xdslctl
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom DSL control
  DEPENDS+=+libcmshelper +libxdslctl
ifeq ($(BRCM_CHIP), 63138)
  DEPENDS+=+libseltctl
endif
endef

define Package/seltctl
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom SELT control
  DEPENDS+=+libcmshelper +libseltctl +libxdslctl
endef

define Package/xtmctl
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom XTM control
  DEPENDS+=+libcmshelper +libatmctl
endef

define Package/smd
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom smd
  DEPENDS+=+libcms +libethswctl +libgponctl +libtmctl +libgponif +libmdm +libnanoxml +libomci +libomcipm_drv  +libvlanctl +librdpactl +TARGET_brcm68xx_tch_GANTU:libpwrctl +TARGET_brcm68xx_tch_GANTH:libpwrctl +(PACKAGE_kmod-brcm-5.02L.02):libethctl +(PACKAGE_kmod-brcm-5.02L.02):libbcmmcast +(PACKAGE_kmod-brcm-5.02L.02):libbridgeutil
endef

define Package/ssk
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom ssk
  DEPENDS+=+libcms +libethswctl +libgponctl +libtmctl +libuci +libgponif +libmdm +libnanoxml +libomci +libomcipm_drv +libvlanctl +librdpactl +TARGET_brcm68xx_tch_GANTU:libpwrctl +TARGET_brcm68xx_tch_GANTH:libpwrctl +(PACKAGE_kmod-brcm-5.02L.02):libethctl +(PACKAGE_kmod-brcm-5.02L.02):libbcmmcast +(PACKAGE_kmod-brcm-5.02L.02):libbridgeutil
endef

define Package/tmsctl
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom TMS control
  DEPENDS+=+libcmshelper +libpthread +TARGET_brcm63xx_tch:kmod-bcm63xx-tch-ethoam
endef

define Package/bcmebtables
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom ebtables utility
  DEPENDS+=@TARGET_brcm68xx_tch +libbcmebtables
endef

define Package/mocad
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom MoCA daemon
  DEPENDS+=@TARGET_brcm63xx_tch +spictl +libmocactl +libpthread +libethswctl
endef

define Package/mocap
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom MoCAp utility
  DEPENDS+=@TARGET_brcm63xx_tch +mocad +libethswctl
endef

define Package/mocactl
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom MoCA Control utility
  DEPENDS+=@TARGET_brcm63xx_tch +mocad +libethswctl
endef

define Package/spictl
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom SPI bus Control utility
  DEPENDS+=@TARGET_brcm63xx_tch
endef

define Package/bcmtmctl
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom 63xx traffic manager userspace utility
  DEPENDS+=@TARGET_brcm63xx_tch +libbcmtm
endef

define Package/bcmnfc
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom 63xx nfc userspace i2c config data
  DEPENDS+=@TARGET_brcm63xx_tch +kmod-bcm63xx-tch-nfc
endef

define Package/dpictl
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom 63xx DPI user space utility
  DEPENDS+=+libdpictl
endef

define Package/tmctl
$(call Package/bcm_userspace/Default/Utility)
  TITLE:=Broadcom Traffic Manager Ctrl
  DEPENDS+=+libtmctl +libcmshelper
endef

define Package/mocad/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/mocad/mocad $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/moca
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/bcmdrivers/opensource/char/moca/impl2/image/moca20core.bin $(1)/etc/moca
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/moca $(1)/etc/init.d
	$(INSTALL_BIN) ./files/usr/bin/mocacfg $(1)/usr/bin
endef

define Package/mocap/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/mocap/mocap $(1)/usr/bin/
endef

define Package/mocactl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/mocactl/mocactl $(1)/usr/bin/
endef

define Package/spictl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/spictl/spictl $(1)/usr/bin/
endef

define Package/arlctl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/arlctl/arlctl $(1)/usr/bin/
endef

define Package/bpmctl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/bpmctl/bpmctl $(1)/usr/bin/
endef

define Package/brctl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/gpl/apps/bridge-utils/$(PATH_OBJ_ARCH)brctl/brctl $(1)/usr/bin/
endef

define Package/ethctl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/ethctl/ethctl $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/ethernet $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/softswitch $(1)/etc/init.d
ifeq ($(CONFIG_PACKAGE_anvil),)
	$(INSTALL_BIN) ./files/etc/init.d/phypower $(1)/etc/init.d
endif
	$(INSTALL_DIR) $(1)/lib/functions
	$(INSTALL_DATA) ./files/lib/functions/phypower.sh $(1)/lib/functions
endef

define Package/ethswctl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/ethswctl/ethswctl $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
endef

define Package/fapctl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/fapctl/fapctl $(1)/usr/bin/
endef

define Package/fcctl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/fcctl/fcctl $(1)/usr/bin/
endef

define Package/gmacctl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/gmacctl/gmacctl $(1)/usr/bin/
endef

define Package/iqctl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/iqctl/iqctl $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/iqos $(1)/etc/init.d
endef

define Package/mcpctl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/mcpctl/mcpctl $(1)/usr/bin/
endef

define Package/pwrctl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/pwrctl/pwrctl $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/power $(1)/etc/init.d
endef

define Package/spuctl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/spuctl/spuctl $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/spu $(1)/etc/init.d
endef

define Package/tmsctl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/tmsctl/$(PATH_TMS)/tmsctl $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/ethoam $(1)/etc/init.d
endef

define Package/vlanctl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/vlanctl/vlanctl $(1)/usr/bin/
endef

define Package/wlctl/install
	$(INSTALL_DIR) $(1)/usr/bin
	#Location moved to dhd dir in 4.16L04
	if [ -f $(KERNEL_BUILD_DIR)/broadcom-git/bcmdrivers/broadcom/net/wl/impl$(CONFIG_BCM_WLAN_IMPL)/dhd/src/wl/exe/wlctl ]; then \
		$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/bcmdrivers/broadcom/net/wl/impl$(CONFIG_BCM_WLAN_IMPL)/dhd/src/wl/exe/wlctl $(1)/usr/bin; \
	else \
		$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/bcmdrivers/broadcom/net/wl/impl$(CONFIG_BCM_WLAN_IMPL)/wl/exe/wlctl $(1)/usr/bin; \
	fi
	ln -s wlctl $(1)/usr/bin/wl
endef

define Package/dhdctl/install
	$(INSTALL_DIR) $(1)/usr/bin
	#Location changed 4.16L04
	if [ -f $(KERNEL_BUILD_DIR)/broadcom-git/bcmdrivers/broadcom/net/wl/impl$(CONFIG_BCM_WLAN_IMPL)/dhd/src/dhd/exe/dhdctl ]; then \
		$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/bcmdrivers/broadcom/net/wl/impl$(CONFIG_BCM_WLAN_IMPL)/dhd/src/dhd/exe/dhdctl $(1)/usr/bin; \
	else \
		$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/bcmdrivers/broadcom/net/wl/impl$(CONFIG_BCM_WLAN_IMPL)/dhd/dhd/exe/dhdctl $(1)/usr/bin; \
	fi
endef

define Package/xdslctl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/xdslctl/xdslctl $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/xdsl $(1)/etc/init.d
ifeq ($(strip $(CONFIG_KERNEL_DSL_BONDING)),y)
	$(LN) xdslctl $(1)/usr/bin/xdslctl0
	$(LN) xdslctl $(1)/usr/bin/xdslctl1 
endif
endef

define Package/seltctl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/seltctl/seltctl $(1)/usr/bin/
endef

define Package/xtmctl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/atmctl/xtmctl $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
ifeq ($(CONFIG_KERNEL_DSL_BONDING),y)
	$(INSTALL_BIN) ./files/etc/init.d/xtm_bond $(1)/etc/init.d/xtm
else
	$(INSTALL_BIN) ./files/etc/init.d/xtm $(1)/etc/init.d
endif
endef

define Package/smd/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/smd/smd $(1)/bin/
endef

define Package/ssk/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/ssk/ssk $(1)/bin/
endef

define Package/bcmebtables/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/gpl/apps/ebtables/ebtables-v2.0.10-4/ebtables $(1)/bin/
endef

define Package/bcmtmctl/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/bcmtm/bcmtmctl $(1)/usr/bin/
endef

define Package/bcmnfc/install
	$(INSTALL_DIR) $(1)/etc/nfc
	$(INSTALL_DATA) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/libs/nfc/Projects/bte/build/*A2*05*52*ncd $(1)/etc/nfc/BCM20795A2.ncd
endef

define Package/dpictl/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/dpictl/dpictl $(1)/bin/
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/dpieng/tdts_rule_agent $(1)/bin/
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_DATA) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/dpieng/rule.trf $(1)/etc/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/dpi $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/usr/lib/lua/hostmanager
	$(INSTALL_BIN) ./files/usr/lib/lua/hostmanager/extinfo.lua $(1)/usr/lib/lua/hostmanager/
endef

define Package/tmctl/install
	$(INSTALL_DIR) $(1)/usr/bin $(1)/lib/network/system
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/tmctl/tmctl $(1)/usr/bin/
endef


$(eval $(call BuildPackage,arlctl))
$(eval $(call BuildPackage,bpmctl))
$(eval $(call BuildPackage,brctl))
$(eval $(call BuildPackage,ethctl))
$(eval $(call BuildPackage,ethswctl))
$(eval $(call BuildPackage,fapctl))
$(eval $(call BuildPackage,fcctl))
$(eval $(call BuildPackage,gmacctl))
$(eval $(call BuildPackage,iqctl))
$(eval $(call BuildPackage,mcpctl))
$(eval $(call BuildPackage,pwrctl))
$(eval $(call BuildPackage,spuctl))
$(eval $(call BuildPackage,tmsctl))
$(eval $(call BuildPackage,vlanctl))
$(eval $(call BuildPackage,wlctl))
$(eval $(call BuildPackage,dhdctl))
$(eval $(call BuildPackage,xdslctl))
$(eval $(call BuildPackage,xtmctl))
$(eval $(call BuildPackage,smd))
$(eval $(call BuildPackage,ssk))
$(eval $(call BuildPackage,bcmebtables))
$(eval $(call BuildPackage,seltctl))
$(eval $(call BuildPackage,bcmtmctl))
$(eval $(call BuildPackage,bcmnfc))
$(eval $(call BuildPackage,dpictl))
$(eval $(call BuildPackage,mocad))
$(eval $(call BuildPackage,mocap))
$(eval $(call BuildPackage,mocactl))
$(eval $(call BuildPackage,spictl))
$(eval $(call BuildPackage,tmctl))

##
# GPON
##

define Package/bdmf_shell
$(call Package/bcm_userspace/Default/GPON)
  TITLE:=Broadcom bdmf shell
  DEPENDS+=@TARGET_brcm68xx_tch||TARGET_brcm63xx_arm_tch||TARGET_brcm63xx_tch +libpthread
endef

define Package/gponctl
$(call Package/bcm_userspace/Default/GPON)
  TITLE:=Broadcom gponctl
  DEPENDS+=@TARGET_brcm68xx_tch +libgponctl +libuci
endef

define Package/gponif
$(call Package/bcm_userspace/Default/GPON)
  TITLE:=Broadcom gponif
  DEPENDS+=@TARGET_brcm68xx_tch +libgponif
endef

define Package/gpond
$(call Package/bcm_userspace/Default/GPON)
  TITLE:=Broadcom gpond
  DEPENDS+=@TARGET_brcm68xx_tch +libgponctl
endef

define Package/omcipmd
$(call Package/bcm_userspace/Default/GPON)
  TITLE:=Broadcom omcipmd
  DEPENDS+=@TARGET_brcm68xx_tch +libomci +libomcipm_drv +libgponif +libgponctl +libcms +libethswctl +libmdm +libnanoxml +libvlanctl +librdpactl +TARGET_brcm68xx_tch_GANTU:libpwrctl +TARGET_brcm68xx_tch_GANTH:libpwrctl
endef

define Package/omcipmdrvctl
$(call Package/bcm_userspace/Default/GPON)
  TITLE:=Broadcom omcipmdrvctl
  DEPENDS+=@TARGET_brcm68xx_tch +libcms +libethswctl +libgponctl +libtmctl +libgponif +libmdm +libnanoxml +libomci +libvlanctl +libomcipm_drv +librdpactl +TARGET_brcm68xx_tch_GANTU:libpwrctl +TARGET_brcm68xx_tch_GANTH:libpwrctl +(PACKAGE_kmod-brcm-5.02L.02):libethctl +(PACKAGE_kmod-brcm-5.02L.02):libbcmmcast +(PACKAGE_kmod-brcm-5.02L.02):libbridgeutil
endef

define Package/omcid
$(call Package/bcm_userspace/Default/GPON)
  TITLE:=Broadcom omcid
  DEPENDS+=@TARGET_brcm68xx_tch +libcms +libethswctl +libgponctl +libtmctl +libgponif +libmdm +libnanoxml +libomci +libvlanctl +libomcipm_drv +librdpactl +librt +TARGET_brcm68xx_tch_GANTU:libpwrctl +TARGET_brcm68xx_tch_GANTH:libpwrctl +(PACKAGE_kmod-brcm-5.02L.02):libethctl +(PACKAGE_kmod-brcm-5.02L.02):libbcmmcast +(PACKAGE_kmod-brcm-5.02L.02):libbridgeutil
endef

define Package/ponwiz
$(call Package/bcm_userspace/Default/GPON)
  TITLE:=Broadcom ponwiz
  DEPENDS+=@TARGET_brcm68xx_tch +libpthread +libomci +libomcipm_drv +libstlport  +libgponif +libgponctl +libcms +libethswctl +libmdm +libnanoxml +libvlanctl +librdpactl
endef

define Package/consoled
$(call Package/bcm_userspace/Default/GPON)
  TITLE:=Broadcom consoled
  DEPENDS+=@TARGET_brcm68xx_tch +libcms +libethswctl +libgponctl +libtmctl +libgponif +libmdm +libnanoxml +libomci +libomcipm_drv +libvlanctl +librdpactl +libpthread +TARGET_brcm68xx_tch_GANTU:libpwrctl +TARGET_brcm68xx_tch_GANTH:libpwrctl +(PACKAGE_kmod-brcm-5.02L.02):libethctl +(PACKAGE_kmod-brcm-5.02L.02):libbcmmcast +(PACKAGE_kmod-brcm-5.02L.02):libbridgeutil
endef

define Package/gponmcpd
$(call Package/bcm_userspace/Default/GPON)
  TITLE:=Broadcom multicast proxy daemon
  DEPENDS+=@TARGET_brcm68xx_tch +libgponctl +libgponif +libtmctl +libomci +libomcipm_drv +libcms +libethswctl +libmdm +libnanoxml +libsnoopctl +libvlanctl +kmod-SKIPLOG +kmod-nfnetlink-queue +librdpactl +TARGET_brcm68xx_tch_GANTU:libpwrctl +TARGET_brcm68xx_tch_GANTH:libpwrctl +(PACKAGE_kmod-brcm-5.02L.02):libbcmmcast +(PACKAGE_kmod-brcm-5.02L.02):libbridgeutil
endef

define Package/wanconf
$(call Package/bcm_userspace/Default/GPON)
  TITLE:=Broadcom utility to detect wan interface
  DEPENDS+=kmod-brcm-5.02L.02
  DEPENDS+=@TARGET_brcm68xx_tch +libgponctl +librdpactl +librt +libcmshelper
endef

define Package/bdmf_shell/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/public/apps/bdmf_shell/bdmf_shell $(1)/bin/
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/public/apps/bdmf_shell/scripts/bs $(1)/bin/
	if [ -e $(KERNEL_BUILD_DIR)/broadcom-git/userspace/public/apps/bdmf_shell/scripts/runner ]; then $(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/public/apps/bdmf_shell/scripts/runner $(1)/bin/ ; fi
endef

define Package/gponctl/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/gponctl/gponctl $(1)/bin/
endef

define Package/gponif/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/gponif/gponif $(1)/bin/
endef

define Package/gpond/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/gpond/gpond $(1)/bin/
endef

define Package/omcipmd/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/omcipmd/omcipmd $(1)/bin/
endef

define Package/omcipmdrvctl/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/omcipmdrvctl/omcipmdrvctl $(1)/bin/
endef

define Package/omcid/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/omcid/omcid $(1)/bin/
endef

define Package/ponwiz/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/ponwiz/onu/images/ponwiz $(1)/bin/
endef

define Package/consoled/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/consoled/consoled $(1)/bin/
endef

define Package/gponmcpd/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/private/apps/mcpd/mcpd $(1)/bin/
endef

define Package/wanconf/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/public/apps/wanconf/wanconf $(1)/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(KERNEL_BUILD_DIR)/broadcom-git/userspace/public/apps/wanconf/scripts/wanconf.sh $(1)/etc/init.d/
endef

$(eval $(call BuildPackage,bdmf_shell))
$(eval $(call BuildPackage,gponctl))
$(eval $(call BuildPackage,gponif))
$(eval $(call BuildPackage,gpond))
$(eval $(call BuildPackage,ponwiz))
$(eval $(call BuildPackage,omcipmd))
$(eval $(call BuildPackage,omcipmdrvctl))
$(eval $(call BuildPackage,omcid))
$(eval $(call BuildPackage,consoled))
$(eval $(call BuildPackage,gponmcpd))
$(eval $(call BuildPackage,wanconf))


###
# Technicolor
###

define Package/bcm_userspace-tch
$(call Package/bcm_userspace/Default/Technicolor)
  TITLE:=Technicolor utilities
  DEPENDS+= +bcmswconfig-tch
endef

define Package/bcm_userspace-tch/description
  Technicolor utilities for Broadcom 63xx platforms
endef

define Package/bcm_userspace_fap-tch
$(call Package/bcm_userspace/Default/Technicolor)
  TITLE:=Technicolor FAP utilities
  DEPENDS:=+liblua
endef

define Package/bcm_userspace_runner-tch
$(call Package/bcm_userspace/Default/Technicolor)
  TITLE:=Technicolor Runner utilities
  DEPENDS:=+liblua
endef

define Package/bcmswconfig-tch
$(call Package/bcm_userspace/Default/Technicolor)
  TITLE:=Broadcom switch config utility
  DEPENDS+= +libuci +mdkshell +cext
endef

define Package/bcmvopiconfig-tch
$(call Package/bcm_userspace/Default/Technicolor)
  TITLE:=Broadcom VOPI config utility
  DEPENDS+= +libuci +vlanctl
endef

define Package/bcm_userspace-tch/install
	$(INSTALL_DIR) $(1)/lib/functions
	$(INSTALL_DATA) ./files/lib/functions/phypower.sh $(1)/lib/functions
	$(INSTALL_DIR) $(1)/lib/network
	$(INSTALL_DATA) ./files/lib/network/switch.sh $(1)/lib/network
endef

define Package/bcm_userspace_fap-tch/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) ./files/usr/bin/setupfapbcmtm.lua $(1)/usr/bin/setupfapbcmtm
endef

define Package/bcm_userspace_runner-tch/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) ./files/usr/bin/setuprunner.lua $(1)/usr/bin/setuprunner
endef

define Package/bcmswconfig-tch/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/bcmswconfig $(1)/usr/bin
endef

define Package/bcmvopiconfig-tch/install
	$(INSTALL_DIR) $(1)/usr/bin $(1)/lib/network/system
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/bcmvopiconfig $(1)/usr/bin
	$(INSTALL_DATA) ./files/lib/network/system/bcmvopi.sh $(1)/lib/network/system
endef

$(eval $(call BuildPackage,bcm_userspace-tch))
$(eval $(call BuildPackage,bcm_userspace_fap-tch))
$(eval $(call BuildPackage,bcm_userspace_runner-tch))
$(eval $(call BuildPackage,bcmswconfig-tch))
$(eval $(call BuildPackage,bcmvopiconfig-tch))

