#
# Copyright (C) 2008-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk

PKG_NAME:=samba-tch
PKG_VERSION:=3.0.37
PKG_RELEASE:=5

PKG_MD5SUM:=11ed2bfef4090bd5736b194b43f67289

PKG_SOURCE_URL:=ftp://ftp.samba.org/pub/samba/old-versions/
PKG_SOURCE:=samba-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/samba-$(PKG_VERSION)
PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/nls.mk

define Package/samba-tch/Default
  SECTION:=samba-tch
  CATEGORY:=Technicolor
  SUBMENU:=Network
  MAINTAINER:=Technicolor <linuxgw@technicolor.com>
  URL:=http://www.samba.org/
endef

define Package/samba-tch
$(call Package/samba-tch/Default)
  TITLE:=SMB server for file and printer sharing
  DEPENDS:=+libpthread +libpopt $(ICONV_DEPENDS) +UCLIBC_BACKTRACE_SUPPORT:libubacktrace +SAMBA-TCH_CONFIG_PRINTING:cups +SAMBA-TCH_CONFIG_PRINTING:cups-client
endef

define Package/samba-tch-nmbd
$(call Package/samba-tch/Default)
  TITLE:=NetBIOS name server
  DEPENDS:=samba-tch
endef

define Package/samba-tch-mountcifs
$(call Package/samba-tch/Default)
  SECTION:=utils
  CATEGORY:=Technicolor
  SUBMENU:=Network
  TITLE:=Mount utility for samba shares
endef

define Package/samba-tch/description
SMB server for file and printer sharing
Also contains a SMB password utility (smbpasswd)

Made small with patches taken from AVM GPL releases and freetz 
endef

define Package/samba-tch-nmbd/description
NetBIOS name server

Made small with patches taken from AVM GPL releases and freetz 
endef

define Package/samba-tch-mountcifs/description
An user space helper utility for mounting remote CIFS shares.
endef

define Package/samba-tch/conffiles
/etc/config/samba
/etc/samba/smb.conf.template
endef

define Package/samba-tch/config
	source "$(SOURCE)/Config.in"
endef

TARGET_CFLAGS+=$(if $(CONFIG_SAMBA-TCH_CONFIG_DEBUG),-DSAMBA_DEBUG,-DMAX_DEBUG_LEVEL=2)
TARGET_CFLAGS+=$(if $(CONFIG_SAMBA-TCH_CONFIG_SMALLER),-DAVM_SMALLER)
TARGET_CFLAGS+=$(if $(CONFIG_SAMBA-TCH_CONFIG_PRINTING),,-DAVM_NO_PRINTING)
CONFIGURE_PATH:=source
CONFIGURE_ARGS+= \
	$(if $(CONFIG_SAMBA-TCH_CONFIG_DEBUG),--enable-debug) \
	$(if $(CONFIG_SAMBA-TCH_CONFIG_PRINTING),--enable-cups=yes --enable-iprint=yes,--enable-cups=no --enable-iprint=no) \
	--without-krb5 \
	--without-ads \
	--without-ldap \
	--enable-largefile \
	--with-configdir=/etc/samba \
	--with-libiconv=$(ICONV_PREFIX) \
	--with-privatedir=/etc/samba

# Make sure we tell the configure script that we support negative enum values and want to use setresuid
CONFIGURE_VARS+= \
	SMB_BUILD_CC_NEGATIVE_ENUM_VALUES=yes \
	samba_cv_USE_SETEUID=no \
	samba_cv_have_setresuid=yes \
	samba_cv_USE_SETRESUID=yes \
	samba_cv_HAVE_C99_VSNPRINTF=yes \
	samba_cv_have_longlong=yes \
	ac_cv_type_long_long=yes \

MAKE_PATH=source
MAKE_FLAGS += \
	$(if $(CONFIG_SAMBA-TCH_CONFIG_PRINTING),,OWRT_NO_PRINTING=1) \
	$(if $(CONFIG_SAMBA-TCH_CONFIG_SMALLER),OWRT_SMALLER=1)

define Build/Compile
	#remove version
	awk 'BEGIN { rc = 1 } \
	       /'define[[:space:]]SAMBA_VERSION_STRING'/ { $$$$3 = $(if $(CONFIG_SAMBA-TCH_REMOVE_VERSION),"\"\"",$$$$3 ); rc = 0 } \
	       { print } \
	       END { exit(rc) }' $(PKG_BUILD_DIR)/source/include/version.h \
	       >$(PKG_BUILD_DIR)/source/include/version.h.new && \
	mv $(PKG_BUILD_DIR)/source/include/version.h.new $(PKG_BUILD_DIR)/source/include/version.h || exit 1;
	# Ugly fix for parallel building (without this some generated files will be missing upon clean build)
	$(call Build/Compile/Default, proto_exists)
	$(call Build/Compile/Default, \
		$(if $(CONFIG_PACKAGE_samba-tch),bin/smbd bin/smbpasswd) \
		$(if $(CONFIG_PACKAGE_samba-tch-nmbd),bin/nmbd) \
		$(if $(CONFIG_PACKAGE_samba-tch-mountcifs),bin/mount.cifs bin/umount.cifs) \
	)
endef

define Package/samba-tch/install
	$(INSTALL_DIR) $(1)/etc/{samba,init.d,config}
	$(INSTALL_BIN) ./files/samba.init $(1)/etc/init.d/samba
	$(INSTALL_DATA) ./files/samba.config $(1)/etc/config/samba
	$(INSTALL_DATA) ./files/smb.conf.template $(1)/etc/samba/
	touch $(1)/etc/samba/smbpasswd
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/source/bin/smbpasswd $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/source/bin/smbd $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/etc/hotplug.d/mount
	$(INSTALL_BIN) ./files/00-samba $(1)/etc/hotplug.d/mount
	$(INSTALL_DIR) $(1)/etc/hotplug.d/usb
	$(INSTALL_BIN) ./files/21-samba-printer $(1)/etc/hotplug.d/usb
	$(INSTALL_DIR) $(1)/lib/functions
	$(INSTALL_BIN) ./files/samba_printer.sh $(1)/lib/functions
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) ./files/usr/bin/dfree $(1)/usr/bin/dfree

    # should be moved into its own package
	$(INSTALL_DIR) $(1)/lib/functions
	$(INSTALL_BIN) ./files/contentsharing.sh $(1)/lib/functions
	$(INSTALL_BIN) ./files/00-mounts $(1)/etc/hotplug.d/mount
	$(INSTALL_BIN) ./files/mountd.init $(1)/etc/init.d/mountd
	$(INSTALL_DATA) ./files/mounts.config $(1)/etc/config/mounts
endef

define Package/samba-tch-nmbd/install
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/samba-nmbd.init $(1)/etc/init.d/samba-nmbd
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/source/bin/nmbd $(1)/usr/sbin/
endef

define Package/samba-tch-mountcifs/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/source/bin/mount.cifs $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/source/bin/umount.cifs $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,samba-tch))
$(eval $(call BuildPackage,samba-tch-nmbd))
$(eval $(call BuildPackage,samba-tch-mountcifs))
