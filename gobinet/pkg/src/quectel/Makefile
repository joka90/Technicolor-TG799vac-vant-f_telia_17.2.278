obj-m := GobiNet.o
GobiNet-objs := GobiUSBNet.o QMIDevice.o QMI.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
OUTPUTDIR=/lib/modules/`uname -r`/kernel/drivers/net/usb/

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

marvell:
	$(MAKE) ARCH=arm CROSS_COMPILE=${MARVELL_CCPREFIX} -C $(MARVELL_KDIR) M=$(PWD) modules

pi: 
	$(MAKE) ARCH=arm CROSS_COMPILE=${PI_CCPREFIX} -C $(PI_KDIR) M=$(PWD) modules

ow: 
	$(MAKE) ARCH=mips CROSS_COMPILE=${OW_CCPREFIX} -C $(OW_KDIR) M=$(PWD) modules

install: all
	mkdir -p $(OUTPUTDIR)
	cp -f GobiNet.ko $(OUTPUTDIR)
	depmod

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions Module.* modules.order
