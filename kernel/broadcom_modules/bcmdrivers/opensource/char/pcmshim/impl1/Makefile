# File: bcmdrivers/char/pcmshim/impl1/Makefile
#
# Makefile for the pcm_dma driver.
#
obj-$(CONFIG_BCM_PCMSHIM) += pcmshim.o

EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -Werror -Wfatal-errors

-include $(TOPDIR)/Rules.make

clean:
	rm -f core *.o *.a *.s

