include $(TOPDIR)/rules.mk

PKG_NAME:=mini-tr64
PKG_VERSION:=1.0
PKG_SOURCE_URL:=git@gitolite-openwrt:/openwrt/pkg/mini-tr64.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=62b490c975867e2ec02a40d06830ee0388346fc8
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/mini-tr64
	SECTION:=minitr064d
	CATEGORY:=Technicolor
	SUBMENU:=Management
	TITLE:=Mini TR064
	DEPENDS:= +libopenssl +libnfnetlink +libuuid +librt +mappings-minitr064d
	MAINTAINER:=Technicolor <linuxgw@technicolor.com>
endef

define Package/mini-tr64/description
	reduced TR-064 implementation
endef

define Package/mini-tr64/install
	$(INSTALL_DIR) $(1)/usr/sbin/ $(1)/etc/init.d $(1)/etc/config $(1)/etc/hotplug.d/iface
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bin/minitr064d $(1)/usr/sbin/minitr064d
	$(INSTALL_BIN) ./files/minitr064d.init $(1)/etc/init.d/minitr064d
	$(INSTALL_DATA) ./files/minitr064d.hotplug $(1)/etc/hotplug.d/iface/50-minitr064d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bin/computeHA1 $(1)/usr/sbin/computeHA1
endef

$(eval $(call BuildPackage,mini-tr64))
