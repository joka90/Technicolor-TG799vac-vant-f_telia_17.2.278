
openssl: conditional_build 

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

export LINUX_VER_STR TOOLCHAIN_PREFIX
CONFIG_ID_FILE = BRCM_CONFIG_ID_$(TOOLCHAIN_PREFIX)_$(LINUX_VER_STR)

ifeq ($(strip $(DO_BUILD_OPENSSL))_$(strip $(BUILD_OPENSSL_VERSION_1_0_1E)),y_y)
conditional_build: all
else
conditional_build:
	@echo "skipping openssl 1.0.1e (not configured)"
endif


check_untar:
ifeq ($(wildcard openssl-1.0.1e/Configure),)
	(tar xkzf openssl-1.0.1e.tar.gz 2> /dev/null || true)
	echo "Applying patches to openssl-1.0.1e"
	patch -R -p1 -b -dopenssl-1.0.1e < bcm_patches/lib_config.patch
	patch -R -p1 -b -dopenssl-1.0.1e < bcm_patches/no_heartbeats.patch
	ln -s ../../crypto/rc5/rc5.h openssl-1.0.1e/include/openssl/rc5.h
endif
	@echo "openssl-1.0.1e is untarred"

check_config: check_untar
ifeq ($(wildcard openssl-1.0.1e/$(CONFIG_ID_FILE)),)
	-[ -e openssl-1.0.1e/Makefile ] && cd openssl-1.0.1e  && make clean
	rm -f openssl-1.0.1e/BRCM_CONFIG_ID_*
endif
	touch openssl-1.0.1e/$(CONFIG_ID_FILE)

all: check_config
	mkdir -p $(INSTALL_DIR)/lib/public/
	$(MAKE) -j 1 -C openssl-1.0.1e all
	$(MAKE) -j 1 -C openssl-1.0.1e install

clean:
	-[ -e openssl-1.0.1e/Makefile ] && cd openssl-1.0.1e && make clean

distclean:
	rm -rf openssl-1.0.1e

shell:
	bash -i

