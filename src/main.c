#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "cpu.h"
#include "memory.h"
#include "ui.h"

int main(void) {
    cpu_set_t current = {0}, previous = {0};
    memory_stats_t memory = {0};

    if (read_cpu_stats(&previous) != 0) {
        return -1;
    }

    if (read_memory_stats(&memory) != 0) {
        return -1;
    }
     
    size_t core_count = previous.count;
    double *cpu_usage = malloc(core_count * sizeof(double));

    initialize_ui();

    // immidiately diplay UI with CPU values equal to 0
    int cpu_rows = draw_cpu(cpu_usage, core_count);
    draw_memory(&memory, cpu_rows);

    while (1) {
        sleep(1);

        if (read_cpu_stats(&current) != 0) {
            free(cpu_usage);
            return -1;
        }

        for (size_t i = 0; i < core_count; i++) {
            cpu_usage[i] = calculate_core_usage(&current.cores[i], &previous.cores[i]);
        }
        
        cpu_rows = draw_cpu(cpu_usage, core_count);
        
        read_memory_stats(&memory);
        draw_memory(&memory, cpu_rows);

        free_cpu_set(&previous);
        previous = current;
        current.cores = NULL; // prevent double free
        current.count = 0;
    }

    shutdown_ui();
    
    free(cpu_usage);
    free_cpu_set(&current);
    free_cpu_set(&previous);
    return 0;
}
