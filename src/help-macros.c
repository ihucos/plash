// Lists all available macros.

#define USAGE "usage: plash list-macros\n"

#include <unistd.h>

#include <plash.h>

int help_macros_main(int argc, char *argv[]) {
  eval_main(2, (char*[]){"eval", "--help", NULL});
}
