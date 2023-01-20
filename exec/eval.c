// usage: plash eval --macro1 arg1 arg2 --macro2 arg1 ...
// Generates a build script. It prints the shell script generated from
// evaluating the macros passed as args. `plash build` passes its arguments to
// this script in order to get a shell script with the build instructions.

#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <plash.h>

#define QUOTE_REPLACE "'\"'\"'"
#define eval_with_args(...) eval_with_args_array((char *[]){__VA_ARGS__, NULL})
#define HELP                                                                   \
"--hint               write a hint for consumer programs\n"\
"--entrypoint         hint default command for this build\n"\
"--entrypoint-script  write lines to /entrypoint and hint it as default command\n"\
"--env                Use env from host when running image\n"\
"--env-prefix         Use all envs with given prefix when running image\n"\
"--eval-file          evaluate file content as expressions\n"\
"--eval-github        eval a file (default 'plashfile') from a github repo\n"\
"--eval-stdin         evaluate expressions read from stdin\n"\
"--hash-path          recursively hash files and add as cache key\n"\
"--import-env         import environment variables from host while building\n"\
"--invalidate-layer   invalidate the cache of the current layer\n"\
"--layer              hints the start of a new layer\n"\
"--mount              Mount filesystem when running image (\"src:dst\" supported)\n"\
"--run                directly emit shell script\n"\
"--run-stdin          run commands read from stdin\n"\
"--write-file         write lines to a file\n"\
"--write-script       write an executable (755) file to the filesystem\n"\
"--from               guess from where to take the image\n"\
"--from-docker        use image from local docker\n"\
"--from-github        build and use a file (default 'plashfile') from a github repo\n"\
"--from-id            specify the image from an image id\n"\
"--from-lxc           use images from images.linuxcontainers.org\n"\
"--from-map           use resolved map as image\n"\
"--from-url           import image from an url\n"\
"--apk                install packages with apk\n"\
"--apt                install packages with apt\n"\
"--defpm              define a new package manager\n"\
"--dnf                install packages with dnf\n"\
"--emerge             install packages with emerge\n"\
"--npm                install packages with npm\n"\
"--pacman             install packages with pacman\n"\
"--pip                install packages with pip\n"\
"--pip3               install packages with pip3\n"\
"--yum                install packages with yum\n"\
"--A                  alias for: --from alpine:edge --apk [ARG1 [ARG2 [...]]]\n"\
"--C                  alias for: --from centos:8 --yum [ARG1 [ARG2 [...]]]\n"\
"--D                  alias for: --from debian:sid --apt [ARG1 [ARG2 [...]]]\n"\
"--F                  alias for: --from fedora:32 --dnf [ARG1 [ARG2 [...]]]\n"\
"--G                  alias for: --from gentoo:current --emerge [ARG1 [ARG2 [...]]]\n"\
"--R                  alias for: --from archlinux:current --pacman [ARG1 [ARG2 [...]]]\n"\
"--U                  alias for: --from ubuntu:focal --apt [ARG1 [ARG2 [...]]]\n"\
"--f                  alias for: --from [ARG1 [ARG2 [...]]]\n"\
"--l                  alias for: --layer [ARG1 [ARG2 [...]]]\n"\
"--x                  alias for: --run [ARG1 [ARG2 [...]]]\n"

static char **tokens, *arg;

char *quote(char *str) {
  size_t i, j, quotes_found = 0, quoted_counter = 0;
  for (i = 0; str[i]; i++) {
    if (str[i] == '\'')
      quotes_found++;
  }
  char *quoted = malloc((quotes_found * strlen(QUOTE_REPLACE) + strlen(str)) +
                        2   // for the enclousing quotes
                        + 1 // for the string terminator
  );
  quoted[quoted_counter++] = '\'';
  for (i = 0; str[i]; i++) {
    if (str[i] == '\'') {
      // append the string QUOTE_REPLACE
      for (j = 0; QUOTE_REPLACE[j]; j++)
        quoted[quoted_counter++] = QUOTE_REPLACE[j];
    } else {
      quoted[quoted_counter++] = str[i];
    }
  }
  quoted[quoted_counter++] = '\'';
  quoted[quoted_counter++] = '\0';

  return quoted;
}

char *call_cached(char *subcommand, char *arg) {
  char *cache_key, *image_id;
  asprintf(&cache_key, "%s:%s", subcommand, arg) != -1 || pl_fatal("asprintf");

  for (size_t i = 0; cache_key[i]; i++) {
    if (cache_key[i] == '/')
      cache_key[i] = '%';
  }

  image_id = pl_call("map", cache_key);
  if (strlen(image_id) == 0) {
    image_id = pl_call(subcommand, arg);
    pl_call("map", cache_key, image_id);
  }
  return image_id;
}

