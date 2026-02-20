#ifndef CPU_H
#define CPU_H

typedef struct {
    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
    unsigned long long steal;
} cpu_stats;

int read_cpu_stats(cpu_stats *stats);
double calculate_cpu_usage(const cpu_stats *current, const cpu_stats *previous);

#endif // CPU_H
