CC=musl-gcc
CFLAGS=-static -Ilib/c

all: sudo plash

sudo: lib/exec/sudo.c lib/c/plash.c
	$(CC) $(CFLAGS) -o lib/exec/sudo lib/exec/sudo.c

plash: bin/plash.c lib/c/plash.c
	$(CC) $(CFLAGS) -o bin/plash bin/plash.c
clean:
	rm lib/exec/sudo
