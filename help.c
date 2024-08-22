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
      "    plash [recall] push:dir [ID] ARG          -  Export image to a directory\n"
      "    plash [recall] push:tarfile [ID] ARG      -  Export image to a file\n\n"
      "  Image Commands:\n"
      "    plash [recall] [cached] build [ID] CODE  -  Build a new image\n"
      "    plash [recall] mount [ID] MOUNTDIR        -  Mount image to the host "
      "filesystem\n"
      "    plash [recall] mounted [ID] [CMD ...]     -  Run command on a mounted "
      "image\n"
      "    plash [recall] nodepath [--allow-0] [ID]  -  Print filesystem path of an "
      "image\n"
      "    plash [recall] parent [ID]                -  Print the parents image\n"
      "    plash [recall] rm [ID]                    -  Remove image and its children\n"
      "    plash [recall] run [ID] ...               -  Run command in image\n"
      "    plash [recall] run:persist [ID] DIR ...   -  Run and persist fs changes at DIR\n"
      "    plash [recall] stack [ID] DIR             -  Create a new image specyfing "
      "its layer\n\n"
      "  Other Commands:\n"
      "    plash chroot DIR [CMD ...] -  Flavored chroot\n"
      "    plash clean                -  Remove internal unsused files\n"
      "    plash data                 -  Print application data path\n"
      "    plash help                 -  Print help message\n"
      "    plash init                 -  Initialize data dir\n"
      "    plash map KEY [ID]         -  Map lorem ipsum\n"
      "    plash mkdtemp              -  Create tempory data directory\n"
      "    plash purge                -  Remove all application data\n"
      "    plash shrink               -  Remove half of all images\n"
      "    plash sudo ...             -  Run program as 'userspace root'\n"
      "    plash version              -  Print version\n",
      stderr);
  return EXIT_SUCCESS;
}
