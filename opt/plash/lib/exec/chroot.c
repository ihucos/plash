// usage: plash chroot -c ROOTFS [*OPTS] [CMD1 [ CMD2 ... ]]
//
// Chroot to the specified root file system.
//
// Supported options:
//
// -c ROOTFS
//        The root filesystem to chroot to.
//
// -m MOUNT
//        Mount a path to the same location inside the container, can be
//        specified multiple times.
//
// -e ENV
//        Whitelist environment variable for import into container.
//
// -E ENV
//        Whitelist all environment variables specified in given environment
//        variable. Separator is ':'.
//
// -i
//        Start with empty environment. -e and -E implies -i.

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

#define OPTSTRING "c:m:e:E:i"

char *get_default_root_shell() {
  struct passwd *pwd = getpwuid(0);
  if (pwd == NULL) {
    return "/bin/sh";
  } else {
    return pwd->pw_shell;
  }
}

int main(int argc, char *argv[]) {

  // don't let getopt print error messages
  opterr = 0;

  //
  // save and validate the user arguments
  //
  int opt;
  char *chroot_dir = NULL;
  int manage_envs = 0;
  while ((opt = getopt(argc, argv, OPTSTRING)) != -1) {
    switch (opt) {
    case 'c':
      chroot_dir = optarg;
      break;
      if (!(optarg[0] == '/'))
        pl_fatal("mount path must be absolute");
    case 'i':
    case 'e':
    case 'E':
      manage_envs = 1;
      break;
    case ':':
      pl_usage();
      break;
    case '?':
      pl_usage();
    }
  }
  if (chroot_dir == NULL)
    pl_usage();

  //
  // get "userspace root"
  //
  pl_unshare_user();
  pl_unshare_mount();

  char *origpwd = get_current_dir_name();

  if (chdir(chroot_dir) == -1)
    pl_fatal("chdir %s", chroot_dir);

  optind = 1; // reset the getopt
  while ((opt = getopt(argc, argv, OPTSTRING)) != -1) {
    switch (opt) {
    case 'm':
      pl_bind_mount(optarg, optarg + 1);
      break;
    case 'e':
      pl_whitelist_env(optarg);
      break;
    case 'E':
      pl_whitelist_envs_from_env(optarg);
      break;
    }
  }

  if (manage_envs)
    pl_whitelist_env(NULL);

  //
  // chroot, then reconstruct working directory
  //
  chroot(".") != -1 || pl_fatal("chroot");
  pl_chdir(origpwd);

  //
  // build up the arguments to run
  //
  char **run_args = argv + optind;
  if (*run_args == NULL) {
    run_args = (char *[]){get_default_root_shell(), "-l", NULL};
  } else {
    run_args = argv + optind;
  }

  //
  // exec!
  //
  execvp(*run_args, run_args);
  if (errno == ENOENT) {
    fprintf(stderr, "%s: command not found\n", *run_args);
    return 127;
  }
  pl_fatal("exec");
}
