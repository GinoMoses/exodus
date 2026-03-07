#ifndef SYSTEM_H
#define SYSTEM_H

typedef struct {
    long uptime_seconds;
    double load_1min;
    double load_5min;
    double load_15min;
    int process_count;
    char kernel_version[128];
} system_stats_t;

int read_system_stats(system_stats_t *stats);

#endif // SYSTEM_H
