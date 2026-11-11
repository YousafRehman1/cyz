// Mock Q10: start a child and terminate it with SIGTERM.
// gcc -Wall -Wextra q10_signal_kill.c -o q10 && ./q10
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>
int main(void) {
    pid_t p = fork();
    if (p == -1) { perror("fork"); return 1; }
    if (p == 0) {
        char *argv[] = {"sleep", "30", NULL};
        execvp(argv[0], argv); perror("execvp"); _exit(127);
    }
    if (kill(p, SIGTERM) == -1) perror("kill");
    int status;
    if (waitpid(p, &status, 0) == -1) { perror("waitpid"); return 1; }
    if (WIFSIGNALED(status))
        printf("Child terminated by signal %d\n", WTERMSIG(status));
    else if (WIFEXITED(status))
        printf("Child exit code %d\n", WEXITSTATUS(status));
    return 0;
}
