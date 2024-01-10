
#define USAGE "usage: plash chroot DIR [ CMD1 [ CMD2 ... ] ]\n"

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

int is_delimited_substring2(char *haystack, char *needle, char *delim) {
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

int chroot_main(int argc, char *argv[]) {

  if (argc < 2) {
    fputs(USAGE, stderr);
    return EXIT_FAILURE;
  }
  char *chroot_to = argv[1];
  char *origpwd = get_current_dir_name();

  //
  // Unshare mount ns
  //
  pl_unshare_mount();

  //
  // mount
  //
  if (chdir(chroot_to) == -1)
    pl_fatal("chdir %s", chroot_to);
  pl_bind_mount("/tmp", "tmp");
  pl_bind_mount("/home", "home");
  pl_bind_mount("/root", "root");
  pl_bind_mount("/sys", "sys");
  pl_bind_mount("/dev", "dev");
  pl_bind_mount("/proc", "proc");
  pl_bind_mount("/", "host");
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

    if (!(strcmp(name, "TERM") == 0 || strcmp(name, "DISPLAY") == 0 ||
          strcmp(name, "PLASH_DATA") == 0) &&
        !is_delimited_substring2(plash_export_vars, name, ":")) {

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
