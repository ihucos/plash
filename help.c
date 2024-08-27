// Prints all commands

#define USAGE "usage: plash help\n"

#include <stdio.h>
#include <stdlib.h>

int help_main(int argc, char *argv[]) {
  fputs(
      "Build and run layered root filesystems.\n\n"
      "USAGE:\n\n"
      "  Import Image:\n"
      "    plash pull:docker IMAGE[:TAG]  -  Pull image from docker cli\n"
      "    plash pull:lxc DISTRO:VERSION  -  Download image from "
      "images.linuxcontainers.org\n"
      "    plash pull:tarfile ARG         -  Import the image from an file\n"
      "    plash pull:url ARG             -  Download image from an url\n\n"
      "  Export Image:\n"
      "    plash push:dir ID ARG          -  Export image to a directory\n"
      "    plash push:tarfile ID ARG      -  Export image to a file\n\n"
      "  Image Commands:\n"
      "    plash build ID [CMD ...]       -  Build a new image\n"
      "    plash mount ID MOUNTDIR        -  Mount image to the host "
      "filesystem\n"
      "    plash mounted ID [CMD ...]     -  Run command on a mounted "
      "image\n"
      "    plash nodepath [--allow-0] ID  -  Print filesystem path of an "
      "image\n"
      "    plash parent ID                -  Print the parents image\n"
      "    plash rm ID                    -  Remove image and its children\n"
      "    plash run ID ...               -  Run command in image\n"
      "    plash do PLASH_CMD ...         -  Apply `recall` and `cache` automatically\n"
      "    plash cache PLASH_CMD ...      -  Cache image id output by argv\n"
      "    plash run:persist ID DIR ...   -  Run and persist fs changes at DIR\n"
      "    plash stack ID DIR             -  Create a new image specyfing "
      "its layer\n"
      "    plash check ID PATH            -  Invalidate ongoing caches if PATH changed.\n\n"
      "  Other Commands:\n"
      "    plash recall PLASHCMD *ARGS    -  Save returned ID and/or reuse last saved ID\n"
      "    plash chroot DIR [CMD ...]     -  Flavored chroot\n"
      "    plash clean                    -  Remove internal unsused files\n"
      "    plash data                     -  Print application data path\n"
      "    plash help                     -  Print help message\n"
      "    plash init                     -  Initialize data dir\n"
      "    plash map KEY [ID]             -  Map lorem ipsum\n"
      "    plash mkdtemp                  -  Create tempory data directory\n"
      "    plash purge                    -  Remove all application data\n"
      "    plash shrink                   -  Remove half of all images\n"
      "    plash sudo ...                 -  Run program as 'userspace root'\n"
      "    plash version                  -  Print version\n",
      stderr);
  return EXIT_SUCCESS;
}
