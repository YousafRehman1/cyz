#define _GNU_SOURCE
#include "shell.h"
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

bool is_builtin(const char *name) {
    if (name == NULL) return false;
    return strcmp(name, "cd") == 0 || strcmp(name, "pwd") == 0 ||
           strcmp(name, "exit") == 0 || strcmp(name, "ps") == 0 ||
           strcmp(name, "kill") == 0;
}

int add_job(Shell *shell, pid_t pid, const char *name) {
    if (shell->job_count >= MAX_JOBS) {
        fprintf(stderr, "Background job table is full\n");
        return -1;
    }
    Job *j = &shell->jobs[shell->job_count++];
    j->pid = pid;
    snprintf(j->name, sizeof(j->name), "%s", name);
    return 0;
}

void remove_job(Shell *shell, pid_t pid) {
    for (int i = 0; i < shell->job_count; ++i) {
        if (shell->jobs[i].pid == pid) {
            // Replace with last entry: O(1) removal, job order can change.
            shell->jobs[i] = shell->jobs[shell->job_count - 1];
            shell->job_count--;
            return;
        }
    }
}

// Non-blocking cleanup: a finished background child otherwise becomes zombie.
void reap_jobs(Shell *shell) {
    int status;
    pid_t done;
    while ((done = waitpid(-1, &status, WNOHANG)) > 0) {
        remove_job(shell, done);
    }
    if (done == -1 && errno != ECHILD)
        perror("waitpid WNOHANG");
}

static bool tracked_pid(const Shell *shell, pid_t pid) {
    for (int i = 0; i < shell->job_count; ++i)
        if (shell->jobs[i].pid == pid) return true;
    return false;
}

// Called for a shell builtin (normally in the parent). When inside a pipeline
// or background command, it runs in a CHILD so it cannot change parent state.
int run_builtin(Shell *shell, const Stage *s, bool in_child) {
    const char *cmd = s->argv[0];
    if (!is_builtin(cmd)) return -1; // -1 means "not a builtin"

    if (strcmp(cmd, "pwd") == 0) {
        if (s->argc != 1) {
            fprintf(stderr, "pwd: no arguments expected\n");
            return 1;
        }
        char *cwd = getcwd(NULL, 0); // GNU extension allocates enough space
        if (cwd == NULL) { perror("pwd"); return 1; }
        printf("%s\n", cwd);
        free(cwd);
        return 0;
    }

    if (strcmp(cmd, "cd") == 0) {
        if (s->argc > 2) {
            fprintf(stderr, "cd: too many arguments\n");
            return 1;
        }
        const char *path = (s->argc == 2) ? s->argv[1] : getenv("HOME");
        if (path == NULL) {
            fprintf(stderr, "cd: HOME is not set\n");
            return 1;
        }
        if (chdir(path) == -1) { perror("cd"); return 1; }
        return 0;
    }

    if (strcmp(cmd, "exit") == 0) {
        if (s->argc != 1) {
            fprintf(stderr, "exit: no arguments expected\n");
            return 1;
        }
        if (!in_child) shell->should_exit = true;
        return 0;
    }

    if (strcmp(cmd, "ps") == 0) {
        if (s->argc != 1) {
            fprintf(stderr, "ps: no arguments expected\n");
            return 1;
        }
        if (!in_child) reap_jobs(shell);
        printf("PID\tNAME\n");
        for (int i = 0; i < shell->job_count; ++i) {
            printf("%ld\t%s\n", (long)shell->jobs[i].pid,
                   shell->jobs[i].name);
        }
        return 0;
    }

    // This assignment's kill only targets PIDs tracked by our mini shell.
    if (strcmp(cmd, "kill") == 0) {
        if (s->argc != 2) {
            fprintf(stderr, "usage: kill PID\n");
            return 1;
        }
        char *end = NULL;
        errno = 0;
        long raw = strtol(s->argv[1], &end, 10);
        if (errno != 0 || end == s->argv[1] || *end != '\0' ||
            raw <= 0 || raw > INT_MAX) {
            fprintf(stderr, "kill: invalid PID\n");
            return 1;
        }
        pid_t target = (pid_t)raw;
        if (!tracked_pid(shell, target)) {
            fprintf(stderr, "kill: PID not tracked by mysh\n");
            return 1;
        }
        if (kill(target, SIGTERM) == -1) {
            if (errno == ESRCH) {
                // Might have exited between the lookup and signal.
                if (!in_child) reap_jobs(shell);
            }
            perror("kill");
            return 1;
        }
        // Reap the child; this can block if a process ignores SIGTERM.
        // No waitpid in pipeline child: it is NOT parent of tracked jobs.
        if (!in_child) {
            int st;
            pid_t done;
            do { done = waitpid(target, &st, 0); }
            while (done == -1 && errno == EINTR);
            if (done == -1 && errno != ECHILD) {
                perror("waitpid kill");
                return 1;
            }
            remove_job(shell, target);
        }
        return 0;
    }
    return 1;
}
