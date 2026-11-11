// Mock Q8: child exits with status 42; parent reads it with waitpid macros.
// gcc -Wall -Wextra q08_wait_status.c -o q08 && ./q08
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
int main(void) {
    pid_t p = fork();
    if (p == -1) { perror("fork"); return 1; }
    if (p == 0) _exit(42);
    int status;
    if (waitpid(p, &status, 0) == -1) { perror("waitpid"); return 1; }
    if (WIFEXITED(status))
        printf("Child returned %d\n", WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        printf("Child killed by signal %d\n", WTERMSIG(status));
    return 0;
}
