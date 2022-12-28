
#define _GNU_SOURCE
#include <stdio.h>
#include <stdio.h>
#include <string.h>

#include <plash.h>

#define NEXT *(++argv)
#define NEXTVAL assert_isval(NEXT)
#define CURRENT *argv
#define FORVALS while(isval(NEXT))

#define CASE(macro) } else if (strcmp(CURRENT, macro) == 0) {




int isval(char *val){
    if (val == NULL) return 0;
    if (val[0] == '-') return 0;
    return 1;
}

char *assert_isval(char *val){
    if (!isval(val)) pl_fatal("argument for macro is missing");
    return val;
}


char *escape(char *str){
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
            char *filename = NEXTVAL;
            printf("touch %s\n", escape(filename));
            FORVALS printf("echo %s >> %s\n", escape(CURRENT), escape(filename));
            
        CASE("--env")
            FORVALS printf("echo %s >> /.plashenvsprefix\n", CURRENT);

        CASE("--from")
            char *image_id = pl_call_cached("import-lxc", NEXT); 
            printf("### plash hint: image=%s\n", image_id);
            NEXT;

        CASE("--apk")
            puts("apk update");
            FORVALS printf("apk add %s\n", escape(CURRENT));


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
            pl_fatal("unkown macro: %s", CURRENT);
        }
    }
}
