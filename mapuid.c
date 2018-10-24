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

#define MAX_USERLEN 32
#define MAX_USERLEN_STR "32"
// #define MAX_USERNAME_LENGTH_STRING #MAX_USERNAME_LENGTH


void find_subid(unsigned long id, char *id_name, const char *file, unsigned long range[2]){

        char label[MAX_USERLEN];
        unsigned long ulong_label;

        FILE *fd = fopen(file, "r");
        if (NULL == fd) {
                if (errno == ENOENT){
                        range[0] = 0;
                        range[1] = 0;
                        return;
                }
                perror("could not open subuid/subgid file");
                exit(EXIT_FAILURE);
        }
        const char *scan = "%"MAX_USERLEN_STR"[^:\n]:%lu:%lu\n"; // move as constant
        while (3 == fscanf(fd, scan, label, range, range+1)){
                errno = 0;
                if ((strcmp(id_name, label) == 0) ||
                                strtoul(label, NULL, 10) == id && errno == 0)
                        return;
        }
        range[0] = 0;
        range[1] = 0;
        return;
}
        
int main(int argc, char* argv[]) {
        
        struct passwd *pwent = getpwuid(getuid());
        struct group *grent = getgrgid(getgid());
        unsigned long uidrange[2], gidrange[2];

        if (NULL == pwent) {perror("uid not in passwd"); exit(1);}
        find_subid(pwent->pw_uid, pwent->pw_name, "/etc/subuid", uidrange);
        if (uidrange[0] == 0) {
                printf("could not find subuid\n");
                exit(1);
        }

       if (NULL == grent) {perror("gid not in db"); exit(1);}
       find_subid(grent->gr_gid, grent->gr_name, "/etc/subgid", gidrange);


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
               getpid(), 0, 1000, 1, 1, gidrange[0], gidrange[1]
       )){
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
        execlp("id", "id", NULL);
}
