// Mock Q5: implement "seq 10 | head -n 4 | wc -l" with 3 children.
// gcc -Wall -Wextra q05_three_commands.c -o q05 && ./q05
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
int main(void) {
    int pipes[2][2];
    if (pipe(pipes[0]) == -1) { perror("pipe 0"); return 1; }
    if (pipe(pipes[1]) == -1) {
        perror("pipe 1"); close(pipes[0][0]); close(pipes[0][1]); return 1;
    }
    char *cmd1[] = {"seq", "10", NULL};
    char *cmd2[] = {"head", "-n", "4", NULL};
    char *cmd3[] = {"wc", "-l", NULL};
    char **cmds[3] = {cmd1, cmd2, cmd3};
    pid_t children[3];
    int started = 0;
    for (int i = 0; i < 3; ++i) {
        pid_t pid = fork();
        if (pid == -1) { perror("fork"); break; }
        if (pid == 0) {
            if (i > 0 && dup2(pipes[i - 1][0], STDIN_FILENO) == -1)
                { perror("dup2 in"); _exit(1); }
            if (i < 2 && dup2(pipes[i][1], STDOUT_FILENO) == -1)
                { perror("dup2 out"); _exit(1); }
            for (int k = 0; k < 2; ++k) {
                close(pipes[k][0]); close(pipes[k][1]);
            }
            execvp(cmds[i][0], cmds[i]);
            perror("execvp"); _exit(127);
        }
        children[started++] = pid;
    }
    // Parent closes ALL pipe descriptors; otherwise wc may wait for EOF.
    for (int k = 0; k < 2; ++k) {
        close(pipes[k][0]); close(pipes[k][1]);
    }
    for (int i = 0; i < started; ++i)
        if (waitpid(children[i], NULL, 0) == -1) perror("waitpid");
    return started == 3 ? 0 : 1;
}
