CFLAGS=-I.

all: dist/plash

dist/plash: %: *.c
	make dist
	$(CC) $(CFLAGS) *.c -o dist/plash

install:
	cp dist/plash /usr/local/bin/plash

clean:
	rm -f $(INCLUDES_PLASH_H) dist/plash

format:
	clang-format -i *.c
