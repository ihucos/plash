// Delete half of the older containers.
// Containers with a lower build id will be deleted first.

#define USAGE "usage: plash shrink\n"

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <plash.h>

#define ITERDIR_BEGIN(path)                                                    \
  DIR *dirp;                                                                   \
  struct dirent *dir;                                                          \
  dirp = opendir(path);                                                        \
  if (dirp == NULL)                                                            \
    pl_fatal("opendir");                                                       \
  while ((dir = readdir(dirp)) != NULL) {

#define ITERDIR_CLOSE()                                                        \
  }                                                                            \
  closedir(dirp);

int is_leave(char *nodepath) {
  int is_leave = 1;
  ITERDIR_BEGIN(nodepath)
  if (atoi(dir->d_name)) {
    is_leave = 0;
    break;
  }
  ITERDIR_CLOSE();
  return is_leave;
}

int count_images() {
  int count = 0;
  ITERDIR_BEGIN(".")
  if (dir->d_type == DT_LNK) {
    char *nodepath = realpath(dir->d_name, NULL);
    if (nodepath == NULL && errno == ENOENT)
      continue;
    if (nodepath == NULL)
      pl_fatal("realpath");
    count++;
  }
  ITERDIR_CLOSE()
  return count;
}

char *get_oldest_leave() {
  char *oldest_leave = NULL;
  char *oldest_leave_dup;
  char *nodepath;
  ITERDIR_BEGIN(".")
  if ((dir->d_type != DT_LNK) ||                          // its' not a link
      ((nodepath = realpath(dir->d_name, NULL)) == NULL)) // or a broken link
    continue;
  if (is_leave(nodepath) &&
      (oldest_leave == NULL ||                 // this is the first item or
       atoi(oldest_leave) > atoi(dir->d_name)) // this item is even smaller
  )
    oldest_leave = dir->d_name;
  ITERDIR_CLOSE()
  oldest_leave_dup = strdup(oldest_leave);
  if (oldest_leave_dup == NULL)
    pl_fatal("dup");
  return oldest_leave_dup;
}

int shrink_main(int argc, char *argv[]) {
  char *image_id;
  char *plash_data = pl_cmd(data_main);
  if (chdir(plash_data) == -1)
    pl_fatal("chdir %s", plash_data);
  if (chdir("index") == -1) pl_fatal("chdir index");

  int images_count = count_images() - 1; // substract special root image
  if (!images_count) {
    fprintf(stderr, "You have no images\n");
    return 0;
  }
  fprintf(stderr, "You have %d images.\n", images_count);
  for (int to_delete = ((images_count + 1) / 2); to_delete; to_delete--) {
    char *o = get_oldest_leave();
    if (o[0] == '0' && o[1] == '\0') {
      fprintf(stderr, "Nothing to delete.\n");
      break;
    }
    fprintf(stderr, "Deleting image id: %s\n", o);
    pl_cmd(rm_main, o);
  }
  fprintf(stderr, "You have %d images.\n", count_images() - 1);
  return 0;
}
