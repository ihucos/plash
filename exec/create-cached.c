//// usage: plash create-cached IMAGE-ID [ CMD1 [ CMD2 ... ] ]


#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <plash.h>

#define BUF_SIZE 1024


// djb2 non-cryptografic hash function found here: http://www.cse.yorku.ca/~oz/hash.html
unsigned long hash(unsigned char *str)
{
  unsigned long hash = 5381;
  int c;

  while (c = *str++)
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  return hash;
}


int main(int argc, char *argv[]) {

  // read input
  char buf[BUF_SIZE];
  char* inp = fgets(buf, BUF_SIZE, stdin);
  char *cache_key = NULL;
  char *image_id;

  // generate cache key
  asprintf(&cache_key, "create-cached:%lu\n", hash(inp)) || pl_fatal("asprintf");

  // return the cached image id or generate one and cache it
  char * cached_image_id = pl_call("map", cache_key);
  if (strcmp(cached_image_id, "") != 0){
    puts(cached_image_id);
  } else {


    argv[1] = "create";
    FILE* file_stdin;
    FILE* file_stdout;
    pl_spawn_process(argv + 1, &file_stdin, NULL, NULL);


    fputs(inp, file_stdout);

    pl_call("map", cached_image_id, image_id);
    puts(image_id);
  }
}
