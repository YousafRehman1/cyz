// Mock Q4: implement "seq 10 | wc -l" using one pipe and two children.
// gcc -Wall -Wextra q04_two_pipes.c -o q04 && ./q04
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
int main(void) {
    int p[2];
    if (pipe(p) == -1) { perror("pipe"); return 1; }
    pid_t a = fork();
    if (a == -1) { perror("fork A"); close(p[0]); close(p[1]); return 1; }
    if (a == 0) {
        if (dup2(p[1], STDOUT_FILENO) == -1) { perror("dup2 A"); _exit(1); }
        close(p[0]); close(p[1]);
        char *args[] = {"seq", "10", NULL};
        execvp(args[0], args); perror("seq"); _exit(127);
    }
    pid_t b = fork();
    if (b == -1) {
        perror("fork B"); close(p[0]); close(p[1]);
        waitpid(a, NULL, 0); return 1;
    }
    if (b == 0) {
        if (dup2(p[0], STDIN_FILENO) == -1) { perror("dup2 B"); _exit(1); }
        close(p[0]); close(p[1]);
        char *args[] = {"wc", "-l", NULL};
        execvp(args[0], args); perror("wc"); _exit(127);
    }
    close(p[0]); close(p[1]); // Parent MUST close both ends.
    if (waitpid(a, NULL, 0) == -1) perror("waitpid A");
    if (waitpid(b, NULL, 0) == -1) perror("waitpid B");
    return 0;
}
