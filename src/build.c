// Builds an image. Command line options are evaluated as macros. Use `plash
// help-macros` to list all available macros.
//
// Example:
//
// $ plash build -f ubuntu --run 'touch a'
// --> touch a
// --:
// 66
//
// $ plash build -f ubuntu:warty --run 'touch a' --layer --run 'touch b'
// --> touch b
// --:
// 67
//
// $ plash build -f ubuntu:warty --apt nmap
// --> apt-get update
// Hit:1 http://security.ubuntu.com/ubuntu bionic-security InRelease
// Get:2 http://archive.ubuntu.com/ubuntu bionic InRelease [235 kB]
// <snip>
// Setting up nmap (7.60-1ubuntu2) ...
// Processing triggers for libc-bin (2.26-0ubuntu2) ...
// --:
// 68

#define USAGE "usage: plash build --macro1 ar1 arg2 --macro2 arg1 ...\n"

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <plash.h>

#define PLASH_HINT_IMAGE "### plash hint: image="
#define PLASH_HINT_LAYER "### plash hint: layer"

#define MAX_BYTES_PER_LAYER 1024 * 4

void handle_plash_create_exit(pid_t pid) {
  int status;
  waitpid(pid, &status, 0);
  if (!WIFEXITED(status)) {
    pl_fatal("plash create subprocess exited abornmally");
  }
  int exit_status = WEXITSTATUS(status);
  if (exit_status != 0)
    exit(1);
}

void handle_plash_eval_exit(pid_t pid) {
  int status;
  waitpid(pid, &status, 0);
  if (!WIFEXITED(status)) {
    pl_fatal("subprocess exited abornmally");
  }
  if (WEXITSTATUS(status) != 0) {
    exit(1);
  }
}

// djb2 non-cryptografic hash function found here:
// http://www.cse.yorku.ca/~oz/hash.html
unsigned long hash(unsigned char *str) {
  unsigned long hash = 5381;
  int c;

  while (c = *str++)
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  return hash;
}

char *call_plash_create(char *image_id, char *shell_input) {
  //// run plash create to create this layer
  FILE *create_stdin, *create_stdout;
  pid_t create_pid =
      pl_spawn_process((char *[]){"plash", "create", image_id, "sh", NULL},
                       &create_stdin, &create_stdout, NULL);

  // send shell input to plash create
  fputs(shell_input, create_stdin);
  fclose(create_stdin);

  // exit program if plash create failed
  handle_plash_create_exit(create_pid);

  // get the image id plash create spitted out
  image_id = pl_nextline(create_stdout);
  if (image_id == NULL)
    pl_fatal("plash create did not print the image id");
  image_id = strdup(image_id);
  if (image_id == NULL)
    pl_fatal("strdup");

  fclose(create_stdout);

  // print some ASCII to the user
  fputs("---\n", stderr);

  return image_id;
}

char *cached_call_plash_create(char *base_image_id, char *shell_input) {
  char *image_id;
  char *cached_image_id;
  char *cache_key = NULL;
  asprintf(&cache_key, "layer:%s:%lu\n", base_image_id, hash(shell_input)) ||
      pl_fatal("asprintf");
  cached_image_id = pl_cmd(map_main, cache_key);
  if (strcmp(cached_image_id, "") != 0) {
    return cached_image_id;
  } else {
    image_id = call_plash_create(base_image_id, shell_input);
    pl_cmd(map_main, cache_key, image_id);
    return image_id;
  }
}

int build_main(int argc, char *argv[]) {
  char create_stdin_buf[MAX_BYTES_PER_LAYER];
  char *image_id;
  char *base_image_id;
  char *line;

  // mold args for plash eval process
  char *args[argc + 1];
  size_t i = 0;
  args[i++] = "plash";
  args[i++] = "eval";
  argv++;
  while (*argv)
    args[i++] = *(argv++);
  args[i++] = NULL;

  // run plash eval to get build shell script
  FILE *eval_stdout;
  pid_t eval_pid = pl_spawn_process(args, NULL, &eval_stdout, NULL);

  // wait for program to exit and handle its bad exit status code.
  handle_plash_eval_exit(eval_pid);

  // read first line
  line = pl_nextline(eval_stdout);

  // First line must be the image id hint
  if (line == NULL ||
      strncmp(line, PLASH_HINT_IMAGE, strlen(PLASH_HINT_IMAGE)) != 0) {
    pl_fatal("First thing given must be the base image to use");
  }

  // parse image id from first output line. We need to know which is the base
  // image id in order to start building
  base_image_id = line + strlen(PLASH_HINT_IMAGE);
  base_image_id = strdup(base_image_id);
  if (base_image_id == NULL)
    pl_fatal("strdup");
  image_id = base_image_id;

  // go trough all lines returned by plash eval
  while (!feof(eval_stdout)) {

    line = pl_nextline(eval_stdout);

    // This is an empty layer, skip it.
    if (line == NULL || (strcmp(line, PLASH_HINT_LAYER) == 0))
      continue;

    FILE *create_stdin =
        fmemopen(create_stdin_buf, sizeof(create_stdin_buf), "w");
    if (create_stdin == NULL)
      pl_fatal("fmemopen");

    // some extras before evaluating the build shell script
    fputs("PS4='--> '\n", create_stdin);

    // Hack for ubuntu, where for whatever reason PATH is not exported;
    fputs("export PATH\n", create_stdin);

    fputs("set -ex\n", create_stdin);

    //// redirect all lines from the eval subcommand to create subcommand. Stop
    // if a new layer is found
    fputs(line, create_stdin);
    fputs("\n", create_stdin);
    while ((line = pl_nextline(eval_stdout)) &&
           strcmp(line, PLASH_HINT_LAYER) != 0) {
      fputs(line, create_stdin);
      fputs("\n", create_stdin);
    }

    // flush stuff in create_stdin to appear in create_stdin_buf
    fflush(create_stdin);

    // get our new image_id where we can build our next layer on top of it.
    image_id = cached_call_plash_create(image_id, create_stdin_buf);
  }

  if (strcmp(base_image_id, image_id) == 0) {
    // This happens of "plash build -f 1" invocations. Let's just validate that
    // image is correct.
    pl_cmd(nodepath_main, image_id);
  }
  puts(image_id);
  return 0;
}
