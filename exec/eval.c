
#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdio.h>
#include <string.h>

#include <plash.h>

#define NEXT *(++argv)
#define NEXTVAL assert_isval(NEXT)
#define CURRENT *argv
#define FORVALS while(isval(NEXT))

#define CASE(macro) } else if (strcmp(CURRENT, macro) == 0) {
#define ARGSMIN(min) if (count_vals(argv) < min) pl_fatal( \
        "macro %s needs at least %ld args", *argv, min)

#define ARGS(argscount) if (count_vals(argv) != argscount) pl_fatal( \
        "macro %s needs exactly %ld args", *argv, argscount)



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
    FORVALS {};
    return argv - argvc - 1;
}


char *puthint(char *name, char *val){
    printf("### plash hint: %s=%s\n", name, val);
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
            FORVALS puts(CURRENT);

        CASE("--write-file")
            ARGSMIN(1);
            char *filename = NEXTVAL;
            printf("touch %s\n", quote(filename));
            FORVALS printf("echo %s >> %s\n", quote(CURRENT), quote(filename));
            
        CASE("--env")
            ARGSMIN(1);
            FORVALS printf("echo %s >> /.plashenvsprefix\n", quote(CURRENT));

        CASE("--from")
            ARGS(1);
            puthint("image", pl_call_cached("import-lxc", NEXT));
            NEXT;

        CASE("--entrypoint")
            puthint("exec", NEXT);
            NEXT;

        CASE("--entrypoint-script")
            puthint("exec", NEXT);
            puts("touch /entrypoint");
            puts("chmod 755 /entrypoint");
            FORVALS printf("echo %s >> /entrypoint\n", quote(CURRENT));

        CASE("--apk")
            puts("apk update");
            FORVALS printf("apk add %s\n", quote(CURRENT));


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