int isarg(char *val) {
  if (val == NULL)
    return 0;
  if (val[0] == '-')
    return 0;
  return 1;
}

char *getarg_or_null() {
  if (isarg(*(tokens + 1))) {
    tokens++;
    arg = *tokens;
    return arg;
  } else {
    return NULL;
  }
}

char *getarg() {
  char *arg = getarg_or_null();
  if (!arg) {
    while ((!isarg(arg)))
      pl_fatal("missing arg for: %s", *tokens);
  }
  return arg;
}

size_t countargs() {
  size_t argscount = 0;
  char **orig_tokens = tokens;
  while (getarg_or_null())
    argscount++;
  tokens = orig_tokens;
  return argscount;
}

void printarg(char *fmt) { printf(fmt, quote(arg)); }

void eachline(char *fmt) {
  while (getarg_or_null())
    printarg(fmt);
}

void pkg(char *cmd_prefix) {
  if (!isarg(*(tokens + 1))) {
    return;
  }
  printf("%s", cmd_prefix);
  while (getarg_or_null())
    printf(" %s", quote(arg));
  printf("\n");
}

void printhint(char *name, char *val) {
  if (val != NULL) {
    printf("### plash hint: %s=%s\n", name, val);
  } else {
    printf("### plash hint: %s\n", name);
  }
}

int tokenis(char *macro) { return (strcmp(*tokens, macro) == 0); }

void eval_with_args_array(char **middel_args) {

  size_t pre_args_len = 0;
  while (middel_args[pre_args_len])
    pre_args_len++;

  char *args[countargs() + pre_args_len +
             2   // for the elements "plash" and "eval" (see below)
             + 1 // for the NULL terminator
  ];

  size_t index = 0;
  args[index++] = "plash";
  args[index++] = "eval";
  for (int j = 0; middel_args[j]; j++) {
    args[index++] = middel_args[j];
  }
  while (getarg_or_null()) {
    args[index++] = arg;
  }
  args[index++] = NULL;
  _pl_run(args);
}

