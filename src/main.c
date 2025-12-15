#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include "parse.h"
#include "execute.h"
#include "builtin.h"
#include "signals.h"
#include "logger.h"
#include "jobs.h"

static void handle_reaped(int reap_fd, Jobs *jobs, int log_fd) {
    Reaped buf[32];
    for (;;) {
        ssize_t n = signals_read_reaped(reap_fd, buf, 32);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            return;
        }
        if (n == 0) return;

        size_t count = (size_t)n / sizeof(Reaped);
        for (size_t i = 0; i < count; i++) {
            char *cmdline = jobs_take_cmdline(jobs, buf[i].pid);
            if (cmdline) {
                logger_log(log_fd, buf[i].pid, cmdline, buf[i].status);
                free(cmdline);
            }
        }
        if ((size_t)n < sizeof(buf)) return;
    }
}

int main(void) {
    int log_fd = logger_open("myshell.log");

    Jobs jobs;
    jobs_init(&jobs);

    int sigchld_pipe[2];
    if (signals_init(sigchld_pipe) < 0) {
        perror("signals_init");
        return 1;
    }

    char line[4096];

    while (1) {
        handle_reaped(sigchld_pipe[0], &jobs, log_fd);

        printf("myshell> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) break;

        handle_reaped(sigchld_pipe[0], &jobs, log_fd);

        const char *err = NULL;
        Command *cmd = parse_line(line, &err);
        if (!cmd) {
            if (err && strcmp(err, "empty") != 0) {
                fprintf(stderr, "myshell: %s\n", err);
            }
            continue;
        }

        int b = BUILTIN_NONE;
        if (!cmd->has_pipe) b = builtin_execute(cmd);

        if (b == BUILTIN_EXIT) {
            free_command(cmd);
            break;
        }

        if (b == BUILTIN_NONE) {
            execute_command(cmd, log_fd, &jobs);
        }

        free_command(cmd);
    }

    handle_reaped(sigchld_pipe[0], &jobs, log_fd);

    jobs_cleanup(&jobs);
    logger_close(log_fd);
    close(sigchld_pipe[0]);
    close(sigchld_pipe[1]);
    return 0;
}

