// Set or get a container id bound to the calling process

#define USAGE "usage: plash this [ CONTAINER ]\n"

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <plash.h>

int this_main(int argc, char *argv[]) {
  char *cache_key;

  if (asprintf(&cache_key, "this:%d", getsid(0)) == -1)
    pl_fatal("asprintf");

  execlp("/proc/self/exe", "plash", "map", cache_key, argv[1], NULL);
  pl_fatal("execlp");

}
