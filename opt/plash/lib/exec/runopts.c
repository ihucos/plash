// usage: plash run CONTAINER [ CMD1 [ CMD2 ... ] ]
//
// Run a container. If no command is specified, the containers default root
// shell is executed.
//
// The following host file systems are mapped to the container:
// - /tmp
// - /home
// - /root
// - /etc/resolv.conf
// - /sys
// - /dev
// - /proc
//
// If you want more control about how the container is run, use `plash runopts`
//
// Parameters may be interpreted as build instruction.


#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <plash.h>

int main(int argc, char* argv[]){
        char *templ = NULL,
             *mountpoint = NULL,
             **templ_array,
             *plash_data_env =  getenv("PLASH_DATA"),

             *container_arg = NULL,
             *changesdir_arg = NULL;

        assert(plash_data_env);

        if (asprintf(&mountpoint, "%s/mnt", plash_data_env) < 0)
                pl_fatal("asprintf");

        if (mount("tmpfs", mountpoint, "tmpfs", MS_MGC_VAL, NULL) < 0)
                pl_fatal("mount");

        int c;
        char *newargv[argc + 2];
        unsigned char i = 0;
        newargv[i++] = "plash";
        newargv[i++] = "chroot";

        while ((c = getopt (argc, argv, "c:d:m:")) != -1)
                switch (c) {
                        case 'm':
                                newargv[i++] = "-m";
                                newargv[i++] = optarg;
                                break;
                        case 'd':
                                changesdir_arg = optarg;
                                break;
                        case 'c':
                                container_arg = optarg;
                                break;
        }

        newargv[i++] = mountpoint;
        while (newargv[i++] = *(optind + argv++));
        newargv[i++] = NULL;

        if (!changesdir_arg){
                pid_t sid = getsid(0);
                if (asprintf(&templ, "%s/tmp/plashtmp_%d_%d_XXXXXX", plash_data_env, getsid(0), getpid()) < 0)
                        pl_fatal("asprintf");
                templ_array = &templ;
                changesdir_arg = mkdtemp(*templ_array);
        }
        if (!container_arg)
                pl_fatal("need container arg (-c)");

        // call subcommand
        pid_t pid = fork(); 
        if ( pid == 0 ) {
                execlp("plash", "plash", "mount", container_arg, mountpoint, changesdir_arg, NULL); 
                pl_fatal("execl");
        }
  
        int status; 
        waitpid(pid, &status, 0); 
        if ( WIFEXITED(status)) { 
            int exit_status = WEXITSTATUS(status);         
            if (exit_status) pl_fatal("child died baldy");
        } 

        //for(int i = 0; newargv[i]; i++){
        //    puts(newargv[i]);
        //}
        //exit(1);

        execvp(newargv[0], newargv);
        pl_fatal("execvp");
}

