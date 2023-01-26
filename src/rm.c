// Deletes the given image atomically. Running containers based on that image
// have an undefined behaviour.

#define USAGE "usage: plash rm IMAGE_ID\n"

#include <stdio.h>
#include <unistd.h>

#include <plash.h>


int rm_main(int argc, char *argv[]) {
  if (argc != 2) {
    fputs(USAGE, stderr);
    return 1;
  }

  char *nodepath = pl_cmd(nodepath_main, argv[1]);
  char *tmp = pl_cmd(mkdtemp_main);

  pl_unshare_user();

  if (rename(nodepath, tmp) == -1)
    pl_fatal("rename %s %s", nodepath, tmp);

  execlp("rm", "rm", "-rf", tmp, NULL);
  pl_fatal("execlp");
  return 0;
}
