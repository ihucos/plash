// Prints all commands

#define USAGE "usage: plash help\n"

#include <stdio.h>
#include <stdlib.h>

int help_main(int argc, char *argv[]) {
  fputs(
      "USAGE: plash ...\n\n"
      "  Import Image:\n"
      "    [cached] pull:docker IMAGE[:TAG]  -  Pull image from docker cli\n"
      "    [cached] pull:lxc DISTRO:VERSION  -  Download image from "
      "images.linuxcontainers.org\n"
      "    [cached] pull:tarfile ARG         -  Import the image from an file\n"
      "    [cached] pull:url ARG             -  Download image from an url\n\n"
      "  Export Image:\n"
      "    [noid] push:dir [ID] ARG          -  Export image to a directory\n"
      "    [noid] push:tarfile [ID] ARG      -  Export image to a file\n\n"
      "  Image Commands:\n"
      "    [noid] [cached] create [ID] CODE  -  Create a new image\n"
      "    [noid] mount [ID] MOUNTDIR        -  Mount image to the host "
      "filesystem\n"
      "    [noid] mounted [ID] [CMD ...]     -  Run command on a mounted "
      "image\n"
      "    [noid] nodepath [--allow-0] [ID]  -  Print filesystem path of an "
      "image\n"
      "    [noid] parent [ID]                -  Print the parents image\n"
      "    [noid] rm [ID]                    -  Remove image and its children\n"
      "    [noid] run [ID] [CMD ...]         -  Run command in image\n"
      "    [noid] stack [ID] DIR             -  Create a new image specyfing "
      "its layer\n\n"
      "  Other Commands:\n"
      "    clean         -  Remove internal unsused files\n"
      "    mkdtemp       -  Create tempory data directory\n"
      "    data          -  Print application data path\n"
      "    purge         -  Remove all application data\n"
      "    shrink        -  Remove half of all images\n"
      "    help          -  print help message\n"
      "    map KEY [ID]  -  map lorem ipsum\n"
      "    sudo ...      -  run program as 'userspace root'\n"
      "    version       -  print version\n"
      "    init          -  initialize data dir\n",
      stderr);
  return EXIT_SUCCESS;
}
