# File: bcmdrivers/opensource/char/bdmf/impl1/Makefile
#
# Makefile for Broadlight Device Management Framework (BDMF), includes build for CPE and Maple. CPE build includes BDMF
# kernel module, BDMF simulator library for x86, and BDMF shell user-space client.
#

# Set the following options according to configuration
src 				?= $(shell pwd)
BDMFDIR				?= $(src)
CONFIG_SYSTEM		?= linux
CONFIG_XGPON 		?= n
CONFIG_BDMF_SHELL	?= y
CONFIG_REMOTE_SHELL	?= n
CONFIG_DB			?= n
CONFIG_DRIVER_TEST	?= n
CONFIG_BDMF_CODEGEN ?= y
CONFIG_BDMF_HISTORY	?= y
 
REMOTE_SHELL_CC ?= gcc
REMOTE_SHELL_CFLAGS ?= -g -Wall -Werror -I$(BDMFDIR)/system -I$(BDMFDIR)/system/$(CONFIG_SYSTEM) 
REMOTE_SHELL_LIBS = -lrt -lpthread 

#
# End of configuration section
#
EXTRA_INCLUDES  = -I$(BDMFDIR) -I$(BDMFDIR)/framework 
EXTRA_INCLUDES += -I$(BDMFDIR)/system -I$(BDMFDIR)/system/$(CONFIG_SYSTEM) 
ifeq ("$(CONFIG_SYSTEM)", "linux")
	EXTRA_INCLUDES += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)
	EXTRA_INCLUDES += -I$(INC_BRCMSHARED_PUB_PATH)/bcm963xx
	EXTRA_INCLUDES += -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)
	EXTRA_INCLUDES += -I$(KERNEL_DIR)/net/core
endif

ifeq ("$(RDPA_PLATFORM)" ,"oren")
	EXTRA_INCLUDES += -I$(INC_BRCMSHARED_PUB_PATH)/rdp -I$(INC_BRCMSHARED_PUB_PATH)/rdp/base
endif
ifeq ("$(RDPA_PLATFORM)" ,"dsl")
	EXTRA_INCLUDES += -I$(INC_BRCMSHARED_PUB_PATH)/rdp -I$(INC_BRCMSHARED_PUB_PATH)/rdp/base
endif
ifneq (x"$(V)", x"1")
	SILENT_BUILD = @
endif

ifeq ("$(CONFIG_SYSTEM)", "sim")
	CC 	   ?= gcc
	AR	   ?= ar 
	CFLAGS = -g -Wall -Werror -std=gnu99 -m32 -march=i386 $(EXTRA_OPTIONS) $(EXTRA_INCLUDES) $(EXTRA_DEFINES)
	LFLAGS = -m32 -march=i386 -g 
	ARFLAGS = -cqs 
	LIBS   = -lrt -lpthread
	EXTRA_DEFINES += -DBDMF_SYSTEM_SIM
	REMOTE_SHELL_CFLAGS += -DBDMF_SYSTEM_SIM
	CONFIG_REMOTE_SHELL = y
	BDMF_OUTDIR = $(PROFILE_DIR)/sim/bdmf
	CC_CMD = $(CC) -c $(CFLAGS) -o
endif

OBJS = 	$(BDMF_OUTDIR)/framework/bdmf_init.o
OBJS += $(SYSTEM_OBJS) $(FRAMEWORK_OBJS) $(DRIVER_OBJS)
OBJS += $(BDMF_OUTDIR)/framework/bdmf_fini.o

ifeq ("$(CONFIG_SYSTEM)", "linux")
	CONFIG_BDMF_CODEGEN := n
	EXTRA_CFLAGS = -Werror $(EXTRA_INCLUDES) $(EXTRA_DEFINES)
	SYSTEM_OBJS += system/linux/bdmf_chrdev.o
	obj-$(CONFIG_BCM_BDMF) = bdmf.o
	bdmf-objs = $(OBJS)
	EXTRA_DEFINES += -DBDMF_SYSTEM_LINUX
	REMOTE_SHELL_CFLAGS += -DBDMF_SYSTEM_LINUX
	REMOTE_SHELL_CC = $(CROSS)gcc
	REMOTE_SHELL_DEPS += $(BDMFDIR)/system/$(CONFIG_SYSTEM)/bdmf_chrdev.h
	BDMF_OUTDIR = .
