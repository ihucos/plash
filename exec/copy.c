// usage: plash copy CONTAINER DIR
//
// Copy the container's root filesystem to directory.

#include <plash.h>
#include <stdio.h>
#include <unistd.h>

int copy_main(int argc, char *argv[]) {
  if (argc != 3)
    pl_usage();
  char *container = argv[1];
  char *outdir = argv[2];
  char *tmpout = pl_call("mkdtemp");

  pl_unshare_user();
  pl_call("with-mount", container, "cp", "-r", ".", tmpout);
  if (rename(tmpout, outdir) == -1)
    pl_fatal("rename %s %s", tmpout, outdir);
  return 0;
}
