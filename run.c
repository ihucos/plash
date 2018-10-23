#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
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


void setup_map(uid_t uid, gid_t gid) {

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

        map_id("/proc/self/uid_map", uid);
        map_id("/proc/self/gid_map", gid);
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


int main(int argc, char* argv[]) {

        if (argc < 3){
                fprintf(stderr, "bad usage\n");
                exit(1);
        }


        int uid = getuid();
        int gid = getgid();

        if (uid == 0) {
	        if (-1 == unshare(CLONE_NEWNS))
                        err("could not unshare")
        } else {
	        if (-1 == unshare(CLONE_NEWNS | CLONE_NEWUSER))
                        err("could not unshare")

                setup_map(uid, gid);
        }

        rootfs_mount("/dev",  argv[1], "/dev");
        rootfs_mount("/home", argv[1], "/home");
        rootfs_mount("/proc", argv[1], "/proc");
        rootfs_mount("/root", argv[1], "/root");
        rootfs_mount("/sys",  argv[1], "/sys");
        rootfs_mount("/tmp",  argv[1], "/tmp");

        char *origpwd;
        if (NULL == (origpwd = get_current_dir_name()))
            err("error calling get_current_dir_name")

        if (-1 == chroot(argv[1]))
                err("could not chroot to %s", argv[1]);

        if (-1 == chdir(origpwd)){
                if (-1 == chdir("/"))
                        err("could not chdir")
        }

        if (-1 == execvp(argv[2], argv+2))
                err("could not exec %s", argv[2]);

}
