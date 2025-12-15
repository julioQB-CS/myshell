#include "logger.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

int logger_open(const char *path) {
    return open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
}

void logger_log(int fd, pid_t pid, const char *cmdline, int status) {
    if (fd < 0) return;

    int code = -1;
    int sig = 0;

    if (WIFEXITED(status)) {
        code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        sig = WTERMSIG(status);
        code = 128 + sig;
    }

    char buf[512];
    if (sig) {
        int n = snprintf(buf, sizeof(buf),
                         "[pid=%d] cmd=\"%s\" status=%d signal=%d\n",
                         (int)pid, cmdline ? cmdline : "", code, sig);
        if (n > 0) write(fd, buf, (size_t)n);
    } else {
        int n = snprintf(buf, sizeof(buf),
                         "[pid=%d] cmd=\"%s\" status=%d\n",
                         (int)pid, cmdline ? cmdline : "", code);
        if (n > 0) write(fd, buf, (size_t)n);
    }
}

void logger_close(int fd) {
    if (fd >= 0) close(fd);
}

