// usage: plash nodepath CONTAINER [--allow-root-container] 
// Prints the path to a given container. The --allow-root-container option
// allows the root container ("0") to be specified as container.
//
// Parameters may be interpreted as build instruction.
//
// Examples:
// $ plash nodepath 19
// /home/ihucos/.plashdata/layer/0/2/19
// $ plash nodepath 19 | xargs tree

#define _GNU_SOURCE
#include <assert.h>
#include <ctype.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>

#include <plash.h>


int main(int argc, char* argv[]) {

	char *container;
	char *indexlink;
	char *nodepath;
	char *plash_data = getenv("PLASH_DATA");
        assert(plash_data);

	if (argc < 2){
	    // plash_die_with_usage();
	    pl_fatal("this is usage");
	}

	if (0 == strcmp(argv[1], "0") && (
		argc <= 2 || 0 != strcmp(argv[2], "--allow-root-container"))){
            pl_fatal("container must not be the special root container ('0')");
	}


	asprintf(&indexlink, "%s/index/%s", plash_data, argv[1]
		) != -1 || pl_fatal("asprintf");
	if (NULL == (nodepath = realpath(indexlink, NULL))){
		errno = 0;
		pl_fatal("no container %s", argv[1]);
	}
	free(indexlink);
	printf("%s\n", nodepath);
}
