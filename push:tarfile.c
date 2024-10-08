//
// Export container as tar archive. It exports the file system of a container
// to the given file as a compressed tar archive.  If no file is supplied or the
// file is '-' the tar archive wil be printed to stdout instead.

#define USAGE "usage: plash export-tar CONTAINER [ FILE | - ]\n"

#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include <plash.h>

int push_tarfile_main(int argc, char *argv[]) {
  char *image_id, *file;
  if (!(image_id = argv[1])) {
    fputs(USAGE, stderr);
    return EXIT_FAILURE;
  }
  if (!(file = argv[2]))
    file = "-";
  pl_unshare_user();
  execvp("/proc/self/exe", (char *[]){"plash", "mounted", image_id, "tar",
                                      "-cf", file, ".", NULL});
}
