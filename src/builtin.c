#include "builtin.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int builtin_execute(Command *cmd) {
    if (!cmd || cmd->argc == 0 || !cmd->argv || !cmd->argv[0]) return BUILTIN_NONE;

    if (strcmp(cmd->argv[0], "exit") == 0 || strcmp(cmd->argv[0], "quit") == 0) {
        return BUILTIN_EXIT;
    }

    if (strcmp(cmd->argv[0], "cd") == 0) {
        const char *path = NULL;
        if (cmd->argc >= 2) path = cmd->argv[1];
        if (!path || path[0] == '\0') path = getenv("HOME");
        if (!path) {
            fprintf(stderr, "myshell: cd: HOME not set\n");
            return BUILTIN_HANDLED;
        }
        if (chdir(path) < 0) {
            perror("cd");
        }
        return BUILTIN_HANDLED;
    }

    return BUILTIN_NONE;
}