int eval_main(int argc, char *argv[]) {

  // handle help flag
  if (argv[1] && (strcmp(argv[1], "--help")) == 0){
      fprintf(stderr, "%s", HELP);
      return 0;
  }

  tokens = argv;
  while (*(++tokens)) {
    arg = NULL;

    if (tokenis("--from") || tokenis("-f")) {
      getarg();
      int i, only_digits = 1;
      for (i = 0; arg[i]; i++) {
        if (!isdigit(arg[i]))
          only_digits = 0;
      }
      if (only_digits) {
        pl_run("plash", "eval", "--from-id", arg);
      } else {
        pl_run("plash", "eval", "--from-lxc", arg);
      }

    } else if (tokenis("--hint")) {
      printhint(getarg(), getarg_or_null());

    } else if (tokenis("--from-id")) {
      printhint("image", getarg());

    } else if (tokenis("--from-docker")) {
      printhint("image", call_cached("import-docker", getarg()));

    } else if (tokenis("--from-url")) {
      printhint("image", call_cached("import-url", getarg()));

    } else if (tokenis("--from-map")) {
      char *image_id = pl_call("map", getarg());
      if (image_id[0] == '\0') {
        pl_fatal("No such map: %s", arg);
      }
      printhint("image", image_id);

    } else if (tokenis("--entrypoint-script")) {
      pl_run("plash", "eval", "--entrypoint", "/entrypoint");
      eval_with_args("--write-script", "/entrypoint");

    } else if (tokenis("--write-file")) {
      char *filename = getarg();
      printf("touch %s\n", quote(filename));
      while (getarg_or_null())
        printf("echo %s >> %s\n", quote(arg), quote(filename));

    } else if (tokenis("--write-script")) {
      char *script = getarg();
      eval_with_args("--write-file", arg);
      printf("chmod 755 %s\n", quote(script));

    } else if (tokenis("--eval-url")) {
      pl_pipe(
          (char *[]){"curl", "--fail", "--no-progress-meter", getarg(), NULL},
          (char *[]){"plash", "eval-plashfile", NULL});

    } else if (tokenis("--eval-github")) {
      char *url, *user_repo_pair, *file;
      user_repo_pair = getarg();
      if (strchr(user_repo_pair, '/') == NULL)
        pl_fatal("--eval-github: user-repo-pair must include a slash (got %s)",
                 user_repo_pair);
      if (!(file = getarg_or_null()))
        file = "plashfile";
      asprintf(&url, "https://raw.githubusercontent.com/%s/master/%s",
               user_repo_pair, file) != -1 ||
          pl_fatal("asprintf");
      pl_pipe((char *[]){"curl", "--fail", "--no-progress-meter", url, NULL},
              (char *[]){"plash", "eval-plashfile", NULL});

    } else if (tokenis("--hash-path")) {
      while (getarg_or_null()) {
        printf(": hash-path ");
        fflush(stdout);
        pl_pipe((char *[]){"tar", "-c", arg, NULL},
                (char *[]){"sha512sum", NULL});
      }

    } else if (tokenis("--entrypoint")) {
      printf("echo -n 'exec ' >> /.plashentrypoint\n");
      printf("chmod +x /.plashentrypoint\n");
      while (getarg_or_null()){
	printf("echo -n %s ' ' >> /.plashentrypoint\n", quote(quote(arg)));
      }
      printf("echo ' \"$@\"' >> /.plashentrypoint\n");

    } else if (tokenis("--env")) {
      eachline("echo %s >> /.plashenvs\n");

    } else if (tokenis("--env-prefix")) {
      eachline("echo %s >> /.plashenvsprefix\n");

    } else if (tokenis("--mount")) {
      eachline("echo %s >> /.plashmount\n");

    } else if (tokenis("--eval-file")) {
      pl_run("plash", "eval-plashfile", getarg());

    } else if (tokenis("--eval-stdin")) {
      pl_run("plash", "eval-plashfile");

    } else if (tokenis("--from-lxc")) {
      printhint("image", call_cached("import-lxc", getarg()));

    } else if (tokenis("--run-stdin")) {
      int c;
      while ((c = getchar()) != EOF)
        putchar(c);

    } else if (tokenis("--from-url")) {
      printhint("image", getarg());

    } else if (tokenis("--import-env")) {
      char *env, *export_as, *env_val;
      while (getarg_or_null()) {
        env = strtok(arg, ":");
        export_as = strtok(NULL, ":");
        if (export_as == NULL) {
          export_as = env;
        }
        char *env_val = getenv(env);
        if (env_val != NULL) {
          // export_as is not escaped!
          printf("%s=%s\n", export_as, quote((env_val)));
        }
      }
    } else if (tokenis("--invalidate-layer")) {
      struct timespec tp;
      clock_gettime(CLOCK_MONOTONIC, &tp);
      printf(": invalidate cache with %ld\n", tp.tv_nsec);
    } else if (tokenis("--layer") || tokenis("-l")) {
      printhint("layer", NULL);

    } else if (tokenis("--run") || tokenis("-x")) {
      while (getarg_or_null())
        printf("%s\n", arg);

    } else if (tokenis("--apk")) {
      pkg("apk update\napk add");

    } else if (tokenis("--apt")) {
      pkg("apt-get update\napt-get install -y");

    } else if (tokenis("--dnf")) {
      pkg("dnf install -y");

    } else if (tokenis("--emerge")) {
      pkg("emerge");

    } else if (tokenis("--npm")) {
      pkg("npm install -g");

    } else if (tokenis("--pacman")) {
      pkg("pacman -Sy --noconfirm");

    } else if (tokenis("--pip")) {
      pkg("pip install");

    } else if (tokenis("--pip3")) {
      pkg("pip3 install");

    } else if (tokenis("--yum")) {
      pkg("yum install -y");

    } else if (tokenis("-A")) {
      eval_with_args("--from", "alpine:edge", "--apk");

    } else if (tokenis("-U")) {
      eval_with_args("--from", "ubuntu:jammy", "--apt");

    } else if (tokenis("-F")) {
      eval_with_args("--from", "fedora:37", "--dnf");

    } else if (tokenis("-D")) {
      eval_with_args("--from", "debian:sid", "--apt");

    } else if (tokenis("-C")) {
      eval_with_args("--from", "centos:9-Stream", "--yum");

    } else if (tokenis("-R")) {
      eval_with_args("--from", "archlinux:current", "--pacman");

    } else if (tokenis("-G")) {
      eval_with_args("--from", "gentoo:current", "--emerge");

    } else if (tokenis("--#") || tokenis("-#")) {
      while (getarg_or_null())
        ;

    } else if (isarg(*tokens)) {
      errno = 0;
      pl_fatal("expected macro, got value: %s", *tokens);
    } else {
      errno = 0;
      pl_fatal("unknown macro: %s", *tokens);
    }

    // Somehow because we are calling subprocesses we need this to ensure the
    //  order of lines written to stdout stays in the expected order.
    fflush(stdout);
    return 0;
  }
}
