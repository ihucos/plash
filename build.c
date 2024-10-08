// Builds a new container from a command. If no command is passed, a shell is
// started. The new container is printed to stdout, all other output goes to
// stderr. Note that a new container is only returned if the build command
// returns 0 (success) as exit code.  For most cases use `plash build` for a
// higher level interface.
//
// Example:
//
// $ plash build 7 ./buildscript.sh
// 42
//
// $ sudo plash build 3
// /home/fulano # echo 'hello' > /file
// /home/fulano # exit 0
// 71
//
// $ plash b build -f ubuntu -- touch /myfile
// 44

#define USAGE "usage: plash build CONTAINER [ CMD1 [ CMD2 ... ] ]\n"

#define _GNU_SOURCE

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <plash.h>

int build_main(int argc, char *argv[]) {

  char *plash_data = plash("data");
  char *image_id = argv[1];
  if (image_id == NULL) {
    fputs(USAGE, stderr);
    return EXIT_FAILURE;
  }

  // validate image exists
  plash("nodepath", image_id);

  argv++; // chop argv[0]
  argv++; // chop image_id

  char *changesdir = plash("mkdtemp");

  pl_array_add("/proc/self/exe");
  pl_array_add("run:persist");
  pl_array_add(image_id);
  pl_array_add(changesdir);

  if (*argv) {

    while (*argv) {
      pl_array_add(*argv);
      argv++;
    }
  } else {
    pl_array_add("/bin/sh");
  }

  pid_t pid = fork();
  if (pid == 0) {

    // redirect stdout to stderr
    if (dup2(STDERR_FILENO, STDOUT_FILENO) == -1)
      pl_fatal("dup2");

    pl_array_add(NULL);

    // exec away
    execvp(pl_array[0], pl_array);

    pl_fatal("execvp");


  }

  int status;
  if (waitpid(pid, &status, 0) < 0)
    pl_fatal("waitpid");

  if (!WIFEXITED(status))
    pl_fatal("plash run:persist subprocess exited abnormally");
  int exit = WEXITSTATUS(status);
  if (exit != 0) {
    errno = 0;
    pl_fatal("build failed with exit status %d", exit);
  }

  char *changesdir_data = NULL;
  asprintf(&changesdir_data, "%s/data", changesdir) != -1 ||
      pl_fatal("asprintf");
  execlp("/proc/self/exe", "plash", "stack", image_id, changesdir_data, NULL);
  pl_fatal("execlp");
}
