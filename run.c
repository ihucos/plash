#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <sched.h>
#include <libgen.h>
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

void map_id(const char *file, u_int32_t id){
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

        argv[0] = progname;
        if (-1 == execvp(progname, argv))
                err("could not exec %s", progname);

}
