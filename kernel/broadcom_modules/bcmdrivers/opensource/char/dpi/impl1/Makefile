dpicore-objs := dpi.o

obj-$(CONFIG_BCM_DPI) += dpicore.o

EXTRA_CFLAGS += -I$(BRCMDRIVERS_DIR)/broadcom/char/dpiengine/impl$(CONFIG_BCM_DPI_IMPL)/include


clean:
	rm -f core *.o *.a *.s .*.cmd *.ko
