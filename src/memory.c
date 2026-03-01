#include <stdio.h>
#include <string.h>

#include "memory.h"

int read_memory_stats(memory_stats_t *memory) {
    if (!memory) {
        fprintf(stderr, "Invalid memory_stats_t pointer\n");
        return -1;
    }

    FILE *file = fopen("/proc/meminfo", "r");
    
    if (!file) {
        perror("Error opening /proc/meminfo\n");
        return -1;
    }

    memory->total = 0;
    memory->free = 0;
    memory->buffers = 0;
    memory->cached = 0;
    memory->used = 0;

    char key[64];
    unsigned long long value;
    char unit[8];

    while (fscanf(file, "%63s %llu %7s", key, &value, unit) == 3) {
        if (strcmp(key, "MemTotal:") == 0) memory->total = value;
        else if (strcmp(key, "MemFree:") == 0) memory->free = value;
        else if (strcmp(key, "Buffers:") == 0) memory->buffers = value;
        else if (strcmp(key, "Cached:") == 0) memory->cached = value;

        if (memory->total && memory->free && memory->buffers && memory->cached) break;  
    }

    fclose(file);

    if (memory->total == 0) {
        fprintf(stderr, "Failed to read total memory from /proc/meminfo\n");
        return -1;
    } else {
        memory->used = memory->total - memory->free - memory->buffers - memory->cached;
    }

    return 0;
}
