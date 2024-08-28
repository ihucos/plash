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

#define USAGE "usage: plash map KEY [ IMAGE_ID ]\n"

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

char *get(char const *linkpath) {
  char *nodepath;
  nodepath = realpath(linkpath, NULL);
  if (nodepath == NULL) {
    if (errno == ENOENT)
      return NULL;
    pl_fatal("realpath");
  }
  return basename(nodepath);
}

char *del(char const *linkpath) {
  if (unlink(linkpath) == -1) {
    if (errno == ENOENT)
      return NULL;
    pl_fatal("unlink");
  }
  return NULL;
}

char *set(char const *linkpath, char *container_id) {
  char *nodepath;

  nodepath = nodepath_call(container_id, NULL);

  if (chdir(plash("mkdtemp")) == -1)
    pl_fatal("chdir");
  if (asprintf(&nodepath, "..%s", nodepath + strlen(plash_data)) == -1)
    pl_fatal("asprintf");
  if (symlink(nodepath, "link") == -1)
    pl_fatal("symlink");
  if (rename("link", linkpath) == -1)
    pl_fatal("rename");

  return NULL;
}

char *map_call(char *key, char *id) {

  char *linkpath;

  plash_data = data_call();
  assert(plash_data);
  assert(plash_data[0] == '/');

  // validate map key
  if (!key[0])
    pl_fatal("empty map name not allowed");
  else if (strchr(key, '/') != NULL)
    pl_fatal("'/' not allowed in map name");

  // the location of the symlink for this map key
  if (asprintf(&linkpath, "%s/map/%s", plash_data, key) == -1)
    pl_fatal("asprintf");

  if (id == NULL) {
    return get(linkpath);
  }
  if (!id[0]) {
    return del(linkpath);
    return EXIT_SUCCESS;
  }
  return set(linkpath, id);
}

int map_main(int argc, char *argv[]) {
  if (argc < 2) {
    fputs(USAGE, stderr);
    return EXIT_FAILURE;
  }
  char *ret = map_call(argv[1], argv[2]);
  if (ret != NULL) {
    puts(ret);
  }
  return EXIT_SUCCESS;
}
