//
// Initialize build data. Run this on a new system before anything else.

#define USAGE "usage: plash init\n"

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdlib.h>

#include <plash.h>

void ensuredir(char *pathname, mode_t mode) {
  if (mkdir(pathname, mode) == -1 && errno != EEXIST)
    pl_fatal("mkdir %s", pathname);
}

int init_main(int argc, char *argv[]) {

  char *plash_data = pl_call("data");

  ensuredir(plash_data, 0700);
  if (chdir(plash_data) == -1)
    pl_fatal("chdir %s", plash_data);
  ensuredir("index", 0775);
  ensuredir("map", 0775);
  ensuredir("tmp", 0775);
  ensuredir("mnt", 0775);
  ensuredir("layer", 0775);
  ensuredir("layer/0", 0775);
  ensuredir("layer/0/_data", 0775);
  ensuredir("layer/0/_data/root", 0775);

  int fd = open("id_counter", O_CREAT | O_WRONLY | O_EXCL, 0775);
  if (fd == -1 && errno != EEXIST)
    pl_fatal("open");
  close(fd);

  if (symlink("../layer/0", "index/0") == -1 && errno != EEXIST)
    pl_fatal("symlink");
  return EXIT_SUCCESS;
}
