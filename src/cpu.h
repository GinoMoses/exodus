#ifndef CPU_H
#define CPU_H

#include <stddef.h>
// man /proc/stat
typedef struct {
    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
    unsigned long long steal;
} cpu_core_stats_t;

typedef struct {
    cpu_core_stats_t *cores;
    size_t count;
} cpu_set_t;

int read_cpu_stats(cpu_set_t *stats);
double calculate_core_usage(const cpu_core_stats_t *current, const cpu_core_stats_t *previous);
void free_cpu_set(cpu_set_t *stats);

#endif // CPU_H
