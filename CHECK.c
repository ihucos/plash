#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#include <plash.h>

#define BUFFER_SIZE 1024



unsigned long djb2_hash_file(int fd) {
    unsigned long hash = 5381;
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;

    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0) {
        for (ssize_t i = 0; i < bytesRead; i++) {
            puts(buffer);
            hash = ((hash << 5) + hash) + buffer[i];
        }
    }

    return hash;
}

unsigned long run_hash_output(char *args[]){

    int pipe_fd[2];
    pid_t pid;

    // Create a pipe
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        close(pipe_fd[1]);
        dup2(pipe_fd[0], STDOUT_FILENO);
        close(pipe_fd[0]);

        puts("hi");
        /* execlp(, "plash", "noid", "run", "ls", NULL); */
        pl_exec_add("/proc/self/exe");
        pl_exec_add("noid");
        pl_exec_add("run");
        pl_exec_add("ls");
        pl_exec_add("-l");
        pl_exec_add(NULL);


    } else {
        close(pipe_fd[0]);
        waitpid(pid, NULL, 0);
        return djb2_hash_file(pipe_fd[1]);
    }


}

int check_main(int argc, char *argv[]) {
        printf("Hash of child process output: %lu\n", run_hash_output((char **){NULL}));

}
