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

void wait_for_plash_create(pid_t pid) {
  int status;
  waitpid(pid, &status, 0);
  if (!WIFEXITED(status)) {
    pl_fatal("plash create subprocess exited abornmally");
  }
  int exit_status = WEXITSTATUS(status);
  if (exit_status != 0)
    exit(1);
}

int main(int argc, char *argv[]) {
  char *image_id;
  char *line;

  // create args for subprocess
  char *args[argc + 1];
  size_t i = 0;
  args[i++] = "plash";
  args[i++] = "eval";
  argv++;
  while (*argv)
    args[i++] = *(argv++);
  args[i++] = NULL;

  FILE *eval_out;
  FILE *eval_err;
  pid_t eval_pid = spawn_process(args, NULL, &eval_out, &eval_err);

  // read first
  line = nextline(eval_out);

  // parse image id from first output line. We need to know with which base
  // image id to start.
  if (line != NULL &&
      strncmp(line, PLASH_HINT_IMAGE, strlen(PLASH_HINT_IMAGE)) == 0) {
    image_id = line + strlen(PLASH_HINT_IMAGE);
    image_id[strcspn(image_id, "\n")] = '\0';
  } else {
    pl_fatal("First thing given must be an image id");
  }

  FILE *create_in, *create_out;
  while (!feof(eval_out)) {

    pid_t create_pid =
        spawn_process((char *[]){"plash", "create", image_id, "sh", NULL},
                      &create_in, &create_out, NULL);

    // some extras before evaluating build shell script
    fprintf(create_in, "PS4='--> '\n");
    fprintf(create_in, "set -ex\n");

    // pipe lines from eval subcommand to create subcommand
    while ((line = nextline(eval_out))) {
      if ((strcmp(line, PLASH_HINT_LAYER "\n") == 0))
        break;
      fprintf(create_in, "%s", line);
    }

    // we are done with this layer, close the plash create and gets its created
    // image id to use for the next layer.
    fprintf(stderr, "--:\n");
    fclose(create_in);

    wait_for_plash_create(create_pid);

    image_id = nextline(create_out);
    image_id[strcspn(image_id, "\n")] = '\0';
    fclose(create_out);
  }

  int status;
  waitpid(eval_pid, &status, 0);
  if (!WIFEXITED(status)) {
    pl_fatal("subprocess exited abornmally");
  }
  if (WEXITSTATUS(status) != 0) {
    int has_err_output = 0;
    char *errline;
    while ((errline = nextline(eval_err))){
      has_err_output = 1;
      fprintf(stderr, "%s", errline);
    }
    if (!has_err_output)
      pl_fatal("Plash eval exited badly providing no error message to stderr");
    exit(1);
  }

  puts(image_id);
}
