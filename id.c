
#define USAGE "usage: plash id ID\n"

#include <stdio.h>
#include <stdlib.h>

#include <plash.h>

int id_main(int argc, char *argv[]) {
  if (argc != 2){
    fputs(USAGE, stderr);
    return EXIT_FAILURE;
  }
  puts(argv[1]);
  return EXIT_SUCCESS;
}
