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

# Download from http://sourceforge.net/projects/expat/files/expat/2.1.0/expat-2.1.0.tar.gz/download
APP = expat-2.1.0
LIB = libexpat.so.1.6.0
INSTALL_LIB_1 = libexpat.so.1
INSTALL_LIB_2 = libexpat.so
INCLUDE_DIR = $(BCM_FSBUILD_DIR)/public/include/expat

.PHONY: check_untar_patch_configure conditional_build check_versions

ifneq ($(strip $(BUILD_XMPP)),)
conditional_build: 
	if [ ! -e $(APP) ]; then \
	   mkdir $(APP); cd $(APP); \
	   tar --strip-components=1 -xzf ../$(APP).tar.gz; \
	   ./configure --host=$(TOOLCHAIN_PREFIX) --prefix=$(CURR_DIR); \
	   cd ..; \
	fi; \
	make -C $(APP) ; \
	make -C $(APP) install; \
	cp lib/$(LIB) $(INSTALL_DIR)/lib/public/.; \
	if [ -e $(INSTALL_DIR)/lib/public/$(INSTALL_LIB_1) ]; then \
		rm -f $(INSTALL_DIR)/lib/public/$(INSTALL_LIB_1); \
	fi; \
	ln -s $(LIB) $(INSTALL_DIR)/lib/public/$(INSTALL_LIB_1); \
	if [ -e $(INSTALL_DIR)/lib/public/$(INSTALL_LIB_2) ]; then \
		rm -f $(INSTALL_DIR)/lib/public/$(INSTALL_LIB_2); \
	fi; \
	ln -s $(LIB) $(INSTALL_DIR)/lib/public/$(INSTALL_LIB_2); \
	if [ ! -e $(INCLUDE_DIR) ]; then \
	   mkdir $(INCLUDE_DIR); \
	fi; \
	cp include/* $(INCLUDE_DIR)/.; \
	echo "Done building $(APP)"
else
conditional_build: sanity_check
	@echo "skipping $(APP) (not configured)"
endif

# NOTE: make clean from within app does not do a proper job, so wiping out
# entire directory to ensure consistency.
clean:
	rm -f $(INSTALL_DIR)/lib/public/$(INSTALL_LIB_2)
	rm -f $(INSTALL_DIR)/lib/public/$(INSTALL_LIB_1)
	rm -f $(INSTALL_DIR)/lib/public/$(LIB)
	rm -rf bin share include lib $(APP) $(INCLUDE_DIR)

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean
	
