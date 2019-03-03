CC=musl-gcc
CFLAGS=-static -Ilib/c

all: sudo

sudo: lib/exec/sudo.c lib/c/plash.c
	$(CC) $(CFLAGS) -o lib/exec/sudo lib/exec/sudo.c

clean:
	rm lib/exec/sudo
