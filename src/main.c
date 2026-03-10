#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "cpu.h"
#include "memory.h"
#include "ui.h"
#include "input.h"
#include "network.h"
#include "system.h"
#include "process.h"

int main(void) {
    cpu_set_t current = {0}, previous = {0};
    memory_stats_t memory = {0};
    network_stats_t network = {0};
    system_stats_t system = {0};
    process_list_t processes = {0};

    if (read_cpu_stats(&previous) != 0) {
        return -1;
    }

    if (read_memory_stats(&memory) != 0) {
        return -1;
    }
     
    size_t core_count = previous.count;
    double *cpu_usage = calloc(core_count, sizeof(double));

    initialize_ui();
    update_ui(cpu_usage, core_count, &memory, &network, &system, &processes);

    while (1) {
        // instead of sleep use time to check for input
        for (size_t i = 0; i < 10; i++) {
            usleep(100000);

            input_action_t action = handle_input();
            if (action == INPUT_QUIT) {
                goto cleanup;
            } else if (action == INPUT_ARROW_UP) {
                scroll_process_list(-1);
                refresh_process_window(&processes);
            } else if (action == INPUT_ARROW_DOWN) {
                scroll_process_list(1);
                refresh_process_window(&processes);
            } else if (action == INPUT_PAGE_UP) {
                scroll_process_list(-10);
                refresh_process_window(&processes);
            } else if (action == INPUT_PAGE_DOWN) {
                scroll_process_list(10);
                refresh_process_window(&processes);
            }
        }

        if (read_cpu_stats(&current) != 0) {
            free(cpu_usage);
            shutdown_ui();
            return -1;
        }

        for (size_t i = 0; i < core_count; i++) {
            cpu_usage[i] = calculate_core_usage(&current.cores[i], &previous.cores[i]);
        }
        
        read_memory_stats(&memory);
        read_network_stats(&network, NULL);
        read_system_stats(&system);
        read_processes(&processes);
        calculate_process_stats(&processes, memory.total);

        update_ui(cpu_usage, core_count, &memory, &network, &system, &processes);
        
        free_cpu_set(&previous);
        previous = current;
        current.cores = NULL;
        current.count = 0;
    }

    cleanup:
        free_cpu_set(&current);
        free_cpu_set(&previous);
        free(cpu_usage);
        free_process_list(&processes);
        shutdown_ui();
        return 0;
}

