CC=musl-gcc
CFLAGS=-static -Ilib/c

INCLUDES_PLASH_H=bin/plash tests/c/pl_setup_user_ns lib/exec/sudo lib/exec/nodepath lib/exec/add-layer
INCLUDES_PLASH_C=tests/c/pl_parse_subid


all: $(INCLUDES_PLASH_H) $(INCLUDES_PLASH_C) lib/c/plash.o

$(INCLUDES_PLASH_H): %: %.c lib/c/plash.c 

lib/c/plash.o: lib/c/plash.c
	$(CC) --shared -fPIC --static lib/c/plash.c -o lib/c/plash.o

clean:
	rm -f $(INCLUDES_PLASH_H) $(INCLUDES_PLASH_C) lib/c/plash.o

