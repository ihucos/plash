#define _GNU_SOURCE
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

int main(int argc, char *argv[]) {

  if (argc <= 1) {
    fprintf(stderr, "Build and run containers, try `plash help`\n");
    return EXIT_FAILURE;
  }

  // Check if plash is being used as shebang interpreter
  /* if (strchr(argv[1], '/') != NULL) */
  /*   return exec_main(argc, argv); */

  DISPATCH("version", version_main);
  DISPATCH("cached", cached_main);
  DISPATCH("clean", clean_main);
  DISPATCH("build", build_main);
  DISPATCH("parent", parent_main);
  DISPATCH("data", data_main);
  DISPATCH("help", help_main);
  DISPATCH("init", init_main);
  DISPATCH("map", map_main);
  DISPATCH("mkdtemp", mkdtemp_main);
  DISPATCH("mount", mount_main);
  DISPATCH("mounted", mounted_main);
  DISPATCH("nodepath", nodepath_main);
  DISPATCH("recall", recall_main);
  DISPATCH("pull:docker", pull_docker_main);
  DISPATCH("pull:lxc", pull_lxc_main);
  DISPATCH("pull:tarfile", pull_tarfile_main);
  DISPATCH("pull:url", pull_url_main);
  DISPATCH("purge", purge_main);
  DISPATCH("push:dir", push_dir_main);
  DISPATCH("push:tarfile", push_tarfile_main);
  DISPATCH("rm", rm_main);
  DISPATCH("run", run_main);
  DISPATCH("run:persist", run_main);
  DISPATCH("chroot", chroot_main);
  DISPATCH("shrink", shrink_main);
  DISPATCH("stack", stack_main);
  DISPATCH("sudo", sudo_main);
  DISPATCH("version", version_main);
  DISPATCH("check", check_main);
  DISPATCH("do", do_main);

  errno = 0;
  pl_fatal("no such command: %s (try `plash help`)", argv[1]);
}
