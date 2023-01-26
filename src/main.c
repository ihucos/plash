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

void D(char *arr[]) {
  int ai;
  for (ai = 0; arr[ai]; ai++)
    fprintf(stderr, "%s, ", arr[ai]);
  fprintf(stderr, "\n");
}

int is_cli_param(char *param) {
  switch (strlen(param)) {
  case 1:
    return 0;
  case 2:
    return param[0] == '-' && param[1] != '-';
  default:
    return param[0] == '-';
  }
}

void reexec_insert_run(int argc, char **argv) {
  //  it: plash -A xeyes -- xeyes
  // out: plash b -A xeyes -- xeyes

  char *newargv_array[argc + 3];
  char **newargv = newargv_array;

  *(newargv++) = *(argv++);
  *(newargv++) = "b";
  *(newargv++) = "run";
  while (*(newargv++) = *(argv++))
    ;

  execvp(newargv_array[0], newargv_array);
  pl_fatal("execvp");
}

int main(int argc, char *argv[]) {

  if (argc <= 1) {
    fprintf(stderr, "build and run containers, try --help\n");
    return 1;
  }

  if (is_cli_param(argv[1]) &&
      !(!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help") ||
        !strcmp(argv[1], "--version") || !strcmp(argv[1], "--help-macros")))
    reexec_insert_run(argc, argv);

  // Check if plash is being used as shebang interpreter
  if (strchr(argv[1], '/') != NULL) return exec_main(argc, argv);

  DISPATCH("data", data_main);
  DISPATCH("mkdtemp", mkdtemp_main);
  DISPATCH("nodepath", nodepath_main);
  DISPATCH("import-lxc", import_lxc_main);
  DISPATCH("with-mount", with_mount_main);
  DISPATCH("export-tar", export_tar_main);
  DISPATCH("create", create_main);
  DISPATCH("import-tar", import_tar_main);
  DISPATCH("init", init_main);
  DISPATCH("purge", purge_main);
  DISPATCH("import-url", import_url_main);
  DISPATCH("sudo", sudo_main);
  DISPATCH("clean", clean_main);
  DISPATCH("version", version_main);
  DISPATCH("--version", version_main);
  DISPATCH("parent", parent_main);
  DISPATCH("add-layer", add_layer_main);
  DISPATCH("map", map_main);
  DISPATCH("shrink", shrink_main);
  DISPATCH("run", run_main);
  DISPATCH("help", help_main);
  DISPATCH("--help", help_main);
  DISPATCH("-h", help_main);
  DISPATCH("help-macros", help_macros_main);
  DISPATCH("--help-macros", help_macros_main);
  DISPATCH("runb", runb_main);
  DISPATCH("eval", eval_main);
  DISPATCH("rm", rm_main);
  DISPATCH("mount", mount_main);
  DISPATCH("copy", copy_main);
  DISPATCH("b", b_main);
  DISPATCH("build", build_main);
  DISPATCH("import-docker", import_docker_main);
  DISPATCH("eval-plashfile", eval_plashfile_main);

  errno = 0;
  pl_fatal("no such command: %s (try `plash help`)", argv[1]);
}
