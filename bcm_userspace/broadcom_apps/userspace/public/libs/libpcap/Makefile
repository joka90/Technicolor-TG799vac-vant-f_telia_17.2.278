
libpcap: conditional_build 

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

export LINUX_VER_STR TOOLCHAIN_PREFIX
CONFIG_ID_FILE = BRCM_CONFIG_ID_$(TOOLCHAIN_PREFIX)_$(LINUX_VER_STR)


ifneq ($(strip $(BUILD_LIBPCAP)),)
conditional_build: all
else
conditional_build:
	@echo "skipping libpcap (not configured)"
endif


check_untar:
ifeq ($(wildcard libpcap/configure),)
	mkdir -p libpcap
	cd libpcap ; tar --strip-components=1 -xzf ../libpcap.tar.gz
endif
	@echo "libpcap is untarred"

check_config: check_untar
ifeq ($(wildcard libpcap/$(CONFIG_ID_FILE)),)
	-[ -e libpcap/Makefile ] && cd libpcap  && make distclean
	rm -f libpcap/BRCM_CONFIG_ID_*
endif
	cd libpcap ; ac_cv_linux_vers=$(LINUX_VER_STR)  ./configure --host=$(TOOLCHAIN_PREFIX) --with-pcap=linux --prefix=$(BCM_FSBUILD_DIR)/public/
	cd libpcap ; touch $(CONFIG_ID_FILE)

all: check_config
	mkdir -p $(INSTALL_DIR)/lib/public/
	cd libpcap ; make
	cd libpcap ; make install
	cp -d $(BCM_FSBUILD_DIR)/public/lib/libpcap.so* $(INSTALL_DIR)/lib/public/

clean:
	rm -f $(INSTALL_DIR)/lib/public/libpcap.so*
	-[ -e libpcap/Makefile ] && cd libpcap && make clean uninstall
	rm -f libpcap/net

bcm_dorel_distclean: distclean

distclean: clean
	rm -f libpcap/BRCM_CONFIG_ID_*
	-[ -e libpcap/Makefile ] && cd libpcap && make distclean

shell:
	bash -i

