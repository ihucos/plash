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

int main(int argc, char **argv) {

  if (argc < 2) {
    pl_usage();
  }

  char *container_id = argv[1];
  char *changesdir = pl_mkdtemp();

  char *cmd_prefix[] = {"runopts",
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

  char *cmd_wrapper[] = {"sh", "-lc", "exec env \"$@\"", "--", NULL};

  char **new_argv =
      malloc(sizeof(cmd_prefix) + sizeof(cmd_wrapper) + sizeof(char*)*(argc+1));
  size_t index = 0;
  char **c;

  for (c = cmd_prefix; *c; c++)
    new_argv[index++] = *c;
  if (argv[2]) // if any command specified
    for (c = cmd_wrapper; *c; c++)
      new_argv[index++] = *c;

  argv++;
  argv++;
  for (c = argv; *c; c++)
    new_argv[index++] = *c;
  new_argv[index++] = NULL;

  // size_t i = 0;
  // for (; new_argv[i]; i++) puts(new_argv[i]);

  execv(pl_path("runopts"), new_argv);
  pl_fatal("exec");
}
