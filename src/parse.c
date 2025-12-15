#include "parse.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static char *xstrdup(const char *s) {
    size_t n = strlen(s);
    char *p = (char *)malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n + 1);
    return p;
}

static char *trim_copy(const char *line) {
    while (*line && (*line == ' ' || *line == '\t' || *line == '\n' || *line == '\r')) line++;
    size_t n = strlen(line);
    while (n > 0 && (line[n-1] == ' ' || line[n-1] == '\t' || line[n-1] == '\n' || line[n-1] == '\r')) n--;
    char *out = (char *)malloc(n + 1);
    if (!out) return NULL;
    memcpy(out, line, n);
    out[n] = '\0';
    return out;
}

static int is_special_char(char c) {
    return (c == '<' || c == '>' || c == '|' || c == '&');
}

static int add_token(char ***tokens, int *ntok, int *cap, const char *start, size_t len) {
    if (*ntok + 1 >= *cap) {
        int newcap = (*cap == 0) ? 16 : (*cap * 2);
        char **tmp = (char **)realloc(*tokens, sizeof(char *) * newcap);
        if (!tmp) return -1;
        *tokens = tmp;
        *cap = newcap;
    }
    char *t = (char *)malloc(len + 1);
    if (!t) return -1;
    memcpy(t, start, len);
    t[len] = '\0';
    (*tokens)[*ntok] = t;
    (*ntok)++;
    (*tokens)[*ntok] = NULL;
    return 0;
}

static int tokenize(const char *s, char ***out_tokens, int *out_n) {
    char **tokens = NULL;
    int ntok = 0, cap = 0;

    const char *p = s;
    while (*p) {
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;

        if (*p == '>') {
            if (*(p+1) == '>') {
                if (add_token(&tokens, &ntok, &cap, p, 2) < 0) goto fail;
                p += 2;
            } else {
                if (add_token(&tokens, &ntok, &cap, p, 1) < 0) goto fail;
                p += 1;
            }
            continue;
        }

        if (is_special_char(*p)) {
            if (add_token(&tokens, &ntok, &cap, p, 1) < 0) goto fail;
            p += 1;
            continue;
        }

        const char *start = p;
        while (*p && !isspace((unsigned char)*p) && !is_special_char(*p)) p++;
        if (p > start) {
            if (add_token(&tokens, &ntok, &cap, start, (size_t)(p - start)) < 0) goto fail;
        }
    }

    *out_tokens = tokens;
    *out_n = ntok;
    return 0;

fail:
    if (tokens) {
        for (int i = 0; i < ntok; i++) free(tokens[i]);
        free(tokens);
    }
    return -1;
}

static void free_tokens(char **tokens) {
    if (!tokens) return;
    for (int i = 0; tokens[i]; i++) free(tokens[i]);
    free(tokens);
}

static int argv_push(char ***argv, int *argc, int *cap, const char *s) {
    if (*argc + 1 >= *cap) {
        int newcap = (*cap == 0) ? 8 : (*cap * 2);
        char **tmp = (char **)realloc(*argv, sizeof(char *) * newcap);
        if (!tmp) return -1;
        *argv = tmp;
        *cap = newcap;
    }
    (*argv)[*argc] = xstrdup(s);
    if (!(*argv)[*argc]) return -1;
    (*argc)++;
    (*argv)[*argc] = NULL;
    return 0;
}

static Command *parse_segment(char **tokens, int start, int end, const char **err_msg) {
    Command *cmd = (Command *)calloc(1, sizeof(Command));
    if (!cmd) {
        *err_msg = "out of memory";
        return NULL;
    }

    char **argv = NULL;
    int argc = 0, cap = 0;

    for (int i = start; i < end; i++) {
        char *t = tokens[i];

        if (strcmp(t, "<") == 0) {
            if (i + 1 >= end) {
                *err_msg = "syntax error near <";
                goto fail;
            }
            free(cmd->in_file);
            cmd->in_file = xstrdup(tokens[i+1]);
            if (!cmd->in_file) { *err_msg = "out of memory"; goto fail; }
            i++;
            continue;
        }

        if (strcmp(t, ">") == 0) {
            if (i + 1 >= end) {
                *err_msg = "syntax error near >";
                goto fail;
            }
            free(cmd->out_file);
            cmd->out_file = xstrdup(tokens[i+1]);
            if (!cmd->out_file) { *err_msg = "out of memory"; goto fail; }
            cmd->out_append = 0;
            i++;
            continue;
        }

        if (strcmp(t, ">>") == 0) {
            if (i + 1 >= end) {
                *err_msg = "syntax error near >>";
                goto fail;
            }
            free(cmd->out_file);
            cmd->out_file = xstrdup(tokens[i+1]);
            if (!cmd->out_file) { *err_msg = "out of memory"; goto fail; }
            cmd->out_append = 1;
            i++;
            continue;
        }

        if (argv_push(&argv, &argc, &cap, t) < 0) {
            *err_msg = "out of memory";
            goto fail;
        }
    }

    if (argc == 0) {
        *err_msg = "empty command";
        goto fail;
    }

    cmd->argv = argv;
    cmd->argc = argc;
    return cmd;

fail:
    if (argv) {
        for (int i = 0; i < argc; i++) free(argv[i]);
        free(argv);
    }
    free(cmd->in_file);
    free(cmd->out_file);
    free(cmd);
    return NULL;
}

