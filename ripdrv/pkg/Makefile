#
# Copyright (c) 2012 Technicolor
# All Rights Reserved
#
# This program contains proprietary information which is a trade
# secret of TECHNICOLOR and/or its affiliates and also is protected as
# an unpublished work under applicable Copyright laws. Recipient is
# to retain this program in confidence and is not permitted to use or
# make copies thereof other than as permitted in a written agreement
# with TECHNICOLOR, UNLESS OTHERWISE EXPRESSLY ALLOWED BY APPLICABLE LAWS.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ripdrv
PKG_VERSION:=2014.01
PKG_SOURCE_URL:=git@gitolite-openwrt:/openwrt/pkg/rip.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=f8f09b2422274bde6a436f0a4e52e45e89a686b2
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz
PKG_BUILD_DEPENDS:=ripdrv/host
PROVIDES:=ripimport

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/host-build.mk

define KernelPackage/ripdrv
  SUBMENU:=Technicolor specific kernel modules
  TITLE:=RIP driver (Remote Inventory Prom)
  DEPENDS:=+kmod-tommath +kmod-tomcrypt +kmod-platform
  FILES:=$(PKG_BUILD_DIR)/ripdrv.ko
  KCONFIG:=
  MAINTAINER:=Technicolor <linuxgw@technicolor.com>
endef

define KernelPackage/ripdrv/description
 Kernel module that provides access to the eRIPv2 on Technicolor gateways.
endef

define KernelPackage/ripdrv/config
	config RIP_ON_UBI
		bool "Enable UBI support in RIP"
		default n
		depends on PACKAGE_kmod-ripdrv
		help
			Enable UBI support in RIP driver

	config RIPDRV_CRYPTO
		bool "Support for encrypted parameters"
		depends on PACKAGE_kmod-ripdrv
		default n
		help
			Support for encrypted parameters on secureboot platforms

	config RIPDRV_EXPOSE_PRIVATES
		bool "Expose private data (e.g. client cert) in procfs"
		default n
		depends on RIPDRV_CRYPTO
		help
			Expose private data (e.g. client cert) in procfs. Normally this should not be done
			as this is a security risk! Private data should be protected with SPF.

	config RIPDRV_INTEGRITY_ONLY
		bool "RIP driver with only integrity check support"
		default n
		depends on RIPDRV_CRYPTO
		help
			RIP driver with only integrity check support. This is for eRIP for platforms that are signed and not encrypted.

	config RIPDRV_SEPARATE_CERT_PRIVKEY
		bool "Separate CERT and Privkey"
		default n
		depends on RIPDRV_CRYPTO
		help
			Separate CERT and Privkey

	config RIPDRV_ANVIL
		bool

endef

ifneq ($(CONFIG_TARGET_brcm68xx_tch),)
  PKG_EXTRA_CFLAGS += \
	-DSET_GPON_SERIAL_NUM
endif

ifneq ($(CONFIG_TARGET_comcerto2000)$(CONFIG_TARGET_brcm63xx_tch)$(CONFIG_TARGET_brcm63xx_arm_tch)$(CONFIG_TARGET_brcm68xx_tch),)
  PKG_EXTRA_CFLAGS += \
	-DSET_MAC_ADDRESS
endif

ifneq ($(CONFIG_ARCH_BCM63XX),)
  PKG_EXTRA_CFLAGS += \
	-I$(LINUX_DIR)/../../shared/opensource/include/bcm963xx
endif

EXTRA_KCONFIG:= \
       CONFIG_RIPDRV=m

ifneq ($(CONFIG_RIPDRV_CRYPTO),)
EXTRA_KCONFIG += \
	CONFIG_RIPDRV_CRYPTO_SUPPORT=y
PKG_EXTRA_CFLAGS += \
	-DCONFIG_RIPDRV_CRYPTO_SUPPORT \
	-DDEBUG
endif

ifneq ($(CONFIG_RIPDRV_EXPOSE_PRIVATES),)
EXTRA_KCONFIG += \
	CONFIG_RIPDRV_EXPOSE_PRIVATES=y
PKG_EXTRA_CFLAGS += \
	-DCONFIG_RIPDRV_EXPOSE_PRIVATES
endif

ifneq ($(CONFIG_RIPDRV_SEPARATE_CERT_PRIVKEY),)
EXTRA_KCONFIG += \
	CONFIG_RIPDRV_SEPARATE_CERT_PRIVKEY=y
PKG_EXTRA_CFLAGS += \
	-DCONFIG_RIPDRV_SEPARATE_CERT_PRIVKEY
endif

ifneq ($(CONFIG_RIPDRV_INTEGRITY_ONLY),)
EXTRA_KCONFIG += \
	CONFIG_RIPDRV_INTEGRITY_ONLY=y
PKG_EXTRA_CFLAGS += \
	-DCONFIG_RIPDRV_INTEGRITY_ONLY
endif

ifneq ($(CONFIG_RIPDRV_ANVIL),)
EXTRA_KCONFIG += \
	CONFIG_RIPDRV_ANVIL=y
PKG_EXTRA_CFLAGS += \
	-DCONFIG_RIPDRV_ANVIL
endif

ifneq ($(CONFIG_RIP_ON_UBI),)
EXTRA_KCONFIG += \
	CONFIG_RIP_ON_UBI=y
endif

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	EXTRA_CFLAGS="$(PKG_EXTRA_CFLAGS)" \
	$(EXTRA_KCONFIG)

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

define Build/InstallDev
	$(INSTALL_DIR) $(STAGING_DIR)/usr/include
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/include/*.h $(STAGING_DIR)/usr/include
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/target/linux/include/*.h $(STAGING_DIR)/usr/include
endef

define KernelPackage/ripdrv/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) ./files/*.sh $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/etc/uci-defaults
	$(INSTALL_BIN) ./files/00-ripcontent $(1)/etc/uci-defaults/tch_0000-ripcontent
	$(INSTALL_DIR) $(1)/lib/preinit
	$(INSTALL_DATA) ./files/lib/preinit/97_rip $(1)/lib/preinit/97_rip
	$(INSTALL_DATA) ./files/lib/preinit/98_boardname $(1)/lib/preinit/98_boardname
endef

define KernelPackage/ripdrv-crypto/install
	$(call KernelPackage/ripdrv/install,$(1))
endef

$(eval $(call KernelPackage,ripdrv))

define Host/Configure
	(cd $(HOST_BUILD_DIR); \
		CFLAGS="$(HOST_CFLAGS) $(EXTRA_CFLAGS)" \
		CXXFLAGS="$(HOST_CFLAGS) $(EXTRA_CFLAGS)" \
		LDFLAGS="$(HOST_LDFLAGS) $(EXTRA_LDFLAGS)" \
		cmake \
			-DCMAKE_INSTALL_PREFIX=$(STAGING_DIR_HOST) \
	)
endef

$(eval $(call HostBuild))
