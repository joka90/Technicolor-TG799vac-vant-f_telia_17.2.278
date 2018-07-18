# File: bcmdrivers/char/dect/impl1/Makefile
#
# Makefile for the dect driver.
#

.PHONY: build_source clean_source

obj-$(CONFIG_BCM_DECTSHIM) += dectshim.o
dectshim-objs := dectshimdrv.o dect_shim.o 

EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PUB_PATH)/pmc
					
$(obj)/dectshim.a: build_source
		
# If Src/ directory doesnt exist and .zip exists, unzip source --> will get Src/ directory
# If Src/ directory exist, build from source
# If no Src/ and .zip, just use existing dect_driver.bin
build_source:  
	echo Building Dectshim Driver Source...;
	
clean_source:
	echo Cleaning Dectshim Driver Source...;

-include $(TOPDIR)/Rules.make

clean: clean_source
	rm -f core dectshim.o dectshim.ko dectshim.mod.* built-in.o *.a *.s modules.order;
