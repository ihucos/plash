// usage: plash rm IMAGE_ID
// Deletes the given image atomically. Running containers based on that image
// have an undefined behaviour.

#include <stdio.h>
#include <unistd.h>
#include <plash.h>

int main(int argc, char *argv[]) {
    if (argc != 2)
        pl_usage();

    char *nodepath = pl_call("nodepath", argv[1]);
    char *tmp = pl_call("mkdtemp");

    pl_unshare_user();

    if (rename(nodepath, tmp) == -1) pl_fatal("rename %s %s", nodepath, tmp);

    execvp("rm", (char*[]){"rm", "-rf", tmp, NULL});
    pl_fatal("execvp");
}
