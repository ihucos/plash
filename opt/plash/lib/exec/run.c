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
             *tmpdir,
             *plash_data_env =  getenv("PLASH_DATA");


        assert(plash_data_env);

        if (asprintf(&mountpoint, "%s/mnt", plash_data_env) < 0)
                pl_fatal("asprintf");

        pid_t sid = getsid(0);
        if (asprintf(&templ, "%s/tmp/plashtmp_%d_%d_XXXXXX", plash_data_env, getsid(0), getpid()) < 0)
                pl_fatal("asprintf");
        templ_array = &templ;
        tmpdir = mkdtemp(*templ_array);

        if (mount("tmpfs", mountpoint, "tmpfs", MS_MGC_VAL, NULL) < 0)
                pl_fatal("mount");

        pid_t pid = fork(); 
        if ( pid == 0 ) {
                execlp("plash", "plash", "mount", argv[1], mountpoint, tmpdir, NULL); 
                pl_fatal("execl");
        }
  
        int status; 
        waitpid(pid, &status, 0); 
  
        if ( WIFEXITED(status)) { 
            int exit_status = WEXITSTATUS(status);         
            if (exit_status) pl_fatal("child died baldy");
        } 

        char *newargv[argc + 1];
        int i = 0;
        newargv[i++] = "plash";
        newargv[i++] = "chroot";
        newargv[i++] = mountpoint;
        argv += 2;
        while (newargv[i++] = *(argv++));

        execvp(newargv[0], newargv);
        pl_fatal("execvp");
}

