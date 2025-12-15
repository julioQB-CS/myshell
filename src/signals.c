#include "signals.h"
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

static int g_sigchld_wfd = -1;

static void sigchld_handler(int signo) {
    (void)signo;
    int saved = errno;

    Reaped r;
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        r.pid = pid;
        r.status = status;
        if (g_sigchld_wfd >= 0) {
            ssize_t w = write(g_sigchld_wfd, &r, sizeof(r));
            (void)w;
        }
    }

    errno = saved;
}

static int set_cloexec(int fd) {
    int flags = fcntl(fd, F_GETFD);
    if (flags < 0) return -1;
    return fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
}

static int set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL);
    if (flags < 0) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int signals_init(int sigchld_pipe[2]) {
    if (pipe(sigchld_pipe) < 0) return -1;

    set_cloexec(sigchld_pipe[0]);
    set_cloexec(sigchld_pipe[1]);
    set_nonblock(sigchld_pipe[0]);

    g_sigchld_wfd = sigchld_pipe[1];

    signal(SIGINT, SIG_IGN);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    if (sigaction(SIGCHLD, &sa, NULL) < 0) return -1;
    return 0;
}

ssize_t signals_read_reaped(int fd, Reaped *buf, size_t max) {
    if (max == 0) return 0;
    return read(fd, buf, sizeof(Reaped) * max);
}

