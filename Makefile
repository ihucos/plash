CC      = musl-gcc
CFLAGS  = -static
SHELL   = /bin/bash

%: %.c plash/libexec/utils.c
	$(CC) $(CFLAGS) $< -o $@

all:
	@for d in plash/libexec/plash-*.c; do \
		$(MAKE) "$${d::-2}"; \
	done
