// Create a container from a tar file.
// If the TARFILE argument is ommited, the tar file is read from stdin.

#define USAGE "usage: plash import-tar [ TARFILE ]\n"

#include <plash.h>
#include <stddef.h>
#include <unistd.h>

int import_tar_main(int argc, char *argv[]) {
  char *tarfile = argv[1];
  if (tarfile == NULL) {
    tarfile = "-";
  }
  char *tmpdir = pl_call("mkdtemp");
  pl_run("/proc/self/exe", "sudo", "tar", "-C", tmpdir, "-xf", tarfile);
  execlp("/proc/self/exe", "plash", "add-layer", "0", tmpdir, NULL);
  pl_fatal("execlp");
}
