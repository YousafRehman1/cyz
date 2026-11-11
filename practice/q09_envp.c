// Mock Q9: inspect argc/argv/envp from main signature in reference sheet.
// gcc -Wall -Wextra q09_envp.c -o q09 && ./q09 hello
#include <stdio.h>
#include <string.h>
int main(int argc, char *argv[], char *envp[]) {
    printf("argc=%d\n", argc);
    for (int i = 0; i < argc; ++i)
        printf("argv[%d]=%s\n", i, argv[i]);
    // envp contains KEY=VALUE strings, ends with NULL.
    for (int i = 0; envp[i] != NULL; ++i)
        if (strncmp(envp[i], "HOME=", 5) == 0)
            printf("%s\n", envp[i]);
    return 0;
}
