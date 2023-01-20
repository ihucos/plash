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

#include <data.h>
#include <mkdtemp.h>
#include <nodepath.h>
#include <import-lxc.h>
#include <with-mount.h>
#include <export-tar.h>
#include <create.h>
#include <import-tar.h>
#include <init.h>
#include <purge.h>
#include <import-url.h>
#include <sudo.h>
#include <clean.h>
#include <version.h>
#include <parent.h>
#include <add-layer.h>
#include <map.h>
#include <shrink.h>
#include <run.h>
#include <help.h>
#include <help-macros.h>
#include <runb.h>
#include <eval.h>
#include <rm.h>
#include <mount.h>
#include <copy.h>
#include <b.h>
#include <build.h>
#include <import-docker.h>
#include <eval-plashfile.h>

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
    fprintf(stderr, "plash is a container build and run engine, try --help\n");
    return 1;
  }

  if (is_cli_param(argv[1]) &&
      !(!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help") ||
        !strcmp(argv[1], "--version") || !strcmp(argv[1], "--help-macros")))
    reexec_insert_run(argc, argv);

  if (strcmp(argv[1], "data") == 0) return data_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "mkdtemp") == 0) return mkdtemp_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "nodepath") == 0)return nodepath_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "import-lxc") == 0) return import_lxc_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "with-mount") == 0) return with_mount_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "export-tar") == 0) return export_tar_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "create") == 0) return create_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "import-tar") == 0) return import_tar_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "init") == 0) return init_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "purge") == 0) return purge_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "import-url") == 0) return import_url_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "sudo") == 0) return sudo_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "clean") == 0) return clean_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "version") == 0) return version_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "parent") == 0) return parent_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "add-layer") == 0) return add_layer_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "map") == 0) return map_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "shrink") == 0) return shrink_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "run") == 0) return run_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "help") == 0) return help_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "help-macros") == 0) return help_macros_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "runb") == 0) return runb_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "eval") == 0) return eval_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "rm") == 0) return rm_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "mount") == 0) return mount_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "copy") == 0) return copy_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "b") == 0) return b_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "build") == 0) return build_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "import-docker") == 0) return import_docker_main(argc - 1, argv + 1);
  if (strcmp(argv[1], "eval-plashfile") == 0) return eval_plashfile_main(argc - 1, argv + 1);
  errno = 0;
  pl_fatal("no such command: %s (try `plash help`)", argv[1]);
}
