
#define _GNU_SOURCE
#include <stdio.h>
#include <stdio.h>
#include <string.h>

#include <plash.h>

#define NEXT *(++argv)
#define NEXTVAL assert_isval(NEXT)
#define CURRENT *argv
#define FORVALS while(isval(NEXT))

#define IS(macro) (strcmp(CURRENT, macro) == 0)


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
    asprintf(&cache_key, "%s:%s", subcommand, arg) != -1 || pl_fatal("asprintf");
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
        if IS("-x"){
            FORVALS puts(CURRENT);

        } else if IS("--write-file") {
            char *filename = NEXTVAL;
            printf("touch %s\n", escape(filename));
            FORVALS printf("echo %s >> %s\n", escape(CURRENT), escape(filename));
            
        } else if IS("--env") {
            FORVALS printf("echo %s >> /.plashenvsprefix\n", CURRENT);


        } else if IS("--from") {
            char *image_id = pl_call_cached("import-lxc", NEXT); 
            printf("### plash hint: image=%s\n", image_id);
            NEXT;

        } else {
            pl_fatal("unkown macro: %s", CURRENT);
        }
    }
}
