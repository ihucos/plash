#define USAGE "usage: plash do PLASH_CMD ...\n"

#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <plash.h>

int do_main(int argc, char *argv[]) {
  if (argc < 2) {
    fputs(USAGE, stderr);
    return EXIT_FAILURE;
  }
  pl_array_init();
  pl_array_add("/proc/self/exe");
  pl_array_add("recall");

  if (
    strcmp(argv[1], "build") == 0 ||
    strncmp(argv[1], "pull:", strlen("pull:")) == 0
  ) {
    pl_array_add("cache");
  }
  for (int i = 1; argv[i]; i++) {
    pl_array_add(argv[i]);
  }
  pl_array_add(NULL);

  execvp(pl_array[0], pl_array);
  pl_fatal("execv");
  return EXIT_SUCCESS;
}
