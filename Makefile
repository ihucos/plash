CFLAGS=-Iheaders/ -I./

INCLUDES_PLASH_H=bin/plash bin/plash-exec tests/C/pl_setup_user_ns 
INCLUDES_PLASH_C=tests/C/pl_parse_subid


all: $(INCLUDES_PLASH_H) $(INCLUDES_PLASH_C)

$(INCLUDES_PLASH_H): %: %.c plash.c data.c mkdtemp.c nodepath.c sudo.c add-layer.c map.c parent.c run.c runb.c mount.c rm.c copy.c eval.c import-tar.c export-tar.c init.c import-url.c version.c with-mount.c eval-plashfile.c create.c b.c shrink.c purge.c import-docker.c import-lxc.c clean.c help-macros.c build.c help.c

clean:
	rm -f $(INCLUDES_PLASH_H) $(INCLUDES_PLASH_C)

format:
	find . -iname *.h -o -iname *.c | xargs clang-format -i -style="{CommentPragmas: '^ usage:'}"
	file * | grep -i python | cut -d':' -f1 | xargs black
	black .
