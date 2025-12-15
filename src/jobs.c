#include "jobs.h"
#include <stdlib.h>
#include <string.h>

static char *xstrdup(const char *s) {
    size_t n = strlen(s);
    char *p = (char *)malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n + 1);
    return p;
}

void jobs_init(Jobs *jobs) {
    jobs->head = NULL;
}

int jobs_add(Jobs *jobs, pid_t pid, const char *cmdline) {
    Job *j = (Job *)malloc(sizeof(Job));
    if (!j) return -1;
    j->pid = pid;
    j->cmdline = xstrdup(cmdline ? cmdline : "");
    if (!j->cmdline) {
        free(j);
        return -1;
    }
    j->next = jobs->head;
    jobs->head = j;
    return 0;
}

char *jobs_take_cmdline(Jobs *jobs, pid_t pid) {
    Job *prev = NULL;
    Job *cur = jobs->head;
    while (cur) {
        if (cur->pid == pid) {
            if (prev) prev->next = cur->next;
            else jobs->head = cur->next;
            char *s = cur->cmdline;
            free(cur);
            return s;
        }
        prev = cur;
        cur = cur->next;
    }
    return NULL;
}

void jobs_cleanup(Jobs *jobs) {
    Job *cur = jobs->head;
    while (cur) {
        Job *n = cur->next;
        free(cur->cmdline);
        free(cur);
        cur = n;
    }
    jobs->head = NULL;
}

