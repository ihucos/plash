#define USAGE "usage: plash cached PLASH_CMD ...\n"
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <plash.h>

char *cacheable_commands[] = {
  "build",
  "pull:docker",
  "pull:lxc",
  "pull:tarfile",
  "pull:url",
  NULL
};

int is_cacheable_command(char *cmd) {
  for (int i = 0; cacheable_commands[i]; i++) {
    if (strcmp(cacheable_commands[i], cmd) == 0)
      return 1;
  }
  return 0;
}

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
  asprintf(&cache_key, "cached:%lu", h) || pl_fatal("asprintf");
  return cache_key;
}

int cached_main(int argc, char *argv[]) {
  if (argc < 2) {
    fputs(USAGE, stderr);
    return EXIT_FAILURE;
  }

  if (!is_cacheable_command(argv[1])) {
    pl_fatal("Not a cacheable plash command: %s", argv[1]);
    return EXIT_SUCCESS;
  }

  char *cache_key = get_cache_key(argv + 1);
  char *image_id = plash("map", cache_key);
  if (strcmp(image_id, "") != 0) {
    puts(image_id);
  } else {
    argv[0] = "/proc/self/exe";
    image_id = pl_firstline(pl_check_output(argv));
    plash("map", cache_key, image_id);
    puts(image_id);
  }
  return EXIT_SUCCESS;
}
