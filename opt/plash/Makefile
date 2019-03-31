CC=musl-gcc
CFLAGS=-static -Ilib/c

INCLUDES_PLASH_H=bin/plash tests/c/pl_setup_user_ns lib/exec/sudo lib/exec/nodepath lib/exec/chroot lib/exec/runopts
INCLUDES_PLASH_C=tests/c/pl_parse_subid

all: $(INCLUDES_PLASH_H) $(INCLUDES_PLASH_C)

$(INCLUDES_PLASH_H): %: %.c lib/c/plash.c 

clean:
	rm -f $(INCLUDES_PLASH_H) $(INCLUDES_PLASH_C)
