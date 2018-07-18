
all:	build

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

CXX := $(CC)
CFLAGS := -lstlport
CXXFLAGS := -lstlport
LDFLAGS := -lstlport -L$(USERSPACE_PUBLIC_LIBS_DIR)/stlport/lib

export CXX CFLAGS CXXFLAGS LDFLAGS CC USERSPACE_PUBLIC_LIBS_DIR

build:
	(tar xkfz iperf-2.0.5.tar.gz 2> /dev/null || true)
	if [ ! -e ./iperf-2.0.5/Makefile ];then \
	cd ./iperf-2.0.5; ./configure --host=$(TOOLCHAIN_PREFIX) --target=$(TOOLCHAIN_PREFIX) ac_cv_func_malloc_0_nonnull=yes; \
	fi
	echo "configure is done"
	$(MAKE) -C ./iperf-2.0.5 srcdir=./ builddir=./
	install -m 755 ./iperf-2.0.5/src/iperf $(INSTALL_DIR)/bin/iperf

clean:
	if [ -f ./iperf-2.0.5/Makefile ];then $(MAKE) -C ./iperf-2.0.5 distclean srcdir=./ builddir=./;fi;

shell:
	bash

