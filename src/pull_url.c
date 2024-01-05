// Import a container from an url.

#define USAGE "usage: plash import-url URL\n"

#define _GNU_SOURCE
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <plash.h>

int pull_url_main(int argc, char *argv[]) {
  char *url = argv[1];
  if (url == NULL) {
    fputs(USAGE, stderr);
    return EXIT_FAILURE;
  }

  char *rootfs = NULL;
  asprintf(&rootfs, "%s/rootfs", pl_call("mkdtemp")) != -1 ||
      pl_fatal("asprintf");
  pl_run("curl", "--progress-bar", "--fail", "--location", "--output", rootfs,
         url);
  execlp("/proc/self/exe", "plash", "import-tar", rootfs, NULL);
  pl_fatal("execlp");
}