endif

FRAMEWORK_OBJS = \
	$(BDMF_OUTDIR)/framework/bdmf_attr.o		\
	$(BDMF_OUTDIR)/framework/bdmf_memarea.o	\
	$(BDMF_OUTDIR)/framework/bdmf_utils.o	\
	$(BDMF_OUTDIR)/framework/bdmf_type.o		\
	$(BDMF_OUTDIR)/framework/bdmf_object.o	\
	$(BDMF_OUTDIR)/framework/bdmf_trace.o

SYSTEM_OBJS += \
	$(BDMF_OUTDIR)/system/bdmf_session.o		\
	$(BDMF_OUTDIR)/system/bdmf_errno.o			\

SRCS = $(OBJS:.o=.c)
DOC_HDRS = 	$(wildcard framework/*.h)
DOC_HDRS += $(wildcard shell/*.h)
DOC_HDRS += $(wildcard db_engine/*.h)
DOC_HDRS += $(wildcard transport/*.h)
REFMAN_DIR = doc/latex
REFMAN_FILE = bdmf_refman.pdf
REFMAN = $(REFMAN_DIR)/$(REFMAN_FILE)

ifeq ("$(CONFIG_XGPON)", "y")
	EXTRA_DEFINES += -DCONFIG_XGPON
endif

ifeq ("$(RDPA_PLATFORM)", "dsl")
ifeq ($(strip $(BRCM_CHIP)),63138)
	EXTRA_DEFINES += -DDSL_63138
endif
ifeq ($(strip $(BRCM_CHIP)),63148)
	EXTRA_DEFINES += -DDSL_63148
endif

endif

ifeq ("$(CONFIG_DB)", "y")
	EXTRA_DEFINES += -DBDMF_DB_ENGINE
	EXTRA_INCLUDES += -I$(BDMFDIR)/db_engine
	FRAMEWORK_OBJS += $(BDMF_OUTDIR)/db_engine/db_engine.o
endif

ifeq ("$(CONFIG_BDMF_SHELL)", "y")
	EXTRA_DEFINES += -DBDMF_SHELL
	EXTRA_INCLUDES += -I$(BDMFDIR)/shell
	FRAMEWORK_OBJS += $(BDMF_OUTDIR)/framework/bdmf_mon.o
	SYSTEM_OBJS += $(BDMF_OUTDIR)/shell/bdmf_shell.o 
	ifeq ("$(CONFIG_REMOTE_SHELL)", "y")
		EXTRA_DEFINES += -DBDMF_SHELL_SERVER
		SYSTEM_OBJS += $(BDMF_OUTDIR)/shell/bdmf_shell_server.o 
	endif
endif

ifeq ("$(CONFIG_DRIVER_TEST)", "y")
	DRIVER_OBJS += $(BDMF_OUTDIR)/drivers/test1_drv.o $(BDMF_OUTDIR)/drivers/test2_drv.o
endif

ifeq ("$(CONFIG_SYSTEM)", "sim")
	SYSTEM_OBJS += $(BDMF_OUTDIR)/system/$(CONFIG_SYSTEM)/bdmf_buf.o $(BDMF_OUTDIR)/system/$(CONFIG_SYSTEM)/bdmf_system.o
	LFLAGS += -lpthread
else
	SYSTEM_OBJS +=$(BDMF_OUTDIR)/system/$(CONFIG_SYSTEM)/bdmf_system_common.o
ifeq ("$(CONFIG_CM3390)", "y")	 
	SYSTEM_OBJS +=$(BDMF_OUTDIR)/system/$(CONFIG_SYSTEM)/cm/bdmf_system.o
	EXTRA_INCLUDES +=-I$(BDMFDIR)/system/$(CONFIG_SYSTEM)/cm
else	
	SYSTEM_OBJS +=$(BDMF_OUTDIR)/system/$(CONFIG_SYSTEM)/ce/bdmf_system.o
	EXTRA_INCLUDES +=-I$(BDMFDIR)/system/$(CONFIG_SYSTEM)/ce
endif
endif

ifeq ("$(CONFIG_BDMF_DEBUG)", "y")
	EXTRA_DEFINES += -DBDMF_DEBUG
endif

ifeq ("$(CONFIG_BDMF_CODEGEN)", "y")
	EXTRA_DEFINES += -DBDMF_CODEGEN
	FRAMEWORK_OBJS += $(BDMF_OUTDIR)/framework/bdmf_codegen.o
endif

ifeq ("$(CONFIG_BDMF_HISTORY)", "y")
	EXTRA_DEFINES += -DBDMF_HISTORY
	FRAMEWORK_OBJS += $(BDMF_OUTDIR)/framework/bdmf_history.o
endif

# 
# Simulation build
#
ifeq ("$(CONFIG_SYSTEM)", "sim")

bdmf_sim: $(BDMF_OUTDIR)/libbdmf.a $(BDMF_OUTDIR)/bdmf

$(BDMF_OUTDIR)/.dir_created: 
	mkdir -p $(BDMF_OUTDIR)/
	mkdir -p $(BDMF_OUTDIR)/framework
	mkdir -p $(BDMF_OUTDIR)/transport
	mkdir -p $(BDMF_OUTDIR)/system/sim
	mkdir -p $(BDMF_OUTDIR)/shell
	$(SILENT_BUILD)touch $(BDMF_OUTDIR)/.dir_created

-include $(OBJS:.o=.d)

$(BDMF_OUTDIR)/%.d: %.c Makefile $(BDMF_OUTDIR)/.dir_created
	@echo "Making dependency for $< --> $@"
	$(SILENT_BUILD)$(CC) -MM -MT $(BDMF_OUTDIR)/$(<:.c=.o) $(CFLAGS) $< | sed 's,\($*\.o\)[ :]*,\1 $@ : ,g' > $@

$(BDMF_OUTDIR)/%.o: %.c $(BDMF_OUTDIR)/.dir_created
	@echo "CC $< --> $@"
	$(SILENT_BUILD)$(CC_CMD) $@ $<

endif

$(BDMF_OUTDIR)/bdmf: $(OBJS) main.o
	@echo LD $(BDMF_OUTDIR)bdmf 
	$(SILENT_BUILD)$(CC) -o $@ $(LFLAGS) $(OBJS) main.o $(LIBS)

libbdmf.a: $(BDMF_OUTDIR)/libbdmf.a

bdmf_test: $(BDMF_OUTDIR)/bdmf

$(BDMF_OUTDIR)/libbdmf.a: $(OBJS)
	$(SILENT_BUILD)$(AR) $(ARFLAGS) $@ $(OBJS)

docs: $(REFMAN) 

$(REFMAN): $(DOC_HDRS) Doxyfile DoxygenLayout.xml
	$(SILENT_BUILD)doxygen
	$(SILENT_BUILD)make -C $(REFMAN_DIR)
	$(SILENT_BUILD)mv -f $(REFMAN_DIR)/refman.pdf $(REFMAN)

clean:
	$(SILENT_BUILD)rm -fr `find $(BDMF_OUTDIR)/. -name '*.[o,d]'`
	$(SILENT_BUILD)rm -fr `find . -name '*~'`
	$(SILENT_BUILD)rm -fr `find . -name '*.ko'`
	$(SILENT_BUILD)rm -fr `find . -name '.*.cmd'`
	$(SILENT_BUILD)rm -fr bdmf.mod.c modules.order Module.symvers .tmp_versions
	$(SILENT_BUILD)rm -fr $(BDMF_OUTDIR)/bdmf $(BDMF_OUTDIR)/bdmf_shell $(BDMF_OUTDIR)/libbdmf.a

clobber: clean
	$(SILENT_BUILD)rm -fr doc
