// Prints the path of a given image.
//
// The --allow-root-container option
// allows the root image ("0") to be specified as image.
//
// Example:
// $ plash nodepath 19
// /home/ihucos/.plashdata/layer/0/2/19
// $ plash nodepath 19 | xargs tree

#define USAGE "usage: plash nodepath CONTAINER [--allow-root-container]\n"

#define _GNU_SOURCE
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <plash.h>

char * nodepath_call(char *id, char *flag) {

  int i = 0;
  char *nodepath, *plash_data;

  // validate/normalize input
  if (!id[0] || strspn(id, "0123456789") != strlen(id))
    pl_fatal("image arg must be a positive number, got: %s", id);

  if (0 == strcmp(id, "0") &&
      (flag != NULL && strcmp(flag, "--allow-root-container") != 0)) {
    pl_fatal("image must not be the special root image ('0')");
  }

  plash_data = data_call();
  if (chdir(plash_data) == -1 || chdir("index") == -1)
    pl_fatal("run `plash init`: chdir: %s", plash_data);

  if (!(nodepath = realpath(id, NULL))) {
    errno = 0;
    pl_fatal("no image: %s", id);
  }
  return nodepath;
}

int nodepath_main(int argc, char *argv[]) {
  if (argc < 2) {
    fputs(USAGE, stderr);
    return EXIT_FAILURE;
  }
  puts(nodepath_call(argv[1], argv[2]));
}
