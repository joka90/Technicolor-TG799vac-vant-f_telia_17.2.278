#
# Makefile for flash utilities
# [No explicit reason for not supporting static compiles]
#
CFLAGS = -s -Os -fomit-frame-pointer -Wall \
         -I$(BUILD_DIR)/kernel -I.

# flash_eraseall
SRCS = flash_info.c flash_erase.c flashcp.c flash_eraseall.c crc32.c
OBJS = flash_info.o flash_erase.o flashcp.o flash_eraseall.o crc32.o
UTILS= flash_info flash_erase flash_cp flash_eraseall


all: $(UTILS) $(SRCS)

install::
	install -m 755 flash_info $(INSTALL_DIR)/bin
	$(STRIP) $(INSTALL_DIR)/bin/flash_info
	install -m 755 flash_erase $(INSTALL_DIR)/bin
	$(STRIP) $(INSTALL_DIR)/bin/flash_erase
	install -m 755 flash_cp $(INSTALL_DIR)/bin
	$(STRIP) $(INSTALL_DIR)/bin/flash_cp
	install -m 755 flash_eraseall $(INSTALL_DIR)/bin
	$(STRIP) $(INSTALL_DIR)/bin/flash_eraseall

dynamic: all install

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

flash_info: flash_info.o
	$(CC) $(CFLAGS) -o $@ $^

flash_erase: flash_erase.o
	$(CC) $(CFLAGS) -o $@ $^

flash_cp: flashcp.o
	$(CC) $(CFLAGS) -o $@ $^

flash_eraseall: crc32.o flash_eraseall.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	-rm -f  $(UTILS) $(OBJS)
	-rm -f  $(INSTALL_DIR)/bin/flash_info		\
		$(INSTALL_DIR)/bin/flash_erase		\
		$(INSTALL_DIR)/bin/flash_eraseall	\
		$(INSTALL_DIR)/bin/flash_cp

