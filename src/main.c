#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "cpu.h"
#include "ui.h"

int main() {
    cpu_set current = {0}, previous = {0};

    if (read_cpu_stats(&previous) != 0) {
        return -1;
    }
    
    size_t core_count = previous.count;
    double *usage = malloc(core_count * sizeof(double));

    while (1) {
        sleep(1);

        if (read_cpu_stats(&current) != 0) {
            free(usage);
            return -1;
        }

        for (size_t i = 0; i < core_count; i++) {
            usage[i] = calculate_core_usage(&current.cores[i], &previous.cores[i]);
        }

        for (size_t i = 0; i < core_count; i++) {
            printf("Core %zu: %.2f%%\n", i+1, usage[i]);
        }

        cpu_set temp = previous;
        previous = current;
        current = temp;
    }

    free_cpu_set(&current);
    free_cpu_set(&previous);
    return 0;
}
