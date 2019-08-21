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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <plash.h>

int main(int argc, char *argv[]) {
  char *plash_data;

  if (argc != 3)
    pl_usage();

  pl_unshare_user();

  char *nodepath = pl_check_output((char*[]){
        "plash", "nodepath", argv[1], "--allow-root-container", NULL});

  plash_data = getenv("PLASH_DATA");
  assert(plash_data);
  assert(plash_data[0] == '/');

  char *templ;
  if (asprintf(&templ,
               "%s/tmp/plashtmp_%d_%d_XXXXXX",
               plash_data,
               getsid(0),
               getpid)
          == -1)
    pl_fatal("asprintf");

  char *prepared_new_node = mkdtemp(templ);

  if (chmod(prepared_new_node, 0755) == -1)
        pl_fatal("chmod");

  if (chdir(prepared_new_node) == -1)
        pl_fatal("chdir");

  if (mkdir("_data", 0755) == -1)
        pl_fatal("mkdir");

   if (rename(argv[2], "_data/root") == -1)
       pl_fatal("rename: %s", argv[2]);

  char *src;
  off_t node_id_candidate;
  for (;;) {

      char *dst;

      if (chdir(plash_data) == -1)
            pl_fatal("chdir");
   
       int fd = open("id_counter", O_WRONLY | O_APPEND);
       if (fd < 0)
           pl_fatal("open");

       if (write(fd, "A", 1) != 1)
           pl_fatal("write");

       node_id_candidate = lseek(fd, 0, SEEK_CUR);
       if (node_id_candidate == -1)
           pl_fatal("lseek");

       if (close(fd) == -1)
           pl_fatal("close");
       
      if (chdir("index") == -1)
            pl_fatal("chdir");


      if (asprintf(&src, "..%s/%ld", nodepath + strlen(plash_data), node_id_candidate) == -1)
          pl_fatal("asprintf");
       

      if (asprintf(&dst, "%ld", node_id_candidate) == -1)
          pl_fatal("asprintf");

       symlink(src, dst);
       if (errno == EEXIST){
           // if taken, try again
           continue;
       } else if (errno) {
           pl_fatal("symlink");
       }
       break;

   }
   rename(prepared_new_node, src);
   if (errno) pl_fatal("rename");
   fprintf(stdout, "%ld\n", node_id_candidate);

}
