// usage: plash build --macro1 ar1 arg2 --macro2 arg1 ...
// Builds an image. Command line options are evaluated as macros. Use `plash
// help-macros` to list all available macros.
//
// Example:
//
// $ plash build -f ubuntu --run 'touch a'
// --> touch a
// --:
// 66
//
// $ plash build -f ubuntu:warty --run 'touch a' --layer --run 'touch b'
// --> touch b
// --:
// 67
//
// $ plash build -f ubuntu:warty --apt nmap
// --> apt-get update
// Hit:1 http://security.ubuntu.com/ubuntu bionic-security InRelease
// Get:2 http://archive.ubuntu.com/ubuntu bionic InRelease [235 kB]
// <snip>
// Setting up nmap (7.60-1ubuntu2) ...
// Processing triggers for libc-bin (2.26-0ubuntu2) ...
// --:
// 68


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <plash.h>

#include <unistd.h>
#include <sys/wait.h>


#define PLASH_HINT_IMAGE "### plash hint: image="
#define PLASH_HINT_LAYER "### plash hint: layer"


int spawn_process_for_output(char *const argv[]) {
    int fd[2];
    pid_t pid;

    if(pipe(fd) == -1) {
        perror("pipe");
        return -1;
    }

    pid = fork();
    if (pid == -1) {
        perror("pipe");
        return -1;
    }

    if (pid == 0) {
        // child process
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        execvp(argv[0], argv);
        perror("execvp");
        _exit(1);
    } else {
        // parent process
        close(fd[1]);
        return fd[0];
    }
}

int spawn_process(char *const argv[]) {
    int fd[2];
    pid_t pid;

    if (pipe(fd) == -1) {
        perror("pipe");
        return -1;
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        // child process
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO);
        execvp(argv[0], argv);
        perror("execvp");
        _exit(1);
    } else {
        // parent process
        close(fd[0]);
        return fd[1];
    }
}


int main(int argc, char *argv[]) {
  // Forward args to plash eval and get its output
  char *args[argc + 1];
  size_t i = 0;
  args[i++] = "plash";
  args[i++] = "eval";
  argv++;
  while (*argv) args[i++] = *(argv++);
  args[i++] = NULL;
  char *shellscript = pl_check_output(args);
  //int eval_fd = spawn_process_for_output(args);


  // count newlines
  int newlines = 0;
  for(i = 0; shellscript[i]; i++){
    if (shellscript[i] == '\n') newlines++;
  }
  

  // lines to buff
  char *lines[newlines + 1];
  i = 0;
  char *line = strtok(shellscript, "\n");
  while (line != NULL) {
    lines[i++] = line;
    line = strtok(NULL, "\n");
  }
  lines[i++] = NULL;


  // detect used image id
  char *image_id = NULL;
  for (i = 0; i < newlines; i++){
    if (strncmp(lines[i], PLASH_HINT_IMAGE, strlen(PLASH_HINT_IMAGE)) == 0){
      image_id = lines[i] + strlen(PLASH_HINT_IMAGE);
    }
  }
  if (image_id == NULL) pl_fatal("no image specified");

  int fd = spawn_process((char*[]){"plash", "create", image_id, "sh", "-ex", NULL});


  for (i = 0; i < newlines; i++){
	write(fd, lines[i], strlen(lines[i]));
	write(fd, "\n", 1);
  }

}




















//define PLASH_HINT_IMAGE "### plash hint: image="
//define PLASH_HINT_LAYER "### plash hint: layer"
//
//include <stdio.h>
//
//include <plash.h>
//include <string.h>
//
//char *get_image_id(char *shellscript){
//
//  char *found;
//
//  // Check if image hint is on the first line
//  if (strncmp(shellscript, PLASH_HINT_IMAGE, strlen(PLASH_HINT_IMAGE)) == 0){
//    found = shellscript;
//    found += strlen(PLASH_HINT_IMAGE);
//  } else {
//    // search for it further. It must be the first token in new line
//    found=strstr(shellscript, "\n" PLASH_HINT_IMAGE);
//    if (found == NULL) pl_fatal("no image specified");
//    found += strlen("\n" PLASH_HINT_IMAGE);
//  }
//
//  // find the index of the next newline
//  size_t newline_index = strcspn(found, "\n");
//
//  // that newline is now a string null terminator because this is C
//  found[newline_index] = '\0';
//
//  // Copy our precious substring out of the shellscript string
//  char *image_id = strdup(found);
//  if (image_id == NULL) pl_fatal("strdup");
//
//  // repair the argument the caller has trusted to this function.
//  found[newline_index] = '\n';
//
//  return image_id;
//}
//
//char *next_layer(char *shellscript){
//
//}
//
//int main(int argc, char *argv[]) {
//
//  // Forward args to plash eval and get its output
//  char *args[argc + 1];
//  int i = 0;
//  args[i++] = "plash";
//  args[i++] = "eval";
//  argv++;
//  while (*argv) args[i++] = *(argv++);
//  args[i++] = NULL;
//  char *shellscript = pl_check_output(args);
//
//  char *found;
//
//  found = strstr(shellscript, "\n" PLASH_HINT_LAYER "\n");
//  if (found == NULL) puts("well, only one layer");
//  *found = '\0';
//  puts(shellscript);
//  puts("==========");
//
//
//  found = strstr(found, "\n" PLASH_HINT_LAYER "\n");
//  if (found == NULL) puts("well, only one layer");
//  *found = '\0';
//  puts(shellscript);
//  puts("==========");
//
//}

//import os
//import re
//import sys
//
//from plash.eval import get_hint_values, hint, remove_hint_values
//from plash import utils
//
//utils.assert_initialized()
//
//script = utils.plash_call("eval", *sys.argv[1:], strip=False)
//
//# split the script in its layers
//layers = script.split(hint("layer") + "\n")
//layers = [remove_hint_values(l) for l in layers]
//layers = [l for l in layers if l]
//
//utils.plash_call("nodepath", image_hint)
//current_container = image_hint
//os.environ["PS4"] = "--> "
//for layer in layers:
//    cache_key = utils.hashstr(b":".join([current_container.encode(), layer.encode()]))
//    next_container = utils.plash_call("map", cache_key)
//    if not next_container:
//
//        shell = (
//            # for some reason in ubuntu the path is not exported
//            # in a way this is a hack and should be fixed in ubuntu
//            "export PATH\n"
//            "set -ex\n" + layer
//        )
//        next_container = utils.plash_call(
//            "create", current_container, "/bin/sh", "-l", input=shell
//        )
//        utils.plash_call("map", cache_key, next_container)
//        print("--:", file=sys.stderr)
//    current_container = next_container
//build_container = current_container
//
//print(current_container)