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

PKG_NAME:=bankmgr
PKG_VERSION:=1.0
PKG_SOURCE_URL:=git@gitolite-openwrt:/openwrt/pkg/bankmgr.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=584b1258adea34c8f1fa13fadb02164498c410a2


PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

include $(INCLUDE_DIR)/package.mk




define KernelPackage/bankmgr
  SUBMENU:=Technicolor specific kernel modules
  TITLE:=Bankmanager kernel module
  DEPENDS:= @TARGET_brcm63xx_tch||TARGET_brcm68xx_tch
  FILES:=$(PKG_BUILD_DIR)/bankmgr.ko
  KCONFIG:=
  MAINTAINER:=Technicolor <linuxgw@technicolor.com>
  AUTOLOAD:=$(call AutoLoad,43,bankmgr,1)
  DEFAULT:=
endef

define KernelPackage/bankmgr/description
 Kernel module that provides access to the banktable via a proc interface.
endef

EXTRA_KCONFIG:= \
	CONFIG_BANKMGR=m CONFIG_RAWSTORAGE_VERSION=2

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	$(EXTRA_KCONFIG)

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

$(eval $(call KernelPackage,bankmgr))
