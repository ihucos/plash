// Copy the images root filesystem to directory.

#define USAGE "usage: plash copy IMAGE_ID DIR\n"

#include <plash.h>
#include <stdio.h>
#include <unistd.h>

int copy_main(int argc, char *argv[]) {
  if (argc != 3) {
    fputs(USAGE, stderr);
    return 1;
  }
  char *container = argv[1];
  char *outdir = argv[2];
  char *tmpout = pl_call("mkdtemp");

  pl_unshare_user();
  pl_call("with-mount", container, "cp", "-r", ".", tmpout);
  if (rename(tmpout, outdir) == -1)
    pl_fatal("rename %s %s", tmpout, outdir);
  return 0;
}
