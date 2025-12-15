#ifndef BUILTIN_H
#define BUILTIN_H

#include "parse.h"

enum {
    BUILTIN_NONE = 0,
    BUILTIN_HANDLED = 1,
    BUILTIN_EXIT = 2
};

int builtin_execute(Command *cmd);

#endif

