#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_USERLEN 32
#define MAX_USERLEN_STR "32"
// #define MAX_USERNAME_LENGTH_STRING #MAX_USERNAME_LENGTH

typedef struct find_subid_struct {
        int found;
        unsigned long start;
        unsigned long count;
} find_subid_t;

find_subid_t find_subid(unsigned long id, char *id_name, const char *file){
    char label[MAX_USERLEN];
    unsigned long start, count, ulong_label;
    int found, found_status = 0;
    find_subid_t subidv;
    FILE *fd = fopen(file, "r");
    if (NULL == fd) {
            perror("could not open subuid/subgid file");
            exit(EXIT_FAILURE);
    }
    for (;;){
            if (3 != fscanf(fd, "%"MAX_USERLEN_STR"[^:\n]:%lu:%lu\n",
                    label, &start, &count))
                    break;

            if (strcmp(id_name, label) == 0){
                found_status = 1;
                break;
            } else {
                errno = 0;
                ulong_label = strtoul(label, NULL, 10);
                if (errno == 0 && ulong_label == id) {
                        found_status = 1;
                        break;
                };
            }
    }
    subidv.found = found_status;
    subidv.start = start;
    subidv.count = count;
    return subidv;


    
}
        
int main(int argc, char* argv[]) {
        
        struct passwd *pwent = getpwuid(getuid());
        if (NULL == pwent) {perror("uid not in passwd"); exit(1);}
        find_subid_t uidmap = find_subid(pwent->pw_uid, pwent->pw_name, "/etc/subuid");

        struct group *grent = getgrgid(getgid());
        if (NULL == grent) {perror("gid not in db"); exit(1);}
        find_subid_t gidmap = find_subid(grent->gr_gid, grent->gr_name, "/etc/subgid");

        printf("%i  %lu %lu\n", gidmap.found, gidmap.start, gidmap.count);

        if (-1 == unshare(CLONE_NEWNS | CLONE_NEWUSER)){
                perror("could not unshare");
                exit(EXIT_FAILURE);
        }

}
