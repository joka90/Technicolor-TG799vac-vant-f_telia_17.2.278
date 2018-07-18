
libjsonc: conditional_build 

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

ifeq ($(strip $(BRCM_USER_SSP)),y)
CFLAGS+=$(SSP_TYP_COMPILER_OPTS)
LDFLAGS+=-L$(INSTALL_DIR)/lib/public $(SSP_LIBS)
endif


export LINUX_VER_STR TOOLCHAIN_PREFIX
CONFIG_ID_FILE = BRCM_CONFIG_ID_$(TOOLCHAIN_PREFIX)_$(LINUX_VER_STR)


ifneq ($(strip $(BUILD_LIBJSONC)),)
conditional_build: all
else
conditional_build:
	@echo "skipping libjsonc (not configured)"
endif


check_untar:
ifeq ($(wildcard jsonc/configure),)
	mkdir -p jsonc
	(cd jsonc ; tar --strip-components=1  -xkzf ../json-c-0.11.tar.gz ; true)
endif
	@echo "jsonc is untarred"

check_config: check_untar
	cd jsonc ; ac_cv_func_malloc_0_nonnull=yes ac_cv_func_realloc_0_nonnull=yes ./configure --host=$(TOOLCHAIN_PREFIX) --prefix=$(BCM_FSBUILD_DIR)/public/

all: check_config
	mkdir -p $(INSTALL_DIR)/lib/public/
	cd jsonc ; make
	cd jsonc ; make install
	cp -d $(BCM_FSBUILD_DIR)/public/lib/libjson*.so* $(INSTALL_DIR)/lib/public/
	$(STRIP) $(INSTALL_DIR)/lib/public/libjson*.so*

clean:
	rm -f $(INSTALL_DIR)/lib/public/libjson*.so*
	-[ -e jsonc/Makefile ] && cd jsonc && make clean uninstall 
	-rm -rf jsonc


bcm_dorel_distclean: distclean

distclean: clean
	rm -f jsonc/*

shell:
	bash -i

