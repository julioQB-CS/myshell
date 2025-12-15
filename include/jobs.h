#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>

typedef struct Job {
    pid_t pid;
    char *cmdline;
    struct Job *next;
} Job;

typedef struct Jobs {
    Job *head;
} Jobs;

void jobs_init(Jobs *jobs);
int jobs_add(Jobs *jobs, pid_t pid, const char *cmdline);
char *jobs_take_cmdline(Jobs *jobs, pid_t pid);
void jobs_cleanup(Jobs *jobs);

#endif

