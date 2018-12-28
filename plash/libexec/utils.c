
#define plash_fatal(...) {\
fprintf(stderr, "%s", "plash error: ");\
fprintf(stderr, __VA_ARGS__);\
if (errno != 0){\
        fprintf(stderr, ": %s", strerror(errno));\
}\
fprintf(stderr, "\n");\
exit(1);\
}

int plash_fatal_memory(){
	plash_fatal("memory allocation seems to have failed");
	return 1;
}

char* plash_get_data(){
	char *homedir;
	char *plashdata;
	uid_t uid;

	if (NULL != (plashdata = getenv("PLASH_DATA"))) {
		return plashdata;
	}

	uid = getuid();
	if (uid){
		if (NULL == (homedir = getenv("HOME"))) {
		    homedir = getpwuid(uid)->pw_dir;
		}
		asprintf(&plashdata, "%s/.plashdata", homedir
		        ) != -1 || plash_fatal_memory();
		return plashdata;
	} else {
		return "/var/lib/plash";
	}
}
