#***********************************************************************
#
#  Copyright (c) 2006  Broadcom Corporation
#  All Rights Reserved
#
#***********************************************************************/


#
# You do not need to modify this Makefile to add an app.
# Simply create a .autodetect in the subdir and this Makefile will enter
# that directory during make and make clean.  (Auto detected directories
# are listed in the AUTODETECT_SUBDIRS variable.  Type make show_subdirs)
#
HARDCODED_SUBDIRS := atm2684 bridge-utils busybox sysstat dproxy-nexgen \
           ftpd iproute2 udhcp urlfilterd zebra \
           dynahelper samba iperf conntrack \
           openl2tpd rng-tools-2 pptp \
           mtd-util \
           felix_framework


SUBDIRS = $(HARDCODED_SUBDIRS) \
          $(AUTODETECT_SUBDIRS_BASE)


# === this is the default target ===
# Need to add a level of indirection here so that AUTODETECT_SUBDIRS and
# MODSW_SUBDIRS can be populated in make.common/make.modsw before we build.
#
all : make_all_subdirs


#
# Set our CommEngine directory (by splitting the pwd into two words
# at /userspace and taking the first word only).
# Then include the common defines under CommEngine.
# You do not need to modify this part.
#
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
export BUILD_DIR

include $(BUILD_DIR)/make.common

# include make.modsw for autodetect variables
include $(BUILD_DIR)/make.modsw


#
# We need to declare these directory names as phony targets,
# otherwise make will think the directories themselves are
# the targets and will never execute the cmds.
# See GNU Make, 4.6 
#
.PHONY : $(SUBDIRS)


base : $(SUBDIRS)

make_all_subdirs : base


clean: generic_subdir_clean



##############################################################
# 
# Rules for inidividual apps
# apps can be compiled or not depending on the profile (which
# is modified via make menuconfig)
#
##############################################################


$(AUTODETECT_SUBDIRS_BASE): 
	$(MAKE) -C $@


ifneq ($(strip $(BUILD_BRCTL)),)
bridge-utils: sanity_check
	mkdir -p $(INSTALL_DIR)/bin
	(tar xkfj bridge-utils.tar.bz2 2> /dev/null || true)
	$(MAKE) -C bridge-utils $(BUILD_BRCTL)
else
bridge-utils: sanity_check
endif


ifeq ($(strip $(BRCM_IKOS)),y)
BRCMCONFIG=ikos.config
else
BRCMCONFIG=brcm.config
endif
export BRCMCONFIG

PERL5LIB=$(HOSTTOOLS_PERL_LIB)
export PERL5LIB

ifneq ($(strip $(BUILD_BUSYBOX)),)
busybox: sanity_check
	mkdir -p $(INSTALL_DIR)/bin
	(tar xkfj busybox.tar.bz2 2> /dev/null || true)
	cd busybox; chmod -f +w .config .config.old ; cp -f .config .config.old; cat $(BRCMCONFIG) > .config ; \
	for i in config.d/[0-9]*.conf ; do echo "config script $$i" ; perl $$i ; done
	$(MAKE) -C busybox include/autoconf.h
	$(MAKE) -C busybox install
else
busybox: sanity_check
	@echo "skipping busybox (not configured)"
endif

ifneq ($(strip $(BUILD_SYSSTAT)),)
sysstat: sanity_check
	mkdir -p $(INSTALL_DIR)/bin
	(tar xkfj sysstat.tar.bz2 2> /dev/null || true)
	cd sysstat
	$(MAKE) -C sysstat install
else
sysstat: sanity_check
	@echo "skipping sysstat (not configured)"
endif


ifneq ($(strip $(BUILD_DPROXY)),)
dproxy-nexgen: sanity_check
	mkdir -p $(INSTALL_DIR)/bin
	(tar xkfj dproxy-nexgen.tar.bz2 2> /dev/null || true)
	$(MAKE) -C dproxy-nexgen $(BUILD_DPROXY)
else
dproxy-nexgen: sanity_check
	@echo "skipping dproxy (not configured)"
endif


# in userapps->Dynamic ALG loader
ifneq ($(strip $(BUILD_DYNAHELPER)),)
dynahelper: sanity_check
	$(MAKE) -C dynahelper $(BUILD_DYNAHELPER)
else
dynahelper: sanity_check
	@echo "skipping dynahelper (not configured)"
endif


# ftpd does sanity check in its own Makefile
ifneq ($(strip $(BUILD_FTPD)),)
ftpd:
	(tar xkfj ftpd.tar.bz2 2> /dev/null || true)
	$(MAKE) -C ftpd $(BUILD_FTPD)
