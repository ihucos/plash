// usage: plash export-tar CONTAINER [ FILE | - ]
//
// Export container as tar archive. It exports the file system of a container
// to the given file as a compressed tar archive.  If no file is supplied or the
// file is '-' the tar archive wil be printed to stdout instead.

#include <stddef.h>
#include <unistd.h>

#include <plash.h>

int export_tar_main(int argc, char *argv[]) {
  char *image_id, *file;
  if (!(image_id = argv[1]))
    pl_usage();
  if (!(file = argv[2]))
    file = "-";
  pl_unshare_user();
  execvp("plash", (char *[]){"plash", "with-mount", image_id, "tar", "-cf",
                             file, ".", NULL});
}
