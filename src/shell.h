#ifndef SHELL_H
#define SHELL_H

// Shared structures and function declarations for our mini shell.
// Build: make  (getline mode; see README to enable official linenoise)

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#define MAX_ARGS 64
#define MAX_STAGES 16
#define MAX_SEGMENTS 32
#define MAX_TOKENS 256
#define MAX_JOBS 256

// One command between pipes, e.g. "grep x < in.txt".
typedef struct {
    char *argv[MAX_ARGS];         // NULL-terminated arguments for execvp()
    int argc;                     // Number of actual arguments
    char *infile;                 // Filename after '<'; NULL when unused
    char *outfile;                // Filename after '>' or '>>'
    bool append;                  // true means >>, false means >
} Stage;

// One pipeline; one input line may contain multiple segments separated by '&'.
typedef struct {
    Stage stages[MAX_STAGES];
    int count;                    // Number of stages in pipeline
    bool background;              // True when segment ends with '&'
} Segment;

// Tokens own their allocated string memory. A Stage points into these strings.
typedef struct {
    char *items[MAX_TOKENS];
    int count;
} Tokens;

typedef struct {
    pid_t pid;
    char name[128];
} Job;

typedef struct {
    Job jobs[MAX_JOBS];
    int job_count;
    bool should_exit;
} Shell;

// parser.c
int tokenize(const char *line, Tokens *tokens);
void free_tokens(Tokens *tokens);
int parse_segments(const Tokens *tokens, Segment segments[], int *count);

// builtins.c
bool is_builtin(const char *name);
int run_builtin(Shell *shell, const Stage *stage, bool in_child);
int add_job(Shell *shell, pid_t pid, const char *name);
void remove_job(Shell *shell, pid_t pid);
void reap_jobs(Shell *shell);

// exec.c
int redirect_stage(const Stage *stage);
int run_segment(Shell *shell, const Segment *segment);

#endif
