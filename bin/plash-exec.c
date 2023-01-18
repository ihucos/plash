// usage: plash-exec file [arg1 [arg2 [arg3 ...]]]
// Used as shebang. It runs a plash buildfile.

#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>

#include <plash.h>

int main(int argc, char *argv[]) {
  if (argc < 2) pl_usage();

  // cd to the dir where the plashfile is to build here
  char *plashfile = realpath(argv[1], NULL);
  if (plashfile == NULL) pl_fatal("realpath");
  char *plashfileCopy = strdup(plashfile);
  if (plashfileCopy == NULL) pl_fatal("strdup");
  if (chdir(dirname(plashfileCopy)) == -1) pl_fatal("chdir");

  pl_exec_add("plash");
  pl_exec_add("b");
  pl_exec_add("run");
  pl_exec_add("--eval-file");
  pl_exec_add(plashfile);
  pl_exec_add("--");
  pl_exec_add("/.plashentrypoint");
  argv++;
  while(*argv++) pl_exec_add(*argv);
  pl_exec_add(NULL);
}
