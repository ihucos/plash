
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

#define next() (*(++tokens))
#define current (*tokens)
#define eachargs while (isvalorprev(next()))
#define commandsbegin                                                          \
  int main(int argc, char *argv[]) {                                           \
    tokens = argv;                                                             \
    while (next()) {                                                          \
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

void line(char *format, ...) {
  va_list args;
  va_start(args, format);
  va_end(args);
  asprintf(&format, "%s\n", format) != -1 || pl_fatal("asprintf");
  vprintf(format, args);
}

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

void linecurrent(char *format) { line(format, quote(current)); }

int isval(char *val) {
  if (val == NULL)
    return 0;
  if (val[0] == '-')
    return 0;
  return 1;
}

int isvalorprev(char *val) {
  if (isval(val)) {
    return 1;
  } else {
    tokens--;
    return 0;
  }
}

void eachline(char *arg) { eachargs linecurrent(arg); }

char *assert_isval(char *val) {
  if (!isval(val))
    pl_fatal("argument for macro is missing");
  return val;
}

size_t countvals() {
  char **tokens_copy = tokens;
  while (isval(*(++tokens_copy)))
    ;
  return tokens_copy - tokens - 1;
}

void args(int argscount) {
  if (countvals() != argscount)
    pl_fatal("macro %s needs exactly %ld args", *tokens, argscount);
}

void argsmin(int min) {
  if (countvals() < min)
    pl_fatal("macro %s needs at least %ld args", *tokens, min);
}

void pkg(char *cmd_prefix) {
  argsmin(1);
  printf("%s", cmd_prefix);
  eachargs printf(" %s", quote(current));
  printf("\n");
}

void SHELL(char *shell_cmd) {
  int child_exit;
  if (system(NULL) == 0)
    pl_fatal("No shell is available in the system");
  int status = system(shell_cmd);
  if (status == -1)
    pl_fatal("system(%s) returned %d", shell_cmd, status);
  if (!WIFEXITED(status))
    pl_fatal("system(%s) exited abnormally", shell_cmd);
  if (child_exit = WEXITSTATUS(status))
    pl_fatal("%s: exited status %d", shell_cmd, child_exit);
}

void hint(char *name, char *val) {
  if (val != NULL) {
    line("### plash hint: %s=%s", name, val);
  } else {
    line("### plash hint: %s", name);
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

commandsbegin

command("-x") {
  eachargs line(current);
}

command("--layer") {
  args(0);
  hint("layer", NULL);
}

command("--write-file") {
  argsmin(1);
  char *filename = next();
  line("touch %s", quote(filename));
  eachargs line("echo %s >> %s", quote(current), quote(filename));
}
command("--env") {
  argsmin(1);
  eachline("echo %s >> /.plashenvs");
}
command("--env-prefix") {
  argsmin(1);
  eachline("echo %s >> /.plashenvsprefix");
}
command("--from-lxc") {
  args(1);
  hint("image", pl_call_cached("import-lxc", next()));
}
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
  args(1);
  hint("image", pl_call_cached("import-docker", next()));
}
command("--from-url") {
  args(1);
  hint("image", pl_call_cached("import-url", next()));
}
command("--from-map") {
  args(1);
  char *image_id = pl_call("map", next());
  if (image_id[0] == '\0') {
    pl_fatal("No such map: %s", current);
  }
  hint("image", image_id);
}
command("--from-url") {
  args(1);
  hint("image", next());
}
command("--entrypoint") {
  args(1);
  hint("exec", next());
}

command("--entrypoint-script") {
  argsmin(1);
  hint("exec", "/entrypoint");
  line("touch /entrypoint");
  line("chmod 755 /entrypoint");
  eachline("echo %s >> /entrypoint");

  // package managers
}
command("--apt") { pkg("apt-get update\napt-get install -y"); }
command("--apk") { pkg("apk update\napk add"); }
command("--yum") { pkg("yum install -y"); }
command("--dnf") { pkg("dnf install -y"); }
command("--pip") { pkg("pip install"); }
command("--pip3") { pkg("pip3 install"); }
command("--npm") { pkg("npm install -g"); }
command("--pacman") { pkg("pacman -Sy --noconfirm"); }
command("--emerge") { pkg("emerge"); }
command("--eval-url") {
  argsmin(1);
  pl_pipe((char *[]){"curl", "--fail", "--no-progress-meter", next(), NULL},
          (char *[]){"plash", "eval-plashfile", NULL});
}
command("--eval-file") {
  argsmin(1);
  pl_run((char *[]){"plash", "eval-plashfile", next(), NULL});
}

command("--eval-stdin") {
  args(0);
  pl_run((char *[]){"plash", "eval-plashfile", NULL});
  next();
}
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
