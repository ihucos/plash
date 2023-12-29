// Generates a build script. It prints the shell script generated from
// evaluating the macros passed as args. `plash build` passes its arguments to
// this script in order to get a shell script with the build instructions.

#define USAGE "usage: plash eval --macro1 arg1 arg2 --macro2 arg1 ...\n"

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
#define HELP \
"@from {id,docker,url,lxc,map} <id>\n" \
"@include        Include a Plash script\n" \
"@file           Write to a file\n" \
"@script         Write an executable file\n" \
"@hash           Generate a hash of a file to use as a cache key\n" \
"@env            Import environment variables\n" \
"@polute         Invalidate the cache\n" \
"@layer          Create a new layer\n" \
"@pass           Used to end other macros\n"

#define CMD(cmd) } else if (strcmp(*tokens, cmd) == 0) {
#define CMDARG(cmd, arg) } else if ((strcmp(*tokens, cmd) == 0) && (strcmp(*(tokens+1), arg) == 0)) {tokens++;

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

int is_build_cmd(char *val) {
  if (val == NULL)
    return 1;
  if (val[0] == '@')
    return 1;
  return 0;
}

char *getarg_or_null() {
  if (!is_build_cmd(*(tokens + 1))) {
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
    while ((is_build_cmd(arg)))
      pl_fatal("missing arg for: %s", *(tokens-1));
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

void next_token(){
    if (0){

    CMDARG("@from", "id")
      printf("@from-id %s\n", getarg());

    /* CMD("@from") */
    /*   getarg(); */
    /*   int i, only_digits = 1; */
    /*   for (i = 0; arg[i]; i++) { */
    /*     if (!isdigit(arg[i])) */
    /*       only_digits = 0; */
    /*   } */
    /*   if (only_digits) { */
    /*     pl_run("/proc/self/exe", "eval", "@from-id", arg); */
    /*   } else { */
    /*     pl_run("/proc/self/exe", "eval", "@from-lxc", arg); */
    /*   } */


    CMDARG("@from", "docker")
      printf("@from-id %s\n", call_cached("import-docker", getarg()));

    CMDARG("@from", "url")
      printf("@from-id %s\n", call_cached("import-url", getarg()));

    CMDARG("@from", "lxc")
      printf("@from-id %s\n", call_cached("import-lxc", getarg()));

    CMD("@from")
	    pl_fatal("@from: second arg unknown: %s", getarg());

    CMD("@from-map")
      char *image_id = pl_call("map", getarg());
      if (image_id[0] == '\0') {
        pl_fatal("No such map: %s", arg);
      }
      printf("@from-id %s\n", image_id);

    /* CMD("@include-github") */
    /*   char *url, *user_repo_pair, *file; */
    /*   user_repo_pair = getarg(); */
    /*   if (strchr(user_repo_pair, '/') == NULL) */
    /*     pl_fatal("--eval-github: user-repo-pair must include a slash (got %s)", */
    /*              user_repo_pair); */
    /*   if (!(file = getarg_or_null())) */
    /*     file = "plashfile"; */
    /*   asprintf(&url, "https://raw.githubusercontent.com/%s/master/%s", */
    /*            user_repo_pair, file) != -1 || */
    /*       pl_fatal("asprintf"); */
    /*   pl_pipe((char *[]){"curl", "--fail", "--no-progress-meter", url, NULL}, */
    /*           (char *[]){"/proc/self/exe", "eval-plashfile", NULL}); */

    CMD("@include")
      char *url = getarg();
      if (url[0] == '/' || (url[0] == '.' && url[1] == '/')) {
        pl_run("/proc/self/exe", "eval-plashfile", url);
      } else {
        pl_pipe(
            (char *[]){"curl", "--fail", "--no-progress-meter", url, NULL},
            (char *[]){"/proc/self/exe", "eval-plashfile", NULL});
      }


    CMD("@file")
      char *filename = getarg();
      printf("touch %s\n", quote(filename));
      while (getarg_or_null())
        printf("echo %s >> %s\n", quote(arg), quote(filename));

    CMD("@script")
      char *filename = getarg();
      printf("touch %s\n", quote(filename));
      while (getarg_or_null())
        printf("echo %s >> %s\n", quote(arg), quote(filename));
      printf("chmod 755 %s\n", quote(filename));

    CMD("@hash")
      while (getarg_or_null()) {
        printf(": hash %s", arg);
        fflush(stdout);
        pl_pipe((char *[]){"tar", "-c", arg, NULL},
                (char *[]){"sha512sum", NULL});
      }

    CMD("@env")
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

    CMD("@polute")
      struct timespec tp;
      clock_gettime(CLOCK_MONOTONIC, &tp);
      printf(": invalidate cache with %ld\n", tp.tv_nsec);

    CMD("@layer")
      printf("@layer\n");

    CMD("@pass")
      

    CMD("") // ignore newlines
 
    } else if (!is_build_cmd(*tokens)) {
        printf("%s\n", *tokens);  
    } else {
      errno = 0;
      pl_fatal("unknown macro: %s", *tokens);
    }


}

int eval_main(int argc, char *argv[]) {

  // handle help flag
  if (argv[1] && (strcmp(argv[1], "--help")) == 0) {
    fprintf(stderr, "%s", HELP);
    return EXIT_SUCCESS;
  }

  tokens = argv;
  while (*(++tokens)) {
    arg = NULL;

    next_token();

    // Somehow because we are calling subprocesses we need this to ensure the
    //  order of lines written to stdout stays in the expected order.
    fflush(stdout);
  }
  return EXIT_SUCCESS;
}
