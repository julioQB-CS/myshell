#ifndef PARSE_H
#define PARSE_H

typedef struct Command {
    char **argv;
    int argc;

    char *in_file;
    char *out_file;
    int out_append;

    int background;

    int has_pipe;
    struct Command *pipe_cmd;

    char *rawline;
} Command;

Command *parse_line(const char *line, const char **err_msg);
void free_command(Command *cmd);

#endif

