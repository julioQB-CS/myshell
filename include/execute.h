#ifndef EXECUTE_H
#define EXECUTE_H

#include "parse.h"
#include "jobs.h"

int execute_command(Command *cmd, int log_fd, Jobs *jobs);

#endif

