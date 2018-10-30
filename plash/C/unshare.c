#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <libgen.h>
#include <limits.h>
#include <pwd.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define STR_EXPAND(name) #name
#define STR(name) STR_EXPAND(name)
#define MAX_USERLEN 32
#define SCAN_ID_RANGE "%" STR(MAX_USERLEN) "[^:\n]:%lu:%lu\n"
#define fatal(...) {\
fprintf(stderr, "%s", "myprog: ");\
fprintf(stderr, __VA_ARGS__);\
if (errno != 0){\
        fprintf(stderr, ": %s\n", strerror(errno));\
}\
exit(1);\
}
int fullmap_find(
                unsigned long id,
                char *id_name,
                const char *file,
                unsigned long range[2]) {
        char label[MAX_USERLEN];
        FILE *fd;
        if (NULL == (fd = fopen(file, "r")))
                return -1;
        while (3 == fscanf(fd, SCAN_ID_RANGE, label, range, range+1)){
                errno = 0;
                if ((strcmp(id_name, label) == 0) ||
                                (strtoul(label, NULL, 10) == id && errno == 0)){
                        errno = 0;
                        return 0;
                }
        }
        errno = 0;
        return -1;
}
void fullmap_run(uid_t getuid, gid_t getgid, unsigned long uidrange[2], unsigned long gidrange[2]){
        int fd[2];
        pid_t child;
        if (-1 == pipe(fd))
                fatal("could not create pipe");
        child = fork();
        if (-1 == child)
                fatal("could not fork")
        if (0 == child){
                close(fd[1]);
                dup2(fd[0], 0);
                execlp("sh", "sh", "-xe", NULL);
        }
        if (-1 == unshare(CLONE_NEWNS | CLONE_NEWUSER)){
                fatal("could not unshare");
        }
        if (0 > dprintf(
                        fd[1],
                        "newuidmap %u %u %u %u %u %lu %lu\n" \
                        "newgidmap %u %u %u %u %u %lu %lu\n" \
                        "exit 0\n",
                        getpid(), 0, getuid, 1, 1, uidrange[0], uidrange[1],
                        getpid(), 0, getgid, 1, 1, gidrange[0], gidrange[1])){
                fatal("could not send data to child with dprintf");
        }
        close(fd[0]);
        close(fd[1]);
        int status;
        waitpid(child, &status, 0);
        if (!WIFEXITED(status))
                fatal("child exited abnormally");
        status = WEXITSTATUS(status);
        if (status){
                fatal("child exited with %d", status);
        }
}
int fullmap_setup() {
        struct passwd *pwent = getpwuid(getuid());
        struct group *grent = getgrgid(getgid());
        if (NULL == pwent) {perror("uid not in passwd"); exit(1);}
        if (NULL == grent) {perror("gid not in db"); exit(1);}
        unsigned long uidrange[2], gidrange[2];
        
        if (-1 == fullmap_find(pwent->pw_uid, pwent->pw_name, "/etc/subuid", uidrange) ||
            -1 == fullmap_find(grent->gr_gid, grent->gr_name, "/etc/subgid", gidrange)) {
                return -1;
        }
        
        fullmap_run(getuid(), getgid(), uidrange, gidrange);
        return 0;
}
void singlemap_map(const char *file, uid_t id){ // assuming uid_t == gid_t
        FILE *fd;
	if (NULL == (fd = fopen(file, "w"))) {
                fatal("could not open %s", file);
        }
        fprintf(fd, "0 %u 1\n", id);
        if (errno != 0)
                fatal("could not write to %s", file);
        fclose(fd);
}
void singlemap_deny_setgroups() {
	FILE *fd = fopen("/proc/self/setgroups", "w");
	if (NULL == fd) {
		if (errno != ENOENT) 
                        fatal("could not open /proc/self/setgroups");
        }
        fprintf(fd, "deny");
        if (errno != 0) {
                fatal("could not write to /proc/self/setgroups");
                exit(1);
        }
        fclose(fd);
}
void singlemap_setup(){
        uid_t uid = getuid();
        gid_t gid = getgid();
        if (-1 == unshare(CLONE_NEWNS | CLONE_NEWUSER))
                fatal("could not unshare");
        singlemap_deny_setgroups();
        singlemap_map("/proc/self/uid_map", uid);
        singlemap_map("/proc/self/gid_map", gid);
}

// int main(int argc, char* argv[]) {
//         //singlemap_setup();
//         fullmap_setup();
//         //if (-1 == fullmap_setup()){
//         //        printf("call the other unshare");
//         //}
//         execlp("id", "id", NULL);
// }
