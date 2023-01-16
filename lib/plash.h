#include <stdio.h>
#include <sys/types.h>

#define pl_call(...) pl_firstline(pl_check_output((char *[]){"plash", __VA_ARGS__, NULL}))
#define pl_run(...) _pl_run((char *[]){__VA_ARGS__, NULL})

char *pl_check_output(char *argv[]);

char *pl_check_output2(char *pathname, char *argv[]);

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

pid_t pl_spawn_process(char **cmd, FILE **p_stdin, FILE **p_stdout, FILE **p_stderr);
