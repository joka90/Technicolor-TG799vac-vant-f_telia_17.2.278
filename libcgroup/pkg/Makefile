#
# Copyright (C) 2006-2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libcgroup
PKG_VERSION:=0.41
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=https://sourceforge.net/projects/libcg/files/libcgroup/v$(PKG_VERSION)
PKG_MD5SUM:=3dea9d50b8a5b73ff0bf1cdcb210f63f

PKG_LICENSE:=LGPL-2.1
PKG_LICENSE_FILES:=COPYING

PKG_FIXUP:=autoreconf
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/libcgroup
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=Library for config cgroup
  DEPENDS:=+@KERNEL_CONNECTOR +@KERNEL_PROC_EVENTS +libpthread +librt
  URL:=https://sourceforge.net/projects/libcg
  MAINTAINER:=libcg-bugs <libcg-bugs@lists.sourceforge.net>
endef

define Package/libcgroup/description
  Control groups provides a file system interface to manipulate and control
  the details on task grouping including creation of new task groups (control groups),
  permission handling and task assignment.
endef

# don't build pam module to avoid cross compile error on some host
CONFIGURE_ARGS += \
          --enable-pam=no 

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libcgroup.{la,so*} $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/* $(1)/usr/lib/pkgconfig/
endef

define Package/libcgroup/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libcgroup.so* $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/* $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/usr/sbin
	$(CP) $(PKG_INSTALL_DIR)/usr/sbin/* $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/cgconfig $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/cgred $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_BIN) ./files/etc/cgconfig.conf $(1)/etc
	$(INSTALL_BIN) ./files/etc/cgrules.conf $(1)/etc
endef

$(eval $(call BuildPackage,libcgroup))
