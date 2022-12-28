
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <plash.h>

#define NEXT *(++argv)
#define NEXTVAL assert_isval(NEXT)
#define CURRENT *argv
#define EACHARGS while(isval(NEXT))

#define CASE(macro) } else if (strcmp(CURRENT, macro) == 0) {
#define ARGSMIN(min) if (count_vals(argv) < min) pl_fatal( \
        "macro %s needs at least %ld args", *argv, min)

#define ARGS(argscount) if (count_vals(argv) != argscount) pl_fatal( \
        "macro %s needs exactly %ld args", *argv, argscount)

#define LINECURRENT(format) LINE(format, quote(CURRENT))


#define EACHLINE(arg) EACHARGS LINECURRENT(arg) // this officially crazy

void LINE(char *format, ...) {
  va_list args;
  va_start(args, format);
  va_end(args);
  asprintf(&format, "%s\n", format) != -1 || pl_fatal("asprintf");
  vprintf(format, args);
}

int isval(char *val){
    if (val == NULL) return 0;
    if (val[0] == '-') return 0;
    return 1;
}

char *assert_isval(char *val){
    if (!isval(val)) pl_fatal("argument for macro is missing");
    return val;
}


size_t count_vals(char **argv){
    char **argvc = argv;
    EACHARGS {};
    return argv - argvc - 1;
}


char *PUTHINT(char *name, char *val){
    LINE("### plash hint: %s=%s", name, val);
}

char *quote(char *str){
    return str;
}


char *pl_call_cached(char *subcommand, char *arg){
    char *cache_key, *image_id;
    asprintf(&cache_key, "lxc:%s", /*subcommand,*/ arg) != -1 || pl_fatal("asprintf");
    image_id = pl_call("map", cache_key);
    if (strlen(image_id) == 0){
        image_id = pl_call(subcommand, arg);
        pl_call("map", cache_key, image_id);
    }
    return image_id;
}

int main(int argc, char *argv[]) {
    NEXT;
    while (CURRENT){
        if (0){

        CASE("-x")
            EACHARGS LINE(CURRENT);

        CASE("--write-file")
            ARGSMIN(1);
            char *filename = NEXT;
            LINE("touch %s", quote(filename));
            EACHARGS LINE("echo %s >> %s", quote(CURRENT), quote(filename));
            
        CASE("--env")
            ARGSMIN(1);
            EACHLINE("echo %s >> /.plashenvsprefix");

        CASE("--from")
            ARGS(1);
            PUTHINT("image", pl_call_cached("import-lxc", NEXT));
            NEXT;

        CASE("--entrypoint")
            ARGS(1);
            PUTHINT("exec", NEXT);
            NEXT;

        CASE("--entrypoint-script")
            ARGSMIN(1);
            PUTHINT("exec", "/entrypoint");
            LINE("touch /entrypoint");
            LINE("chmod 755 /entrypoint");
            EACHLINE("echo %s >> /entrypoint");

         // package managers
        CASE("--apk")
            ARGSMIN(1);
            LINE("apk update");
            EACHLINE("apk add %s");
        CASE("--yum")
            ARGSMIN(1);
            EACHLINE("yum install -y %s");
        CASE("--dnf")
            ARGSMIN(1);
            EACHLINE("dnf install -y %s");
        CASE("--pip")
            ARGSMIN(1);
            EACHLINE("pip install %s");
        CASE("--pip3")
            ARGSMIN(1);
            EACHLINE("pip3 install %s");
        CASE("--npm")
            ARGSMIN(1);
            EACHLINE("npm install -g %s");
        CASE("--pacman")
            ARGSMIN(1);
            EACHLINE("pacman -Sy --noconfirm %s");
        CASE("--emerge")
            ARGSMIN(1);
            EACHLINE("emerge", "emerge {}");


        //CASE("--github") // make it --from-url!
            //char *url, *user_repo_pair, *file;
            //user_repo_pair = NEXTVAL;
            //NEXT;
            //if (!isval(CURRENT)){
            //    file = "plashfile"
            //} else {
            //    file = CURRENT
            //}
            //asprintf(url, ""https://raw.githubusercontent.com/%s/master/%s"", user_repo_pair, file) != -1 || pl_fatal("asprintf");
            //content = fetch(url)
            //...


        } else {
            errno = 0;
            pl_fatal("unkown macro: %s", CURRENT);
        }
    }
}
