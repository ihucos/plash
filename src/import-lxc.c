// Import an image from https://images.linuxcontainers.org/
// if --dry is set, the image url is printed but not imported

#define USAGE "usage: plash import-lxc DISTRIBUTION:RELEASE [--dry]\n"

#define HOME_URL "https://images.linuxcontainers.org"
#define INDEX_URL "https://images.linuxcontainers.org/meta/1.0/index-user"
#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <stdlib.h>

#include <plash.h>

char *getarch() {
  struct utsname unameData;
  uname(&unameData);
  if ((strcmp(unameData.machine, "x86_64")) == 0) {
    return "amd64";
  } else if ((strcmp(unameData.machine, "aarch64")) == 0) {
    return "arm64";
  }
  char *dup = strdup(unameData.machine);
  if (dup == NULL)
    pl_fatal("strdup");
  return dup;
}

int import_lxc_main(int argc, char *argv[]) {

  char *prefix, *url_part, *rootfs_url, *resp, *image_name, *distro, *version,
      *line;

  // read args
  if (argc < 2) {
    fputs(USAGE, stderr);
    return EXIT_FAILURE;
  }
  image_name = argv[1];
  int dry = argv[2] && strcmp(argv[2], "--dry") == 0;

  // split the first arg
  distro = strtok(image_name, ":");
  version = strtok(NULL, ":");
  if (version == NULL)
    pl_fatal("also specify the release, example: debian:sid");

  resp = pl_check_output((char *[]){"curl", "--progress-bar", "--fail",
                                    "--location", INDEX_URL, NULL});
  asprintf(&prefix, "%s;%s;%s;default;", distro, version, getarch()) != -1 ||
      pl_fatal("asprintf");

  line = strtok(resp, "\n");
  while (line != NULL) {
    if (strncmp(line, prefix, strlen(prefix)) == 0) {
      url_part = line + strlen(prefix);   // rewind away from our found prefix
      url_part += strcspn(url_part, ";"); // go to the next ";"
      url_part += 1;                      // chop that last ";"
      asprintf(&rootfs_url, HOME_URL "%srootfs.tar.xz", url_part) != -1 ||
          pl_fatal("asprintf");

      // we got the root file system process with it how the user requested
      if (dry) {
        puts(rootfs_url);
        return EXIT_SUCCESS;
      } else {
        execlp("/proc/self/exe", "plash", "import-url", rootfs_url, NULL);
        pl_fatal("execlp");
      }
    }

    line = strtok(NULL, "\n");
  }

  errno = 0;
  pl_fatal("%s:%s not listed in " HOME_URL "/", distro, version);
}
