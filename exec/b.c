// usage: plash b PLASHCMD *BUILDARGS [-- *CMDARGS]
// Build then run utility. Builds the given build commands and calls the given
// plash command with the builded container.
//
// Example:
// $ plash b run -A
// $ plash b run --eval-file ./plashfile -- ls

#include <stdio.h>
#include <string.h>

#include <plash.h>

int main(int argc, char *argv[]){
  if (argc < 2) pl_usage();
  char **buildargs = argv;
  char **cmdargs = (char*[]){NULL};
  char *cmd = strdup(argv[1]);
  pl_exec_add("plash");
  pl_exec_add(cmd);

  *argv = "plash";
  argv++;
  *argv = "build";
  argv++;

  while(*(++argv)){
    if ((*argv)[0] == '-' && (*argv)[1] == '-' && (*argv)[2] == '\0'){
      *argv = NULL;
      cmdargs = argv+1;
    }
  }

  pl_exec_add(pl_check_output(buildargs));
  while(*(cmdargs)) pl_exec_add(*cmdargs++);
  pl_exec_add(NULL);



}
