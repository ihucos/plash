// usage: plash chroot ROOTFS [CMD1 [CMD2 ...]]
#define _GNU_SOURCE
#include <errno.h>
#include <libgen.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <unistd.h>

#include <plash.h>


#define PRESET_PATH "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"


int main(int argc, char* argv[]) {
	char i,
	     *token,
	     *str,
	     *origpwd = get_current_dir_name();

        if (argc < 2) pl_usage();

	if (!origpwd)  pl_fatal("get_current_dir_name");



        while ((getopt(argc, argv, "m:")) != -1);
	if (chdir(argv[optind]) < 0) pl_fatal("cd %s", argv[1]);
        optind = 0;

        int c;
        while ((c = getopt (argc, argv, "m:")) != -1)
                switch (c) {
                        case 'm':
                                // check for absolute
		                if (0 < mount(optarg, optarg+1, "none",
		                                MS_MGC_VAL|MS_BIND|MS_REC, NULL)){
		                	if (errno != ENOENT){
		                		pl_fatal("rbind for %s", optarg);
		                	}
		                }
                                break;
	}

	if (chroot(argv[optind]) < 0)  pl_fatal("could not chroot to %s",
	                           get_current_dir_name());

	/* chdir back or fallback to / */
	if (chdir(origpwd) < 0){
		if (-1 == chdir("/"))
			pl_fatal("chdir(\"/\")");
	}

	/* setup environment fo exec */
	if (str = getenv("PLASH_EXPORT")) {
		str = strdup(str);
		token = strtok(str, ":");
		while(token){
			pl_whitelist_env(token);
			token = strtok(NULL, ":");
		}
		free(str);
	}
	putenv("PATH=" PRESET_PATH);
	pl_whitelist_env("TERM");
	pl_whitelist_env("DISPLAY");
	pl_whitelist_env("HOME");
	pl_whitelist_env("PATH");
	pl_whitelist_env(NULL);

	execvp(argv[optind+1], argv + optind + 1);
        pl_fatal(argv[optind+1]);
}
