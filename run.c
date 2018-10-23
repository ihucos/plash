#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

#define err(...) {\
fprintf(stderr, "%s", "myprog: ");\
fprintf(stderr, __VA_ARGS__);\
fprintf(stderr, ": %s\n", strerror(errno));\
exit(1);\
}

void map_id(const char *file, uid_t id){ // assuming uid_t == gid_t
        //
        // echo "0 $(id -u) 1" > /proc/self/uid_map
        //
	char *map;
	int fd = open(file, O_WRONLY);
	if (fd < 0) {
                err("Could not open %s", file);
        }
        asprintf(&map, "0 %u 1\n", id);
        //printf("%s\n", map);
        if (-1 == write(fd, map, sizeof(map)))
                err("could not write to %s", file);
        close(fd);
}

void deny_setgroups() {
	FILE *fd = fopen("/proc/self/setgroups", "w");
	if (fd < 0) {
		if (errno != ENOENT) 
                        err("could not open setgroups");
        }
        if (fprintf(fd, "%s", "deny") < 0){ // FIXME: error handling broken
                fprintf(stderr, "output error writing to /proc/self/setgroups\n");
                exit(1);
        }
        fclose(fd);
}

void rootfs_mount(const char *hostdir, const char *rootfs, const char *rootfsdir) {
        char *dst;
        struct stat sb;

        if (! (stat(hostdir, &sb) == 0 && S_ISDIR(sb.st_mode)))
                return;

        if (-1 == asprintf(&dst, "%s/%s", rootfs, hostdir)) {
                fprintf(stderr, "myprog: asprintf returned -1\n");
                exit(1);
        }
        if (-1 == mount(hostdir, dst, "none", MS_MGC_VAL|MS_BIND|MS_REC, NULL))
                err("could not rbind mount %s -> %s", hostdir, dst)
}

void find_rootfs(char **rootfs) {
        char *executable;
        if (NULL == (executable = realpath("/proc/self/exe", NULL)))
                err("could not call realpath");
        char *executable_dir = dirname(executable);
        if (-1 == asprintf(rootfs, "%s/rootfs", executable_dir)){
                fprintf(stderr, "myprog: asprintf returned -1\n");
                exit(1);
        }
}


int main(int argc, char* argv[]) {


        char *argv0 = strdup(argv[0]);
        char *progname = basename(argv0);
        char *rootfs;
        find_rootfs(&rootfs);

        int uid = getuid();
        int gid = getgid();

        if (uid == 0) {
	        if (-1 == unshare(CLONE_NEWNS))
                        err("could not unshare")
        } else {
	        if (-1 == unshare(CLONE_NEWNS | CLONE_NEWUSER))
                        err("could not unshare")
                        deny_setgroups();
                        map_id("/proc/self/uid_map", uid);
                        map_id("/proc/self/gid_map", gid);
        }

        rootfs_mount("/dev",  rootfs, "/dev");
        rootfs_mount("/home", rootfs, "/home");
        rootfs_mount("/proc", rootfs, "/proc");
        rootfs_mount("/root", rootfs, "/root");
        rootfs_mount("/sys",  rootfs, "/sys");
        rootfs_mount("/tmp",  rootfs, "/tmp");

        char *origpwd;
        if (NULL == (origpwd = get_current_dir_name()))
            err("error calling get_current_dir_name")

        if (-1 == chroot(rootfs))
                err("could not chroot to %s", rootfs);

        if (-1 == chdir(origpwd)){
                if (-1 == chdir("/"))
                        err("could not chdir")
        }

        char *env[UCHAR_MAX + 1];
        unsigned char env_index; // let it overflow if maximum is reached
        char *envname, *envval;

        char *str = getenv("PLASH_EXPORT");
        if (str != NULL ) {
                envname = strtok(str, ":");
                while( envname != NULL ) {
                   envval = getenv(envname);
                   if (NULL != envval)
                        asprintf(env + env_index++, "%s=%s", envname, envval);
                   envname = strtok(NULL, ":");
                }
        }
   

        char *always_export[] = {"TERM", "DISPLAY", "HOME", NULL};
        for (size_t i=0; always_export[i] != NULL; i++) {
                envname = always_export[i];
                envval = getenv(envname);
                if (NULL == envval)
                        continue;
                asprintf(env + env_index++, "%s=%s", envname, envval);
        }

        env[env_index++] = NULL;

        argv[0] = progname;
        if (-1 == execvpe(progname, argv, env))
                err("could not exec %s", progname);

}
