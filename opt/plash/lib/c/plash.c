#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <pwd.h>
#include <grp.h>
#include <sched.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>


enum {
	SETUP_NO_UID = 0x01,
	SETUP_NO_GID = 0x02,
	SETUP_ERROR = 0x04,
};


typedef struct {
        char *prog;
        char *id_str;
        char *pid_str;
        char *file;
        char *query1;
        char *query2;

} fork_exec_newmap_t;


int pl_fatal(char *format, ...){
        va_list args;
        va_start(args, format);
        va_end(args);
        fprintf(stderr, "plash error: ");
        vfprintf(stderr, format, args);
        if (errno != 0) fprintf(stderr, ": %s", strerror(errno));
        fprintf(stderr, "\n");
        exit(1);
}

char* pl_path(const char *relpath){
        char *addedpath, *normalizedpath;
        static char *herepath = NULL;
        if (!herepath){
                if (!(herepath = realpath("/proc/self/exe", NULL)))
                        pl_fatal("realpath");
                herepath = dirname(herepath);
        }
        if (asprintf(&addedpath, "%s/%s", herepath, relpath) == -1)
                pl_fatal("asprintf");
        if (!(normalizedpath = realpath(addedpath, NULL)))
                pl_fatal("realpath %s", addedpath);
        free(addedpath);
        return normalizedpath;
}


int pl_printf_to_file(const char *file, const char *format, ...){
        FILE *fd;
        if (! (fd = fopen(file, "w")))
                return 0;
        va_list args;
        va_start(args, format);
        vfprintf(fd, format, args) >= 0 || pl_fatal("vfprintf %s", file);
        va_end(args);
        if (errno)
                pl_fatal("could not write to %s", file);
        fclose(fd);
        return 1;
}


int pl_parse_subid(
                    const char *file,
                    const char *query1,
                    const char *query2,
                    char **from,
                    char **to) {
        FILE *fd;
        size_t read, user_size = 0, from_size = 0, to_size = 0;
        char *label = NULL;
        // try to open file
        if (! (fd = fopen(file, "r"))){
                errno = 0;
                return 0;
        }
        for (;;){
                if ((read = getdelim(&label, &user_size, ':', fd)) == -1) break;
                label[read-1] = 0;
                if ((read = getdelim(from, &from_size, ':', fd)) == -1) break;
                (*from)[read-1] = 0;
                if ((read = getdelim(to, &to_size, '\n', fd)) == -1) break;
                if (feof(fd)){read++; realloc(*to, read);}
                (*to)[read-1] = 0;

                if (               (query1 && 0 == strcmp(query1, label))
                                || (query2 && 0 == strcmp(query2, label))){
                        free(label);
                        return 1;
                }
        }
        if (label) free(label);
        return 0;
}


void pl_whitelist_env(char *env_name){
	char *n, *v;
	static size_t env_counter = 0;
	if (!env_name)
		environ[env_counter++] = NULL;
	else {
		for(size_t i=env_counter; environ[i]; i++){
			for(
			    n = env_name, v = environ[i];
			    *n && *v && *n == *v;
			    n++, v++);
				if (*v == '=' && *n == 0)
					environ[env_counter++] = environ[i];
		}
	}
}


void pl_whitelist_envs_from_env(const char *export_env){
        char *str;
        char *token;
	if ((str = getenv(export_env))) {
		str = strdup(str);
		token = strtok(str, ":");
		while(token){
			pl_whitelist_env(token);
			token = strtok(NULL, ":");
		}
		free(str);
	}
}


void pl_bind_mount(const char* src, const char* dst){
	if (0 < mount(src, dst, "none",
	                MS_MGC_VAL|MS_BIND|MS_REC, NULL)){
		if (errno != ENOENT){
			pl_fatal("rbind %s to %s", src, dst);
		}
	}
}


void pl_chdir(const char* newdir){
	if (-1 == chdir(newdir)){
		if (-1 == chdir("/"))
			pl_fatal("chdir(\"/\")");
	}
}


void pl_chroot(const char* rootfs){
	char *origpwd = get_current_dir_name();
	if (!origpwd) pl_fatal("get_current_dir_name");
	chroot(rootfs
	      ) != -1 || pl_fatal("could not chroot to %s", rootfs);
        pl_chdir(origpwd);
}


int pl_fork_exec_newmap(fork_exec_newmap_t args){
        char *from = NULL;
        char *to = NULL;
        pid_t child = fork();
        if (child) return child;
        if (!pl_parse_subid(args.file, args.query2, args.query1, &from, &to))
                exit(127);
        execlp(args.prog, args.prog,
               args.pid_str, "0", args.id_str, "1",
               "1", from, to, NULL);
        if (errno == ENOENT) exit(127);
        pl_fatal("execlp");
}


