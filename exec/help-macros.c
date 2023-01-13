// usage: plash list-macros
// Lists all available macros.

#include <unistd.h>

#include <plash.h>

int main(int argc, char *argv[]) {
  execlp("plash", "plash", "eval", "--help", NULL);
  pl_fatal("execlp");
}
