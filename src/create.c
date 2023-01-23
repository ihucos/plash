// Creates a new container from a command. If no command is passed, a shell is
// started. The new container is printed to stdout, all other output goes to
// stderr. Note that a new container is only returned if the build command
// returns 0 (success) as exit code.  For most cases use `plash build` for a
// higher level interface.
//
// Example:
//
// $ plash create 7 ./buildscript.sh
// 42
//
// $ sudo plash create 3
// /home/fulano # echo 'hello' > /file
// /home/fulano # exit 0
// 71
//
// $ plash b create -f ubuntu -- touch /myfile
// 44

#define USAGE "usage: plash create CONTAINER [ CMD1 [ CMD2 ... ] ]\n"

#define _GNU_SOURCE

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <utils.h>

int create_main(int argc, char *argv[]) {

  char *plash_data = pl_call("data");
  char *image_id = argv[1];
  if (image_id == NULL) {
    fputs(USAGE, stderr);
    return 1;
  }

  // validate image exists
  pl_call("nodepath", image_id);

  argv++; // chop argv[0]
  argv++; // chop image_id

  char *changesdir = pl_call("mkdtemp");

  pl_exec_add("plash");
  pl_exec_add("runb");
  pl_exec_add(image_id);
  pl_exec_add(changesdir);

  if (*argv) {

    // Use login shell
    pl_exec_add("/bin/sh");
    pl_exec_add("-lc");
    pl_exec_add("exec env \"$@\"");
    pl_exec_add("--");

    while (*argv) {
      pl_exec_add(*argv);
      argv++;
    }
  } else {
    pl_exec_add("/bin/sh");
  }

  pid_t pid = fork();
  if (pid == 0) {

    // redirect stdout to stderr
    if (dup2(STDERR_FILENO, STDOUT_FILENO) == -1)
      pl_fatal("dup2");

    // exec away
    pl_exec_add(NULL);
  }

  int status;
  if (waitpid(pid, &status, 0) < 0)
    pl_fatal("waitpid");

  if (!WIFEXITED(status))
    pl_fatal("plash runb subprocess exited abnormally");
  int exit = WEXITSTATUS(status);
  if (exit != 0) {
    errno = 0;
    pl_fatal("build failed with exit status %d", exit);
  }

  char *changesdir_data = NULL;
  asprintf(&changesdir_data, "%s/data", changesdir) != -1 ||
      pl_fatal("asprintf");
  execlp("plash", "plash", "add-layer", image_id, changesdir_data, NULL);
  pl_fatal("execlp");
}