else
ftpd:
	@echo "skipping ftpd (not configured)"
endif


# Build the iproute2 package whenever IPROUTE2, IPV6, or BRCM_KERNEL_NETQOS
# is defined.  In make menuconfig, "traffic control" enables BRCM_KERNEL_NETQOS
ifneq ("$(BUILD_IPROUTE2)$(BUILD_IPV6)$(BRCM_KERNEL_NETQOS)$(BUILD_GPONRG_OMCI_FULL)","")
DO_BUILD_IPROUTE2=y
endif

ifneq ($(strip $(DO_BUILD_IPROUTE2)),)
iproute2: sanity_check
	(tar xkfj iproute2.tar.bz2 2> /dev/null || true)
	$(MAKE) -C iproute2 install 
else
iproute2: sanity_check
	@echo "skipping iproute2 (not configured)"
endif

ifneq ($(strip $(BRCM_KERNEL_AUXFS_JFFS2)),)
# Always make flash utilities and installed executables
mtd-util:
	(cd mtd-util; $(MAKE); $(MAKE) install)
else
mtd-util:
	@echo "BRCM_KERNEL_AUXFS_JFFS2 not configured, skipping"
endif

ifneq ($(strip $(BUILD_IPTABLES)),)
ifneq ($(strip $(BUILD_URLFILTER)),)
urlfilterd: sanity_check
	$(MAKE) -C urlfilterd $(BUILD_URLFILTER)
else
urlfilterd: sanity_check
endif
else
urlfilterd: sanity_check
	@echo Warning: To use urlfilterd, you need to enable iptables!!!!!
endif


ifneq ($(strip $(BUILD_UDHCP)),)
udhcp: sanity_check
	$(MAKE) -C udhcp $(BUILD_UDHCP)
else
udhcp: sanity_check
	@echo "skipping udhcp (not configured)"
endif


ifneq ($(strip $(BUILD_ZEBRA)),)
zebra: sanity_check
	(tar xkfj zebra.tar.bz2 2> /dev/null || true)
	$(MAKE) -C zebra $(BUILD_ZEBRA)
else
zebra: sanity_check
	@echo "skipping zebra (not configured)"
endif


ifneq ($(strip $(BUILD_SAMBA)),)
samba: sanity_check
	(tar xkfz samba.tar.gz 2> /dev/null || true)
	$(MAKE) -C ./samba -j 1
else
samba: sanity_check 
	@echo "skipping $@ (not configured)"
endif

ifneq ($(strip $(BUILD_IPERF)),)
iperf: sanity_check
	$(MAKE) -C ./iperf -j 1
else
iperf: sanity_check
	@echo "skipping $@ (not configured)"
endif

ifneq ($(strip $(BUILD_L2TPAC)),)
openl2tpd: sanity_check
	(tar xkjf openl2tpd.tar.bz2 2> /dev/null || true)
	$(MAKE) -C openl2tpd
else
openl2tpd: sanity_check
	@echo "skipping openl2tpd (not configured)"
endif

ifneq ($(strip $(BUILD_RNGD)),)
rng-tools-2: sanity_check
	mkdir -p $(INSTALL_DIR)/bin
	(tar xkfz rng-tools-2.tar.gz 2> /dev/null || true)
	($(MAKE) -C rng-tools-2 $(BUILD_RNGD))
else
rngd: sanity_check
	@echo "skipping rng-tools-2 (not configured)"
endif

ifneq ($(strip $(BUILD_PPTPAC)),)
pptp: sanity_check
	(tar xkjf pptp.tar.bz2 2> /dev/null || true)
	$(MAKE) -C pptp
else
pptp: sanity_check
	@echo "skipping pptp (not configured)"
endif

ifneq ($(strip $(BUILD_CONNTRACK_TOOLS)),)
conntrack: sanity_check
	(tar xkfz conntrack.tar.gz 2> /dev/null || true)
	$(MAKE) -C ./conntrack
else
conntrack: sanity_check
	@echo "skipping $@ (not configured)"
endif

ifneq ($(strip $(BUILD_OSGI_FELIX)),)
felix_framework: sanity_check
	cd felix_framework; (tar xkfz ../felix.tar.gz 2> /dev/null || true)
	$(MAKE) -C ./felix_framework
#this bundle is used for debugging only.  Felix will run in the background, so this needs to be removed.
#	rm -f $(INSTALL_DIR)/usr/local/felix/bundle/org.apache.felix.gogo.shell-0.7.0-SNAPSHOT.jar
else
felix_framework: sanity_check
	@echo "skipping $@ (not configured)"
endif

