
#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <plash.h>

#define QUOTE_REPLACE "'\"'\"'"

#define current (*tokens)

static char **tokens;

int tokenis(char *macro) { return (strcmp(current, macro) == 0); }

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

int isval(char *val) {
  if (val == NULL)
    return 0;
  if (val[0] == '-')
    return 0;
  return 1;
}

char *next_or_null() {
  if (isval(*(tokens + 1))) {
    tokens++;
    return *tokens;
  } else {
    return NULL;
  }
}

char *next() {
  char *next = next_or_null();
  if (!next) {
    while ((!isval(next)))
      pl_fatal("missing arg for: %s", *tokens);
  }
  return next;
}

void eachline(char *fmt) {
  while (next_or_null())
    printf(fmt, quote(current));
}

void pkg(char *cmd_prefix) {
  printf("%s", cmd_prefix);
  while (next_or_null())
    printf(" %s", quote(current));
  printf("\n");
}

void printhint(char *name, char *val) {
  if (val != NULL) {
    printf("### plash hint: %s=%s\n", name, val);
  } else {
    printf("### plash hint: %s\n", name);
  }
}

char *pl_call_cached(char *subcommand, char *arg) {
  char *cache_key, *image_id;
  asprintf(&cache_key, "lxc:%s", /*subcommand,*/ arg) != -1 ||
      pl_fatal("asprintf");

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

int main(int argc, char *argv[]) {
  tokens = argv;
  while (*(++tokens)) {

    if (tokenis("--write-file")) {
      next();
      printf("touch %s\n", quote(current));
      while (next_or_null())
        printf("echo %s >> %s\n", quote(current), quote(current));
      while (next_or_null())
        printf("echo %s >> %s\n", quote(current), quote(current));

    } else if (tokenis("--from") || tokenis("-f")) {
      next();
      int i, only_digits = 1;
      for (i = 0; current[i]; i++) {
        if (!isdigit(current[i]))
          only_digits = 0;
      }
      if (only_digits) {
        pl_run("plash", "eval", "--from-id", current);
      } else {
        pl_run("plash", "eval", "--from-lxc", current);
      }

    } else if (tokenis("--from-id")) {
      printhint("image", next());

    } else if (tokenis("--from-docker")) {
      printhint("image", pl_call_cached("import-docker", next()));

    } else if (tokenis("--from-url")) {
      printhint("image", pl_call_cached("import-url", next()));

    } else if (tokenis("--from-map")) {
      char *image_id = pl_call("map", next());
      if (image_id[0] == '\0') {
        pl_fatal("No such map: %s", current);
      }
      printhint("image", image_id);

    } else if (tokenis("--entrypoint-script")) {
      printhint("exec", "/entrypoint");
      printf("touch /entrypoint\n");
      printf("chmod 755 /entrypoint\n");
      eachline("echo %s >> /entrypoint\n");

    } else if (tokenis("--eval-url")) {
      pl_pipe((char *[]){"curl", "--fail", "--no-progress-meter", next(), NULL},
              (char *[]){"plash", "eval-plashfile", NULL});

    } else if (tokenis("--eval-github")) {
      char *url, *user_repo_pair, *file;
      user_repo_pair = next();
      if (strchr(user_repo_pair, '/') == NULL)
        pl_fatal("--eval-github: user-repo-pair must include a slash (got %s)",
                 user_repo_pair);
      next();
      if (!isval(current)) {
        file = "plashfile";
      } else {
        file = current;
      }
      asprintf(&url, "https://raw.githubusercontent.com/%s/master/%s",
               user_repo_pair, file) != -1 ||
          pl_fatal("asprintf");
      pl_pipe((char *[]){"curl", "--fail", "--no-progress-meter", url, NULL},
              (char *[]){"plash", "eval-plashfile", NULL});

    } else if (tokenis("--hash-path")) {
      while (next_or_null()) {
        printf(": hash-path ");
        fflush(stdout);
        pl_pipe((char *[]){"tar", "-c", current, NULL},
                (char *[]){"sha512sum", NULL});
      }
    } else if (tokenis("--entrypoint")) {
      printhint("exec", next());

    } else if (tokenis("--env")) {
      eachline("echo %s >> /.plashenvs\n");

    } else if (tokenis("--env-prefix")) {
      eachline("echo %s >> /.plashenvsprefix\n");

    } else if (tokenis("--eval-file")) {
      pl_run("plash", "eval-plashfile", next());

    } else if (tokenis("--eval-stdin")) {
      pl_run("plash", "eval-plashfile");

    } else if (tokenis("--from-lxc")) {
      printhint("image", pl_call_cached("import-lxc", next()));

    } else if (tokenis("--from-url")) {
      printhint("image", next());

    } else if (tokenis("--import-env")) {
      { puts(quote(next())); }

    } else if (tokenis("--layer")) {
      printhint("layer", NULL);

    } else if (tokenis("--run") || tokenis("-x")) {
      while (next_or_null())
        printf("%s", current);

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

    } else if (isval(current)){
      errno = 0;
      pl_fatal("expected macro, got value: %s", current);
    } else {
      errno = 0;
      pl_fatal("unknown macro: %s", current);
    }
  }
}
