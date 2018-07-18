#
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=libtommath
PKG_VERSION:=1.0
PKG_SOURCE_URL:=https://github.com/libtom/libtommath
PKG_SOURCE_VERSION:=b8527e92f8401a79b86e779caaaf00dca83f73c7
PKG_SOURCE_PROTO:=git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.bz2
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define KernelPackage/tommath
  SUBMENU:=Cryptographic API modules
  TITLE:=Tommath
  DEPENDS:=
  FILES:=$(PKG_BUILD_DIR)/tommath.ko
  KCONFIG:=
  AUTOLOAD:=$(call AutoLoad,47,tommath)
  MAINTAINER:=Technicolor <linuxgw@technicolor.com>
endef

define KernelPackage/tommath/description
 Kernel module to provide tommath math functions
endef

EXTRA_KCONFIG:= \
	CONFIG_CRYPTO_TOMMATH=m

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	$(EXTRA_KCONFIG)

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) TARGET_CFLAGS=-Wframe-larger-than=8192 \
		modules
endef

define Build/InstallDev
	$(INSTALL_DIR) $(STAGING_DIR)/usr/include
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/*.h $(STAGING_DIR)/usr/include
endef

$(eval $(call KernelPackage,tommath))
