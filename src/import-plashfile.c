// Evaluates the given plashfile or read it from stdin.

#define USAGE "usage: plash eval-file [ FILE ]\n"


#define _GNU_SOURCE

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <plash.h>

#define CMD(bcmd, cmd) } else if (strncmp(bcmd " ", line, strlen(bcmd " ")) == 0 || strcmp(bcmd, line) == 0) {pl_exec_add(cmd); passargs(line);

void passargs(char *line){
  char *lineCopy = strdup(line);
  if (lineCopy == NULL)
    pl_fatal("strdup");
  char *token = strtok(lineCopy, " ");
  while (token = strtok(NULL, " "))
    pl_exec_add(token);
}

int import_plashfile(int argc, char *argv[]) {
  int is_first_line = 1;
  size_t read;
  char *line = NULL;
  size_t len = 0;
  FILE *fp;

  if (argc < 2) {
    fp = stdin;
  } else {
    fp = fopen(argv[1], "r");
    if (fp == 0)
      pl_fatal("fopen: %s", argv[1]);
  }

  pl_exec_add("/proc/self/exe");
  pl_exec_add("build");

  // for each line
  while ((read = getline(&line, &len, fp)) != -1) {
    line[strcspn(line, "\n")] = 0; // chop newline char

    // ignore shebang
    if (is_first_line && line[0] == '#' && line[1] == '!')
      continue;

    if (0){

    CMD("FROM", "--from")
    CMD("LAYER", "--layer")
    CMD("SCRIPT", "--script")
    CMD("FILE", "--file")
    CMD("HASH", "--hash")
    CMD("ENV", "--env")
    CMD("POLUTE", "--polute")
    CMD("PASS", "--pass")

    } else {
      pl_exec_add(strdup(line));
    }
  }
  is_first_line = 0;
  pl_exec_add(NULL);
}







  /* while ((read = getline(&line, &len, fp)) != -1) { */
  /*   lineCopy = strdup(line); */
  /*   if (lineCopy == NULL) */
  /*     pl_fatal("strdup"); */
  /*   lineCopy[strcspn(line, "\n")] = 0; // chop newline char */

  /*   // ignore shebang */
  /*   if (is_first_line && lineCopy[0] == '#' && lineCopy[1] == '!') */
  /*     continue; */

  /*   if (lineCopy[0] == '@') { */

  /*     // tokenize line */
  /*     char *token = strtok(lineCopy, " "); */


  /*     if (asprintf(&token, "--%s", token + 1) == -1) */
  /*       pl_fatal("asprintf"); */

  /*     pl_exec_add(token); */
  /*     while (token = strtok(NULL, " ")) */
  /*       pl_exec_add(token); */

  /*   } else { */
  /*     // line is token */
  /*     pl_exec_add(lineCopy); */
  /*   } */
  /*   is_first_line = 0; */
  /* } */
  /* pl_exec_add(NULL); */
/* } */
