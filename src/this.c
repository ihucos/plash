// Set or get a container id bound to the calling process

#define USAGE "usage: plash this [ CONTAINER ]\n"

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <plash.h>

int this_main(int argc, char *argv[]) {
  char *cache_key;

  if (asprintf(&cache_key, "this:%d", getsid(0)
	) == -1)
    pl_fatal("asprintf");

  char *image_id = pl_call("map", cache_key);

  if (strcmp(image_id, "") == 0){
    errno = 0;
    pl_fatal("current image not set");
  }

  if (!argv[1]){
    puts(image_id);
    return EXIT_SUCCESS;
  }

  pl_exec_add("/proc/self/exe");
  pl_exec_add(argv[1]);
  pl_exec_add(image_id);
  argv++;
  while (*(argv++))
    pl_exec_add(*argv);
  pl_exec_add(NULL);

}
