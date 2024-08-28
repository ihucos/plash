#define USAGE "usage: plash cache PLASH_CMD ...\n"
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <plash.h>

// djb2 non-cryptografic hash function found here:
// http://www.cse.yorku.ca/~oz/hash.html
unsigned long myhash(unsigned char *str) {
  unsigned long hash = 5381;
  int c;

  while (c = *str++)
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  return hash;
}

char *get_cache_key(char **args) {
  unsigned long h = 0;
  char *cache_key;

  while (*args) {
    h += myhash(*args);
    args++;
  }
  asprintf(&cache_key, "cache:%lu", h) || pl_fatal("asprintf");
  return cache_key;
}

int cache_main(int argc, char *argv[]) {
  if (argc < 2) {
    fputs(USAGE, stderr);
    return EXIT_FAILURE;
  }

  char *cache_key = get_cache_key(argv + 1);
  char *image_id = map_call(cache_key, NULL);
  if (image_id != NULL) {
    puts(image_id);
  } else {
    argv[0] = "/proc/self/exe";
    image_id = pl_firstline(pl_check_output(argv));
    map_call(cache_key, image_id);
    puts(image_id);
  }
  return EXIT_SUCCESS;
}
