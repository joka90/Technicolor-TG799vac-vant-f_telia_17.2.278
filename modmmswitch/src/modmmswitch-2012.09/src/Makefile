TARGET = modmmswitch
OBJS = mmswitch.o mmswitch_netlink.o mmconn.o mmconn_netlink.o mmconnuser.o mmconnuser_netlink.o mmconnkernel.o mmconnmulticast.o mmconntone.o mmconnrelay.o mmconnrtcp.o mmconnstats.o mmcommon.o
MDIR = drivers/misc
CURRENT = $(shell uname -r)


GCOV_PROFILE := y
EXTRA_CFLAGS = -g -Werror -Wall -DTRACE_LEVEL=4 -I $(src)/../include/ -DMMPBX_DSP_SUPPORT_RTCPXR
ifndef KDIR
KDIR = /lib/modules/$(CURRENT)/build
endif
PWD = $(shell pwd)
DEST = /lib/modules/$(CURRENT)/kernel/$(MDIR)

obj-m += $(TARGET).o
$(TARGET)-objs := $(OBJS)

default:
	make -C $(KDIR) M=$(PWD) modules

install:
	sudo su -c "cp -v $(TARGET).ko $(DEST) && /sbin/depmod -a"

clean:
	rm -f *.o *.ko .*.cmd .*.flags *.mod.c
	make -C $(KDIR) M=$(PWD) clean

-include $(KDIR)/Rules.make
