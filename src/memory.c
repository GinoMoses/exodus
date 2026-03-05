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
    memory->available = 0;
    memory->shmem = 0;
    memory->sreclaimable = 0;

    char key[64];
    unsigned long long value;
    char unit[8];

    while (fscanf(file, "%63s %llu %7s", key, &value, unit) == 3) {
        if (strcmp(key, "MemTotal:") == 0) memory->total = value;
        else if (strcmp(key, "MemFree:") == 0) memory->free = value;
        else if (strcmp(key, "Buffers:") == 0) memory->buffers = value;
        else if (strcmp(key, "Cached:") == 0) memory->cached = value;
        else if (strcmp(key, "Available:") == 0) memory->available = value;
        else if (strcmp(key, "Shmem:") == 0) memory->shmem = value;
        else if (strcmp(key, "SReclaimable:") == 0) memory->sreclaimable = value;
    }

    fclose(file);

    if (memory->total == 0) {
        fprintf(stderr, "Failed to read total memory from /proc/meminfo\n");
        return -1;
    }  

    if (memory->available > 0) {
        memory->used = memory->total - memory->available;
    } else {
        // fallback for kernels pre 3.14
        memory->used = memory->total - memory->free - memory->buffers - memory->cached;
    }

    return 0;
}
