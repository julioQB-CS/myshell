#ifndef LOGGER_H
#define LOGGER_H

#include <sys/types.h>

int logger_open(const char *path);
void logger_log(int fd, pid_t pid, const char *cmdline, int status);
void logger_close(int fd);

#endif

