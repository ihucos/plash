// Create a temporary directory in the plash data.

#define USAGE "usage: plash mkdtemp\n"

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <plash.h>

int mkdtemp_main(int argc, char *argv[]) {
  char *tmpdir, *tmpdir_templ;
  if (asprintf(&tmpdir_templ, "%s/tmp/plashtmp_%d_%d_XXXXXX", data_call(),
               getsid(0), getppid()) == -1)
    pl_fatal("asprintf");
  tmpdir = mkdtemp(tmpdir_templ);
  if (tmpdir == NULL)
    pl_fatal("mkdtemp: %s", tmpdir_templ);
  puts(tmpdir);
  return EXIT_SUCCESS;
}
