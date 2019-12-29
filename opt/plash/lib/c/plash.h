int pl_fatal(char *format, ...);

char *pl_path(const char *relpath);

void pl_whitelist_env(char *env_name);

void pl_setup_user_ns();

void pl_setup_mount_ns();

char *pl_check_output(char *argv[]);

char *pl_check_call(char *argv[]);

void pl_usage();

void pl_unshare_mount();

void pl_unshare_user();

void pl_whitelist_envs_from_env(const char *export_env);

void pl_bind_mount(const char *src, const char *dst);

void pl_chdir(const char *newdir);

void pl_chroot(const char *rootfs);

char *pl_mkdtemp();

void pl_handle_build_args(int *argc, char *argv[]);
