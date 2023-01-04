
#define _GNU_SOURCE
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>

#include <plash.h>

#define QUOTE_REPLACE "'\"'\"'"

#define NEXT (*(++tokens))
#define CURRENT (*tokens)
#define EACHARGS while(isval(NEXT))
#define CASE(macro) } else if (strcmp(CURRENT, macro) == 0) {

static char **tokens;

void line(char *format, ...) {
  va_list args;
  va_start(args, format);
  va_end(args);
  asprintf(&format, "%s\n", format) != -1 || pl_fatal("asprintf");
  vprintf(format, args);
}


char *quote(char *str){

    size_t i,
           j,
           quotes_found = 0,
           quoted_counter = 0;

    for (i = 0; str[i]; i++){
        if (str[i] == '\'') quotes_found++;
    }

    char *quoted = malloc(
            (quotes_found * strlen(QUOTE_REPLACE) + strlen(str))
            + 2 // for the enclousing quotes
            + 1 // for the string terminator
            );

    quoted[quoted_counter++] = '\'';
    for (i = 0; str[i]; i++){
        if (str[i] == '\''){
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


void linecurrent(char *format) {
    line(format, quote(CURRENT));
}

int isval(char *val){
    if (val == NULL) return 0;
    if (val[0] == '-') return 0;
    return 1;
}


void eachline(char *arg) {
    EACHARGS linecurrent(arg);
}


char *assert_isval(char *val){
    if (!isval(val)) pl_fatal("argument for macro is missing");
    return val;
}


size_t countvals(){
    char **tokens_copy = tokens;
    while(isval(*(++tokens_copy)));
    return tokens_copy - tokens - 1;
}


void args(int argscount){
    if (countvals() != argscount)
        pl_fatal("macro %s needs exactly %ld args", *tokens, argscount);
}


void argsmin(int min){
    if (countvals() < min)
        pl_fatal("macro %s needs at least %ld args", *tokens, min);
}


void PKG(char *cmd_prefix) {
    argsmin(1); printf("%s", cmd_prefix);
    EACHARGS printf(" %s", quote(CURRENT));
    printf("\n");
}


void SHELL(char *shell_cmd){
    int child_exit;
    if (system(NULL) == 0)
        pl_fatal("No shell is available in the system");
    int status = system(shell_cmd);
    if(status == -1)
        pl_fatal("system(%s) returned %d", shell_cmd, status);
    if (!WIFEXITED(status))
        pl_fatal("system(%s) exited abnormally", shell_cmd);
    if (child_exit = WEXITSTATUS(status))
        pl_fatal("%s: exited status %d", shell_cmd, child_exit);

}

void hint(char *name, char *val){
    if (val != NULL){
        line("### plash hint: %s=%s", name, val);
    } else {
        line("### plash hint: %s", name);
    }
}


char *pl_call_cached(char *subcommand, char *arg){
    char *cache_key, *image_id;
    asprintf(&cache_key, "lxc:%s", /*subcommand,*/ arg) != -1 || pl_fatal("asprintf");

    for (size_t i = 0; cache_key[i]; i++) {
        if (cache_key[i] == '/') cache_key[i] = '%';
    }

    image_id = pl_call("map", cache_key);
    if (strlen(image_id) == 0){
        image_id = pl_call(subcommand, arg);
        pl_call("map", cache_key, image_id);
    }
    return image_id;
}


int main(int argc, char *argv[]) {
    tokens = argv;
    NEXT;
    while (CURRENT){
        printf("X %s\n", CURRENT);

        // TODO
        // a little magic: remove possible shebang
        //if lines and lines[0].startswith("#!/"):
        //    lines.pop(0)


        //CASE("-#")
        //    EACHARGS {};

        //CASE("--#")
        //    EACHARGS {};

        if (0){

        CASE("-x")
            EACHARGS line(CURRENT);

        CASE("--layer")
            args(0);
            hint("layer", NULL);
            NEXT;

        CASE("--write-file")
            argsmin(1);
            char *filename = NEXT;
            line("touch %s", quote(filename));
            EACHARGS line("echo %s >> %s", quote(CURRENT), quote(filename));
            
        CASE("--env")
            argsmin(1);
            eachline("echo %s >> /.plashenvs");

        CASE("--env-prefix")
            argsmin(1);
            eachline("echo %s >> /.plashenvsprefix");

        CASE("--from-lxc")
            args(1);
            hint("image", pl_call_cached("import-lxc", NEXT));
            NEXT;


        CASE("--from")
            NEXT;
            
            puts("meh");
            int i, only_digits = 1;
            for (i = 0; CURRENT[i]; i++){
                if (!isdigit(CURRENT[i])) only_digits = 0;
            }

            if (only_digits){
                execvp(argv[0], (char*[]){argv[0], "--from-id", CURRENT, NULL});
            } else {
                execvp(argv[0], (char*[]){argv[0], "--from-lxc", CURRENT, NULL});
            }
            puts(argv[0]);
            pl_fatal("execvp");

        CASE("--from-docker")
            args(1);
            hint("image", pl_call_cached("import-docker", NEXT));
            NEXT;

        CASE("--from-url")
            args(1);
            hint("image", pl_call_cached("import-url", NEXT));
            NEXT;

        CASE("--from-map")
            args(1);
            char *image_id = pl_call("map", NEXT);
            if (image_id[0] == '\0'){
                pl_fatal("No such map: %s", CURRENT);
            }
            hint("image", image_id);
            NEXT;

        CASE("--from-url")
            args(1);
            hint("image", NEXT);
            NEXT;

        CASE("--entrypoint")
            args(1);
            hint("exec", NEXT);
            NEXT;

        CASE("--entrypoint-script")
            argsmin(1);
            hint("exec", "/entrypoint");
            line("touch /entrypoint");
            line("chmod 755 /entrypoint");
            eachline("echo %s >> /entrypoint");

         // package managers
        CASE("--apt")
            PKG("apt-get update\napt-get install -y");
        CASE("--apk")
            PKG("apk update\napk add");
        CASE("--yum")
            PKG("yum install -y");
        CASE("--dnf")
            PKG("dnf install -y");
        CASE("--pip")
            PKG("pip install");
        CASE("--pip3")
            PKG("pip3 install");
        CASE("--npm")
            PKG("npm install -g");
        CASE("--pacman")
            PKG("pacman -Sy --noconfirm");
        CASE("--emerge")
            PKG("emerge");


        CASE("--eval-url")
            argsmin(1);
            pl_pipe(
                    (char*[]){"curl", "--fail", "--no-progress-meter", NEXT, NULL},
                    (char*[]){"plash", "eval-plashfile", NULL}
                   );
            NEXT;

        CASE("--eval-file")
            argsmin(1);
            pl_run((char*[]){"plash", "eval-plashfile", NEXT, NULL});
            NEXT;

        CASE("--eval-stdin")
            args(0);
            pl_run((char*[]){"plash", "eval-plashfile", NULL});
            NEXT;
            NEXT;

        CASE("--eval-github")
            char *url, *user_repo_pair, *file;
            user_repo_pair = NEXT;
            if (strchr(user_repo_pair, '/') == NULL)
                pl_fatal("--eval-github: user-repo-pair must include a slash (got %s)", user_repo_pair);
            NEXT;
            if (!isval(CURRENT)){
                file = "plashfile";
            } else {
                file = CURRENT;
            }
            asprintf(&url, "https://raw.githubusercontent.com/%s/master/%s", user_repo_pair, file) != -1 || pl_fatal("asprintf");
            pl_pipe(
                    (char*[]){"curl", "--fail", "--no-progress-meter", url, NULL},
                    (char*[]){"plash", "eval-plashfile", NULL});
            NEXT;

        CASE("--hash-path")
            EACHARGS {
                printf(": hash-path ");
                fflush(stdout);
                pl_pipe(
                    (char*[]){"tar", "-c", CURRENT, NULL},
                    (char*[]){"sha512sum", NULL});
                sleep(1);
            }

        CASE("--import-env")
            puts(quote(NEXT)); NEXT;
            

        } else {
            errno = 0;
            pl_fatal("unknown macro: %s", CURRENT);
        }
    }
}
