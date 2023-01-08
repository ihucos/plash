// usage: plash import-url URL
// Import a container from an url.

#define _GNU_SOURCE
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include <plash.h>

int main(int argc, char *argv[]) {
  char *url = argv[1];
  if (url == NULL)
    pl_usage();

  char *rootfs = NULL;
  asprintf(&rootfs, "%s/rootfs", pl_call("mkdtemp")) != -1 ||
      pl_fatal("asprintf");
  pl_run("curl", "--progress-bar", "--fail", "--location", "--output", rootfs,
         url);
  execlp("plash", "plash", "import-tar", "rootfs", NULL);
  pl_fatal("execlp");
}
