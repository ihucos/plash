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
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <plash.h>

int nodepath_main(int argc, char *argv[]) {

  int i = 0;
  char *nodepath, *plash_data, *image_id = argv[1];

  if (argc < 2) {
    fputs(USAGE, stderr);
    return EXIT_FAILURE;
  }

  // validate/normalize input
  if (!image_id[0] || strspn(image_id, "0123456789") != strlen(image_id)){
    errno = 0;
    pl_fatal("image arg must be a positive number, got: %s", image_id);
  }

  if (0 == strcmp(image_id, "0") &&
      (argc <= 2 || 0 != strcmp(argv[2], "--allow-root-container"))) {
    image_id = pl_call("this");
    if (strcmp(image_id, "") == 0){
      errno = 0;
      pl_fatal("current image not set");
    }
  }

  plash_data = pl_call("data");
  if (chdir(plash_data) == -1 || chdir("index") == -1)
    pl_fatal("run `plash init`: chdir: %s", plash_data);

  if (!(nodepath = realpath(image_id, NULL))) {
    errno = 0;
    pl_fatal("no image: %s", image_id);
  }
  puts(nodepath);
  return EXIT_SUCCESS;
}
