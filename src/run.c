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
    if (strcmp(needle, token) == 0){
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
  char *container_id = argv[1];
  char *origpwd = get_current_dir_name();
  char *plash_data = pl_call("data");
  char *changesdir = pl_call("mkdtemp");

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
  pl_bind_mount("/tmp", "tmp");
  pl_bind_mount("/home", "home");
  pl_bind_mount("/root", "root");
  pl_bind_mount("/sys", "sys");
  pl_bind_mount("/dev", "dev");
  pl_bind_mount("/proc", "proc");
  if (mkdir("host", 0755) == -1)
    pl_fatal("mkdir");
  pl_bind_mount("/", "host");

  // ensure /etc/resolv.conf is a normal file. Because if it where a symlink,
  // mounting over it would not work as expected
  unlink("etc/resolv.conf");
  int fd;
  if ((fd = open("etc/resolv.conf", O_CREAT | O_WRONLY, 0775)) < 0)
    pl_fatal("open");
  close(fd);
  pl_bind_mount("/etc/resolv.conf", "etc/resolv.conf");

  //
  // Prefix all envs with _HOST_
  // Keep the following envs: TERM, DISPLAY and PLASH_DATA
  // Keep any env inside PLASH_EXPORT
  //
  char **env = environ;
  char *name;
  char *hostPrefixedEnv;
  char *plash_export_vars = getenv("PLASH_EXPORT");

  if (!plash_export_vars)
    plash_export_vars = "";


  while (*env != NULL) {

    if (asprintf(&hostPrefixedEnv, "_HOST_%s", *env) == -1)
      pl_fatal("asprintf");

    if (putenv(hostPrefixedEnv) != 0)
      pl_fatal("putenv");

    name = strtok(strdup(*env), "=");

    if (name == NULL)
      pl_fatal("strtok");

 
    if (!(strcmp(name, "TERM") == 0 ||
          strcmp(name, "DISPLAY") == 0 ||
          strcmp(name, "PLASH_DATA") == 0)
        && !is_delimited_substring(plash_export_vars, name, ":")) {

      if (unsetenv(name) != 0)
        pl_fatal("unsetenv");

      free(name);
    }
    env++;
  }

  //
  // chroot, then reconstruct working directory
  //
  chroot(".") != -1 || pl_fatal("chroot");
  pl_chdir(origpwd);

  //
  // build up the arguments to run
  //
  char **run_args = argv + 2;
  if (*run_args == NULL) {
    run_args = (char *[]){pl_get_default_root_shell(), "-l", NULL};
  }

  //
  // exec!
  //
  execvp(*run_args, run_args);
  if (errno == ENOENT) {
    fprintf(stderr, "%s: command not found\n", *run_args);
    return 127;
  }
  pl_fatal("execvp");
}
