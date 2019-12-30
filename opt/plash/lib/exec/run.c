// usage: plash run CONTAINER [ CMD1 [ CMD2 ... ] ]
//
// Run a container. If no command is specified, the containers default root
// shell is executed.
//
// The following host file systems are mapped to the container:
// - /tmp
// - /home
// - /root
// - /etc/resolv.conf
// - /sys
// - /dev
// - /proc
//
// If you want more control about how the container is run, use `plash runopts`
//
// Parameters may be interpreted as build instruction.

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <plash.h>

size_t count_array(char **array) {
  size_t count = 0;
  while (array[++count] != NULL)
    ;
  return count;
}

int main(int argc, char **argv) {

  if (argc < 2) {
    pl_usage();
  }

  char *container_id = argv[1];
  char *changesdir = pl_mkdtemp();

  char **cmd_prefix = (char *[]){"runopts",
                                 "-c",
                                 container_id,
                                 "-d",
                                 changesdir,
                                 "-m",
                                 "/tmp",
                                 "-m",
                                 "/home",
                                 "-m",
                                 "/root",
                                 "-m",
                                 "/etc/resolv.conf",
                                 "-m",
                                 "/sys",
                                 "-m",
                                 "/dev",
                                 "-m",
                                 "/proc",
                                 "--",
                                 NULL};

  char **cmd_wrapper = (char *[]){"sh", "-lc", "exec env \"$@\"", "--", NULL};

  char **exec_argv = malloc((count_array(cmd_prefix) +
                             count_array(cmd_wrapper) + count_array(argv) + 1) *
                            sizeof(char *));

  size_t index = 0;

  for (; *cmd_prefix; cmd_prefix++)
    exec_argv[index++] = *cmd_prefix;

  if (argv[2]) // if any command specified
    for (; *cmd_wrapper; cmd_wrapper++)
      exec_argv[index++] = *cmd_wrapper;

  argv++;
  argv++;
  while (exec_argv[index++] = *(argv++))
    ;

  execv(pl_path("runopts"), exec_argv);
  pl_fatal("exec");
}
