// Run a container. If no command is specified, the containers default root
// shell is executed.
//
// The following host file systems are mapped to the container:
// - /tmp
// - /home
// - /root
// - /etc/resolv.conf
// - /sys
// - /dev
// - /proc
// - /host (contains entire host filesystem)

#define USAGE "usage: plash run IMAGE_ID [ CMD1 [ CMD2 ... ] ]\n"

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

int is_delimited_substring(char *haystack, char *needle, char *delim) {
  char *str = strdup(haystack);
  char *token = strtok(str, delim);
  while (token) {
    if (strcmp(needle, token) == 0) {
      free(str);
      return 1;
    }
    token = strtok(NULL, ":");
  }
  free(str);
  return 0;
}

int run_main(int argc, char *argv[]) {

  if (argc < 2) {
    fputs(USAGE, stderr);
    return EXIT_FAILURE;
  }

  // set the changesdir
  char *changesdir;
  if (strcmp(argv[0], "run:persist") == 0){
    changesdir = argv[2];
    argv[2] = argv[1]; // push the img id parameter forward
    argv++;
  } else {
    changesdir = plash("mkdtemp");
  }

  char *container_id = argv[1];
  char *origpwd = get_current_dir_name();
  char *plash_data = data_call();
  char *mnt;

  //
  // get "userspace root"
  //
  pl_unshare_user();
  pl_unshare_mount();

  //
  // prepare an empty mountpoint
  //
  /* if (chdir(plash_data) == -1) */
  /*   pl_fatal("chdir"); */
  asprintf(&mnt, "%s/mnt", plash_data);
  if (mount("tmpfs", mnt, "tmpfs", MS_MGC_VAL, NULL) == -1)
    pl_fatal("mount");

  //
  // mount root filesystem at the empty mountpoint
  //
  plash("mount", container_id, mnt, changesdir);

  // Ensure /etc/resolv.conf is a normal file. Because if it where a symlink,
  // mounting over it (what happens in "plash chroot" would not work as
  // expected
  char *resolv_file;
  asprintf(&resolv_file, "%s/etc/resolv.conf", mnt);
  unlink(resolv_file);
  int fd;
  if ((fd = open(resolv_file, O_CREAT | O_WRONLY, 0775)) < 0)
    pl_fatal("open %s", resolv_file);
  close(fd);

  /* // Something else to prepare for "plash chroot" */
  /* char *host_file; */
  /* asprintf(&resolv_file, "%s/host", mnt); */
  /* mkdir(host_file, 0755); */


  pl_array_add("/proc/self/exe");
  pl_array_add("chroot");
  pl_array_add(mnt);

  // Use login shell
  pl_array_add("/bin/sh");
  pl_array_add("-lc");
  pl_array_add("exec env \"$@\"");
  pl_array_add("--");

  //
  // build up the arguments to run
  //
  char **run_args = argv + 2;
  for (int i = 0; run_args[i]; i++) {
    pl_array_add(run_args[i]);
  }
  pl_array_add(NULL);
  execvp(pl_array[0], pl_array);
  pl_fatal("execvp");
}
