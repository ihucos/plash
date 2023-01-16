// usage: plash build --macro1 ar1 arg2 --macro2 arg1 ...
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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <plash.h>

#include <sys/wait.h>
#include <unistd.h>

#define PLASH_HINT_IMAGE "### plash hint: image="
#define PLASH_HINT_LAYER "### plash hint: layer"

void plash_create_cache_wrapper(char **argv){



}

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

void handle_plash_eval_exit(pid_t pid, FILE *err) {
  int status;
  waitpid(pid, &status, 0);
  if (!WIFEXITED(status)) {
    pl_fatal("subprocess exited abornmally");
  }
  if (WEXITSTATUS(status) != 0) {
    int has_err_output = 0;
    char *errline;
    while ((errline = pl_nextline(err))) {
      has_err_output = 1;
      fputs(errline, stderr);
    }
    if (!has_err_output)
      pl_fatal("Plash eval exited badly providing no error message to stderr");
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


char * call_plash_create(char *image_id, char * shell_input){
    //// run plash create to create this layer
    FILE *create_stdin, *create_stdout;
    pid_t create_pid = pl_spawn_process(
        (char *[]){"plash", "create", image_id, "sh", NULL},
        &create_stdin, &create_stdout, NULL);
    fputs(shell_input, create_stdin);

     //we are done with this layer, close the plash create and gets its created
     //image id to use for the next layer.
    fclose(create_stdin);
    handle_plash_create_exit(create_pid);
    image_id = pl_nextline(create_stdout);
    image_id = strdup(image_id);
    if (image_id == NULL)
      pl_fatal("strdup");
    fclose(create_stdout);
    fputs("---\n", stderr);
    return image_id;
}


char * cached_call_plash_create(char *base_image_id, char * shell_input){
  char *image_id;
  char *cached_image_id;
  char *cache_key = NULL;
  asprintf(&cache_key, "layer:%s:%lu\n", base_image_id, hash(shell_input)) ||
      pl_fatal("asprintf");
  cached_image_id = pl_call("map", cache_key);
  if (strcmp(cached_image_id, "") != 0) {
    return cached_image_id;
  } else {
    image_id = call_plash_create(base_image_id, shell_input);
    pl_call("map", cache_key, image_id);
    return image_id;
  }
}

int main(int argc, char *argv[]) {
  char create_stdin_buf[1024];
  char *image_id;
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
  FILE *eval_stderr;
  pid_t eval_pid = pl_spawn_process(args, NULL, &eval_stdout, &eval_stderr);

  // read first line
  line = pl_nextline(eval_stdout);

  // First line must be the image id hint
  if (line == NULL ||
      strncmp(line, PLASH_HINT_IMAGE, strlen(PLASH_HINT_IMAGE)) != 0) {

    // maybe plash eval exited badly with an error message. This invocation
    // ensures the user sees that error message.
    handle_plash_eval_exit(eval_pid, eval_stderr);

    pl_fatal("First thing given must be the base image to use");
  }

  // parse image id from first output line. We need to know which is the base
  // image id in order to start building
  image_id = line + strlen(PLASH_HINT_IMAGE);
  image_id = strdup(image_id);
  if (image_id == NULL)
    pl_fatal("strdup");
  
  while (!feof(eval_stdout)) {

    line = pl_nextline(eval_stdout);

    // This is an empty layer, skip it.
    if (line == NULL || (strcmp(line, PLASH_HINT_LAYER) == 0))
      continue;

    FILE *create_stdin = fmemopen(create_stdin_buf, sizeof(create_stdin_buf), "w");
    if (create_stdin == NULL) pl_fatal("fmemopen");

    // some extras before evaluating the build shell script
    fputs("PS4='--> '\n", create_stdin);

    // Hack for ubuntu, where for whatever reason PATH is not exported;
    fputs("export PATH\n", create_stdin);

    fputs("set -ex\n", create_stdin);
   
    //// pipe all lines from the eval subcommand to create subcommand
    fputs(line, create_stdin);
    fputs("\n", create_stdin);
    while ((line = pl_nextline(eval_stdout)) &&
           strcmp(line, PLASH_HINT_LAYER) != 0) {
      fputs(line, create_stdin);
      fputs("\n", create_stdin);
    }

    fflush(create_stdin);
    image_id = cached_call_plash_create(image_id, create_stdin_buf);
  }

  handle_plash_eval_exit(eval_pid, eval_stderr);

  puts(image_id);
}
