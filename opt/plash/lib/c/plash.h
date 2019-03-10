int pl_fatal(char *format, ...);

char* pl_path(const char *relpath);

void pl_whitelist_env(char *env_name);

void pl_setup_user_ns();

char* pl_check_output(char* argv[]);
