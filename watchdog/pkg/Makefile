#Copyright (C) 1996-2012 Michael Meskes

#WATCHDOG is free software; you can redistribute it and/or modify it under
#the terms of the GNU General Public License as published by the Free
#Software Foundation; either version 2, or (at your option) any later
#version.

#WATCHDOG is distributed in the hope that it will be useful, but WITHOUT
#ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
#FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
#more details.

include $(TOPDIR)/rules.mk

PKG_NAME:=watchdog
PKG_VERSION:=5.13
PKG_SOURCE_URL:=git://git.code.sf.net/p/watchdog/code
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=10f6c916e3ebe076cb7bf3fdd7a80c0898d6d01a
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

PKG_FIXUP:=autoreconf
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

ifneq ($(CONFIG_BUSYBOX_CONFIG_WATCHDOG),)
   $(error the watchdog package is mutually exclusive with the busybox watchdog)
endif

define Package/watchdog
	SECTION:=watchdog
	CATEGORY:=Technicolor
	SUBMENU:=General
	TITLE:=Watchdog daemon
	MAINTAINER:=Technicolor <linuxgw@technicolor.com>
endef

define Package/watchdog/description
	Watchdog is a daemon that checks if your system is still working.
endef

CONFIGURE_ARGS += \
	--disable-nfs

# This watchdog executable has to be installed with a different name,
# else conflicts will happen with the busybox watchdog scripts, which
# are stored in hotplug2 and base-files.
# This way, no patches have to be done to base-files or hotplug2 as
# they check for the existance of /sbin/watchdog
#
# /etc/init.d/watchdog from base-files can be removed in the target
# specific base-files.mk install-target rule.
define Package/watchdog/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/watchdog $(1)/usr/sbin/watchdog-tch
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/watchdog.init $(1)/etc/init.d/watchdog-tch
	$(INSTALL_DIR) $(1)/lib/upgrade
	$(INSTALL_BIN) ./files/watchdog.sysupgrade $(1)/lib/upgrade/watchdog.sh
endef

$(eval $(call BuildPackage,watchdog))
