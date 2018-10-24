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
        find_subid_t subidv = find_subid(1000, "ihucos", "/etc/subuid");
        printf("%i  %lu %lu\n", subidv.found, subidv.start, subidv.count);
}
