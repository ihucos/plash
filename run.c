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

void map_id(const char *file, u_int32_t id){
        //
        // echo "0 $(id -u) 1" > /proc/self/uid_map
        //
	char *map;
	int fd = open(file, O_WRONLY);
	if (fd < 0) {
                fprintf(stderr, "could not open %s (%s)\n", file, strerror(errno));
                exit(1);
        }
        asprintf(&map, "0 %u 1\n", id);
        //printf("%s\n", map);
        if (-1 == write(fd, map, sizeof(map))) {
                fprintf(stderr, "could not write to %s (%s)", file, strerror(errno));
                exit(1);
        }
        close(fd);
}

int main() {
        int uid = getuid();
        int gid = getgid();

	if (-1 == unshare(CLONE_NEWNS | CLONE_NEWUSER)) {
            fprintf(stderr, "could not unshare\n");
            return 1;
        }


        //
        // echo deny > /proc/self/setgroups
        //
	int fd = open("/proc/self/setgroups", O_WRONLY);
	if (fd < 0) {
		if (errno != ENOENT) {
                        fprintf(stderr, "could not open setgroups\n");
		        return 1;
                }
        }
        const char *deny = "deny";
        write(fd, deny, sizeof(deny));
        close(fd);

        map_id("/proc/self/uid_map", uid);
        map_id("/proc/self/gid_map", gid);

        printf("%d\n", getuid());
	return 0;

}
