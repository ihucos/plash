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


#define SUBC_UNSHARE_USER      (1 << 0)
#define SUBC_UNSHARE_MOUNT     (1 << 1)
#define SUBC_BUILD             (1 << 2)

#define IS_PARAM(argv) (strlen(argv) >= 2 && argv[0] == '-')


struct cmd_conf {
        char *name;
        int flags;
};

struct cmd_conf commands_array[] = {
        {"add-layer",     SUBC_BUILD | SUBC_UNSHARE_USER},
        {"build",         },
        {"clean",         SUBC_UNSHARE_USER},
        {"copy",          SUBC_BUILD | SUBC_UNSHARE_USER},
        {"create",        SUBC_BUILD},
        {"data",          },
        {"eval",          },
        {"export-tar",    SUBC_BUILD | SUBC_UNSHARE_USER},

        {"help",          },
        {"-h",            },
        {"--help",        },

        {"help-macros",   },
        {"--help-macros", },

        {"import-docker", },
        {"import-lxc",    },
        {"import-tar",    },
        {"import-url",    },
        {"init",          },
        {"map",           },
        {"mount",         SUBC_BUILD},
        {"nodepath",      },
        {"parent",        },
        {"purge",         SUBC_UNSHARE_USER},
        {"rm",            SUBC_BUILD | SUBC_UNSHARE_USER},
        {"run",           SUBC_BUILD},
        {"runopts",       SUBC_UNSHARE_USER | SUBC_UNSHARE_MOUNT},
        {"shallow-copy",  SUBC_BUILD | SUBC_UNSHARE_USER | SUBC_UNSHARE_MOUNT},
        {"shrink",        SUBC_UNSHARE_USER},
        {"sudo",          SUBC_UNSHARE_USER | SUBC_UNSHARE_MOUNT},
        {"test",          },

        {"version",       },
        {"--version",     },

        {"with-mount",    SUBC_BUILD | SUBC_UNSHARE_USER | SUBC_UNSHARE_MOUNT},

        {NULL, 0},

};


int get_cmd_flags(char *cmd){
        struct cmd_conf *thiscmd;
        for(
            thiscmd = commands_array;
            thiscmd->name && strcmp(thiscmd->name, cmd) != 0;
            thiscmd++
        );
        if (! thiscmd->name) return -1;
        return thiscmd->flags;
}


void expand_implicit_run(int *argc_ptr, char ***argv_ptr){
        // from: plash -A xeyes -- xeyes
        //  to: plash run -A xeyes -- xeyes

        int argc = *argc_ptr;
        char **newargv;
        char **argv = *argv_ptr;

        if (!(newargv = malloc((argc + 2) * sizeof(char*))))
               pl_fatal("malloc");

        *(newargv++) = *(argv++);
        *(newargv++) = "run";
        while(*(newargv++) = *(argv++));

        *argv_ptr = newargv - argc - 2;
        *argc_ptr = argc + 1;
}

void D(char *arr[]){
        int ai;
        for(ai=0; arr[ai]; ai++) fprintf(stderr, "%s, ", arr[ai]);
        fprintf(stderr, "\n");
}

void build_argv(int argc, char *argv[]){

                char  *build_array[argc],
                     **build = build_array,
                     **new_argv = argv,
                     **orig_argv = argv;

                // "new_argv" and "build" get  argv[0] as first element
                *new_argv++ = *build++ = *argv++;

                // variable "build" has string "build" as second element
                *build++ = "build";

                // new_argv gets argv[1] as second element
                *new_argv++ = *argv++;

                // wind up all to the "build" variable until the end or "--" is
                // reached
                while((*argv && strcmp(*argv, "--") != 0))
                        *build++ = *argv++;

                // chop "--" from our buffer, if any
                if (*argv) argv++;

                // "build" is done
                *build++ = NULL;

                // new_argv's third element is a freshly builded container id
                *new_argv++ = pl_check_output(build_array);


                // wind up all the rest to new_argv
                while(*argv) *new_argv++ = *argv++;
                *new_argv++ = NULL;
        }


int main(int argc, char* argv[]) {

        struct passwd *pwd;
        int flags;
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

        if (argc <= 1){
                fprintf(stderr, "plash is a container build and run engine, try --help\n");
                return 1;
        }

        //
        // load subcommand flags and handle implicit run
        //
        flags = get_cmd_flags(argv[1]);
        if (-1 == flags){
                if (! IS_PARAM(argv[1]))
                        pl_fatal("no such command: %s (try `plash help`)", argv[1]);
                expand_implicit_run(&argc, &argv);
                assert(strcmp(argv[1], "run") == 0);
                flags = get_cmd_flags("run");
        }

        //
        // handle build arguments
        //
        if (flags & SUBC_BUILD && IS_PARAM(argv[2])) build_argv(argc, argv);

        //
        // setup environment variables
        //
        if (asprintf(&newpath, "%s:%s", bindir, path) == -1)
                pl_fatal("asprintf");
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

                    if (flags & SUBC_UNSHARE_USER && getuid())
                        pl_setup_user_ns();

                    if (flags & SUBC_UNSHARE_MOUNT){
                            if (unshare(CLONE_NEWNS) == -1)
                                pl_fatal("could not unshare mount namespace");
                            if (mount("none", "/", NULL, MS_REC|MS_PRIVATE, NULL) == -1){
                                    if (errno != EINVAL)
                                        pl_fatal("could not change propagation of /");
                                    errno = 0;
                            }
                    }
        }

        //
        // exec lib/exec/<command>
        //
        if (asprintf(&libexecfile, "%s/%s", libexecdir, argv[1]) == -1)
                pl_fatal("asprintf");
        execvp(libexecfile, argv + 1);
        pl_fatal("could not exec %s", libexecfile);
}
