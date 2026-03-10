#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>

typedef struct {
    int pid;
    char user[32];
    char state;
    double cpu_percent;
    double mem_percent;
    char command[256];
    unsigned long long utime;
    unsigned long long stime;
    unsigned long long vsize;
} process_t;

typedef struct {
    process_t *processes;
    size_t count;
    size_t capacity;
} process_list_t;

void calculate_process_stats(process_list_t *current, unsigned long long total_mem_kb);
int read_processes(process_list_t *list);
void free_process_list(process_list_t *list);

#endif // PROCESS_H
