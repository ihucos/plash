CC=musl-gcc
CFLAGS=-static -Ilib/c

INCLUDES_PLASH_H=bin/plash tests/c/pl_setup_user_ns lib/exec/sudo lib/exec/nodepath
INCLUDES_PLASH_C=tests/c/pl_parse_subid

../plash/lib/plash.o: ../plash/lib/c/plash.c
	$(CC) --shared -fPIC --static ../plash/lib/c/plash.c -o ../plash/lib/c/plash.o

all: $(INCLUDES_PLASH_H) $(INCLUDES_PLASH_C)

$(INCLUDES_PLASH_H): %: %.c lib/c/plash.c 

clean:
	rm -f $(INCLUDES_PLASH_H) $(INCLUDES_PLASH_C)
