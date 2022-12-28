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
// $ plash data ls
// id_counter  index  layer  map  mnt  tmp

#define _GNU_SOURCE

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <plash.h>

char *get_home_dir() {
  struct passwd *pwd;
  char *home_env;
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
    if (asprintf(&plash_data, "%s/.plashdata", get_home_dir()) == -1)
      pl_fatal("asprintf");
  }
  return plash_data;
}

int main(int argc, char *argv[]) {
  char *plash_data = get_plash_data();

  // if no arguments, print the plash data directory
  if (argc == 1) {
    puts(plash_data);
    return 0;
  }

  // if arguments given, cd to plash data and exec the arguments
  if (chdir(plash_data) == -1)
    pl_fatal("chdir %s", plash_data);
  argv++;
  pl_unshare_user(); // because some files here might be from a subuid (different uid)
  execvp(argv[0], argv);
  pl_fatal("exec %s", argv[0]);
}
