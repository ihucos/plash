#define _GNU_SOURCE
#include <errno.h>
#include <pwd.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <unistd.h>

#include <plash.h>

int main(int argc, char* argv[]) {

        if (argc <= 1){
                fprintf(stderr, "plash is a container build and run engine, try --help\n");
                return 1;
        }

        struct passwd *pwd;
        char *bindir =     pl_path("../bin"),
             *libexecdir = pl_path("../lib/exec"),
             *libexecrun = pl_path("../lib/exec/run"),
             *pylibdir =   pl_path("../lib/py"),
             *path =       getenv("PATH"),
             *plash_data = getenv("PLASH_DATA"),
             *plash_no_unshare = getenv("PLASH_NO_UNSHARE"),
             *home,
             *libexecfile,
             *newpath;


        if (! plash_data){
            pwd = getpwuid(getuid());
            if (! pwd) pl_fatal("could not determine your home directory");
            if (asprintf(&home, "%s/.plashdata", pwd->pw_dir) == -1)
                    pl_fatal("asprintf");
            if (setenv("PLASH_DATA", home , 1) == -1)
                    pl_fatal("setenv");
        }

        if (asprintf(&libexecfile, "%s/%s", libexecdir, argv[1]) == -1)
                pl_fatal("asprintf");

        if (asprintf(&newpath, "%s:%s", bindir, path) == -1)
                pl_fatal("asprintf");

        if (setenv("PYTHONPATH", pylibdir , 1) == -1)
                pl_fatal("setenv");

        if (setenv("PATH", path ? newpath : bindir, 1) == -1)
                 pl_fatal("setenv");

        if ( 
                (
                        plash_no_unshare == NULL || plash_no_unshare[0] == '\0'
                ) && (
                        strcmp("clean",        argv[1]) == 0 ||
                        strcmp("copy",         argv[1]) == 0 ||
                        strcmp("import-tar",   argv[1]) == 0 ||
                        strcmp("purge",        argv[1]) == 0 ||
                        strcmp("rm",           argv[1]) == 0 ||
                        strcmp("runopts",      argv[1]) == 0 ||
                        strcmp("shallow-copy", argv[1]) == 0 ||
                        strcmp("shrink",       argv[1]) == 0 ||
                        strcmp("sudo",         argv[1]) == 0 ||
                        strcmp("with-mount",   argv[1]) == 0
                )
        ){
                pl_unshare();
                setenv("PLASH_NO_UNSHARE", "1", 0);
        }

        argv++;
        execvp(libexecfile, argv);
        if (errno == ENOENT){
                if (argv[0][0] == '-'){
                        argv--;
                        argv[0] = "run";
                        execvp(libexecrun, argv);
                } else {
                        errno = 0;
                        pl_fatal("no such command: %s (try `plash help`)", argv[0]);
                }
        }
        pl_fatal("could not exec %s", libexecfile);
}
