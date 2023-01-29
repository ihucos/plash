//
// Mount a container-filesystem. Changes to the filesystem will be written to
// CHANGESDIR.

#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include <plash.h>

#define USAGE "usage: plash mount IMAGE_ID MOUNTPOINT [ CHANGESDIR ]\n"

#define CHANGESDIR_ALLOWED_CHARS                                               \
  "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./-_"

char *prepare_lowerdir(char *image_id) {
  char *lowerdir = "";
  char *plash_data = pl_call("data");
  char *nodepath = pl_call("nodepath", image_id);
  int offset = strlen(plash_data) + strlen("/layer/");
  if (strlen(nodepath) < offset) {
    pl_fatal("Unexpected interal error: nodepath command output is shorter "
             "than data command output");
  }
  char *curr;
  char *str = nodepath + offset;
  while (curr = strsep(&str, "/")) {
    asprintf(&lowerdir, "%s/index/%s/_data/root:%s", plash_data, curr,
             lowerdir) != -1 ||
        pl_fatal("asprintf");
  }
  lowerdir[strlen(lowerdir) - 1] = '\0'; // chop the last ":"
  return lowerdir;
}

char *prepare_workdir(char *changesdir) {
  char *workdir;
  asprintf(&workdir, "%s/work", changesdir) != -1 || pl_fatal("asprintf");
  if (mkdir(workdir, 0755) == -1 && errno != EEXIST)
    pl_fatal("mkdir");
  return workdir;
}

char *prepare_datadir(char *changesdir) {
  char *datadir;
  asprintf(&datadir, "%s/data", changesdir) != -1 || pl_fatal("asprintf");
  if (mkdir(datadir, 0755) == -1 && errno != EEXIST)
    pl_fatal("mkdir");
  return datadir;
}

void validate_changesdir(char *changesdir) {
  int found_in_allowed;
  for (size_t i = 0; changesdir[i]; i++) {
    found_in_allowed = 0;
    for (size_t j = 0; strlen(CHANGESDIR_ALLOWED_CHARS) > j; j++) {
      if (changesdir[i] == CHANGESDIR_ALLOWED_CHARS[j]) {
        found_in_allowed = 1;
      }
    }
    if (!found_in_allowed) {
      pl_fatal("Not allowed charachter found in changesdir: %c", changesdir[i]);
    }
  }
}

int mount_main(int argc, char *argv[]) {

  if (argc < 3) {
    {
      fputs(USAGE, stderr);
      return EXIT_FAILURE;
    }
  }
  char *image_id = argv[1];
  char *mountpoint = argv[2];
  char *changesdir = argv[3];

  char *mount_opts;

  if (changesdir != NULL) {
    validate_changesdir(changesdir);
    if (mkdir(changesdir, 0755) == -1 && errno != EEXIST)
      pl_fatal("mkdir %s", changesdir);
    asprintf(&mount_opts, "lowerdir=%s,workdir=%s,upperdir=%s",
             prepare_lowerdir(image_id), prepare_workdir(changesdir),
             prepare_datadir(changesdir)) != -1 ||
        pl_fatal("asprintf");
  } else {
    asprintf(&mount_opts, "lowerdir=%s", prepare_lowerdir(image_id)) != -1 ||
        pl_fatal("asprintf");
  }
  execlp("mount", "mount", "-t", "overlay", "overlay", "-o", mount_opts,
         mountpoint, NULL);
  pl_fatal("execlp");
}
