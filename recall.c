// Set or get a container id bound to the calling process

#define USAGE "usage: plash this [ CONTAINER ]\n"

#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <plash.h>

int recall_main(int argc, char *argv[]) {
  char *cache_key;

  if (asprintf(&cache_key, "this:%d", getsid(0)) == -1)
    pl_fatal("asprintf");

  char *image_id = plash("map", cache_key);

  if (strcmp(image_id, "") == 0) {
    errno = 0;
    pl_fatal("Cannot recall any image id");
  }

  if (!argv[1]) {
    puts(image_id);
    return EXIT_SUCCESS;
  }

  pl_run_add("/proc/self/exe");
  if (strcmp(argv[1], "cached") == 0) {
    pl_run_add(argv[1]);
    pl_run_add(argv[2]);
    pl_run_add(image_id);
    argv++;
  } else {
    pl_run_add(argv[1]);
    pl_run_add(image_id);
  }
  argv++;
  while (*(argv++))
    pl_run_add(*argv);
  char *new_image_id = pl_firstline(pl_run_add(NULL));

  plash("map", cache_key, new_image_id);

  puts(new_image_id);
}
