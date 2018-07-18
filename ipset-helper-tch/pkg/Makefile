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

PKG_NAME:=ipset-helper-tch
PKG_VERSION:=1.0
PKG_RELEASE:=1
PKG_SOURCE_URL:=git@gitolite-openwrt:openwrt/pkg/ipset-helper.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=5172a457bcc579e5ac983761d09b8ad635c03682
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/ipset-helper-tch
	SECTION:=ipset-helper
	CATEGORY:=Technicolor
	SUBMENU:=Network
	TITLE:=IPset configuration
	DEPENDS:=+libip4tc +libip6tc +libxtables +libuci +ipset
	MAINTAINER:=Technicolor <linuxgw@technicolor.com>
endef

define Package/ipset-helper-tch/description
	An application to manage ipset configurations via UCI
endef

define Package/ipset-helper-tch/conffiles
  /etc/config/ipset
endef

define Package/ipset-helper-tch/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/ipset-helper $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/ipset.init $(1)/etc/init.d/ipset
endef

define Package/ipset-helper-tch/postinst
#!/bin/sh
# check if we are on real system
if [ -z "$${IPKG_INSTROOT}" ]; then
        echo "Enabling ipset at start-up"
        /etc/init.d/ipset enable
	[ -f /etc/config/ipset ] || touch /etc/config/ipset
fi
exit 0
endef

define Package/ipset-helper-tch/prerm
#!/bin/sh
# check if we are on real system
if [ -z "$${IPKG_INSTROOT}" ]; then
        echo "Disabling ipset at start-up"
        /etc/init.d/ipset disable
fi
exit 0
endef

$(eval $(call BuildPackage,ipset-helper-tch))
