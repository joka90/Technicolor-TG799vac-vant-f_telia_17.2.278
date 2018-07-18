include $(TOPDIR)/rules.mk

PKG_NAME:=miniupnpd-tch
PKG_VERSION:=1.0
PKG_SOURCE_URL:=git@gitolite-openwrt:/openwrt/pkg/miniupnpd-tch
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=2b266b4c8dd2ab9c46c058c162d9347a959929db
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

include $(TOPDIR)/rules.mk

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(BUILD_VARIANT)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/miniupnpd-tch/Default
  SECTION:=net
  CATEGORY:=Technicolor
  SUBMENU:=Network
  TITLE:=Lightweight UPnP Daemon
  DEPENDS:=+libopenssl +libnfnetlink +libuuid +librt +iptables +libip4tc +IPV6:libip6tc +CUSTO-xdsl:xdslctl-lua-wrapper
  MAINTAINER:=Technicolor <linuxgw@technicolor.com>
endef

define Package/miniupnpd-tch
$(call Package/miniupnpd-tch/Default)
  VARIANT:=scripts
endef

define Package/miniupnpd-tch-igdv2
$(call Package/miniupnpd-tch/Default)
  TITLE += (with igdv2 support)
  DEPENDS:=miniupnpd-tch
  VARIANT:=igdv2
endef

define Package/miniupnpd-tch-igdv1
$(call Package/miniupnpd-tch/Default)
  TITLE += (with igdv1 support)
  DEPENDS:=miniupnpd-tch
  VARIANT:=igdv1
endef

define Package/miniupnpd-tch/description
	lightweight upnp daemon
endef

ifeq ($(BUILD_VARIANT),igdv2)
	TARGET_CFLAGS += -DIGD_V2
endif

define Package/miniupnpd-tch/install
	$(INSTALL_DIR) $(1)/usr/sbin $(1)/usr/bin $(1)/etc/init.d $(1)/etc/config $(1)/etc/uci-defaults $(1)/etc/hotplug.d/iface $(1)/usr/share/miniupnpd-tch
	$(INSTALL_BIN) ./files/miniupnpd-tch.init $(1)/etc/init.d/miniupnpd-tch
	$(INSTALL_BIN) ./files/usr/bin/upnp_wgclp $(1)/usr/bin/upnp_wgclp
	$(INSTALL_CONF) ./files/upnpd.config $(1)/etc/config/upnpd
	$(INSTALL_DATA) ./files/miniupnpd-tch.hotplug $(1)/etc/hotplug.d/iface/50-miniupnpd-tch
	$(INSTALL_DATA) ./files/miniupnpd-tch.defaults $(1)/etc/uci-defaults/99-miniupnpd-tch
	$(INSTALL_DATA) ./files/firewall.include $(1)/usr/share/miniupnpd-tch/firewall.include
endef

define Package/miniupnpd-tch-igdv2/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bin/miniupnpd $(1)/usr/sbin/miniupnpd-igdv2
endef

define Package/miniupnpd-tch-igdv1/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bin/miniupnpd $(1)/usr/sbin/miniupnpd-igdv1
endef

$(eval $(call BuildPackage,miniupnpd-tch))
$(eval $(call BuildPackage,miniupnpd-tch-igdv2))
$(eval $(call BuildPackage,miniupnpd-tch-igdv1))
