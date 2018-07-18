
# brcm begin

#CC=gcc
#CFLAGS= -Wall -g -O2
CFLAGS= -Wall -s
ifeq ($(strip $(BUILD_BRCTL)),static)
CFLAGS += -DBUILD_STATIC
endif

CFLAGS += -I$(TOOLCHAIN)/include
CFLAGS += $(BRCM_WERROR_INCLIDE)

ifneq ($(strip $(BUILD_VLAN_AGGR)),)
CFLAGS += -DCONFIG_BCM_VLAN_AGGREGATION -DCONFIG_BCM_KF_VLAN_AGGREGATION
endif

# brcm end
LDFLAGS=
INCLUDE=-I../libbridge -I$(KERNEL_LINKS_DIR) 
LIBS= -L ../libbridge -lbridge 

prefix=/usr/local
exec_prefix=${prefix}
bindir=${exec_prefix}/bin
sbindir=${exec_prefix}/sbin
mandir=${prefix}/share/man

INSTALL=/usr/bin/install -c


common_SOURCES= brctl_cmd.c brctl_disp.c 
brctl_SOURCES=  brctl.c $(common_SOURCES)

common_OBJECTS= $(common_SOURCES:.c=.o)
brctl_OBJECTS= $(brctl_SOURCES:.c=.o)

OBJECTS= $(common_OBJECTS) $(brctl_OBJECTS)

PROGRAMS= brctl 


all:	$(PROGRAMS)
# brcm befing
dynamic: $(PROGRAMS)

static: $(brctl_OBJECTS) ../libbridge/libbridge.a
	$(AR) rcs brctl.a $(brctl_OBJECTS) $(LIBS)
# brcm end

install: $(PROGRAMS)
	mkdir -p $(DESTDIR)$(sbindir)
	$(INSTALL) -m 755 $(PROGRAMS) $(DESTDIR)$(sbindir)

brctl:	$(brctl_OBJECTS) ../libbridge/libbridge.a
	$(CC) $(LDFLAGS) $(brctl_OBJECTS) $(LIBS) -o brctl

%.o: %.c brctl.h
	$(CC) $(CFLAGS) $(INCLUDE) -c $< 

clean:
	rm -f *.o brctl core

