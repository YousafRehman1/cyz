// Mock Q2: distinguish parent PID, child PID, and fork return value.
// gcc -Wall -Wextra q02_fork_count.c -o q02 && ./q02
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
int main(void) {
    pid_t pid = fork();
    if (pid == -1) { perror("fork"); return 1; }
    if (pid == 0) {
        printf("CHILD: my PID=%ld, parent PID=%ld, fork result=%ld\n",
               (long)getpid(), (long)getppid(), (long)pid);
        return 0;
    }
    printf("PARENT: my PID=%ld, child's PID=%ld\n",
           (long)getpid(), (long)pid);
    if (waitpid(pid, NULL, 0) == -1) { perror("waitpid"); return 1; }
    return 0;
}
