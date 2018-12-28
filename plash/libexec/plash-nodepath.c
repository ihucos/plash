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
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <pwd.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>

#include "./utils.c"


int main(int argc, char* argv[]) {

	char *container;
	char *indexlink;
	char *nodepath;

	//plash_handle_help_flag();
	//plash_handle_build_args();

	if (argc < 2){
	    // plash_die_with_usage();
	    plash_fatal("this is usage");
	}
	container = argv[1];
	char c;
	for(unsigned int i = 0; c = container[i]; i++){
		if (! isdigit(c))
			plash_fatal("container id must be integer, got %s", container);
	}

	if (0 == strcmp(container, "0") && (
		argc <= 2 || 0 != strcmp(argv[2], "--allow-root-container"))){
            plash_fatal("container must not be the special root container ('0')")
	}


	asprintf(&indexlink, "%s/index/%s", plash_get_data(), container
		) != -1 || plash_fatal_memory();
	if (NULL == (nodepath = realpath(indexlink, NULL))){
		errno = 0;
		plash_fatal("no container %s", container)
	}
	free(indexlink);
	printf("%s\n", nodepath);
}
