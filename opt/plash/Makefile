CC=musl-gcc
CFLAGS=-static -Ilib/c

all: bin/plash tests/c/pl_parse_subid

bin/plash: bin/plash.c lib/c/plash.c
	$(CC) $(CFLAGS) -o bin/plash bin/plash.c lib/c/plash.c

tests/c/pl_parse_subid: tests/c/pl_parse_subid.c lib/c/plash.c
	$(CC) $(CFLAGS) -o tests/c/pl_parse_subid tests/c/pl_parse_subid.c

clean:
	rm -f bin/plash tests/c/pl_parse_subid
