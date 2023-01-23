// Setup a Linux user namespace. Then run the specified commands there. The
// default command is the default user shell.
//
// This is useful to access files written to disk by a container, when they
// where written by a non-root user (from the containers perspective). It can
// also be used as a general purpose utility to "fake" root access.

#define USAGE "usage: plash sudo [CMD1 [CMD2 ..]]\n"

#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include <utils.h>

int sudo_main(int argc, char *argv[]) {
  struct passwd *pw = getpwuid(getuid());
  pl_unshare_user();
  pl_unshare_mount();
  char *default_shell = pw ? pw->pw_shell : "/bin/sh";
  if (argc <= 1) {
    execlp(default_shell, default_shell, NULL);
  } else {
    execvp(argv[1], argv + 1);
  }
  pl_fatal("could not exec \"%s\"", argv[1]);
  return 0;
}
