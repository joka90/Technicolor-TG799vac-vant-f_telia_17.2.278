#
# Copyright (C)  2016 technicolor
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=udns
PKG_VERSION:=0.4
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://www.corpit.ru/mjt/udns/
PKG_MD5SUM:=51e141b044b078d71ebb71f823959c1b

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)/Default
  SECTION:=net
  CATEGORY:=Technicolor
  SUBMENU:=Network
  MAINTAINER:=Technicolor <linuxgw@technicolor.com>
  PKGARCH:=all
endef

define Package/$(PKG_NAME)
    $(call Package/$(PKG_NAME)/Default)
    TITLE:=DNS query tools
endef

define Package/$(PKG_NAME)/description
	Simple configuration: support DNS query by Qtype,timeout,attempts
endef

define Build/Configure
	(cd $(PKG_BUILD_DIR) ;\
		./configure )
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -std=gnu99"
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/usr/bin/* $(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))

