// usage: plash mkdtemp
// Create a temporary directory in the plash data.

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <plash.h>

int main() {
  char *tmpdir, *tmpdir_templ;
  tmpdir_templ = pl_sprintf(
		  "%s/tmp/plashtmp_%d_%d_XXXXXX",
		  pl_call("data"),
		  getsid(0),
		  getppid());
  tmpdir = mkdtemp(tmpdir_templ);
  if (tmpdir == NULL)
    pl_fatal("mkdtemp: %s", tmpdir_templ);
  puts(tmpdir);
}
