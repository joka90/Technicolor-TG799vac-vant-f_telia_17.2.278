

ifeq ("$(RDPA_PLATFORM)", "dsl")
DRV_OBJS := unimac/unimac_drv_impl1.o
else
DRV_OBJS := phys/egphy/egphy_drv_impl1.o \
			phys/extphy/extphy_drv_impl1.o \
			phys/phys_common_drv.o  \
			mdio/mdio_drv_impl1.o  \
			unimac/unimac_drv_impl1.o
endif

obj-y +=  $(DRV_OBJS)	




EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/drv/ -I$(INC_BRCMSHARED_PUB_PATH)/drv/phys/ -I$(INC_BRCMSHARED_PUB_PATH)/drv/phys/egphy 
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/drv/phys/extphy -I$(INC_BRCMSHARED_PUB_PATH)/drv/mdio  
EXTRA_CFLAGS += -I$(INC_RDPA_PATH) -I$(INC_BRCMSHARED_PUB_PATH)/rdp/  -I$(INC_BRCMSHARED_PUB_PATH)/rdp/base 
EXTRA_CFLAGS += -I$(INC_BDMF_PATH)/framework -I$(INC_BRCMSHARED_PUB_PATH)/drv/unimac  
EXTRA_CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)



-include $(TOPDIR)/Rules.make

