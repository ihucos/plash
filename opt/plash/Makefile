CC=musl-gcc
CFLAGS=-static -Ilib/c

INCLUDES_PLASH_H=bin/plash tests/C/pl_setup_user_ns lib/exec/sudo lib/exec/nodepath lib/exec/add-layer lib/exec/map lib/exec/parent lib/exec/runopts  lib/exec/data lib/exec/mkdtemp lib/exec/run
INCLUDES_PLASH_C=tests/C/pl_parse_subid

all: $(INCLUDES_PLASH_H) $(INCLUDES_PLASH_C) lib/c/plash.o

$(INCLUDES_PLASH_H): %: %.c lib/c/plash.c 

lib/c/plash.o: lib/c/plash.c
	$(CC) --shared -fPIC --static lib/c/plash.c -o lib/c/plash.o

clean:
	rm -f $(INCLUDES_PLASH_H) $(INCLUDES_PLASH_C) lib/c/plash.o

format:
	find . -iname *.h -o -iname *.c | xargs clang-format -i -style="{CommentPragmas: '^ usage:'}"
	file lib/exec/* | grep -i python | cut -d':' -f1 | xargs black
	black .
