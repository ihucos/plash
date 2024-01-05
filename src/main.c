#define _GNU_SOURCE
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <unistd.h>

#include <plash.h>

#define DISPATCH(command, func)                                                \
  if (strcmp(argv[1], command) == 0)                                           \
    return func(argc - 1, argv + 1);

#define DISPATCH2(cmd1, cmd2, func)                                                \
  if ((strcmp(argv[1], cmd1) == 0) && argv[2] && (strcmp(argv[2], cmd2) == 0))                                          \
    return func(argc - 2, argv + 2);

int main(int argc, char *argv[]) {

  if (argc <= 1) {
    fprintf(stderr, "build and run containers, try `plash --help`\n");
    return EXIT_FAILURE;
  }

  // Check if plash is being used as shebang interpreter
  /* if (strchr(argv[1], '/') != NULL) */
  /*   return exec_main(argc, argv); */

  DISPATCH("version", version_main);
  DISPATCH("cached", cached_main);
  DISPATCH("clean", clean_main);
  DISPATCH("create", create_main);
  DISPATCH("data", data_main);
  DISPATCH("help", help_main);
  DISPATCH("init", init_main);
  DISPATCH("map", map_main);
  DISPATCH("mkdtemp", mkdtemp_main);
  DISPATCH("mount", mount_main);
  DISPATCH("mounted", mounted_main);
  DISPATCH("nodepath", nodepath_main);
  DISPATCH("noid", noid_main);
  DISPATCH2("pull", "docker", pull_docker_main);
  DISPATCH2("pull", "lxc", pull_lxc_main);
  DISPATCH2("pull", "tarfile", pull_tarfile_main);
  DISPATCH2("pull", "url", pull_url_main);
  DISPATCH("purge", purge_main);
  DISPATCH2("push", "dir", push_dir_main);
  DISPATCH2("push", "tarfile", push_tarfile_main);
  DISPATCH("rm", rm_main);
  DISPATCH("run", run_main);
  DISPATCH("runb", runb_main); // ID ONT WANT HTIS ONE
  DISPATCH("shrink", shrink_main);
  DISPATCH("stack", stack_main);
  DISPATCH("sudo", sudo_main);
  DISPATCH("version", version_main);

  errno = 0;
  pl_fatal("no such command: %s (try `plash help`)", argv[1]);
}
