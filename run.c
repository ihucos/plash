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

int main(int argc, char* argv[]) {
        int uid = getuid();
        int gid = getgid();

	if (-1 == unshare(CLONE_NEWNS | CLONE_NEWUSER))
            err("could not unshare")


        //
        // echo deny > /proc/self/setgroups
        //
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

        printf("%d\n", getuid());
        printf("%d\n", getgid());
        if (argc < 2){
                fprintf(stderr, "bad usage\n");
                exit(1);
        }
        execvp(argv[1], argv+1); 

}
