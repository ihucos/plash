CFLAGS=-Iexec/

INCLUDES_PLASH_H=bin/plash bin/plash-exec tests/C/pl_setup_user_ns 
INCLUDES_PLASH_C=tests/C/pl_parse_subid


all: $(INCLUDES_PLASH_H) $(INCLUDES_PLASH_C) exec/plash.o

$(INCLUDES_PLASH_H): %: %.c exec/plash.c exec/data.c exec/mkdtemp.c exec/nodepath.c exec/sudo.c exec/add-layer.c exec/map.c exec/parent.c exec/run.c exec/runb.c exec/mount.c exec/rm.c exec/copy.c exec/eval.c exec/import-tar.c exec/export-tar.c exec/init.c exec/import-url.c exec/version.c exec/with-mount.c exec/eval-plashfile.c exec/create.c exec/b.c exec/shrink.c exec/purge.c exec/import-docker.c exec/import-lxc.c exec/clean.c exec/help-macros.c exec/build.c exec/help.c

clean:
	rm -f $(INCLUDES_PLASH_H) $(INCLUDES_PLASH_C) exec/plash.o

format:
	find . -iname *.h -o -iname *.c | xargs clang-format -i -style="{CommentPragmas: '^ usage:'}"
	file exec/* | grep -i python | cut -d':' -f1 | xargs black
	black .
