// Deletes all build data unatomically. Running containers that rely on the
// build data will enter an undefined state. Plashs behaviour while this command
// did not finish running returning an non zero exit code is undefined. The
// data directory itself is not deleted as it could be a mountpoint.

#define USAGE "usage: plash purge [ --yes ]\n"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <plash.h>

int confirm_via_input() {
  printf("Delete all build data? [y/N] ");
  char inp[sizeof("y\n")];
  fgets(inp, sizeof(inp), stdin);
  return (strcmp(inp, "y\n") == 0);
}

int is_confirmed_via_argv(char **argv) {
  return (argv[1] && (strcmp(argv[1], "--yes") == 0));
}

int purge_main(int argc, char *argv[]) {

  if (!(is_confirmed_via_argv(argv) || confirm_via_input())) {
    fputs("Action not confirmed\n", stderr);
    return EXIT_FAILURE;
  }

  pl_unshare_user();

  char *plash_data = pl_call("data");
  if (chdir(plash_data) == -1)
    pl_fatal("chdir %s", plash_data);

  // this one first
  pl_run("rm", "rm", "-rf", "id_counter", NULL);

  pl_run("rm", "-rf", "index");
  pl_run("rm", "-rf", "layer");
  pl_run("rm", "-rf", "map");
  pl_run("rm", "-rf", "mnt");
  pl_run("rm", "-rf", "tmp");
  fputs("All deleted.\n", stderr);
  return EXIT_SUCCESS;
}
