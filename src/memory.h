#ifndef MEMORY_H 
#define MEMORY_H

// man /proc/meminfo
typedef struct {
    unsigned long long total;
    unsigned long long free;
    unsigned long long buffers;
    unsigned long long cached;
    unsigned long long used;
} memory_stats_t;

int read_memory_stats(memory_stats_t *memory);

#endif // MEMORY_H
