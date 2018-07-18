#
# Copyright (c) 2013 Technicolor
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

PKG_NAME:=technicolor-led
PKG_VERSION:=1.0
PKG_SOURCE_URL:=git@gitolite-openwrt:/openwrt/pkg/leds.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=2df37894291d84bb550cc1bb2c06d3f3a105fa52
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

include $(INCLUDE_DIR)/package.mk


define KernelPackage/technicolor-led
  SUBMENU:=LED modules
  TITLE:=LED support for Technicolor Linux Gateways
  DEPENDS:=+kmod-leds-gpio
  FILES:=$(PKG_BUILD_DIR)/technicolor_led.ko
  KCONFIG:=
  MAINTAINER:=Technicolor <linuxgw@technicolor.com>
  AUTOLOAD:=
  DEFAULT:=y
endef

define KernelPackage/technicolor-led/description
 Kernel module to provide LED support for Technicolor Gateways.
endef

EXTRA_KCONFIG:= \
	CONFIG_TECHNICOLOR_LED=m

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	EXTRA_CFLAGS="-DBOARD_$(SUBTARGET) -DBRCM_CHIP_$(BRCM_CHIP) -I$(LINUX_DIR)/../../shared/opensource/include/bcm963xx"\
	SUBDIRS="$(PKG_BUILD_DIR)" \
	$(EXTRA_KCONFIG)

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

$(eval $(call KernelPackage,technicolor-led))
