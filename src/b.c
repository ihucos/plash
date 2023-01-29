// Build then run utility. Builds the given build commands and calls the given
// plash command with the builded container.
//
// Example:
// $ plash b run -A
// $ plash b run --eval-file ./plashfile -- ls

#define USAGE "usage: plash b PLASHCMD *BUILDARGS [-- *CMDARGS]\n"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <plash.h>

int b_main(int argc, char *argv[]) {
  if (argc < 2) {
    fputs(USAGE, stderr);
    return EXIT_FAILURE;
  }

  char **buildargs = argv;
  char **cmdargs = (char *[]){NULL};
  char *cmd = strdup(argv[1]);
  if (cmd == NULL)
    pl_fatal("strdup");

  // reuse argv to build command to build the image
  *argv = "/proc/self/exe";
  argv++;
  *argv = "build";
  argv++;

  while (*(++argv)) {
    if ((*argv)[0] == '-' && (*argv)[1] == '-' && (*argv)[2] == '\0') {
      // "--" found, cut the array here
      *argv = NULL;
      cmdargs = argv + 1;
    }
  }

  // Run the requested command with the builded image
  pl_exec_add("/proc/self/exe");
  pl_exec_add(cmd);
  pl_exec_add(pl_firstline(pl_check_output(buildargs)));
  while (*(cmdargs))
    pl_exec_add(*cmdargs++);
  pl_exec_add(NULL);
}
