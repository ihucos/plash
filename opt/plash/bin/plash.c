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


char* UNSHARE_USER[] = {
        "add-layer",
        "clean",
        "copy",
        "import-tar",
        "purge",
        "rm",
        "shrink",
        NULL
};

char* UNSHARE_USER_AND_MOUNT[] = {
        "runopts",
        "shallow-copy",
        "sudo",
        "with-mount",
        NULL
};


int in_arrary(char *array[], char *element){
        size_t i;
        for (i = 0; array[i]; i++){
                if (strcmp(array[i], element) == 0) return 1;
        }
        return 0;
}


int main(int argc, char* argv[]) {

        if (argc <= 1){
                fprintf(stderr, "plash is a container build and run engine, try --help\n");
                return 1;
        }

        struct passwd *pwd;
        char *bindir =           pl_path("../bin"),
             *libexecdir =       pl_path("../lib/exec"),
             *libexecrun =       pl_path("../lib/exec/run"),
             *pylibdir =         pl_path("../lib/py"),
             *path =             getenv("PATH"),
             *plash_data =       getenv("PLASH_DATA"),
             *plash_no_unshare = getenv("PLASH_NO_UNSHARE"),
             *home,
             *libexecfile,
             *newpath;

        //
        // validate user input
        //
        // FIXME: TODO: XXX

        if (asprintf(&libexecfile, "%s/%s", libexecdir, argv[1]) == -1)
                pl_fatal("asprintf");

        if (asprintf(&newpath, "%s:%s", bindir, path) == -1)
                pl_fatal("asprintf");

        //
        // setup environment variables
        //
        if (setenv("PYTHONPATH", pylibdir , 1) == -1)
                pl_fatal("setenv");
        if (setenv("PATH", path ? newpath : bindir, 1) == -1)
                 pl_fatal("setenv");
        if (! plash_data){
            pwd = getpwuid(getuid());
            if (! pwd) pl_fatal("could not determine your home directory");
            if (asprintf(&home, "%s/.plashdata", pwd->pw_dir) == -1)
                    pl_fatal("asprintf");
            if (setenv("PLASH_DATA", home , 1) == -1)
                    pl_fatal("setenv");
        }

        //
        // setup unsharing
        //
        if (!plash_no_unshare || plash_no_unshare[0] == '\0'){
                    int unshare_user = in_arrary(UNSHARE_USER, argv[1]);
                    int unshare_all = in_arrary(UNSHARE_USER_AND_MOUNT, argv[1]);
                    if ((unshare_user || unshare_all)  && getuid()) pl_setup_user_ns();
                    if (unshare_all){
                            if (unshare(CLONE_NEWNS) == -1) pl_fatal("could not unshare mount namespace");
                            if (mount("none", "/", NULL, MS_REC|MS_PRIVATE, NULL) == -1){
                                    if (errno != EINVAL) pl_fatal("could not change propagation of /");
                                    errno = 0;
                            }
                    }
        }

        //
        // exec lib/exec/<command>
        //
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
