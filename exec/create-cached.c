//// usage: plash create-cached IMAGE-ID [ CMD1 [ CMD2 ... ] ]

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <plash.h>

#define BUF_SIZE 1024

// djb2 non-cryptografic hash function found here:
// http://www.cse.yorku.ca/~oz/hash.html
unsigned long hash(unsigned char *str) {
  unsigned long hash = 5381;
  int c;

  while (c = *str++)
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  return hash;
}

int main(int argc, char *argv[]) {

  char *cache_key = NULL;
  char *image_id;

  // read input
  char inp[BUF_SIZE];
  size_t read_bytes = fread(inp, 1, sizeof(inp) + 1, stdin);
  inp[read_bytes] = '\0';

  // generate cache key
  asprintf(&cache_key, "create-cached:%lu\n", hash(inp)) ||
      pl_fatal("asprintf");

  // return the cached image id or generate one and cache it
  char *cached_image_id = pl_call("map", cache_key);
  if (strcmp(cached_image_id, "") != 0) {
    puts(cached_image_id);
  } else {

    FILE *file_stdin;
    FILE *file_stdout;
 
    // mold args for plash create process
    char *args[argc + 1];
    size_t i = 0;
    args[i++] = "plash";
    args[i++] = "create";
    argv++;
    while (*argv)
      args[i++] = *(argv++);
    args[i++] = NULL;

    pl_spawn_process(args, &file_stdin, &file_stdout, NULL);

    fputs(inp, file_stdin);

    fclose(file_stdin);

    int status;
    wait(&status);
    if (!WIFEXITED(status)) pl_fatal("child process died badly");

    image_id = pl_nextline(file_stdout);
    pl_call("map", cache_key, image_id);
     puts(image_id);

     fputs("exiting", stderr);
    exit(WEXITSTATUS(status));
  }
}
