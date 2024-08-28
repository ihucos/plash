#define USAGE "usage: plash check IMAGE_ID PATH\n"

#define _GNU_SOURCE

#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include <plash.h>

int check_main(int argc, char *argv[]) {

  if (argc != 3) {
    fputs(USAGE, stderr);
    return EXIT_FAILURE;
  }

  char *image_id = argv[1];
  char *path = argv[2];

  struct stat attr;
  if (stat(path, &attr) == -1) {
    pl_fatal("stat(%s)", path);
  }

  char *cache_key = NULL;
  if (asprintf(&cache_key, "check:%s:%lu:%ld:%ld",
               image_id,
               (unsigned long)attr.st_mtime,
               (unsigned long)attr.st_ctime,
               (long)attr.st_ino) == -1) {
    pl_fatal("asprintf");
  }

  char *existing_image_id = map_call(cache_key, NULL);
  if (existing_image_id != NULL) {
    puts(existing_image_id);
    return EXIT_SUCCESS;
  } else {
    char *fresh_image_id = plash("stack", image_id, plash("mkdtemp"));
    map_call(cache_key, fresh_image_id);
    puts(fresh_image_id);
  }

  return EXIT_SUCCESS;
}
