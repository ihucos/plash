CC=musl-gcc
CFLAGS=-static -Ilib/c

all: bin/plash

bin/plash: bin/plash.c lib/c/plash.c tests/c/plash
	$(CC) $(CFLAGS) -o bin/plash bin/plash.c lib/c/plash.c

tests/c/plash: tests/c/plash.c lib/c/plash.c
	$(CC) $(CFLAGS) -o tests/c/plash tests/c/plash.c

clean:
	rm -f bin/plash
