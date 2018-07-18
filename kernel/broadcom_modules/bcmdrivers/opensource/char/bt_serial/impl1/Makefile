# File: bcmdrivers/opensource/char/bt_serial/impl1/Makefile
#
# Makefile for the bt_serial driver.
#

obj-$(CONFIG_BCM_BT_SERIAL) += bt_serial_drv.o
bt_serial_drv-y := bt_serial.o bt_serial_dma.o

EXTRA_CFLAGS += -Werror -Wall
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PUB_PATH)/pmc
EXTRA_CFLAGS += -DEXPORT_SYMTAB
EXTRA_CFLAGS += -g

-include $(TOPDIR)/Rules.make

clean:
	rm -r core *.o *.a *.s .*.cmd *.ko
