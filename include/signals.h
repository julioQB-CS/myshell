#ifndef SIGNALS_H
#define SIGNALS_H

#include <sys/types.h>
#include <stddef.h>

typedef struct Reaped {
    pid_t pid;
    int status;
} Reaped;

int signals_init(int sigchld_pipe[2]);
ssize_t signals_read_reaped(int fd, Reaped *buf, size_t max);

#endif

