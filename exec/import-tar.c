// usage: plash import-tar [ TARFILE ]
// Create a container from a tar file.
// If the TARFILE argument is ommited, the tar file is read from stdin.

#include <plash.h>
#include <stddef.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  char *tarfile = argv[1];
  if (tarfile == NULL) {
    tarfile = "-";
  }
  char *tmpdir = pl_call("mkdtemp");
  pl_run("plash", "sudo", "tar", "-C", tmpdir, "-xf", tarfile);
  execlp("plash", "plash", "add-layer", "0", tmpdir, NULL);
  pl_fatal("execlp");
}
