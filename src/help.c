// Prints all commands

#define USAGE "usage: plash help\n"

#include <stdio.h>
#include <stdlib.h>

#define HELP                                                                   \
  "plash --help        Alias for `plash help`\n"                               \
  "plash --help-macros Alias for `plash help-macros`\n"                        \
  "plash --version     Alias for `plash version`\n"                            \
  "plash -h            Alias for `plash help`\n"                               \
  "plash add-layer     Stack a layer on top of a container\n"                  \
  "plash build         Builds an image\n"                                      \
  "plash clean         Cleans up plashs internal data\n"                       \
  "plash copy          Copy the container's root filesystem to directory\n"    \
  "plash create        Creates a new container from a command\n"               \
  "plash data          Prints the location of the build data\n"                \
  "plash eval          Generates a build script\n"                             \
  "plash export-tar    Export container as tar archive\n"                      \
  "plash help          Prints help\n"                                          \
  "plash help-macros   Lists all available macros\n"                           \
  "plash import-docker Import image from local docker instance into\n"         \
  "plash import-lxc    Import an image from https://images\n"                  \
  "plash import-tar    Create a container from a tar file\n"                   \
  "plash import-url    Import a container from an url\n"                       \
  "plash init          Initialize build data\n"                                \
  "plash map           Map a container to a key\n"                             \
  "plash mkdtemp       Create a temporary directory in the plash data\n"       \
  "plash mount         Mount a container-filesystem\n"                         \
  "plash nodepath      Prints the path to a given container\n"                 \
  "plash parent        Prints the containers parent container\n"               \
  "plash purge         Deletes all build data unatomically\n"                  \
  "plash rm            Deletes the given image atomically\n"                   \
  "plash run           Run a container\n"                                      \
  "plash runb          Run an image in the build environment\n"                \
  "plash shrink        Delete half of the older containers\n"                  \
  "plash sudo          Setup a Linux user namespace\n"                         \
  "plash test          Run unit tests\n"                                       \
  "plash version       Prints the version number\n"                            \
  "plash with-mount    Execute parameters inside a mounted container\n"        \
  "plash -*            Fallback to `plash b run`\n" \
  "plash */*           Execute subcommand as file (for shebangs)\n"

int help_main(int argc, char *argv[]) {
  fputs(HELP, stderr);
  return EXIT_SUCCESS;
}
