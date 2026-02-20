#include"cpu.h"
#include <stdio.h>

int read_cpu_stats(cpu_stats *stats) {
    FILE *file = fopen("/proc/stat", "r");

    if(!file) {
        printf("Error opening /proc/stat\n");
        return -1;
    }

    fscanf(file, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
        &stats->user,
        &stats->nice,
        &stats->system,
        &stats->idle,
        &stats->iowait,
        &stats->irq,
        &stats->softirq,
        &stats->steal
    );
   
    fclose(file);

    return 0;
}

double calculate_cpu_usage(const cpu_stats *current, const cpu_stats *previous) {
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

    // return CPU usage in %
    return (double)(total_delta - idle_delta) / total_delta * 100.0;
}
