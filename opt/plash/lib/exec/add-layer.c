// usage: plash add-layer PARENT-CONTAINER IMPORT-DIR
// Stack a layer on top of a container.  Please note that it uses the rename
// syscall, this means your IMPORT-DIR will be moved into the plash data
// directory.  The container "0" is the empty root container, use that to start
// a container from scratch.
//
// This subcommand is very low-level, usually you would use `plash build`.
// Parameters may be interpreted as build instruction.
//
// Examples:
//
// Create a container from a complete root file system:
// $ plash add-layer 0 /tmp/rootfs
// 66
//
// Add a new layer on top of an existing container:
// $ plash add-layer 33 /tmp/mylayer
// 67

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <plash.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

  off_t node_id;
  int fd;
  char *linkpath, *node_id_str, *nodepath, *plash_data, *prepared_new_node;

  if (argc != 3)
    pl_usage();

  plash_data = getenv("PLASH_DATA");
  assert(plash_data);
  assert(plash_data[0] == '/');

  nodepath = pl_check_output(
      (char *[]){"plash", "nodepath", argv[1], "--allow-root-container", NULL});

  pl_unshare_user();

  prepared_new_node = pl_call("mkdtemp")

  if (chmod(prepared_new_node, 0755) == -1)
    pl_fatal("chmod");
  if (chdir(prepared_new_node) == -1)
    pl_fatal("chdir");
  if (mkdir("_data", 0755) == -1)
    pl_fatal("mkdir");
  if (rename(argv[2], "_data/root") == -1)
    pl_fatal("rename: %s", argv[2]);
  if (chdir(plash_data) == -1)
    pl_fatal("chdir");
  if ((fd = open("id_counter", O_WRONLY | O_APPEND)) == -1)
    pl_fatal("open");
  if (write(fd, "A", 1) != 1)
    pl_fatal("write");
  if ((node_id = lseek(fd, 0, SEEK_CUR)) == -1)
    pl_fatal("lseek");
  if (close(fd) == -1)
    pl_fatal("close");
  if (asprintf(&node_id_str, "%ld", node_id) == -1)
    pl_fatal("asprintf");
  if (asprintf(&linkpath, "..%s/%ld", nodepath + strlen(plash_data), node_id) ==
      -1)
    pl_fatal("asprintf");
  if (chdir("index") == -1)
    pl_fatal("chdir");
  if (symlink(linkpath, node_id_str) == -1)
    pl_fatal("symlink");
  if (rename(prepared_new_node, linkpath) == -1)
    pl_fatal("rename");
  if (puts(node_id_str) == EOF)
    pl_fatal("puts");
}
