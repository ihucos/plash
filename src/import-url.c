// Import a container from an url.

#define USAGE "usage: plash import-url URL\n"

#define _GNU_SOURCE
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include <plash.h>

int import_url_main(int argc, char *argv[]) {
  char *url = argv[1];
  if (url == NULL) {
    fputs(USAGE, stderr);
    return 1;
  }

  char *rootfs = NULL;
  asprintf(&rootfs, "%s/rootfs", pl_cmd(mkdtemp_main)) != -1 ||
      pl_fatal("asprintf");
  pl_run("curl", "--progress-bar", "--fail", "--location", "--output", rootfs,
         url);
  puts(pl_cmd(import_tar_main, rootfs));
  return 0;
}
