#
# Copyright (c) 2016 Technicolor
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

PKG_NAME:=dhcpsnooper-tch
PKG_VERSION:=1.1
PKG_SOURCE_URL:=git@gitolite-openwrt:/openwrt/pkg/dhcpsnooper.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=b31ae502e9e63c17e001be056352afabb7e0174b
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

PKG_CONFIG_DEPENDS :=

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/dhcpsnooper-tch
  SECTION:=dhcpsnooper
  CATEGORY:=Technicolor
  SUBMENU:=Network
  TITLE:=DHCP snooper
  DEPENDS:=+libubox +libuci +libubus +libnetfilter-queue +iptables-mod-nfqueue
  MAINTAINER:=Alin Nastac <alin.nastac@technicolor.com>
endef

define Package/dhcpsnooper-tch/description
  Daemon implementing layer 2 DHCP snooping.
endef

define Package/dhcpsnooper-tch/conffiles
  /etc/config/dhcpsnooping
endef

define Package/dhcpsnooper-tch/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/dhcpsnooper $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/$(PKG_NAME).init $(1)/etc/init.d/dhcpsnooper
	$(INSTALL_DIR) $(1)/lib/functions
	$(INSTALL_DIR) $(1)/etc/uci-defaults
	$(INSTALL_DATA) ./files/$(PKG_NAME).defaults $(1)/etc/uci-defaults/70-dhcpsnooper
	$(INSTALL_DATA) ./files/firewall.include $(1)/lib/functions/firewall-dhcpsnooper.sh
endef

define Package/dhcpsnooper-tch/postinst
#!/bin/sh
# check if we are on real system
if [ -z "$${IPKG_INSTROOT}" ]; then
        echo "Enabling DHCP snooper at start-up"
        /etc/init.d/dhcpsnooper enable
	[ -f /etc/config/dhcpsnooping ] || touch /etc/config/dhcpsnooping 
fi
exit 0
endef

define Package/dhcpsnooper-tch/prerm
#!/bin/sh
# check if we are on real system
if [ -z "$${IPKG_INSTROOT}" ]; then
        echo "Disabling DHCP snooper at start-up"
        /etc/init.d/dhcpsnooper disable
fi
exit 0
endef

$(eval $(call BuildPackage,dhcpsnooper-tch))
