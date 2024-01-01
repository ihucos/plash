// Used as shebang. It runs a plash buildfile.

#define USAGE "usage: plash exec file [arg1 [arg2 [arg3 ...]]]"

#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <plash.h>

int exec_main(int argc, char *argv[]) {
  if (argc < 2) {
    fputs(USAGE, stderr);
    return EXIT_FAILURE;
  }

  // cd to the dir where the plashfile is to build here
  char *plashfile = realpath(argv[1], NULL);
  if (plashfile == NULL)
    pl_fatal("realpath");
  char *plashfileCopy = strdup(plashfile);
  if (plashfileCopy == NULL)
    pl_fatal("strdup");
  if (chdir(dirname(plashfileCopy)) == -1)
    pl_fatal("chdir");

  pl_call("build", "--include", plashfile, NULL);

  pl_exec_add("/proc/self/exe");
  pl_exec_add("this");
  pl_exec_add("run");
  pl_exec_add("/entrypoint");
  argv++;
  while (*argv++)
    pl_exec_add(*argv);
  pl_exec_add(NULL);
}
