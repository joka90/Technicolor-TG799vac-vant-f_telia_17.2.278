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

PKG_NAME:=technicolor-button
PKG_VERSION:=1.0
PKG_SOURCE_URL:=git@gitolite-openwrt:/openwrt/pkg/buttons.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=6c3a3a2e7fcc910b9085fc7f53219bb60107a498
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

include $(INCLUDE_DIR)/package.mk


define KernelPackage/technicolor-button
  SUBMENU:=Other modules
  TITLE:=Button support for Technicolor gateways 
  DEPENDS:=@(TARGET_brcm63xx_tch||TARGET_brcm68xx_tch||TARGET_comcerto2000||TARGET_ar71xx_tch||TARGET_mdm9x07) +kmod-button-hotplug +kmod-input-gpio-keys-polled
  FILES:=$(PKG_BUILD_DIR)/technicolor_button.ko
  KCONFIG:=
  MAINTAINER:=Technicolor <linuxgw@technicolor.com>
  AUTOLOAD:=$(call AutoLoad,01,technicolor_button)
  DEFAULT:=y if (TARGET_brcm63xx_tch||TARGET_brcm68xx_tch||TARGET_comcerto2000||TARGET_ar71xx_tch||TARGET_mdm9x07)
endef

define KernelPackage/technicolor-button/description
 Kernel module to provide button support for Technicolor gateways.
endef

EXTRA_KCONFIG:= \
	CONFIG_TECHNICOLOR_BUTTON=m

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	EXTRA_CFLAGS="-DBOARD_$(SUBTARGET)"\
	$(EXTRA_KCONFIG)

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

define KernelPackage/technicolor-button/install
	$(INSTALL_DIR) $(1)/etc/hotplug.d/button
	$(INSTALL_DATA) ./files/etc/hotplug.d/button/00-button $(1)/etc/hotplug.d/button/00-button
endef

$(eval $(call KernelPackage,technicolor-button))
