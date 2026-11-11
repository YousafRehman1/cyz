// Mock Q6: launch sleep 2 in background; periodically reap with WNOHANG.
// gcc -Wall -Wextra q06_background.c -o q06 && ./q06
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
int main(void) {
    pid_t pid = fork();
    if (pid == -1) { perror("fork"); return 1; }
    if (pid == 0) {
        char *args[] = {"sleep", "2", NULL};
        execvp(args[0], args);
        perror("execvp"); _exit(127);
    }
    printf("Prompt would return immediately (background PID %ld)\n", (long)pid);
    for (;;) {
        int status;
        pid_t done = waitpid(pid, &status, WNOHANG);
        if (done == -1) {
            if (errno == EINTR) continue;
            perror("waitpid"); return 1;
        }
        if (done == pid) {
            puts("Child has finished and is reaped.");
            break;
        }
        puts("Child still running; shell can do other work.");
        sleep(1); // Demo only: real shell instead reads the next command.
    }
    return 0;
}
