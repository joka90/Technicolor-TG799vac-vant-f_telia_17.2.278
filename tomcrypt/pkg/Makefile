#
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=libtomcrypt
PKG_VERSION:=1.17
PKG_SOURCE_URL:=https://github.com/libtom/libtomcrypt
PKG_SOURCE_VERSION:=f7847938919a15732884641be6bee3e8fa5862dc
PKG_SOURCE_PROTO:=git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.bz2
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define KernelPackage/tomcrypt
  SUBMENU:=Cryptographic API modules
  TITLE:=Tomcrypt
  DEPENDS:= kmod-tommath
  FILES:=$(PKG_BUILD_DIR)/tomcrypt.ko
  KCONFIG:=
  AUTOLOAD:=$(call AutoLoad,48,tomcrypt)
  MAINTAINER:=Technicolor <linuxgw@technicolor.com>
endef

define KernelPackage/tomcrypt/description
 Kernel module to provide tomcrypt crypto functions
endef

EXTRA_KCONFIG:= \
	CONFIG_CRYPTO_TOMCRYPT=m

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	$(EXTRA_KCONFIG)

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

define Build/InstallDev
	$(INSTALL_DIR) $(STAGING_DIR)/usr/include
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/src/headers/*.h $(STAGING_DIR)/usr/include
endef

$(eval $(call KernelPackage,tomcrypt))
