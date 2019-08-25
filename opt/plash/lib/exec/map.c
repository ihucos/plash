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


#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <plash.h>
#include <stdio.h>
#include <libgen.h>
#include <errno.h>
#include <string.h>


int main(int argc, char* argv[]) {

  char *plash_data;
  char *nodepath;
  char *tmpdir;
  char *templ;

  plash_data = getenv("PLASH_DATA"); // XXX security must be normalized with realpath
  assert(plash_data);
  assert(plash_data[0] == '/');

  // validate map name
  if (!argv[1][0])
    pl_fatal("empty map name not allowed");
  else if (strchr(argv[1], '/') != NULL)
    pl_fatal("'/' not allowed in map name");

  if (  chdir(plash_data)  == -1   ) pl_fatal("chdir");
  if (  chdir("map")       == -1   ) pl_fatal("chdir");

  

  if (argc == 2){
    nodepath = realpath(argv[1], NULL);
    if (!nodepath && errno == ENOENT) {return 0;}
    if (!nodepath) pl_fatal("realpath");
    puts(basename(nodepath));
  }

  else if (argc == 3 && !argv[2][0]){
    if (unlink(argv[1]) == -1){
      if (errno == ENOENT) {return 0;}
      pl_fatal("unlink");
    }
  } else if (argc == 3){

      nodepath = pl_check_output((char*[]){
        "plash", "nodepath", argv[2], NULL});

      if (asprintf(
                   &templ,
                   "%s/tmp/plashtmp_%d_%d_XXXXXX",
                   plash_data, getsid(0), getpid) == -1)
        pl_fatal("asprintf");
      if ((tmpdir = mkdtemp(templ)) == NULL) pl_fatal("mkdtemp");
      if (chdir(tmpdir) == -1) pl_fatal("chdir");
      nodepath = nodepath + strlen(plash_data) - 2;
      nodepath[0] = '.';
      nodepath[1] = '.';
      if (symlink(nodepath, "link") == -1) pl_fatal("symlink");
      char *dst;
      if (asprintf(&dst, "../../map/%s", argv[1]) == -1)
        pl_fatal("asprintf");
      if (rename("link", dst) == -1) pl_fatal("rename");

  }

}

//import os
//import sys
//from os.path import join
//
//from plash.utils import (assert_initialized, catch_and_die, die,
//                         die_with_usage,
//                         mkdtemp, nodepath_or_die)
//
//assert_initialized()
//
//plash_data = os.environ["PLASH_DATA"]
//
//try:
//    key = sys.argv[1]
//except IndexError:
//    die_with_usage()
//if os.sep in key:
//    die('map can not contain any {}'.format(repr(os.sep)))
//
//try:
//    value = sys.argv[2]
//except IndexError:
//    value = None
//
//
//def map_set(map_key, container_id):
//    nodepath = nodepath_or_die(container_id)
//    with catch_and_die([OSError], debug='mkdtemp'):
//        tmpdir = mkdtemp()
//        dst = join(tmpdir, 'link')
//        src = os.path.relpath(nodepath, join(plash_data, 'map'))
//        os.symlink(src, dst)
//
//        # rename will overwrite atomically the map key if it already exists,
//        # just symlink would not
//        os.rename(
//            join(tmpdir, 'link'), join(plash_data, 'map', map_key))
//
//
//def map_rm(map_key):
//    with catch_and_die([OSError], debug='unlink'):
//        try:
//            os.unlink(join(plash_data, 'map', map_key))
//            return True
//        except FileNotFoundError:
//            return False
//
//
//def map_get(map_key):
//    try:
//        nodepath = os.path.realpath(join(plash_data, 'map', map_key))
//    except FileNotFoundError:
//        return
//    if os.path.exists(nodepath):
//        return os.path.basename(nodepath)
//
//
//if value == '':
//    map_rm(key)
//elif value is None:
//    cont = map_get(key)
//    if cont:
//        print(cont)
//else:
//    map_set(key, value)
