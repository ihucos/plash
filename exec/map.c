// usage: plash map KEY [ CONTAINER ]
// Map a container to a key. Use an empty container to delete a key.
//
// Example:
//
// $ plash build -f alpine
// 342
//
// $ plash map myfavorite 342
//
// $ plash map myfavorite
// 342
//
// $ plash build --from-map myfavorite
// 342
//
// $ plash map myfavorite ''
//
// $ plash map myfavorite
// $

#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <plash.h>

char *plash_data;

void get(char const *linkpath) {
  char *nodepath;
  nodepath = realpath(linkpath, NULL);
  if (nodepath == NULL) {
    if (errno == ENOENT)
      return;
    pl_fatal("realpath");
  }
  puts(basename(nodepath));
}

void del(char const *linkpath) {
  if (unlink(linkpath) == -1) {
    if (errno == ENOENT)
      return;
    pl_fatal("unlink");
  }
}

void set(char const *linkpath, char *container_id) {
  char *nodepath;

  nodepath = pl_call("nodepath", container_id);

  if (chdir(pl_call("mkdtemp")) == -1)
    pl_fatal("chdir");
  if (asprintf(&nodepath, "..%s", nodepath + strlen(plash_data)) == -1)
    pl_fatal("asprintf");
  if (symlink(nodepath, "link") == -1)
    pl_fatal("symlink");
  if (rename("link", linkpath) == -1)
    pl_fatal("rename");
}

int main(int argc, char *argv[]) {

  char *linkpath;

  if (argc < 2) {
    pl_usage();
  }

  plash_data = pl_call("data");
  assert(plash_data);
  assert(plash_data[0] == '/');

  // validate map key
  if (!argv[1][0])
    pl_fatal("empty map name not allowed");
  else if (strchr(argv[1], '/') != NULL)
    pl_fatal("'/' not allowed in map name");

  // the location of the symlink for this map key
  if (asprintf(&linkpath, "%s/map/%s", plash_data, argv[1]) == -1)
    pl_fatal("asprintf");

  if (argc == 2) {
    get(linkpath);
  } else if (argc == 3 && !argv[2][0]) {
    del(linkpath);
  } else if (argc == 3) {
    set(linkpath, argv[2]);
  } else
    pl_usage();
}
