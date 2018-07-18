#***********************************************************************
# 
#    Copyright (c) 2014 Broadcom Corporation
#    All Rights Reserved
#
#***********************************************************************/

#
# In most cases, you only need to modify this first section.
#
LIB = libwebsockets.so

OBJS = base64-decode.o handshake.o libwebsockets.o service.o \
       pollfd.o output.o parsers.o context.o sha-1.o \
       client.o client-handshake.o client-parser.o \
       ssl.o lws-plat-unix.o server.o server-handshake.o \
       daemonize.o getifaddrs.o


all install: $(LIB)

clean: generic_clean
	rm -f $(INSTALL_DIR)/lib/private/$(LIB)



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

#
# Private apps and libs are allowed to include header files from the
# private and public directories
#
# WARNING: Do not modify this section unless you understand the
# license implications of what you are doing.
#

ALLOWED_LIB_DIRS := /lib:/lib/public

LIB_PATHS = $(patsubst %,-L$(INSTALL_DIR)%,$(subst :, ,$(ALLOWED_LIB_DIRS)))

CFLAGS += -DCMAKE_BUILD -I. -fvisibility=hidden

ifeq ($(strip $(BRCM_USER_SSP)),y)
LIBS   += -L$(INSTALL_DIR)/lib/public $(SSP_LIBS) 
CFLAGS += $(SSP_TYP_COMPILER_OPTS)
endif

# Set flags if user wants to link with OpenSSL
ifneq ($(strip $(DO_BUILD_OPENSSL)),)
LIBS   += -lssl -lcrypto
CFLAGS += -I$(BCM_FSBUILD_DIR)/public/include
endif


#
# Implicit rule will make the .c into a .o
# Implicit rule is $(CC) -c $(CPPFLAGS) $(CFLAGS)
# See Section 10.2 of Gnu Make manual
# 
$(LIB): $(OBJS)
	$(CC) -shared $(BCM_LD_FLAGS) -Wl,--whole-archive,-soname,$@ -o $@ $(OBJS) -Wl,--no-whole-archive $(LIB_PATHS) $(LIBS)



#
# Include the rule for making dependency files.
# The '-' in front of the second include suppresses
# error messages when make cannot find the .d files.
# It will just regenerate them.
# See Section 4.14 of Gnu Make.
#

include $(BUILD_DIR)/make.deprules

-include $(OBJS:.o=.d)
