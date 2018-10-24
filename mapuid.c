#define _GNU_SOURCE
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

#define err(...) {\
fprintf(stderr, "%s", "myprog: ");\
fprintf(stderr, __VA_ARGS__);\
fprintf(stderr, ": %s\n", strerror(errno));\
exit(1);\
}

int bettermap_find(
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
                                strtoul(label, NULL, 10) == id && errno == 0)
                        errno = 0;
                        return 0;
        }
        errno = 0;
        return -1;
}

int bettermap_run(unsigned long uidrange[2], unsigned long gidrange[2]){
        int fd[2];
        pid_t child;
        char readbuffer[2];
        if (-1 == pipe(fd)){
                perror("pipe");
                exit(EXIT_FAILURE);
        }
        child = fork();
        if (-1 == child) perror("fork");
        if (0 == child){
                close(fd[1]);
                dup2(fd[0], 0);
                execlp("sh", "sh", "-e", NULL);
        }

        if (-1 == unshare(CLONE_NEWNS | CLONE_NEWUSER)){
                perror("could not unshare");
                exit(EXIT_FAILURE);
        }

        if (0 > dprintf(
                        fd[1],
                        "newuidmap %lu %lu %lu %lu %lu %lu %lu\n" \
                        "newgidmap %lu %lu %lu %lu %lu %lu %lu\n" \
                        "exit 0\n",
                        getpid(), 0, 1000, 1, 1, uidrange[0], uidrange[1],
                        getpid(), 0, 1000, 1, 1, gidrange[0], gidrange[1])) {
                printf("dprintf failed");
                exit(1);
        }
        close(fd[0]);
        close(fd[1]);

        int status;
        waitpid(child, &status, 0);
        if (!WIFEXITED(status)){
                printf("child exited abnormally");
                exit(1);
        }
        status = WEXITSTATUS(status);
        if (status){
                printf("child exited with %d\n", status);
                exit(1);
        }
}

int bettermap_setup() {
        struct passwd *pwent = getpwuid(getuid());
        struct group *grent = getgrgid(getgid());
        if (NULL == pwent) {perror("uid not in passwd"); exit(1);}
        if (NULL == grent) {perror("gid not in db"); exit(1);}
        unsigned long uidrange[2], gidrange[2];
        
        if (-1 == bettermap_find(pwent->pw_uid, pwent->pw_name, "/etc/subuid", uidrange) ||
            -1 == bettermap_find(grent->gr_gid, grent->gr_name, "/etc/subgid", gidrange)) {
                return -1;
        }
        
        bettermap_run(uidrange, gidrange);
        return 0;
}

void simplemap_map(const char *file, uid_t id){ // assuming uid_t == gid_t
        //
        // echo "0 $(id -u) 1" > /proc/self/uid_map
        //
	char *map;
        FILE *fd;

	if (NULL == (fd = fopen(file, "w"))) {
                err("Could not open %s", file);
        }
        fprintf(fd, "0 %u 1\n", id);
        if (errno != 0)
                err("could not write to %s", file);
        fclose(fd);
}

void simplemap_deny_setgroups() {
	FILE *fd = fopen("/proc/self/setgroups", "w");
	if (NULL == fd) {
		if (errno != ENOENT) 
                        err("could not open setgroups");
        }
        fprintf(fd, "deny");
        if (errno != 0) {
                perror("output error writing to /proc/self/setgroups\n");
                exit(1);
        }
        fclose(fd);
}

void simplemap_setup(){
        uid_t uid = getuid();
        gid_t gid = getgid();
        if (-1 == unshare(CLONE_NEWNS | CLONE_NEWUSER)){
                perror("could not unshare");
                exit(1);
        }
        simplemap_deny_setgroups();
        simplemap_map("/proc/self/uid_map", uid);
        simplemap_map("/proc/self/gid_map", gid);
}

int main(int argc, char* argv[]) {


        simplemap_setup();
        //if (-1 == bettermap_setup()){
        //        printf("call the other unshare");
        //}
        execlp("id", "id", NULL);
}
