#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <plash.h>

int main(){
  char *tmpdir;
  if (asprintf(&tmpdir, "%s/tmp/plashtmp_%d_%d_XXXXXX", pl_call("data"),
               getsid(0), getppid()) == -1)
    pl_fatal("asprintf");
  tmpdir = mkdtemp(tmpdir);
  if (tmpdir == NULL)
    pl_fatal("mkdtemp");
  puts(tmpdir);
}