Command *parse_line(const char *line, const char **err_msg) {
    *err_msg = NULL;

    char *trimmed = trim_copy(line);
    if (!trimmed) {
        *err_msg = "out of memory";
        return NULL;
    }
    if (trimmed[0] == '\0') {
        free(trimmed);
        *err_msg = "empty";
        return NULL;
    }

    char **tokens = NULL;
    int ntok = 0;
    if (tokenize(trimmed, &tokens, &ntok) < 0) {
        free(trimmed);
        *err_msg = "tokenize failed";
        return NULL;
    }

    if (ntok == 0) {
        free_tokens(tokens);
        free(trimmed);
        *err_msg = "empty";
        return NULL;
    }

    int background = 0;
    if (strcmp(tokens[ntok - 1], "&") == 0) {
        background = 1;
        free(tokens[ntok - 1]);
        tokens[ntok - 1] = NULL;
        ntok--;
        if (ntok == 0) {
            free_tokens(tokens);
            free(trimmed);
            *err_msg = "syntax error near &";
            return NULL;
        }
    } else {
        for (int i = 0; i < ntok; i++) {
            if (strcmp(tokens[i], "&") == 0) {
                free_tokens(tokens);
                free(trimmed);
                *err_msg = "& must be at end of line";
                return NULL;
            }
        }
    }

    int pipe_pos = -1;
    for (int i = 0; i < ntok; i++) {
        if (strcmp(tokens[i], "|") == 0) {
            if (pipe_pos != -1) {
                free_tokens(tokens);
                free(trimmed);
                *err_msg = "only one pipe supported";
                return NULL;
            }
            pipe_pos = i;
        }
    }

    Command *cmd = NULL;

    if (pipe_pos == -1) {
        cmd = parse_segment(tokens, 0, ntok, err_msg);
        if (!cmd) {
            free_tokens(tokens);
            free(trimmed);
            return NULL;
        }
    } else {
        if (pipe_pos == 0 || pipe_pos == ntok - 1) {
            free_tokens(tokens);
            free(trimmed);
            *err_msg = "syntax error near |";
            return NULL;
        }

        Command *left = parse_segment(tokens, 0, pipe_pos, err_msg);
        if (!left) {
            free_tokens(tokens);
            free(trimmed);
            return NULL;
        }
        Command *right = parse_segment(tokens, pipe_pos + 1, ntok, err_msg);
        if (!right) {
            free_command(left);
            free_tokens(tokens);
            free(trimmed);
            return NULL;
        }
        left->has_pipe = 1;
        left->pipe_cmd = right;
        cmd = left;
    }

    cmd->background = background;
    cmd->rawline = xstrdup(trimmed);

    free_tokens(tokens);
    free(trimmed);

    if (!cmd->rawline) {
        free_command(cmd);
        *err_msg = "out of memory";
        return NULL;
    }

    return cmd;
}

void free_command(Command *cmd) {
    if (!cmd) return;

    if (cmd->has_pipe && cmd->pipe_cmd) {
        free_command(cmd->pipe_cmd);
        cmd->pipe_cmd = NULL;
    }

    if (cmd->argv) {
        for (int i = 0; i < cmd->argc; i++) free(cmd->argv[i]);
        free(cmd->argv);
    }

    free(cmd->in_file);
    free(cmd->out_file);
    free(cmd->rawline);
    free(cmd);
}

