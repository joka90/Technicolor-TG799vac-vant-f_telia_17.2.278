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

PKG_NAME:=bcm963xx-wdt
PKG_VERSION:=2014.03
PKG_SOURCE_URL:=git@gitolite-openwrt:/openwrt/pkg/bcm963xx-wdt.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=5ad024aea7f092431033ae5fcdf82fc552502b78

PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

include $(INCLUDE_DIR)/package.mk

define KernelPackage/bcm963xx-wdt
  SUBMENU:=Technicolor specific kernel modules
  TITLE:=Watchdog kernel module for BCM963xx platform
  DEPENDS:=@TARGET_brcm63xx_tch||TARGET_brcm68xx_tch
  FILES:=$(PKG_BUILD_DIR)/bcm963xx-wdt.ko
  KCONFIG:=
  MAINTAINER:=Technicolor <linuxgw@technicolor.com>
  AUTOLOAD:=$(call AutoLoad,00,bcm963xx-wdt,1)
  DEFAULT:=y if (TARGET_brcm63xx_tch)
endef

define KernelPackage/bcm963xx-wdt/description
 Kernel module that manages the watchdog on the bcm963xx platform.
endef

EXTRA_KCONFIG:= \
	CONFIG_BCM963XX_WDT=m

ifneq ($(CONFIG_TARGET_brcm68xx_tch),)
GPON_EXTRA_CFLAGS:=-I$(LINUX_DIR)/../../shared/opensource/include/bcm968500/drv
endif

ifdef CONFIG_PACKAGE_kmod-brcm-4.14L.04A
    BCMRELEASE_EXTRA_CFLAGS := -DBCMRELEASE_414L04A
endif

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	EXTRA_CFLAGS="$(GPON_EXTRA_CFLAGS) $(BCMRELEASE_EXTRA_CFLAGS) -I$(LINUX_DIR)/../../shared/broadcom/include/bcm963xx -I$(LINUX_DIR)/../../shared/opensource/include/bcm963xx -I$(LINUX_DIR)/../../bcmdrivers/opensource/include/bcm963xx" \
	$(EXTRA_KCONFIG)

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

$(eval $(call KernelPackage,bcm963xx-wdt))
