// Run an image in the build environment. Filesystem changes are saved to
// CHANGESDIR. This program might be merged together with `plash create` in the
// future.
//
// The following host file systems are mapped to the container:
// - /etc/resolv.conf
// - /sys
// - /dev
// - /proc
// - /home
// - /root

#define USAGE "usage: plash runb IMAGE_ID CHANGESDIR CMD1 [ CMD2 ... ] ]\n"

#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <plash.h>

int runb_main(int argc, char *argv[]) {

  if (argc < 4) {
    fputs(USAGE, stderr);
    return 1;
  }
  char *container_id = argv[1];
  char *changesdir = argv[2];
  char *origpwd = get_current_dir_name();
  char *plash_data = pl_call("data");
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
  pl_call("mount", container_id, "mnt", changesdir);

  //
  // mount
  //
  if (chdir("mnt") == -1)
    pl_fatal("chdir");
  pl_bind_mount("/sys", "sys");
  pl_bind_mount("/dev", "dev");
  pl_bind_mount("/proc", "proc");
  pl_bind_mount("/home", "home");
  pl_bind_mount("/root", "root");

  // ensure /etc/resolv.conf is a normal file. Because if it where a symlink,
  // mounting over it would not work as expected
  unlink("etc/resolv.conf");
  int fd;
  if ((fd = open("etc/resolv.conf", O_CREAT | O_WRONLY)) < 0)
    pl_fatal("open");
  close(fd);
  pl_bind_mount("/etc/resolv.conf", "etc/resolv.conf");

  //
  // Import envs
  //
  pl_whitelist_env("TERM");
  pl_whitelist_env("HOME");
  pl_whitelist_env("DISPLAY");
  pl_whitelist_env("PLASH_DATA");
  pl_whitelist_env(NULL);

  //
  // chroot, then reconstruct working directory
  //
  chroot(".") != -1 || pl_fatal("chroot");
  pl_chdir(origpwd);

  //
  // exec!
  //
  char **run_args = argv + 3;
  execvp(*run_args, run_args);
  pl_fatal("execvp");
}
