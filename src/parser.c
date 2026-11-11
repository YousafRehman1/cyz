#define _POSIX_C_SOURCE 200809L
#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Return 1 for a shell operator. Longest match matters: '>>' is ONE token.
static bool is_operator(const char *s) {
    return strcmp(s, "&") == 0 || strcmp(s, "|") == 0 ||
           strcmp(s, "<") == 0 || strcmp(s, ">") == 0 ||
           strcmp(s, ">>") == 0;
}

// Duplicate n bytes into heap memory and add a terminating '\0'.
static char *copy_part(const char *p, size_t n) {
    char *out = malloc(n + 1);
    if (out == NULL) return NULL;
    memcpy(out, p, n);
    out[n] = '\0';
    return out;
}

void free_tokens(Tokens *tokens) {
    for (int i = 0; i < tokens->count; ++i)
        free(tokens->items[i]);
    tokens->count = 0;
}

// Turns:  cat<in.txt|wc -l>>out.txt
// into:  cat < in.txt | wc -l >> out.txt
// No quoting, wildcard expansion, variables, or escaping: not required by A01.
int tokenize(const char *line, Tokens *tokens) {
    memset(tokens, 0, sizeof(*tokens));
    const char *p = line;

    while (*p != '\0') {
        if (isspace((unsigned char)*p)) { ++p; continue; }
        if (tokens->count >= MAX_TOKENS) {
            fprintf(stderr, "Too many tokens (max %d)\n", MAX_TOKENS);
            goto error;
        }

        const char *start = p;
        size_t n;
        if (*p == '>' && p[1] == '>') {
            p += 2; n = 2;                 // append operator
        } else if (strchr("<>|&", *p) != NULL) {
            ++p; n = 1;                   // single-character operator
        } else {
            while (*p && !isspace((unsigned char)*p) &&
                   strchr("<>|&", *p) == NULL)
                ++p;
            n = (size_t)(p - start);
        }
        tokens->items[tokens->count] = copy_part(start, n);
        if (tokens->items[tokens->count] == NULL) {
            perror("malloc");
            goto error;
        }
        tokens->count++;
    }
    return 0;
error:
    free_tokens(tokens);
    return -1;
}

// Each Stage has at least one executable word. Redirection filenames do not
// appear inside argv. Empty pipelines such as "ls | | wc" are rejected.
int parse_segments(const Tokens *tokens, Segment segments[], int *count) {
    *count = 0;
    if (tokens->count == 0) return 0;     // Empty line is okay.

    memset(segments, 0, sizeof(*segments) * MAX_SEGMENTS);
    int seg = 0, st = 0;
    segments[seg].count = 1;

    for (int i = 0; i < tokens->count; ++i) {
        const char *t = tokens->items[i];
        Stage *s = &segments[seg].stages[st];

        if (strcmp(t, "|") == 0) {
            if (s->argc == 0) goto bad_syntax;
            if (st + 1 >= MAX_STAGES) {
                fprintf(stderr, "Too many pipe stages\n");
                return -1;
            }
            st++;
            segments[seg].count++;
        } else if (strcmp(t, "&") == 0) {
            if (s->argc == 0) goto bad_syntax;
            segments[seg].background = true;
            seg++;
            if (seg >= MAX_SEGMENTS) {
                fprintf(stderr, "Too many commands separated by &\n");
                return -1;
            }
            st = 0;
            if (i != tokens->count - 1)
                segments[seg].count = 1;
        } else if (strcmp(t, "<") == 0 ||
                   strcmp(t, ">") == 0 ||
                   strcmp(t, ">>") == 0) {
            if (i + 1 >= tokens->count ||
                is_operator(tokens->items[i + 1])) {
                fprintf(stderr, "Missing filename after %s\n", t);
                return -1;
            }
            char *file = tokens->items[++i];
            if (strcmp(t, "<") == 0) {
                s->infile = file;
            } else {
                s->outfile = file;
                s->append = (strcmp(t, ">>") == 0);
            }
        } else {
            if (s->argc >= MAX_ARGS - 1) {
                fprintf(stderr, "Too many command arguments\n");
                return -1;
            }
            s->argv[s->argc++] = tokens->items[i];
            s->argv[s->argc] = NULL; // required terminator for execvp
        }
    }
    if (tokens->count > 0 &&
        strcmp(tokens->items[tokens->count - 1], "&") == 0) {
        *count = seg; // All segments ended with '&'; no empty final one.
    } else {
        if (segments[seg].stages[st].argc == 0) goto bad_syntax;
        *count = seg + 1;
    }
    return 0;

bad_syntax:
    fprintf(stderr, "Invalid command syntax (missing command near | or &)\n");
    return -1;
}
