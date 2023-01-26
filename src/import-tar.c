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
  char *tmpdir = pl_cmd(mkdtemp_main);
  pl_cmd(sudo_main, "tar", "-C", tmpdir, "-xf", tarfile);
  puts(pl_cmd(add_layer_main, "0", tmpdir));
  return 0;
}
