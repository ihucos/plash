// usage: plash rm IMAGE_ID
// Deletes the given image atomically. Running containers based on that image
// have an undefined behaviour.

#include <plash.h>
#include <stdio.h>
#include <unistd.h>

int rm_main(int argc, char *argv[]) {
  if (argc != 2)
    pl_usage();

  char *nodepath = pl_call("nodepath", argv[1]);
  char *tmp = pl_call("mkdtemp");

  pl_unshare_user();

  if (rename(nodepath, tmp) == -1)
    pl_fatal("rename %s %s", nodepath, tmp);

  execlp("rm", "rm", "-rf", tmp, NULL);
  pl_fatal("execlp");
  return 0;
}
