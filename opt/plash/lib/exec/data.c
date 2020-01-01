//!/usr/bin/env python3
// usage: plash data [CMD1 [CMD2 ...]]
//
// Prints the location of the build data.
// if arguments are given, these arguments will be execed insite the build data.
//
//
// Example:
//
// $ plash data
// /home/myuser/.plashdata
//
// $ plash data cat config/union_taste
// unionfs-fuse

#define _GNU_SOURCE

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <plash.h>

char *get_home() {
  char *home_env;
  struct passwd *pwd;
  if (!(home_env = getenv("HOME"))) {
    pwd = getpwuid(getuid());
    if (!pwd)
      pl_fatal("could not determine your home directory");
    return pwd->pw_dir;
  }
  return home_env;
}

char *get_plash_data() {
  char *plash_data;
  if (!(plash_data = getenv("PLASH_DATA"))) {
    if (asprintf(&plash_data, "%s/.plashdata", get_home()) == -1)
      pl_fatal("asprintf");
  }
  return plash_data;
}

int main(int argc, char *argv[]) {
  char *plash_data = get_plash_data();

  if (argc == 1) {
    puts(plash_data);
    return 0;
  }

  if (chdir(plash_data) == -1)
    pl_fatal("chdir %s", plash_data);
  argv++;
  execvp(argv[0], argv);
  pl_fatal("exec %s", argv[0]);
}
