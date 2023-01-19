CFLAGS=-Iexec/

INCLUDES_PLASH_H=bin/plash bin/plash-exec tests/C/pl_setup_user_ns exec/sudo exec/nodepath exec/add-layer exec/map exec/parent exec/data exec/mkdtemp exec/run exec/runb exec/mount exec/rm exec/copy exec/eval exec/import-tar exec/export-tar exec/init exec/import-url exec/version exec/with-mount exec/eval-plashfile exec/create exec/b exec/shrink exec/purge exec/import-docker exec/import-lxc exec/clean exec/help-macros exec/build exec/help
INCLUDES_PLASH_C=tests/C/pl_parse_subid


all: $(INCLUDES_PLASH_H) $(INCLUDES_PLASH_C) exec/plash.o

$(INCLUDES_PLASH_H): %: %.c exec/plash.c 

exec/plash.o: exec/plash.c
	$(CC) --shared  exec/plash.c -o exec/plash.o

clean:
	rm -f $(INCLUDES_PLASH_H) $(INCLUDES_PLASH_C) exec/plash.o

format:
	find . -iname *.h -o -iname *.c | xargs clang-format -i -style="{CommentPragmas: '^ usage:'}"
	file exec/* | grep -i python | cut -d':' -f1 | xargs black
	black .
