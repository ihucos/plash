//
// Execute parameters inside a mounted container. Default parameter is the
// default shell. Mounting happens inside an isolated mount namespace.
//
// Example to get how much disk space a container is using:
// $ plash with-mount 70 du -sh
// 7,2M    .

#define _GNU_SOURCE

#define USAGE "usage: plash with-mount CONTAINER [ CMD1 [ CMD2  ... ] ]\n"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <plash.h>

int mounted_main(int argc, char *argv[]) {
  if (argc < 2) {
    fputs(USAGE, stderr);
    return EXIT_FAILURE;
  }
  char *image_id = argv[1];
  char *cmd = argv[2];

  pl_unshare_user();
  pl_unshare_mount();

  char *mountpoint;
  asprintf(&mountpoint, "%s/mnt", plash("data"));
  plash("mount", image_id, mountpoint);
  if (chdir(mountpoint) == -1)
    pl_fatal("chdir");

  if (cmd == NULL) {
    char *default_root_shell = pl_get_default_root_shell();
    execlp(default_root_shell, default_root_shell, NULL);
    pl_fatal("execlp");

  } else {
    argv++; // chop argv[0]
    argv++; // chop the image_id arg;
    execvp(*argv, argv);
    if (errno == ENOENT) {
      fprintf(stderr, "%s: command not found\n", *argv);
      return 127;
    }
    pl_fatal("execvp");
  }
  return EXIT_SUCCESS;
}
