// Mock Q7: use getenv, setenv, unsetenv and check errors.
// gcc -Wall -Wextra q07_env.c -o q07 && ./q07
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
int main(void) {
    const char *home = getenv("HOME");
    printf("HOME=%s\n", home != NULL ? home : "(not set)");
    if (setenv("OS_EVAL", "READY", 1) == -1) {
        perror("setenv"); return 1;
    }
    printf("OS_EVAL=%s\n", getenv("OS_EVAL"));
    if (unsetenv("OS_EVAL") == -1) {
        perror("unsetenv"); return 1;
    }
    puts("OS_EVAL removed");
    return 0;
}
