//
// usage: plash parent CONTAINER
// Prints the containers parent container.
//
// Example:
// $ plash parent 89
// 88

#include <libgen.h>
#include <plash.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
  if (argc != 2)
    pl_usage();

  char *nodepath =
      pl_check_output((char *[]){"plash", "nodepath", argv[1], NULL});

  puts(basename(dirname(nodepath)));
}
