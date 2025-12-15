#include "execute.h"
#include "logger.h"
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

static void child_reset_signals(void) {
    signal(SIGINT, SIG_DFL);
    sigset_t empty;
    sigemptyset(&empty);
    sigprocmask(SIG_SETMASK, &empty, NULL);
}

static void apply_redirs_simple(Command *cmd) {
    if (cmd->in_file) {
        int fd = open(cmd->in_file, O_RDONLY);
        if (fd < 0) {
            perror(cmd->in_file);
            _exit(1);
        }
        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("dup2");
            close(fd);
            _exit(1);
        }
        close(fd);
    }

    if (cmd->out_file) {
        int flags = O_WRONLY | O_CREAT;
        if (cmd->out_append) flags |= O_APPEND;
        else flags |= O_TRUNC;

        int fd = open(cmd->out_file, flags, 0644);
        if (fd < 0) {
            perror(cmd->out_file);
            _exit(1);
        }
        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2");
            close(fd);
            _exit(1);
        }
        close(fd);
    }
}

static void apply_redirs_left_pipe(Command *cmd) {
    if (cmd->in_file) {
        int fd = open(cmd->in_file, O_RDONLY);
        if (fd < 0) {
            perror(cmd->in_file);
            _exit(1);
        }
        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("dup2");
            close(fd);
            _exit(1);
        }
        close(fd);
    }
    if (cmd->out_file) {
        fprintf(stderr, "myshell: output redirection not supported on left side of pipe\n");
        _exit(2);
    }
}

static void apply_redirs_right_pipe(Command *cmd) {
    if (cmd->in_file) {
        fprintf(stderr, "myshell: input redirection not supported on right side of pipe\n");
        _exit(2);
    }
    if (cmd->out_file) {
        int flags = O_WRONLY | O_CREAT;
        if (cmd->out_append) flags |= O_APPEND;
        else flags |= O_TRUNC;

        int fd = open(cmd->out_file, flags, 0644);
        if (fd < 0) {
            perror(cmd->out_file);
            _exit(1);
        }
        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2");
            close(fd);
            _exit(1);
        }
        close(fd);
    }
}

static void block_sigchld(sigset_t *oldmask) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigprocmask(SIG_BLOCK, &set, oldmask);
}

static void restore_mask(sigset_t *oldmask) {
    sigprocmask(SIG_SETMASK, oldmask, NULL);
}

static int run_simple(Command *cmd, int log_fd, Jobs *jobs) {
    sigset_t oldmask;
    block_sigchld(&oldmask);

    pid_t pid = fork();
    if (pid < 0) {
        restore_mask(&oldmask);
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        child_reset_signals();
        apply_redirs_simple(cmd);
        execvp(cmd->argv[0], cmd->argv);
        perror(cmd->argv[0]);
        _exit(127);
    }

    if (cmd->background) {
        jobs_add(jobs, pid, cmd->rawline);
        printf("[bg] started pid %d\n", (int)pid);
        restore_mask(&oldmask);
        return 0;
    }

    int status;
    if (waitpid(pid, &status, 0) < 0) {
        restore_mask(&oldmask);
        perror("waitpid");
        return -1;
    }

    logger_log(log_fd, pid, cmd->rawline, status);
    restore_mask(&oldmask);
    return 0;
}

static int run_pipe(Command *cmd, int log_fd, Jobs *jobs) {
    Command *left = cmd;
    Command *right = cmd->pipe_cmd;

    int pfd[2];
    if (pipe(pfd) < 0) {
        perror("pipe");
        return -1;
    }

    sigset_t oldmask;
    block_sigchld(&oldmask);

    pid_t lp = fork();
    if (lp < 0) {
        restore_mask(&oldmask);
        close(pfd[0]);
        close(pfd[1]);
        perror("fork");
        return -1;
    }

    if (lp == 0) {
        child_reset_signals();
        close(pfd[0]);
        if (dup2(pfd[1], STDOUT_FILENO) < 0) {
            perror("dup2");
            _exit(1);
        }
        close(pfd[1]);
        apply_redirs_left_pipe(left);
        execvp(left->argv[0], left->argv);
        perror(left->argv[0]);
        _exit(127);
    }

    pid_t rp = fork();
    if (rp < 0) {
        restore_mask(&oldmask);
        close(pfd[0]);
        close(pfd[1]);
        perror("fork");
        return -1;
    }

    if (rp == 0) {
        child_reset_signals();
        close(pfd[1]);
        if (dup2(pfd[0], STDIN_FILENO) < 0) {
            perror("dup2");
            _exit(1);
        }
        close(pfd[0]);
        apply_redirs_right_pipe(right);
        execvp(right->argv[0], right->argv);
        perror(right->argv[0]);
        _exit(127);
    }

    close(pfd[0]);
    close(pfd[1]);

    if (cmd->background) {
        jobs_add(jobs, lp, cmd->rawline);
        jobs_add(jobs, rp, cmd->rawline);
        printf("[bg] started pid %d\n", (int)rp);
        restore_mask(&oldmask);
        return 0;
    }

    int st1 = 0, st2 = 0;
    waitpid(lp, &st1, 0);
    waitpid(rp, &st2, 0);

    logger_log(log_fd, lp, cmd->rawline, st1);
    logger_log(log_fd, rp, cmd->rawline, st2);

    restore_mask(&oldmask);
    return 0;
}

int execute_command(Command *cmd, int log_fd, Jobs *jobs) {
    if (!cmd || cmd->argc == 0) return 0;
    if (cmd->has_pipe) return run_pipe(cmd, log_fd, jobs);
    return run_simple(cmd, log_fd, jobs);
}

