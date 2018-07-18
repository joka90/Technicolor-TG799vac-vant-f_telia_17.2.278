CC = gcc
CFLAGS = -Wall -g -O0 -DKEY_PERSISTENCE

progs := ncr-setkey lskeys rmkey

all: $(progs)

ncr-setkey: setkey.c
	$(CC) $(CFLAGS) $< -o $@
lskeys: lskeys.c
	$(CC) $(CFLAGS) $< -o $@
rmkey: rmkey.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o *~ $(progs)
