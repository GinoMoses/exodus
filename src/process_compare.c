#include <string.h>

#include "process.h"

int compare_by_cpu(const void *a, const void *b) {
    const process_t *proc_a = (const process_t *)a;
    const process_t *proc_b = (const process_t *)b;

    if (proc_a->cpu_percent > proc_b->cpu_percent) return -1;
    if (proc_a->cpu_percent < proc_b->cpu_percent) return 1;
    return 0;
}

int compare_by_memory(const void *a, const void *b) {
    const process_t *proc_a = (const process_t *)a;
    const process_t *proc_b = (const process_t *)b;

    if (proc_a->mem_percent > proc_b->mem_percent) return -1;
    if (proc_a->mem_percent < proc_b->mem_percent) return 1;
    return 0;
}

int compare_by_pid(const void *a, const void *b) {
    const process_t *proc_a = (const process_t *)a;
    const process_t *proc_b = (const process_t *)b;

    return proc_a->pid - proc_b->pid;
}

int compare_by_user(const void *a, const void *b) {
    const process_t *proc_a = (const process_t *)a;
    const process_t *proc_b = (const process_t *)b;

    return strcmp(proc_a->user, proc_b->user);
}

int compare_by_command(const void *a, const void *b) {
    const process_t *proc_a = (const process_t *)a;
    const process_t *proc_b = (const process_t *)b;

    return strcmp(proc_a->command, proc_b->command);
}