void pl_setup_user_ns(){

	int setup_report = 0;
	int sig;
	pid_t master_child,
	      uid_child,
	      gid_child;
	siginfo_t uid_child_sinfo,
	          gid_child_sinfo,
		  master_child_sinfo;

	uid_t uid;
	gid_t gid;
	char *uid_str,
	     *gid_str,
	     *pid_str,
	     *username,
	     *groupname;

	uid = getuid();
	gid = getgid();
	asprintf(&uid_str, "%u", uid
	        ) != -1 || pl_fatal("asprintf");
	asprintf(&gid_str, "%u", gid
	        ) != -1 || pl_fatal("asprintf");
	asprintf(&pid_str, "%u", getpid()
	        ) != -1 || pl_fatal("asprintf");
	struct passwd *pw = getpwuid(uid);
	username = pw ? pw->pw_name : NULL;
	struct group *grp = getgrgid(gid);
	groupname = grp ? grp->gr_name : NULL;

	fork_exec_newmap_t map_uid = {
	        .prog="newuidmap",
	        .id_str=uid_str,
	        .file="/etc/subuid",
	        .query1=uid_str,
	        .query2=username,
	        .pid_str=pid_str};

	fork_exec_newmap_t map_gid = {
	        .prog="newgidmap",
	        .id_str=gid_str,
	        .file="/etc/subgid",
	        .query1=gid_str,
	        .query2=groupname,
	        .pid_str=pid_str};

	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGUSR1);
	sigprocmask(SIG_BLOCK, &sigset, NULL);

	(master_child = fork()) != -1 || pl_fatal("fork");
	if (!master_child){

		// wait for parent's signal
		sigwait(&sigset, &sig);

		uid_child = pl_fork_exec_newmap(map_uid);
		gid_child = pl_fork_exec_newmap(map_gid);

		waitid(P_PID, uid_child, &uid_child_sinfo, WEXITED
		      ) != -1 || pl_fatal("waitid");
		waitid(P_PID, gid_child, &gid_child_sinfo, WEXITED
		      ) != -1 || pl_fatal("waitid");

		switch (uid_child_sinfo.si_status){
			case 0: break;
			case 127: setup_report |= SETUP_NO_UID; break;
			default: setup_report |= SETUP_ERROR; break;
		}
		switch (gid_child_sinfo.si_status){
			case 0: break;
			case 127: setup_report |= SETUP_NO_GID; break;
			default: setup_report |= SETUP_ERROR; break;
		}

		exit(setup_report);
	}

	unshare(CLONE_NEWUSER
	       ) != -1 || pl_fatal("could not unshare user namespace "
                                   "(maybe try `sysctl -w kernel.unprivileged_userns_clone=1`)");

	kill(master_child, SIGUSR1);
	waitid(P_PID, master_child, &master_child_sinfo, WEXITED
	      ) != -1 || pl_fatal("waitid");

	if (master_child_sinfo.si_status & SETUP_ERROR){
		pl_fatal("child died badly");
	}
	if (master_child_sinfo.si_status & SETUP_NO_UID){
		pl_printf_to_file("/proc/self/uid_map", "0 %s 1\n", uid_str
		                  ) || pl_fatal("write /proc/self/uid_map");
	}
	if (master_child_sinfo.si_status & SETUP_NO_GID){
		if (!pl_printf_to_file("/proc/self/setgroups", "deny")){
			/* ignore file does not exist, as this happens in older
			* kernels*/
			if (errno != ENOENT)
				pl_fatal("write /proc/self/setgroups");
		};
		pl_printf_to_file("/proc/self/gid_map", "0 %s 1\n", gid_str
		                  ) || pl_fatal("write /proc/self/gid_map");
	}
	free(uid_str);
	free(gid_str);
	free(pid_str);
}

char* pl_check_output(char* argv[]){
  int link[2];
  char static output[4096];

  if (pipe(link) == -1) pl_fatal("pipe");

  switch(fork()){
        case -1:
                 pl_fatal("fork");

        case 0:
                if (dup2(link[1], STDOUT_FILENO) == -1) pl_fatal("dup2");
                if (close(link[0]) ==               -1) pl_fatal("close");
                if (close(link[1]) ==               -1) pl_fatal("close");
                execvp(argv[0], argv);
                pl_fatal("exec");

        default:
                if (close(link[1]) == -1) pl_fatal("close");
                if(read(link[0], output, sizeof(output)) == -1)
                    pl_fatal("read");
                output[strcspn(output, "\n")] = 0;
                if (!output[0]) exit(1);
                return output;
    }
}


void pl_usage(){

        FILE *fp;
        char c,
	     *progc = NULL,
             *prog = realpath("/proc/self/exe", NULL);

        if (prog == NULL) pl_fatal("realpath");

	if (asprintf(&progc, "%s.c", prog) == -1)
		pl_fatal("asprintf");

	if ((fp = fopen(progc, "r")) == NULL)
	        pl_fatal("fopen");

        if (getc(fp) == EOF) pl_fatal("getc");
        if (getc(fp) == EOF) pl_fatal("getc");
        if (getc(fp) == EOF) pl_fatal("getc");
        while (c != '\n'){
                if ((c = getc(fp)) == EOF) pl_fatal("getc");
	        write(STDERR_FILENO, &c, 1);
        }

	exit(1);

}


void pl_setup_mount_ns(){
        if (unshare(CLONE_NEWNS) == -1)
            pl_fatal("could not unshare mount namespace");
        if (mount("none", "/", NULL, MS_REC|MS_PRIVATE, NULL) == -1){
                if (errno != EINVAL)
                    pl_fatal("could not change propagation of /");
                errno = 0;
        }
}


void pl_unshare_user(){
    char *env = getenv("PLASH_NO_UNSHARE");
    if (env && env[0] != '\0') return;

    if (getuid()) pl_setup_user_ns();
}

void pl_unshare_mount(){
    char *env = getenv("PLASH_NO_UNSHARE");
    if (env && env[0] != '\0') return;

    pl_setup_mount_ns();
}
