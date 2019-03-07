// usage: plash sudo [CMD1 [CMD2 ..]]
//
// Setup a Linux user namespace. Then run the specified commands there. The
// default command is the default user shell.
//
// This is useful to access files written to disk by a container, when they
// where written by a non-root user (from the containers perspective). It can
// also be used as a general purpose utility to "fake" root access.


#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include <plash.h>

int main(int argc, char* argv[]) {
        struct passwd *pw = getpwuid(getuid());
        char* default_shell = pw ? pw->pw_shell : "/bin/sh";
        argv++;
        if (argc <= 1) {
                execlp(default_shell, default_shell, NULL);
        } else {
                execvp(argv[0], argv);
        }
        pl_fatal("could not exec \"%s\"", argv[0]);
}
