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
include $(INCLUDE_DIR)/target.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=modmmswitch
PKG_VERSION:=2012.09
PKG_SOURCE_URL:=git@gitolite-openwrt:/openwrt/pkg/modmmswitch.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=10c79557217910593a909ecb4b78e92140955b1e

PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

include $(INCLUDE_DIR)/package.mk

define KernelPackage/modmmswitch
  CATEGORY:=Technicolor
  SUBMENU:=Voice
  TITLE:=MMPBX mmswitch kernel module
  DEPENDS:=
  FILES:=$(PKG_BUILD_DIR)/modmmswitch.ko
  KCONFIG:=
  MAINTAINER:=Technicolor <linuxgw@technicolor.com>
  #AUTOLOAD:=$(call AutoLoad,98,modmmswitch)
  AUTOLOAD:=
endef

define KernelPackage/modmmswitch/description
  Kernel module for MMPBX
endef

EXTRA_KCONFIG:= \
	CONFIG_MODMMSWITCH=m

ifneq (y,$(filter y, $(CONFIG_TARGET_comcerto2000) $(CONFIG_TARGET_ar71xx_tch)))
	PKG_EXTRA_CFLAGS += \
		-DMMPBX_DSP_SUPPORT_RTCPXR
endif

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	EXTRA_CFLAGS="$(PKG_EXTRA_CFLAGS)" \
	$(EXTRA_KCONFIG)

define Build/InstallDev
	$(INSTALL_DIR) $(STAGING_DIR)/usr/include/modmmswitch
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/include/*.h $(STAGING_DIR)/usr/include/modmmswitch
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

$(eval $(call KernelPackage,modmmswitch))
