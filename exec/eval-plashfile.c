// usage: plash eval-file [ FILE ]
// Evaluates the given plashfile or read it from stdin.

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <plash.h>

int main(int argc, char *argv[]) {
  int is_first_line = 1;
  size_t read;
  char * line = NULL;
  char * lineCopy = NULL;
  size_t len = 0;
  FILE *fp;

  if (argc < 2){
    fp = stdin;
  } else {
    fp = fopen(argv[1], "r");
    if (fp == 0) pl_fatal("fopen: %s", argv[1]);
  }

  pl_exec_add("plash");
  pl_exec_add("eval");

  // for each line
  while ((read = getline(&line, &len, fp)) != -1) {
    lineCopy = strdup(line);
    if (lineCopy == NULL) pl_fatal("strdup");
    lineCopy[strcspn(line, "\n")] = 0; // chop newline char

    // ignore shebang
    if (is_first_line && lineCopy[0] == '#' && lineCopy[1] == '!') continue;

    if (lineCopy[0] == '-'){

      // tokenize line
      char *token = strtok(lineCopy, " ");
      pl_exec_add(token);
      while (token = strtok(NULL, " ")) pl_exec_add(token);

    } else {
      // line is token
      pl_exec_add(lineCopy);
    }
    is_first_line = 0;
  }
  pl_exec_add(NULL);
}
