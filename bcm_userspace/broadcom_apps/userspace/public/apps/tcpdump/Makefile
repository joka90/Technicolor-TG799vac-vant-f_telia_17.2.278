
tcpdump: conditional_build 

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

export LINUX_VER_STR TOOLCHAIN_PREFIX
CONFIG_ID_FILE = BRCM_CONFIG_ID_$(TOOLCHAIN_PREFIX)_$(LINUX_VER_STR)
export CFLAGS LDFLAGS
CFLAGS=-I$(BCM_FSBUILD_DIR)/public/include -I$(TOOLCHAIN_INCLUDE_DIR) -isystem $(TOOLCHAIN_INCLUDE_DIR) 
# -DNDEBUG=1 -O0
LDFLAGS=-L$(BCM_FSBUILD_DIR)/public/lib 



ifneq ($(strip $(BUILD_TCPDUMP)),)
conditional_build: all
else
conditional_build:
	@echo "skipping tcpdump (not configured)"
endif


check_untar:
ifeq ($(wildcard tcpdump/configure),)
	mkdir -p tcpdump
	cd tcpdump ; tar --strip-components=1 -xzf ../tcpdump.tar.gz
	patch -p0 < patch.tcpdump_cross
endif
	@echo "tcpdump is untarred"

check_config: check_untar
ifeq ($(wildcard tcpdump/$(CONFIG_ID_FILE)),)
	-make -C tcpdump distclean
endif
	rm -f tcpdump/BRCM_CONFIG_ID_*
	cd tcpdump ; ac_cv_linux_vers=$(LINUX_VER_STR)  ./configure --host=$(TOOLCHAIN_PREFIX) --with-pcap=linux --prefix=$(CURR_DIR) 
	touch tcpdump/$(CONFIG_ID_FILE)

all: check_config
	make -C tcpdump
	make -C tcpdump install
	cp -d sbin/tcpdump $(INSTALL_DIR)/bin

clean:
	rm -f $(INSTALL_DIR)/bin/tcpdump
	-make -C tcpdump clean uninstall
	rm -rf sbin share

bcm_dorel_distclean: distclean

distclean: clean
	rm -f tcpdump/BRCM_CONFIG_ID_*
	-make -C tcpdump distclean

shell:
	bash -i

