// Cleans up plashs internal data.
// - Removes all broken links in $PLASH_DATA/index
// - Removes all broken links in $PLASH_DATA/map
// - Removes unused temporary directories in $PLASH_DATA/tmp

#define USAGE "usage: plash clean\n"

#include <dirent.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <plash.h>

size_t remove_broken_links_here() {
  size_t removed = 0;
  DIR *dir = opendir(".");
  if (dir == NULL)
    pl_fatal("opendir");
  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_LNK) {
      if (access(entry->d_name, F_OK) == -1) {
        if (unlink(entry->d_name) != -1)
          removed++;
      }
    }
  }
  closedir(dir);
  return removed;
}

int is_process_still_running(pid_t pid, pid_t sid) {
  if (getsid(pid) != sid) {
    return EXIT_SUCCESS;
  }
  if (kill(pid, 0) == 0)
    return EXIT_FAILURE;
  return EXIT_SUCCESS;
}

size_t delete_unused_tmpdirs_here() {
  size_t count = 0;
  char *dirname_copy, *pid, *sid;

  DIR *dir = opendir(".");
  if (dir == NULL)
    pl_fatal("opendir");
  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_DIR) {

      // parse sid and pid from the dir name
      if ((dirname_copy = strdup(entry->d_name)) == NULL)
        pl_fatal("strdup");
      strtok(dirname_copy, "_");
      if ((pid = strtok(NULL, "_")) == NULL)
        continue;
      if ((sid = strtok(NULL, "_")) == NULL)
        continue;

      // delete temporary directory if the process that created it already died.
      if (!is_process_still_running(atoll(pid), atoll(sid))) {
        pl_run("rm", "-rf", entry->d_name);
        count++;
      }

      free(dirname_copy);
    }
  }
  closedir(dir);
  return count;
}

int clean_main(int argc, char *argv[]) {
  size_t count;
  pl_unshare_user();
  char *pid, *sid;
  char *plash_data = plash("data");

  // cd index
  if (chdir(plash_data) == -1)
    pl_fatal("chdir");
  if (chdir("index") == -1)
    pl_fatal("chdir");

  // remove broken indexes
  fprintf(stderr, "unlinked indexes: ");
  count = remove_broken_links_here();
  fprintf(stderr, "%ld\n", count);

  // cd map
  if (chdir(plash_data) == -1)
    pl_fatal("chdir");
  if (chdir("map") == -1)
    pl_fatal("chdir");

  // remove broken maps
  fprintf(stderr, "unlinked maps: ");
  count = remove_broken_links_here();
  fprintf(stderr, "%ld\n", count);

  // cd tmp
  if (chdir(plash_data) == -1)
    pl_fatal("chdir");
  if (chdir("tmp") == -1)
    pl_fatal("chdir");

  // remove unused tmp dirs
  fprintf(stderr, "removed tmpdirs: ");
  count = delete_unused_tmpdirs_here();
  fprintf(stderr, "%ld\n", count);
  return EXIT_SUCCESS;
}
