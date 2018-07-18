RSDIR=..

rs_host_test: rs_host_test.c $(RSDIR)/storage_core.c $(RSDIR)/storage_crc32.c 
	$(CC) $^ -o $@ -DRAWSTORAGE_VERSION=2 -Wall -I$(RSDIR)
