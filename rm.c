// Deletes the given image atomically. Running containers based on that image
// have an undefined behaviour.

#define USAGE "usage: plash rm IMAGE_ID\n"

#include <plash.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int rm_main(int argc, char *argv[]) {
  if (argc != 2) {
    fputs(USAGE, stderr);
    return EXIT_FAILURE;
  }

  char *nodepath = plash("nodepath", argv[1]);
  char *tmp = plash("mkdtemp");

  pl_unshare_user();

  if (rename(nodepath, tmp) == -1)
    pl_fatal("rename %s %s", nodepath, tmp);

  execlp("rm", "rm", "-rf", tmp, NULL);
  pl_fatal("execlp");
  return EXIT_SUCCESS;
}
