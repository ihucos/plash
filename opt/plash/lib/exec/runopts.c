// usage: plash runopts -c CONTAINER [*OPTS] [CMD1 [ CMD2 ... ]]
//
// Run a container specifying lower level options.  Usually you'll want to use
// `plash run` instead. If no command is specified the containers default root
// shell will be executed as login shell.
//
// Supported options:
//
// -c CONTAINER
//        The container id to run
//
// -d CHANGESIDR
//        If specified changes to the root file system will be written there
//
// -m MOUNT
//        Mount a path to the same location inside the container, can be
//        specified multiple times
//
// -e ENV
//        Whitelist environment variable for import into container
//
// -E ENV
//        Whitelist all environment variables specified in given environment
//        variable. Separator is ':'
//
// -i
//        Start with empty environment. -e and -E implies -i

#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <unistd.h>

#include <plash.h>

#define OPTSTRING "c:d:m:e:E:i"

int main(int argc, char *argv[]) {

  char *plash_data = pl_call("data");
  char *changesdir = NULL;
  char *container = NULL;

  int opt;

  // validate args and collect changesdir and container
  while ((opt = getopt(argc, argv, "c:d:m:i:e:E:")) != -1) {
    switch (opt) {
    case 'c':
      container = optarg;
      break;
    case 'd':
      changesdir = optarg;
      break;
    case ':':
    case '?':
      pl_usage();
    }
  }

  char *rootfs = "myrootfs";

  // patch argv to remove the -d and replace -c with rootfs
  char **argvptr = argv;
  char **newargv = argv;
  argv++;
  while (*argv){
    if ((!strcmp(*argv, "-d")))
      argv += 2;
    if ((!strcmp(*argv, "-c"))){
      argv += 2;
      //*(newargv++) = "-c"
      //*(newargv++) = rootfs;
    }
    *(newargv++) = *(argv++);
  }
  *(newargv++) = NULL;


  while(*argvptr)
    puts(*(argvptr++));

  return 0;

  //
  // get "userspace root"
  //
  pl_unshare_user();
  pl_unshare_mount();

  //
  // prepare an empty mountpoint
  //
  if (chdir(plash_data) == -1)
    pl_fatal("chdir");
  if (mount("tmpfs", "mnt", "tmpfs", MS_MGC_VAL, NULL) == -1)
    pl_fatal("mount");

  //
  // mount root filesystem at the empty mountpoint
  //
  pl_call("mount", container, "mnt",
          changesdir); // changesdir is ignored if it is NULL


  // execvp(*run_args, run_args);
  //puts(container);
  puts("===");
  while(*argv)
    puts(*(argv++));
}
