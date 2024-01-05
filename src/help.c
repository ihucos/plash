// Prints all commands

#define USAGE "usage: plash help\n"

#include <stdio.h>
#include <stdlib.h>

#define HELP                                                                   \
  "USAGE: plash ...\n"                                                         \
  "  [cached] pull docker IMAGE[:TAG]  -  Pull image from docker cli\n"        \
  "  [cached] pull lxc DISTRO:VERSION  -  Download image from "                \
  "images.linuxcontainers.org\n"                                               \
  "  [cached] pull tarfile ARG         -  Import the image from an file\n"     \
  "  [cached] pull url ARG             -  Download image from an url\n"        \
  "  [noid] push [ID] dir ARG          -  Export image to a directory\n"       \
  "  [noid] push [ID] tarfile ARG      -  Export image to a file\n"            \
  "  [noid] [cached] create [ID] CODE  -  Create a new image\n"                \
  "  [noid] mount [ID] MOUNTDIR        -  Mount image to the host "            \
  "filesystem\n"                                                               \
  "  [noid] mounted [ID] [CMD ...]     -  Run command on a mounted image\n"    \
  "  [noid] nodepath [--allow-0] [ID]  -  Print filesystem path of an image\n" \
  "  [noid] parent [ID]                -  Print the parents image\n"           \
  "  [noid] rm [ID]                    -  Remove image and its children\n"     \
  "  [noid] run [ID] [CMD ...]         -  Run command in image\n"              \
  "  [noid] stack [ID] DIR             -  Create a new image specyfing its "   \
  "layer\n"                                                                    \
  "  clean                             -  Remove internal unsused files \n"    \
  "  mkdtemp                           -  Create tempory data directory \n"    \
  "  data                              -  Print application data path\n"       \
  "  purge                             -  Remove all application data\n"       \
  "  shrink                            -  Remove half of all images \n"        \
  "  help                              -  print help message\n"                \
  "  map KEY [ID]                      -  map lorem ipsum\n"                   \
  "  sudo ...                          -  run program as \"userspace root\"\n" \
  "  version                           -  print version\n" \
  "  init                           -  initialize data dir\n"

int help_main(int argc, char *argv[]) {
  fputs(HELP, stderr);
  return EXIT_SUCCESS;
}
