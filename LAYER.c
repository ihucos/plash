
#define USAGE "usage: plash LAYER CMD...\n"

#include <stdlib.h>

#include <plash.h>

int layer_main(int argc, char *argv[]) {

  if (argc < 1) {
    fputs(USAGE, stderr);
    return EXIT_FAILURE;
  }

  pl_exec_add("/proc/self/exe");
  pl_exec_add("noid");
  pl_exec_add("cached");
  pl_exec_add("create");
  argv++;
  while (*argv) {
    pl_exec_add(*argv);
    argv++;
  }
  pl_exec_add(NULL);

}
