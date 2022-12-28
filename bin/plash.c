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
  int flags;

  if (argc <= 1) {
    fprintf(stderr, "plash is a container build and run engine, try --help\n");
    return 1;
  }

  if (is_cli_param(argv[1]) &&
      !(!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help") ||
        !strcmp(argv[1], "--version") || !strcmp(argv[1], "--help-macros")))
    reexec_insert_run(argc, argv);

  struct passwd *pwd;
  char *bindir = pl_path("../bin"), *libexecdir = pl_path("../exec"),
       *libexecrun = pl_path("../exec/run"),
       *pylibdir = pl_path("../lib/python"), *path_env = getenv("PATH"),
       *libexecfile, *newpath;

  //
  // setup environment variables
  //
  if (asprintf(&newpath, "%s:%s", bindir, path_env) == -1)
    pl_fatal("asprintf");
  if (setenv("PYTHONPATH", pylibdir, 1) == -1)
    pl_fatal("setenv");
  if (setenv("PATH", path_env ? newpath : bindir, 1) == -1)
    pl_fatal("setenv");

  //
  // exec lib/exec/<command>
  //
  if (asprintf(&libexecfile, "%s/%s", libexecdir, argv[1]) == -1)
    pl_fatal("asprintf");
  execvp(libexecfile, argv + 1);

  if (errno != ENOENT)
    pl_fatal("could not exec %s", libexecfile);
  errno = 0;
  pl_fatal("no such command: %s (try `plash help`)", argv[1]);
}
