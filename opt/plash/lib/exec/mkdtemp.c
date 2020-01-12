// usage: plash mkdtemp
// Create a temporary directory in the plash data.

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <plash.h>

int main() {
  char *tmpdir, *tmpdir_templ;
  if (asprintf(&tmpdir_templ, "%s/tmp/plashtmp_%d_%d_XXXXXX", pl_call("data"),
               getsid(0), getppid()) == -1)
    pl_fatal("asprintf");
  tmpdir = mkdtemp(tmpdir_templ);
  if (tmpdir == NULL)
    pl_fatal("mkdtemp: %s", tmpdir_templ);
  puts(tmpdir);
}
