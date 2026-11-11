#define _POSIX_C_SOURCE 200809L
#include "shell.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef USE_LINENOISE
#include "linenoise.h" // Official library: https://github.com/antirez/linenoise
#endif

int main(void) {
    Shell shell = {0}; // Job table starts empty.
    Segment segments[MAX_SEGMENTS];

#ifndef USE_LINENOISE
    char *buffer = NULL; // getline() allocates/reallocates this.
    size_t capacity = 0;
#endif

    while (!shell.should_exit) {
        reap_jobs(&shell); // Remove any finished background processes.

#ifdef USE_LINENOISE
        // Assignment 01 specifically requests the official linenoise library.
        char *line = linenoise("mysh> ");
        if (line == NULL) break; // Ctrl-D / end of input.
        if (*line != '\0') linenoiseHistoryAdd(line);
#else
        // Portable training fallback for systems without downloaded linenoise.
        if (isatty(STDIN_FILENO)) {
            printf("mysh> ");
            fflush(stdout);
        }
        ssize_t n = getline(&buffer, &capacity, stdin);
        if (n == -1) {
            if (ferror(stdin) && errno != EINTR)
                perror("getline");
            break;
        }
        buffer[strcspn(buffer, "\n")] = '\0';
        char *line = buffer;
#endif

        Tokens tokens;
        int command_count = 0;
        if (tokenize(line, &tokens) == 0) {
            if (parse_segments(&tokens, segments, &command_count) == 0) {
                for (int i = 0; i < command_count; ++i) {
                    run_segment(&shell, &segments[i]);
                    if (shell.should_exit) break;
                }
            }
            free_tokens(&tokens); // Pointers in Stage expire here.
        }

#ifdef USE_LINENOISE
        linenoiseFree(line);
#endif
    }

#ifndef USE_LINENOISE
    free(buffer);
#endif
    // The shell is exiting. A production shell may decide to signal/wait for
    // outstanding background jobs; this A01 version lets them continue.
    return 0;
}
