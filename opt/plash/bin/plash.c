#define _GNU_SOURCE
#include <assert.h>
#include <ctype.h>
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


void D(char *arr[]){
        int ai;
        for(ai=0; arr[ai]; ai++) fprintf(stderr, "%s, ", arr[ai]);
        fprintf(stderr, "\n");
}


int is_cli_param(char *param){
        switch(strlen(param)){
                case 1: return 0;
                case 2: return param[0] == '-' && param[1] != '-';
                default: return param[0] == '-';
        }
}


void reexec_insert_run(int argc, char **argv){
        //  it: plash -A xeyes -- xeyes
        // out: plash run -A xeyes -- xeyes

        char *newargv_array[argc + 2];
        char **newargv = newargv_array;

        *(newargv++) = *(argv++);
        *(newargv++) = "run";
        while(*(newargv++) = *(argv++));

        execvp(newargv_array[0], newargv_array);
        pl_fatal("execvp");
}


int main(int argc, char* argv[]) {
        int flags;

        if (argc <= 1){
                fprintf(stderr, "plash is a container build and run engine, try --help\n");
                return 1;
        }

        if (is_cli_param(argv[1]))
            reexec_insert_run(argc, argv);

        //
        // pop any "--" as first argument
        //
        if(argv[2] && strcmp(argv[2], "--") == 0){
                char *cmd = argv[1];
                argv[1] = argv[0];
                argv[2] = cmd;
                argv++;
        }

        struct passwd *pwd;
        char *bindir =               pl_path("../bin"),
             *libexecdir =           pl_path("../lib/exec"),
             *libexecrun =           pl_path("../lib/exec/run"),
             *pylibdir =             pl_path("../lib/py"),
             *path_env =             getenv("PATH"),
             *home_env =             getenv("HOME"),
             *plash_data_env =       getenv("PLASH_DATA"),
             *plash_no_unshare_env = getenv("PLASH_NO_UNSHARE"),
             *libexecfile,
             *newpath;

        //
        // setup environment variables
        //
        if (asprintf(&newpath, "%s:%s", bindir, path_env) == -1)
                pl_fatal("asprintf");
        if (setenv("PYTHONPATH", pylibdir , 1) == -1)
                pl_fatal("setenv");
        if (setenv("PATH", path_env ? newpath : bindir, 1) == -1)
                 pl_fatal("setenv");
        if (! plash_data_env){
            if (! home_env){
                pwd = getpwuid(getuid());
                if (! pwd)
                    pl_fatal("could not determine your home directory");
                home_env = pwd->pw_dir;
            }
            if (asprintf(&home_env, "%s/.plashdata", home_env) == -1)
                    pl_fatal("asprintf");
            if (setenv("PLASH_DATA", home_env , 1) == -1)
                    pl_fatal("setenv");
        }

        //
        // setup unsharing
        //
        if (!plash_no_unshare_env || plash_no_unshare_env[0] == '\0'){
                    // XXXX don't unsahre if program is 'mount'!!!!
                    pl_setup_user_ns();
                    if (unshare(CLONE_NEWNS) == -1)
                        pl_fatal("could not unshare mount namespace");
                    if (mount("none", "/", NULL, MS_REC|MS_PRIVATE, NULL) == -1){
                            if (errno != EINVAL)
                                pl_fatal("could not change propagation of /");
                            errno = 0;
                    }
        }

        //
        // exec lib/exec/<command>
        //
        if (asprintf(&libexecfile, "%s/%s", libexecdir, argv[1]) == -1)
                pl_fatal("asprintf");
        execvp(libexecfile, argv + 1);

        if (errno != ENOENT)
                pl_fatal("could not exec %s", libexecfile);
        errno = 0;
        pl_fatal("no such command: %s (try `plash help`)", argv[1]);
}
