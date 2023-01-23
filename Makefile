CFLAGS=-Isrc/ -Iheaders/

INCLUDES_PLASH_H=tests/C/pl_setup_user_ns 
INCLUDES_PLASH_C=tests/C/pl_parse_subid


all: $(INCLUDES_PLASH_H) $(INCLUDES_PLASH_C) dist/plash

$(INCLUDES_PLASH_H): %: %.c $(filter-out src/main.c, $(wildcard src/*.c))

dist/plash: %: src/*.c
	$(CC) $(CFLAGS) $? -o dist/plash

install:
	cp dist/plash /usr/local/bin/plash

clean:
	rm -f $(INCLUDES_PLASH_H) $(INCLUDES_PLASH_C) dist/plash

format:
	find . -iname *.h -o -iname *.c | xargs clang-format -i -style="{CommentPragmas: '^ usage:'}"
	file src/* | grep -i python | cut -d':' -f1 | xargs black
	black .
