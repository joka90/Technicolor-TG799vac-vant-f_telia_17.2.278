#
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ncr
PKG_VERSION:=0.2-tch
PKG_SOURCE_URL:=git@gitolite-openwrt:openwrt/pkg/ncr-tch
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=37a6ec231b29dcb2af0bca04c51ef18f3b3f07aa
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

PKG_EXTRA_CFLAGS += -DCONFIG_CRYPTO_USERSPACE_ASYMMETRIC
PKG_EXTRA_CFLAGS += -DKEY_PERSISTENCE -I$(PKG_BUILD_DIR)/libtommath -I$(PKG_BUILD_DIR)/libtomcrypt/headers -I$(PKG_BUILD_DIR)/ -DLTC_SOURCE -Wall

include $(INCLUDE_DIR)/package.mk

define KernelPackage/ncr
	 SUBMENU:=Technicolor specific kernel modules
	TITLE:=NCR Linux-kenel cyptographic framework module
	DEPENDS:=+kmod-crypto-cbc +kmod-crypto-ecb +kmod-crypto-iv
	FILES:=$(PKG_BUILD_DIR)/ncrmod.ko
	KCONFIG:=
	AUTOLOAD:=$(call AutoLoad,48,ncrmod)
	MAINTAINER:=Technicolor <linuxgw@technicolor.com>
endef

define Package/ncr-setkey
	CATEGORY:=Technicolor
	SUBMENU:=Testing
	TITLE:=Test Tool Used to Set NCR Master KEY
	DEPENDS:=+kmod-ncr
	MAINTAINER:=Technicolor <linuxgw@technicolor.com>
endef

define KernelPackage/ncr/description
 Kernel module to provide in-kernel cyptographic framework functions
endef

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	EXTRA_CFLAGS="$(PKG_EXTRA_CFLAGS)" \
	$(EXTRA_KCONFIG)

define Build/Compile/kmod
	$(MAKE) -C "$(PKG_BUILD_DIR)" \
		$(MAKE_OPTS) \
		version.h
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

define Build/Compile/ncr-setkey
	$(MAKE) -C $(PKG_BUILD_DIR)/userspace \
		$(TARGET_CONFIGURE_OPTS) \
		CFLAGS="$(TARGET_CFLAGS) -DKEY_PERSISTENCE -I$(PKG_BUILD_DIR)/userspace" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		ncr-setkey

	$(MAKE) -C $(PKG_BUILD_DIR)/userspace \
		$(TARGET_CONFIGURE_OPTS) \
		CFLAGS="$(TARGET_CFLAGS) -DKEY_PERSISTENCE -I$(PKG_BUILD_DIR)/userspace" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		lskeys

	$(MAKE) -C $(PKG_BUILD_DIR)/tests \
		$(TARGET_CONFIGURE_OPTS) \
		CFLAGS="$(TARGET_CFLAGS) -I$(PKG_BUILD_DIR)/tests" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		ncr_aes_test
endef

define Build/Compile
	$(Build/Compile/kmod)
	$(Build/Compile/ncr-setkey)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(STAGING_DIR)/usr/include/
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/*.h $(STAGING_DIR)/usr/include/
endef

define Package/ncr-setkey/install
	$(INSTALL_DIR) $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/userspace/ncr-setkey $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/userspace/lskeys $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tests/ncr_aes_test $(1)/usr/bin/
endef

$(eval $(call KernelPackage,ncr))
$(eval $(call BuildPackage,ncr-setkey))
