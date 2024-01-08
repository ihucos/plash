#include <stdio.h>
#include <sys/types.h>

#define pl_call(...)                                                           \
  pl_firstline(pl_check_output((char *[]){"/proc/self/exe", __VA_ARGS__, NULL}))
#define pl_run(...) _pl_run((char *[]){__VA_ARGS__, NULL})

char *pl_cmd_array(int (*main_func)(int, char *[]), char *args[]);

char *pl_check_output(char *argv[]);

char *pl_firstline(char *str);

int pl_fatal(char *format, ...);

char *pl_path(const char *relpath);

void pl_whitelist_env(char *env_name);

void pl_whitelist_env_prefix(char *env_name);

void pl_setup_user_ns();

void pl_setup_mount_ns();

void pl_usage();

void pl_unshare_mount();

void pl_unshare_user();

void pl_whitelist_envs_from_env(const char *export_env);

void pl_bind_mount(const char *src, const char *dst);

void pl_chdir(const char *newdir);

void pl_chroot(const char *rootfs);

void pl_exec_add(char *arg);

char *pl_pipe(char *const program1[], char *const program2[]);

void _pl_run(char *program[]);

char *pl_get_default_root_shell();

pid_t pl_spawn_process(char **cmd, FILE **p_stdin, FILE **p_stdout,
                       FILE **p_stderr);

char *pl_nextline(FILE *fh);

// all the mains
int version_main(int argc, char *argv[]);
int cached_main(int argc, char *argv[]);
int clean_main(int argc, char *argv[]);
int create_main(int argc, char *argv[]);
int data_main(int argc, char *argv[]);
int help_main(int argc, char *argv[]);
int init_main(int argc, char *argv[]);
int main_main(int argc, char *argv[]);
int map_main(int argc, char *argv[]);
int mkdtemp_main(int argc, char *argv[]);
int mount_main(int argc, char *argv[]);
int mounted_main(int argc, char *argv[]);
int nodepath_main(int argc, char *argv[]);
int noid_main(int argc, char *argv[]);
int pull_docker_main(int argc, char *argv[]);
int pull_lxc_main(int argc, char *argv[]);
int pull_tarfile_main(int argc, char *argv[]);
int pull_url_main(int argc, char *argv[]);
int purge_main(int argc, char *argv[]);
int push_dir_main(int argc, char *argv[]);
int push_tarfile_main(int argc, char *argv[]);
int rm_main(int argc, char *argv[]);
int run_main(int argc, char *argv[]);
int runb_main(int argc, char *argv[]);
int shrink_main(int argc, char *argv[]);
int stack_main(int argc, char *argv[]);
int sudo_main(int argc, char *argv[]);
int version_main(int argc, char *argv[]);
int parent_main(int argc, char *argv[]);
