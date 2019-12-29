
#include <stdio.h>

char *pl_test_check_output(char **argv) {
  puts(argv[1]);
  return "X";
}

void pl_test_exec(char **argv) { puts(argv[0]); }

#define PL_TEST 1
#include <plash.c>

int main() {
  puts("hi");
  pl_reexec_builded(2, (char *[]){"run", "-A"});

  return 1;
}
