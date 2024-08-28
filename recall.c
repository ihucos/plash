#define USAGE "usage: plash recall PLASH_CMD ...\n"

#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <plash.h>

int command_accepts_image_id(char *cmd) {
  return (
    (strcmp(cmd, "build") == 0) ||
    (strcmp(cmd, "nodepath") == 0) ||
    (strcmp(cmd, "parent") == 0) ||
    (strcmp(cmd, "rm") == 0) ||
    (strcmp(cmd, "run") == 0) ||
    (strcmp(cmd, "run:persist") == 0) ||
    (strcmp(cmd, "check") == 0) ||
    (strcmp(cmd, "stack") == 0) ||
    (strcmp(cmd, "id") == 0)
  );
}

int command_prints_image_id(char *cmd) {
  return (
    (strcmp(cmd, "pull:docker") == 0) ||
    (strcmp(cmd, "pull:lxc") == 0) ||
    (strcmp(cmd, "pull:tarfile") == 0) ||
    (strcmp(cmd, "pull:url") == 0) ||
    (strcmp(cmd, "build") == 0) ||
    (strcmp(cmd, "check") == 0) ||
    // (strcmp(cmd, "id") == 0) ||
    (strcmp(cmd, "parent") == 0)
  );
}

int recall_main(int argc, char *argv[]) {

  char *cache_key;
  char *cmd;

  if (asprintf(&cache_key, "recall:%d", getsid(0)) == -1)
    pl_fatal("asprintf");

  if (argc <= 1) {
    fputs(USAGE, stderr);
    return EXIT_FAILURE;
  }

  pl_array_add("/proc/self/exe");
  if (strcmp(argv[1], "cache") == 0) {
    pl_array_add(argv[1]);
    pl_array_add(argv[2]);
    cmd = argv[2];
    argv++;
  } else {
    pl_array_add(argv[1]);
    cmd = argv[1];
  }

  if (command_accepts_image_id(cmd)) {
    char *input_image_id = map_call(cache_key, NULL);
    if (input_image_id == NULL) {
      errno = 0;
      pl_fatal("Cannot recall any image id. Try` plash recall pull...`");
    }
    pl_array_add(input_image_id);
  }

  argv++; while (*(argv++))
    pl_array_add(*argv);
  pl_array_add(NULL);

  if (command_prints_image_id(cmd)) {
    char *output_image_id = pl_firstline(pl_check_output(pl_array));
    map_call(cache_key, output_image_id);
    puts(output_image_id);
  } else {
    // noo deed to call a subprocess, we can exec right away
    execvp(pl_array[0], pl_array);
    pl_fatal("execvp");
  }
  return EXIT_SUCCESS;
}
