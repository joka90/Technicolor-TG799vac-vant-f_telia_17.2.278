
libxml2: conditional_build 

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

export LINUX_VER_STR TOOLCHAIN_PREFIX
CONFIG_ID_FILE = BRCM_CONFIG_ID_$(TOOLCHAIN_PREFIX)_$(LINUX_VER_STR)


ifneq ($(strip $(BUILD_LIBXML2)),)
conditional_build: all
else
conditional_build:
	@echo "skipping libxml2 (not configured)"
endif


check_untar:
ifeq ($(wildcard libxml2/configure),)
	mkdir -p libxml2
	cd libxml2 ; tar --strip-components=1 -xzf ../libxml2-sources-2.9.0.tar.gz
endif
	@echo "libxml2 is untarred"

check_config: check_untar
ifeq ($(wildcard libxml2/$(CONFIG_ID_FILE)),)
	-cd libxml2  && make distclean
	rm -f libxml2/BRCM_CONFIG_ID_*
endif
	-([ -e libxml2/configure ] || (cd libxml2 ; sh autogen.sh))
	cd libxml2 ; ./configure --host=$(TOOLCHAIN_PREFIX) --with-python=no --without-zlib --prefix=$(BCM_FSBUILD_DIR)/public/
	cd libxml2 ; touch $(CONFIG_ID_FILE)

all: check_config
	mkdir -p $(INSTALL_DIR)/lib/public/
	cd libxml2 ; make
	cd libxml2 ; make install
	cp -d $(BCM_FSBUILD_DIR)/public/lib/libxml2*.so* $(INSTALL_DIR)/lib/public/

clean:
	rm -f $(INSTALL_DIR)/lib/public/libxml2*.so*
	-[ -e libxml2/Makefile ] && cd libxml2 && make clean uninstall

bcm_dorel_distclean: distclean

distclean: clean
	rm -f libxml2

shell:
	bash -i

