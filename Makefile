CFLAGS=-I.

INCLUDES_PLASH_H=tests/C/pl_setup_user_ns 
INCLUDES_PLASH_C=tests/C/pl_parse_subid


all: $(INCLUDES_PLASH_H) $(INCLUDES_PLASH_C) dist/plash

$(INCLUDES_PLASH_H): %: %.c $(filter-out main.c, $(wildcard *.c))

dist/plash: %: *.c
	$(CC) $(CFLAGS) $? -o dist/plash

install:
	cp dist/plash /usr/local/bin/plash

clean:
	rm -f $(INCLUDES_PLASH_H) $(INCLUDES_PLASH_C) dist/plash

format:
	clang-format -i *.c
