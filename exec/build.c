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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <plash.h>

#include <sys/wait.h>
#include <unistd.h>

#define PLASH_HINT_IMAGE "### plash hint: image="
#define PLASH_HINT_LAYER "### plash hint: layer"

int spawn_process(char **cmd, FILE **p_stdin, FILE **p_stdout,
                  FILE **p_stderr) {
  int fd_stdin[2], fd_stdout[2], fd_stderr[2];
  pid_t pid;

  if (p_stdin != NULL && pipe(fd_stdin) != 0)
    pl_fatal("pipe");

  if (p_stdout != NULL && pipe(fd_stdout) != 0)
    pl_fatal("pipe");

  if (p_stdout != NULL && pipe(fd_stderr) != 0)
    pl_fatal("pipe");

  pid = fork();
  if (pid == -1)
    pl_fatal("fork");

  if (pid == 0) {

    if (p_stdin != NULL) {
      dup2(fd_stdin[0], STDIN_FILENO);
      close(fd_stdin[0]);
      close(fd_stdin[1]);
    }

    if (p_stdout != NULL) {
      dup2(fd_stdout[1], STDOUT_FILENO);
      close(fd_stdout[0]);
      close(fd_stdout[1]);
    }

    if (p_stderr != NULL) {
      dup2(fd_stderr[1], STDERR_FILENO);
      close(fd_stderr[0]);
      close(fd_stderr[1]);
    }

    execvp(cmd[0], cmd);
    exit(1);
  } else {

    if (p_stdin != NULL) {
      close(fd_stdin[0]);
      *p_stdin = fdopen(fd_stdin[1], "w");
      if (*p_stdin == NULL)
        pl_fatal("fdopen");
    }

    if (p_stdout != NULL) {
      close(fd_stdout[1]);
      *p_stdout = fdopen(fd_stdout[0], "r");
      if (*p_stdout == NULL)
        pl_fatal("fdopen");
    }

    if (p_stderr != NULL) {
      close(fd_stderr[1]);
      *p_stderr = fdopen(fd_stderr[0], "r");
      if (*p_stderr == NULL)
        pl_fatal("fdopen");
    }
  }

  return pid;
}

char *nextline(FILE *fh) {
  static char *line = NULL;
  static size_t len = 0;
  static ssize_t read;
  read = getline(&line, &len, fh);
  if (read == -1) {
    if (ferror(fh))
      pl_fatal("getline");
    else if (feof(fh))
      return NULL;
  }
  return line;
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
    while ((errline = nextline(err))) {
      has_err_output = 1;
      fprintf(stderr, "%s", errline);
    }
    if (!has_err_output)
      pl_fatal("Plash eval exited badly providing no error message to stderr");
    exit(1);
  }
}

int main(int argc, char *argv[]) {
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
  pid_t eval_pid = spawn_process(args, NULL, &eval_stdout, &eval_stderr);

  // read first line
  line = nextline(eval_stdout);

  // parse image id from first output line. We need to know which is the base
  // image id in order to start building
  if (line != NULL &&
      strncmp(line, PLASH_HINT_IMAGE, strlen(PLASH_HINT_IMAGE)) == 0) {
    image_id = line + strlen(PLASH_HINT_IMAGE);
    image_id[strcspn(image_id, "\n")] = '\0';
    image_id = strdup(image_id);
    if (image_id == NULL)
      pl_fatal("strdup");
  } else {
    // maybe plash eval exited badly with an error message. This invocation
    // ensures the user sees that error message.
    handle_plash_eval_exit(eval_pid, eval_stderr);

    pl_fatal("First thing given must be the base image to use");
  }

  while (!feof(eval_stdout)) {

    line = nextline(eval_stdout);

    // This is an empty layer, skip it.
    if (line == NULL || (strcmp(line, PLASH_HINT_LAYER "\n") == 0))
      continue;

    // run plash create to create this layer
    FILE *create_stdin, *create_stdout;
    pid_t create_pid =
        spawn_process((char *[]){"plash", "create", image_id, "sh", NULL},
                      &create_stdin, &create_stdout, NULL);

    // some extras before evaluating the build shell script
    fprintf(create_stdin, "PS4='--> '\n");

    // Hack for ubuntu, where for whatever reason PATH is not exported;
    fprintf(create_stdin, "export PATH\n");

    fprintf(create_stdin, "set -ex\n");

    //// pipe all lines from the eval subcommand to create subcommand
    fputs(line, create_stdin);
    while ((line = nextline(eval_stdout)) &&
           strcmp(line, PLASH_HINT_LAYER "\n") != 0)
      fputs(line, create_stdin);

    // we are done with this layer, close the plash create and gets its
    // created image id to use for the next layer.
    fclose(create_stdin);
    handle_plash_create_exit(create_pid);
    image_id = nextline(create_stdout);
    image_id[strcspn(image_id, "\n")] = '\0';
    image_id = strdup(image_id);
    if (image_id == NULL)
      pl_fatal("strdup");
    fclose(create_stdout);
    fprintf(stderr, "---\n");
  }

  handle_plash_eval_exit(eval_pid, eval_stderr);

  puts(image_id);
}
