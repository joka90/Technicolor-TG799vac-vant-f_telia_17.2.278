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
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=gobinet
PKG_VERSION:=2.32

include $(INCLUDE_DIR)/package.mk

define KernelPackage/gobinet
  SUBMENU:=USB Support
  TITLE:=Sierra Wireless GobiNet driver
  FILES:=$(PKG_BUILD_DIR)/GobiNet.ko
  KCONFIG:=
  DEPENDS:=+kmod-usb-net
  AUTOLOAD:=$(call AutoLoad,80,GobiNet)
  MAINTAINER:=Technicolor <linuxgw@technicolor.com>
endef

EXTRA_KCONFIG:= \
	CONFIG_GOBINET=m

define KernelPackage/gobinet/config
	source "$(SOURCE)/Config.in"
endef

define KernelPackage/gobinet/description
 	Kernel module that provides access to Sierra Wireless Gobi based devices
endef

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	EXTRA_CFLAGS="$(PKG_EXTRA_CFLAGS)" \
	$(EXTRA_KCONFIG)

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
	$(MAKE_OPTS) \
	modules
endef

ifeq ($(CONFIG_GOBINET_QUECTEL),y)
	SRCDIR:=./src/quectel/*
	PATCHDIR:=./patches/quectel
else
	SRCDIR:=./src/sierra/*
	PATCHDIR:=./patches/sierra
endif

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) $(SRCDIR) $(PKG_BUILD_DIR)
	@if [ -d $(PATCHDIR) -a "$$$$(ls $(PATCHDIR) | wc -l)" -gt 0 ]; then \
		$(KPATCH) $(PKG_BUILD_DIR) $(PATCHDIR); \
	fi
endef

$(eval $(call KernelPackage,gobinet))

