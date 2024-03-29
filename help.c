// Prints all commands

#define USAGE "usage: plash help\n"

#include <stdio.h>
#include <stdlib.h>

int help_main(int argc, char *argv[]) {
  fputs(
      "Build and run layered root filesystems.\n\n"
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
      "    [noid] run [ID] ...               -  Run command in image\n"
      "    [noid] run:persist [ID] DIR ...   -  Run and persist fs changes at DIR\n"
      "    [noid] stack [ID] DIR             -  Create a new image specyfing "
      "its layer\n\n"
      "  Other Commands:\n"
      "    chroot DIR [CMD ...] -  Flavored chroot\n"
      "    clean                -  Remove internal unsused files\n"
      "    data                 -  Print application data path\n"
      "    help                 -  print help message\n"
      "    init                 -  initialize data dir\n"
      "    map KEY [ID]         -  map lorem ipsum\n"
      "    mkdtemp              -  Create tempory data directory\n"
      "    purge                -  Remove all application data\n"
      "    shrink               -  Remove half of all images\n"
      "    sudo ...             -  run program as 'userspace root'\n"
      "    version              -  print version\n",
      stderr);
  return EXIT_SUCCESS;
}
