// Prints the containers parent container.
//
// Example:
// $ plash parent 89
// 88

#define USAGE "usage: plash parent IMAGE_ID\n"

#include <libgen.h>
#include <plash.h>
#include <stdio.h>

int parent_main(int argc, char *argv[]) {
  if (argc != 2) {
    fputs(USAGE, stderr);
    return 1;
  }
  char *nodepath = pl_call("nodepath", argv[1]);
  puts(basename(dirname(nodepath)));
  return 0;
}