// Mock Q1: create a child; child runs "ls -l"; parent waits.
// gcc -Wall -Wextra -g q01_fork_exec_wait.c -o q01 && ./q01
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

int main(void) {
    pid_t pid = fork();
    if (pid == -1) { perror("fork"); return 1; }

    if (pid == 0) {                        // Child branch
        printf("Child PID = %ld\n", (long)getpid());
        fflush(stdout);                   // exec may replace stdout buffers
        char *args[] = {"ls", "-l", NULL}; // execvp needs NULL at end
        execvp(args[0], args);             // Looks up ls in PATH
        perror("execvp");                // Runs only if execvp fails
        _exit(127);                       // Do not resume shell code
    }

    int status;
    pid_t done;
    do { done = waitpid(pid, &status, 0); }
    while (done == -1 && errno == EINTR);
    if (done == -1) { perror("waitpid"); return 1; }
    if (WIFEXITED(status))
        printf("Child finished, exit status %d\n", WEXITSTATUS(status));
    else
        printf("Child finished abnormally\n");
    return 0;
}
