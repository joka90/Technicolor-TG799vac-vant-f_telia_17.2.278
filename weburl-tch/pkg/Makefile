#
# Copyright (c) 2014 Technicolor
# All Rights Reserved
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk


PKG_NAME:=weburl-tch
PKG_VERSION:=1.1
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/weburl-tch
  SECTION:=Firewall
  CATEGORY:=Technicolor
  SUBMENU:=Network
  TITLE:=Technicolor WEB URL filter for parental control
  DEPENDS:=+kmod-conntrack-http +kmod-ipt-weburl +iptables-mod-weburl
  MAINTAINER:=Technicolor <linuxgw@technicolor.com>
endef

define KernelPackage/conntrack-http
TITLE:=Netfilter HTTP helper module
SECTION:=kernel
SUBMENU:=Netfilter Extensions
KCONFIG:=CONFIG_NF_CONNTRACK_HTTP
DEPENDS:=+iptables-mod-conntrack-extra +kmod-nf-conntrack
FILES:=$(LINUX_DIR)/net/netfilter/nf_conntrack_http.ko
endef

define KernelPackage/conntrack-http/description
 Kernel module to redirect HTTP request to given URL
endef

define Build/Compile
endef

define KernelPackage/ipt-weburl
TITLE:=Netfilter WEB URL filter match extension kernel module
SECTION:=kernel
SUBMENU:=Netfilter Extensions
KCONFIG:=CONFIG_NETFILTER_XT_MATCH_WEBURL
DEPENDS:=+kmod-nf-conntrack +iptables-mod-conntrack-extra +kmod-nf-ipt
FILES:=$(LINUX_DIR)/net/netfilter/xt_weburl.ko
AUTOLOAD:=$(call AutoLoad,80,xt_weburl)
endef

define Package/weburl-tch/description
  Technicolor WEB URL Filter for parental control site blocking.
endef

define Package/weburl-tch/install
	$(INSTALL_DIR) $(1)/etc/uci-defaults
	$(INSTALL_BIN) files/etc/uci-defaults/weburl-tch.init $(1)/etc/uci-defaults/tch_0080-$(PKG_NAME)

	$(INSTALL_DIR) $(1)/lib/functions
	$(INSTALL_BIN) files/lib/functions/weburl-tch.sh $(1)/lib/functions
	$(INSTALL_DATA) files/etc/weburl_cfg_tmpl $(1)/etc
endef

include $(INCLUDE_DIR)/kernel-defaults.mk

$(eval $(call BuildPackage,weburl-tch))
$(eval $(call KernelPackage,conntrack-http))
$(eval $(call KernelPackage,ipt-weburl))

