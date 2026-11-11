#define _POSIX_C_SOURCE 200809L
#include "shell.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

// Change standard input/output BEFORE execvp(). Called in a child, OR
// temporarily in parent while executing a builtin (then restore descriptors).
int redirect_stage(const Stage *s) {
    if (s->infile != NULL) {
        int fd = open(s->infile, O_RDONLY);
        if (fd == -1) { perror(s->infile); return -1; }
        if (dup2(fd, STDIN_FILENO) == -1) {
            perror("dup2 stdin");
            close(fd);
            return -1;
        }
        if (close(fd) == -1) perror("close input");
    }

    if (s->outfile != NULL) {
        int flags = O_WRONLY | O_CREAT | (s->append ? O_APPEND : O_TRUNC);
        int fd = open(s->outfile, flags, 0644);
        if (fd == -1) { perror(s->outfile); return -1; }
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2 stdout");
            close(fd);
            return -1;
        }
        if (close(fd) == -1) perror("close output");
    }
    return 0;
}

static void wait_for_child(pid_t pid) {
    int status;
    pid_t result;
    do {
        result = waitpid(pid, &status, 0);
    } while (result == -1 && errno == EINTR);

    if (result == -1) perror("waitpid");
    // Optional: inspect WIFEXITED(status) and WEXITSTATUS(status).
}

// A builtin that changes the shell's state (cd/exit) MUST run in the parent.
// Save and restore stdin/stdout if redirection was requested for the builtin.
static int run_parent_builtin(Shell *shell, const Stage *s) {
    int saved_in = -1, saved_out = -1, result = 1;
    bool use_in = s->infile != NULL;
    bool use_out = s->outfile != NULL;

    fflush(stdout); // Flush before changing stdout's destination.
    if (use_in) {
        saved_in = dup(STDIN_FILENO);
        if (saved_in == -1) { perror("dup stdin"); goto cleanup; }
    }
    if (use_out) {
        saved_out = dup(STDOUT_FILENO);
        if (saved_out == -1) { perror("dup stdout"); goto cleanup; }
    }
    if (redirect_stage(s) == -1) goto cleanup;

    result = run_builtin(shell, s, false);
    fflush(stdout); // Flush builtin output BEFORE restoring stdout.

cleanup:
    if (saved_in != -1) {
        if (dup2(saved_in, STDIN_FILENO) == -1) {
            perror("restore stdin"); result = 1;
        }
        if (close(saved_in) == -1) perror("close saved stdin");
    }
    if (saved_out != -1) {
        if (dup2(saved_out, STDOUT_FILENO) == -1) {
            perror("restore stdout"); result = 1;
        }
        if (close(saved_out) == -1) perror("close saved stdout");
    }
    return result;
}

int run_segment(Shell *shell, const Segment *segment) {
    if (segment->count <= 0) return 0;

    // Foreground single-stage builtins run in parent; all other cases fork.
    const Stage *first = &segment->stages[0];
    if (segment->count == 1 && !segment->background &&
        is_builtin(first->argv[0])) {
        return run_parent_builtin(shell, first);
    }

    // Don't exceed the internal table before starting background children.
    if (segment->background &&
        shell->job_count + segment->count > MAX_JOBS) {
        fprintf(stderr, "Not enough room in background job table\n");
        return 1;
    }

    pid_t pids[MAX_STAGES];
    int spawned = 0;
    int prev_read = -1; // Read end of the previous pipe, or none.

    fflush(stdout);     // Avoid duplicate output after fork.

    for (int i = 0; i < segment->count; ++i) {
        int p[2] = {-1, -1};
        const bool last = (i == segment->count - 1);

        if (!last && pipe(p) == -1) {
            perror("pipe");
            goto fork_failure;
        }

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            if (p[0] != -1) close(p[0]);
            if (p[1] != -1) close(p[1]);
            goto fork_failure;
        }

        if (pid == 0) {
            // CHILD: pipe connections are made first.
            if (prev_read != -1 &&
                dup2(prev_read, STDIN_FILENO) == -1) {
                perror("pipe stdin"); _exit(1);
            }
            if (!last && dup2(p[1], STDOUT_FILENO) == -1) {
                perror("pipe stdout"); _exit(1);
            }

            // Close original descriptors after dup2().
            if (prev_read != -1) close(prev_read);
            if (p[0] != -1) close(p[0]);
            if (p[1] != -1) close(p[1]);

            // Explicit <, >, >> take priority over pipe endpoints.
            const Stage *s = &segment->stages[i];
            if (redirect_stage(s) == -1) _exit(1);

            if (is_builtin(s->argv[0])) {
                int code = run_builtin(shell, s, true);
                fflush(stdout); // _exit doesn't flush stdio buffers.
                _exit(code == 0 ? 0 : 1);
            }

            execvp(s->argv[0], s->argv);
            perror(s->argv[0]); // execvp returns ONLY upon failure.
            _exit(127);
        }

        // PARENT: keep PIDs, close unneeded write end immediately.
        pids[spawned++] = pid;
        if (prev_read != -1 && close(prev_read) == -1)
            perror("close old pipe read end");
        if (p[1] != -1 && close(p[1]) == -1)
            perror("close pipe write end");
        prev_read = p[0];
    }

    if (prev_read != -1 && close(prev_read) == -1)
        perror("close final read end");

    if (segment->background) {
        // All pipeline processes are tracked, not just the last one.
        for (int i = 0; i < spawned; ++i)
            add_job(shell, pids[i], segment->stages[i].argv[0]);
    } else {
        for (int i = 0; i < spawned; ++i)
            wait_for_child(pids[i]);
    }
    return 0;

fork_failure:
    if (prev_read != -1) close(prev_read);
    // If a pipeline only partly started, stop children and collect statuses.
    for (int i = 0; i < spawned; ++i) {
        if (kill(pids[i], SIGTERM) == -1 && errno != ESRCH)
            perror("kill after failed fork");
    }
    for (int i = 0; i < spawned; ++i)
        wait_for_child(pids[i]);
    return 1;
}
