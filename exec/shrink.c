// usage: plash shrink
// Delete half of the older containers.
// Containers with a lower build id will be deleted first.

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

#define ITERDIR_END()                                                          \
  }                                                                            \
  closedir(dirp);

int is_leave(char *nodepath) {
  ITERDIR_BEGIN(nodepath)
  if (atoi(dir->d_name))
    return 0;
  ITERDIR_END();
  return 1;
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
  ITERDIR_END()
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
  ITERDIR_END()
  oldest_leave_dup = strdup(oldest_leave);
  if (oldest_leave_dup == NULL) pl_fatal("dup");
  return oldest_leave_dup;
}

int main(int argc, char *argv[]) {
  char *image_id;
  char *plash_data = pl_call("data");
  if (chdir(plash_data) == -1)
    pl_fatal("chdir %s");
  if (chdir("index") == -1)
    pl_fatal("chdir %s");

  int images_count = count_images();
  printf("You have %d images.\n", images_count);
  for (int i = 0; i <= (images_count / 2); i++) {
    char *o =  get_oldest_leave();
    if (o[0] == '0' && o[1] == '\0'){
      printf("Nothing to delete.\n");
      break;
    }
    printf("Deleting image id: %s\n", o);
    pl_call("rm", o);
  }
  printf("You have %d images.\n", count_images());
}
