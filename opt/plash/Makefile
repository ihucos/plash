CC=musl-gcc
CFLAGS=-static -Ilib/c

all: bin/plash

bin/plash: bin/plash.c lib/c/plash.c
	$(CC) $(CFLAGS) -o bin/plash bin/plash.c lib/c/plash.c
clean:
	rm -f bin/plash
