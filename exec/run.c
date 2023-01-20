// usage: plash run IMAGE_ID [ CMD1 [ CMD2 ... ] ]
//
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

void read_envs_from_plashenv() {
  char *line = NULL;
  char *lineCopy = NULL;
  size_t len = 0;
  FILE *fp = fopen(".plashenvs", "r");
  if (fp == NULL)
    return;
  while ((getline(&line, &len, fp)) != -1) {
    line[strcspn(line, "\n")] = 0; // chop newline char
    lineCopy = strdup(line);
    if (!lineCopy) {
      pl_fatal("strdup");
    }
    pl_whitelist_env(lineCopy);
  }
  fclose(fp);
}

void read_envs_from_plashenvprefix() {
  char *line = NULL;
  char *lineCopy = NULL;
  size_t len = 0;
  FILE *fp = fopen(".plashenvsprefix", "r");
  if (fp == NULL)
    return;
  while ((getline(&line, &len, fp)) != -1) {
    line[strcspn(line, "\n")] = 0; // chop newline char
    lineCopy = strdup(line);
    if (!lineCopy) {
      pl_fatal("strdup");
    }
    pl_whitelist_env_prefix(lineCopy);
  }
  fclose(fp);
}

void read_mounts_from_plashmounts() {
  char *line = NULL;
  char *lineCopy = NULL;
  size_t len = 0;
  char *mount;
  FILE *fp = fopen(".plashmounts", "r");
  if (fp == NULL)
    return;
  while ((getline(&line, &len, fp)) != -1) {
    line[strcspn(line, "\n")] = 0; // chop newline char
    lineCopy = strdup(line);
    if (!lineCopy) {
      pl_fatal("strdup");
    }
    mount = strsep(&lineCopy, ":");
    errno = 0;
    if (mount[0] != '/')
      pl_fatal("src mount in /.plashmounts must start with a slash");
    if (lineCopy == NULL) {
      pl_bind_mount(mount, mount + 1);
    } else {
      pl_bind_mount(mount, lineCopy + 1);
      errno = 0;
      if (lineCopy[0] != '/')
        pl_fatal("dst mount in /.plashmounts must start with a slash");
    }
  }
  fclose(fp);
}

int run_main(int argc, char *argv[]) {

  if (argc < 2)
    pl_usage();
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

  // ensure /etc/resolv.conf is a normal file. Because if it where a symlink,
  // mounting over it would not work as expected
  unlink("etc/resolv.conf");
  int fd;
  if ((fd = open("etc/resolv.conf", O_CREAT | O_WRONLY)) < 0)
    pl_fatal("open");
  close(fd);
  pl_bind_mount("/etc/resolv.conf", "etc/resolv.conf");

  read_mounts_from_plashmounts();

  //
  // Import envs
  //
  pl_whitelist_env("TERM");
  pl_whitelist_env("DISPLAY");
  pl_whitelist_env("PLASH_DATA");
  pl_whitelist_envs_from_env("PLASH_EXPORT");
  read_envs_from_plashenv();
  read_envs_from_plashenvprefix();
  pl_whitelist_env(NULL);

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
