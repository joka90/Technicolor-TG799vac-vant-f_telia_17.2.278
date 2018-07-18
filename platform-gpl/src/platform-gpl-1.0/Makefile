obj-${CONFIG_PLATFORM_GPL} += platform-gpl.o

platform-gpl-y := main.o bootloader.o

ifneq ($(CONFIG_KPANIC_IN_PROZONE),)
ccflags-y += -DCONFIG_KPANIC_IN_PROZONE
platform-gpl-y += kmsg_pmem.o
endif

clean:
	rm -f core *.o *.a *.s *~

