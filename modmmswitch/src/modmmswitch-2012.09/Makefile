
ccflags-y := -I$(src)/include -Wall -Wsign-compare

obj-${CONFIG_MODMMSWITCH} += modmmswitch.o

objsrc := src/mmconn.o src/mmconnkernel.o src/mmconnmulticast.o src/mmconn_netlink.o src/mmconnrelay.o src/mmconnrtcp.o src/mmconntone.o src/mmconnuser.o src/mmconnuser_netlink.o src/mmswitch.o src/mmswitch_netlink.o src/mmcommon.o src/mmconnstats.o

modmmswitch-y := $(objsrc)

clean:
	rm -f core *.o *.a *.s *~

