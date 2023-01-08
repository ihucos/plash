CFLAGS=-Ilib/

INCLUDES_PLASH_H=bin/plash tests/C/pl_setup_user_ns exec/sudo exec/nodepath exec/add-layer exec/map exec/parent exec/data exec/mkdtemp exec/run exec/runb exec/mount exec/rm exec/copy exec/eval exec/import-tar exec/export-tar exec/init exec/import-url exec/version
INCLUDES_PLASH_C=tests/C/pl_parse_subid

all: $(INCLUDES_PLASH_H) $(INCLUDES_PLASH_C) lib/plash.o

$(INCLUDES_PLASH_H): %: %.c lib/plash.c 

lib/plash.o: lib/plash.c
	$(CC) --shared -fPIC lib/plash.c -o lib/plash.o

clean:
	rm -f $(INCLUDES_PLASH_H) $(INCLUDES_PLASH_C) lib/plash.o

format:
	find . -iname *.h -o -iname *.c | xargs clang-format -i -style="{CommentPragmas: '^ usage:'}"
	file exec/* | grep -i python | cut -d':' -f1 | xargs black
	black .
