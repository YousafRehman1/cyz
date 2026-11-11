// Mock Q3: redirect an external command's stdout to result.txt.
// gcc -Wall -Wextra q03_redirect.c -o q03 && ./q03
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
int main(void) {
    pid_t pid = fork();
    if (pid == -1) { perror("fork"); return 1; }
    if (pid == 0) {
        int fd = open("result.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) { perror("open"); _exit(1); }
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2"); close(fd); _exit(1);
        }
        if (close(fd) == -1) { perror("close"); _exit(1); }
        char *args[] = {"ls", "-l", NULL};
        execvp(args[0], args);
        perror("execvp"); _exit(127);
    }
    if (waitpid(pid, NULL, 0) == -1) { perror("waitpid"); return 1; }
    puts("Check result.txt for redirected output.");
    return 0;
}
