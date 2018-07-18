

obj-m += bankmgr.o 

bankmgr-y := storage_kernel.o storage_core.o storage_crc32.o bankmgr_core.o bankmgr_os.o bankmgr_proc.o bankmgr_checksum.o legacy_upgrade_info.o main.o

clean:
	rm -f core *.o *.a *.s *~

TEST_BINARY := test/rs_host_test

.PHONY: ${TEST_BINARY}

run_test: ${TEST_BINARY}
	$^

${TEST_BINARY}:
	make -C test rs_host_test

