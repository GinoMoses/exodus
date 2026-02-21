#include "cpu.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int read_cpu_stats(cpu_set *cpu) {
    FILE *file = fopen("/proc/stat", "r");

    if(!file) {
        perror("Error opening /proc/stat\n");
        return -1;
    }

    char line[256];   
    size_t capacity = 4; // initial capacity for 4 cores
    cpu->cores = malloc(capacity * sizeof(cpu_core_stats));
    
    if (!cpu->cores) {
        perror("Memory allocation failed\n");
        fclose(file);
        return -1;
    }

    cpu->count = 0;
    
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "cpu", 3) != 0) break;
        if (line[3] == ' ') continue;
        
        unsigned int core_index;
        cpu_core_stats stats;

        int scanned = sscanf(line, "cpu%u %llu %llu %llu %llu %llu %llu %llu %llu",
                &core_index,
                &stats.user,
                &stats.nice,
                &stats.system,
                &stats.idle,
                &stats.iowait,
                &stats.irq,
                &stats.softirq,
                &stats.steal
        );

        if (scanned < 9) continue;
        // reallocate if more cores present
        if (cpu->count >= capacity) {
            capacity *= 2;
            cpu_core_stats *tmp = realloc(cpu->cores, capacity * sizeof(cpu_core_stats));

            if (!tmp) {
                perror("Memory reallocation failed\n");
                free(cpu->cores);
                fclose(file);
                return -1;
            }

            cpu->cores = tmp;
        }

        cpu->cores[cpu->count++] = stats;
    }
   
    fclose(file);
    return 0;
}

double calculate_core_usage(const cpu_core_stats *current, const cpu_core_stats *previous) {
    // sum of idle jiffies
    unsigned long long prev_idle = previous->idle + previous->iowait;
    unsigned long long curr_idle = current->idle + current->iowait;

    // sum of total jiffies
    unsigned long long prev_total = prev_idle 
    + previous->user + previous->nice + previous->system 
    + previous->irq + previous->softirq + previous->steal;
    unsigned long long curr_total = curr_idle
    + current->user + current->nice + current->system + 
    current->irq + current->softirq + current->steal;

    // calculate the difference in idle and total jiffies
    unsigned long long total_delta = curr_total - prev_total; 
    unsigned long long idle_delta = curr_idle - prev_idle;

    if (total_delta == 0) return 0.0; // avoid dividing by zero

    // return usage in %
    return (double)(total_delta - idle_delta) / total_delta * 100.0;
}

void free_cpu_set(cpu_set *cpu) {
    if (cpu->cores) {
        free(cpu->cores);
        cpu->cores = NULL;
    }

    cpu->count = 0;
}
