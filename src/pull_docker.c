// Import image from local docker instance into.

#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <plash.h>

#define USAGE "usage: plash import-docker IMAGE\n"

void call_docker_pull(char *image) {
  int status, exit;
  pid_t pid = fork();
  if (pid == 0) {

    // redirect stdout to stderr
    if (dup2(STDERR_FILENO, STDOUT_FILENO) == -1)
      pl_fatal("dup2");

    execlp("docker", "docker", "pull", image, NULL);
    pl_fatal("execlp");
  }

  if (waitpid(pid, &status, 0) < 0)
    pl_fatal("waitpid");

  if (!WIFEXITED(status))
    pl_fatal("docker pull subprocess exited abnormally");
  exit = WEXITSTATUS(status);
  if (exit != 0) {
    pl_fatal("docker pull failed with exit status %d", exit);
  }
}

int pull_docker_main(int argc, char *argv[]) {
  if (argc < 2) {
    fputs(USAGE, stderr);
    return EXIT_FAILURE;
  }
  char *image = argv[1];
  call_docker_pull(image);
  char *container_id = pl_firstline(
      pl_check_output((char *[]){"docker", "create", image, "sh", NULL}));
  pl_pipe((char *[]){"docker", "export", container_id, NULL},
          (char *[]){"/proc/self/exe", "import-tar", NULL});
  return EXIT_SUCCESS;
}
