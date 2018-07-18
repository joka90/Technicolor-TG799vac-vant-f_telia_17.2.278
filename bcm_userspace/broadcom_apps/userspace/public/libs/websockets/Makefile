#***********************************************************************
#
#  Copyright (c) 2014  Broadcom Corporation
#  All Rights Reserved
#
#***********************************************************************/

all dynamic install: conditional_build


#
# Set our CommEngine directory (by splitting the pwd into two words
# at /userspace and taking the first word only).
# Then include the common defines under CommEngine.
# You do not need to modify this part.
#
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

include $(BUILD_DIR)/make.common

# Download from http://git.libwebsockets.org/cgi-bin/cgit/libwebsockets/
APP = libwebsockets
VERSION = 1.3.0
LIB = $(APP).so
INSTALL_LIB = $(APP).so
INCLUDE_DIR = $(BCM_FSBUILD_DIR)/public/include/websockets

.PHONY: check_untar_patch_configure conditional_build check_versions clean distclean

ifneq ($(strip $(BUILD_WEB_SOCKETS)),)
ifneq ($(strip $(DO_BUILD_OPENSSL)),)
conditional_build: 
	if [ ! -e $(APP) ]; then \
	   mkdir $(APP); cd $(APP); \
	   tar --strip-components=1 -xjf ../$(APP)-$(VERSION).tar.bz2; \
	   cp -d ../Makefile.bcm lib/Makefile; \
	   cp -d ../lws_config.h lib/lws_config.h; \
	   cp -d ../sha-1.c lib/sha-1.c; \
	   cd ..; \
	   make -C $(APP)/lib/ ; \
	   cp -T $(APP)/lib/$(LIB) $(INSTALL_DIR)/lib/public/$(INSTALL_LIB); \
	   if [ ! -e $(INCLUDE_DIR) ]; then \
	      mkdir $(INCLUDE_DIR); \
	   fi; \
	   cp $(APP)/lib/libwebsockets.h $(INCLUDE_DIR)/.; \
	fi; \
	echo "Done building $(APP)"
else
conditional_build: sanity_check
	@echo "Error $(APP) requires OpenSSL" 1>&2
	exit 1
endif
else
conditional_build: sanity_check
	@echo "skipping $(APP) (not configured)"
endif

# NOTE: make clean from within app does not do a proper job, so wiping out
# entire directory to ensure consistency.
clean:
	rm -f $(INSTALL_DIR)/lib/public/$(INSTALL_LIB)
	rm -rf $(APP) $(INCLUDE_DIR)

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean
	
bcm_dorel_distclean: distclean

