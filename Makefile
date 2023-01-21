CFLAGS=-Isrc/ -Iheaders/

INCLUDES_PLASH_H=bin/plash-exec tests/C/pl_setup_user_ns 
INCLUDES_PLASH_C=tests/C/pl_parse_subid


all: $(INCLUDES_PLASH_H) $(INCLUDES_PLASH_C) bin/plash

$(INCLUDES_PLASH_H): %: %.c $(filter-out src/main.c, $(wildcard src/*.c))

bin/plash: %: src/*.c
	$(CC) $(CFLAGS) $? -o bin/plash


clean:
	rm -f $(INCLUDES_PLASH_H) $(INCLUDES_PLASH_C) bin/plash

format:
	find . -iname *.h -o -iname *.c | xargs clang-format -i -style="{CommentPragmas: '^ usage:'}"
	file src/* | grep -i python | cut -d':' -f1 | xargs black
	black .
