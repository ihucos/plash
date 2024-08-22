// Prints all commands

#define USAGE "usage: plash help\n"

#include <stdio.h>
#include <stdlib.h>

int help_main(int argc, char *argv[]) {
  fputs(
      "Build and run layered root filesystems.\n\n"
      "USAGE:\n\n"
      "  Import Image:\n"
      "    plash [cached] pull:docker IMAGE[:TAG]  -  Pull image from docker cli\n"
      "    plash [cached] pull:lxc DISTRO:VERSION  -  Download image from "
      "images.linuxcontainers.org\n"
      "    plash [cached] pull:tarfile ARG         -  Import the image from an file\n"
      "    plash [cached] pull:url ARG             -  Download image from an url\n\n"
      "  Export Image:\n"
      "    plash [noid] push:dir [ID] ARG          -  Export image to a directory\n"
      "    plash [noid] push:tarfile [ID] ARG      -  Export image to a file\n\n"
      "  Image Commands:\n"
      "    plash [noid] [cached] create [ID] CODE  -  Create a new image\n"
      "    plash [noid] mount [ID] MOUNTDIR        -  Mount image to the host "
      "filesystem\n"
      "    plash [noid] mounted [ID] [CMD ...]     -  Run command on a mounted "
      "image\n"
      "    plash [noid] nodepath [--allow-0] [ID]  -  Print filesystem path of an "
      "image\n"
      "    plash [noid] parent [ID]                -  Print the parents image\n"
      "    plash [noid] rm [ID]                    -  Remove image and its children\n"
      "    plash [noid] run [ID] ...               -  Run command in image\n"
      "    plash [noid] run:persist [ID] DIR ...   -  Run and persist fs changes at DIR\n"
      "    plash [noid] stack [ID] DIR             -  Create a new image specyfing "
      "its layer\n\n"
      "  Other Commands:\n"
      "    plash chroot DIR [CMD ...] -  Flavored chroot\n"
      "    plash clean                -  Remove internal unsused files\n"
      "    plash data                 -  Print application data path\n"
      "    plash help                 -  print help message\n"
      "    plash init                 -  initialize data dir\n"
      "    plash map KEY [ID]         -  map lorem ipsum\n"
      "    plash mkdtemp              -  Create tempory data directory\n"
      "    plash purge                -  Remove all application data\n"
      "    plash shrink               -  Remove half of all images\n"
      "    plash sudo ...             -  run program as 'userspace root'\n"
      "    plash version              -  print version\n",
      stderr);
  return EXIT_SUCCESS;
}
