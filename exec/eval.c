
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
#define eachargs while (isvalorprev(*(++tokens)))
#define commandsbegin                                                          \
  int main(int argc, char *argv[]) {                                           \
    tokens = argv;                                                             \
    while (*(++tokens)) {                                                      \
      if (0) {

#define command(macro)                                                         \
  }                                                                            \
  else if (strcmp(current, macro) == 0) {

#define commandsend                                                            \
  }                                                                            \
  else {                                                                       \
    errno = 0;                                                                 \
    pl_fatal("unknown macro: %s", current);                                    \
  }                                                                            \
  }                                                                            \
  }

static char **tokens;

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

char *next() {
  tokens++;
  if (!isval(*tokens)) {
    tokens--;
    while ((!isval(*tokens)))
      pl_fatal("missing arg for: %s", *tokens);
  }
  return *tokens;
}

int isvalorprev(char *val) {
  if (isval(val)) {
    return 1;
  } else {
    tokens--;
    return 0;
  }
}

void eachline(char *fmt) { eachargs printf(fmt, quote(current)); }

void pkg(char *cmd_prefix) {
  printf("%s", cmd_prefix);
  eachargs printf(" %s", quote(current));
  printf("\n");
}

void printint(char *name, char *val) {
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

commandsbegin;

command("-x") eachargs printf("%s", current);

command("--layer") printint("layer", NULL);

command("--write-file") {
  char *filename = next();
  printf("touch %s\n", quote(filename));
  eachargs printf("echo %s >> %s\n", quote(current), quote(filename));
}
command("--env") eachline("echo %s >> /.plashenvs\n");
command("--env-prefix") eachline("echo %s >> /.plashenvsprefix\n");
command("--from-lxc") printint("image", pl_call_cached("import-lxc", next()));
command("--from") {
  next();

  puts("meh");
  int i, only_digits = 1;
  for (i = 0; current[i]; i++) {
    if (!isdigit(current[i]))
      only_digits = 0;
  }

  if (only_digits) {
    execvp(argv[0], (char *[]){argv[0], "--from-id", current, NULL});
  } else {
    execvp(argv[0], (char *[]){argv[0], "--from-lxc", current, NULL});
  }
  puts(argv[0]);
  pl_fatal("execvp");
}
command("--from-docker") {
  printint("image", pl_call_cached("import-docker", next()));
}
command("--from-url") {
  printint("image", pl_call_cached("import-url", next()));
}
command("--from-map") {
  char *image_id = pl_call("map", next());
  if (image_id[0] == '\0') {
    pl_fatal("No such map: %s", current);
  }
  printint("image", image_id);
}
command("--from-url") printint("image", next());
command("--entrypoint") printint("exec", next());

command("--entrypoint-script") {
  printint("exec", "/entrypoint");
  printf("touch /entrypoint\n");
  printf("chmod 755 /entrypoint\n");
  eachline("echo %s >> /entrypoint\n");

  // package managers
}
command("--apt") pkg("apt-get update\napt-get install -y");
command("--apk") pkg("apk update\napk add");
command("--yum") pkg("yum install -y");
command("--dnf") pkg("dnf install -y");
command("--pip") pkg("pip install");
command("--pip3") pkg("pip3 install");
command("--npm") pkg("npm install -g");
command("--pacman") pkg("pacman -Sy --noconfirm");
command("--emerge") pkg("emerge");
command("--eval-url") {
  pl_pipe((char *[]){"curl", "--fail", "--no-progress-meter", next(), NULL},
          (char *[]){"plash", "eval-plashfile", NULL});
}
command("--eval-file")
    pl_run((char *[]){"plash", "eval-plashfile", next(), NULL});

command("--eval-stdin") pl_run((char *[]){"plash", "eval-plashfile", NULL});
command("--eval-github") {
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
}
command("--hash-path") {
  eachargs {
    printf(": hash-path ");
    fflush(stdout);
    pl_pipe((char *[]){"tar", "-c", current, NULL},
            (char *[]){"sha512sum", NULL});
    sleep(1);
  }
}
command("--import-env") { puts(quote(next())); }

commandsend
