
include $(TOPDIR)/rules.mk

PKG_NAME:=conf-ipset
PKG_VERSION:=1.0

include $(INCLUDE_DIR)/package.mk

#CUSTO STUFF, define before including custo.mk
PKG_CUSTO_FILE_PREFIXES:=conf-ipset
include $(INCLUDE_DIR)/tch/custo.mk

define Package/conf-ipset
  SECTION:=conf-ipset
  CATEGORY:=Technicolor
  SUBMENU:=Configuration
  TITLE:=Default Ipset config
  DEPENDS:=+ipset-helper-tch
  MAINTAINER:=Technicolor <linuxgw@technicolor.com>
endef

define Package/conf-ipset/description
  Configuration for Ipset.
endef

define Package/conf-ipset/install
	$(INSTALL_DIR) $(1)/etc/custo/config
	$(INSTALL_DATA) $(conf-ipset_CUSTO_SELECTED) $(1)/etc/custo/config/ipset
endef

$(eval $(call BuildPackage,conf-ipset))
