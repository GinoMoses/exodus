#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>

#include "cpu.h"
#include "memory.h"
#include "ui.h"
#include "input.h"
#include "network.h"
#include "system.h"
#include "process.h"

static pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool should_quit = false;

static cpu_stats_t current_cpu = {0}, previous_cpu = {0};
static memory_stats_t memory_data = {0};
static network_stats_t network_data = {0};
static system_stats_t system_data = {0};
static process_list_t process_data = {0};
static double *cpu_usage = NULL;
static size_t core_count = 0;

// seperate thread for input 
void* input_thread_func(void* arg) {
    (void)arg;

    char filter_buf[64] = "";

    while (1) {
        pthread_mutex_lock(&data_mutex);
        if (should_quit) {
            pthread_mutex_unlock(&data_mutex);
            break;
        }

        if (should_resize_windows()) {
            initialize_ui(core_count);
        }

        pthread_mutex_unlock(&data_mutex);

        if (is_filter_active()) {
            input_action_t a = handle_input();

            if (a == INPUT_FILTER_CANCEL) {
                filter_buf[0] = '\0';
                pthread_mutex_lock(&data_mutex);
                set_process_filter(filter_buf);
                set_filter_active(0);
                refresh_process_window(&process_data);
                pthread_mutex_unlock(&data_mutex);

            } else if (a == INPUT_FILTER_ACCEPT) {
                pthread_mutex_lock(&data_mutex);
                set_filter_active(0);
                refresh_process_window(&process_data);
                pthread_mutex_unlock(&data_mutex);

            } else if (a == INPUT_FILTER_BACKSPACE) {
                size_t len = strlen(filter_buf);
                if (len > 0) filter_buf[len - 1] = '\0';

                pthread_mutex_lock(&data_mutex);
                set_process_filter(filter_buf);
                refresh_process_window(&process_data);
                pthread_mutex_unlock(&data_mutex);

            } else if (a == INPUT_FILTER_CHAR) {
                char c = input_get_last_char();
                size_t len = strlen(filter_buf);
                if (len + 1 < sizeof(filter_buf)) {
                    filter_buf[len] = c;
                    filter_buf[len + 1] = '\0';

                    pthread_mutex_lock(&data_mutex);
                    set_process_filter(filter_buf);
                    refresh_process_window(&process_data);
                    pthread_mutex_unlock(&data_mutex);
                }
            }

            usleep(10000);
            continue; 
        }

        input_action_t action = handle_input();

        if (action == INPUT_QUIT) {
            pthread_mutex_lock(&data_mutex);
            should_quit = true;
            pthread_mutex_unlock(&data_mutex);
            break;

        } else if (action == INPUT_ARROW_UP) {
            scroll_process_list(-1);
            pthread_mutex_lock(&data_mutex);
            refresh_process_window(&process_data);
            pthread_mutex_unlock(&data_mutex);

        } else if (action == INPUT_ARROW_DOWN) {
            scroll_process_list(1);
            pthread_mutex_lock(&data_mutex);
            refresh_process_window(&process_data);
            pthread_mutex_unlock(&data_mutex);

        } else if (action == INPUT_PAGE_UP) {
            scroll_process_list(-10);
            pthread_mutex_lock(&data_mutex);
            refresh_process_window(&process_data);
            pthread_mutex_unlock(&data_mutex);

        } else if (action == INPUT_PAGE_DOWN) {
            scroll_process_list(10);
            pthread_mutex_lock(&data_mutex);
            refresh_process_window(&process_data);
            pthread_mutex_unlock(&data_mutex);

        } else if (action == INPUT_KILL) {
            pthread_mutex_lock(&data_mutex);
            kill_process(&process_data);
            pthread_mutex_unlock(&data_mutex);

        } else if (action == INPUT_SORT_NEXT) {
            cycle_sort_mode(1);
            pthread_mutex_lock(&data_mutex);
            refresh_process_window(&process_data);
            pthread_mutex_unlock(&data_mutex);

        } else if (action == INPUT_SORT_PREV) {
            cycle_sort_mode(-1);
            pthread_mutex_lock(&data_mutex);
            refresh_process_window(&process_data);
            pthread_mutex_unlock(&data_mutex);

        } else if (action == INPUT_FILTER) {
            strncpy(filter_buf, get_process_filter(), sizeof(filter_buf) - 1);
            filter_buf[sizeof(filter_buf) - 1] = '\0';

            pthread_mutex_lock(&data_mutex);
            set_filter_active(1);
            refresh_process_window(&process_data);
            pthread_mutex_unlock(&data_mutex);
        }

        usleep(10000);
    }

    return NULL;
}

int main(void) {
    if (read_cpu_stats(&previous_cpu) != 0) {
        return -1;
    }

    if (read_memory_stats(&memory_data) != 0) {
        return -1;
    }
     
    core_count = previous_cpu.count;
    cpu_usage = calloc(core_count, sizeof(double));

    initialize_ui(core_count);
    
    pthread_t input_thread;
    if (pthread_create(&input_thread, NULL, input_thread_func, NULL) != 0) {
        fprintf(stderr, "Failed to create input thread\n");
        shutdown_ui();
        return -1;
    }

    pthread_mutex_lock(&data_mutex);
    update_ui(cpu_usage, core_count, &memory_data, &network_data, &system_data, &process_data);
    pthread_mutex_unlock(&data_mutex);

    while (1) {
        pthread_mutex_lock(&data_mutex);
        bool quit = should_quit;
        pthread_mutex_unlock(&data_mutex);

        if (quit) break;

        for (size_t i = 0; i < 10; i++) {
            usleep(100000);

            pthread_mutex_lock(&data_mutex);
            bool quit = should_quit;
            pthread_mutex_unlock(&data_mutex);

            if (quit) break;
        }

        if (read_cpu_stats(&current_cpu) != 0) {
            pthread_mutex_lock(&data_mutex);
            should_quit = true;
            pthread_mutex_unlock(&data_mutex);
            break;
        }

        for (size_t i = 0; i < core_count; i++) {
            cpu_usage[i] = calculate_core_usage(&current_cpu.cores[i], &previous_cpu.cores[i]);
        }
        
        read_memory_stats(&memory_data);

        char iface[32];
        const char *iface_ptr = NULL;
        
        if (get_default_route_interface(iface, sizeof(iface)) == 0) {
            iface_ptr = iface;
        }
        
        read_network_stats(&network_data, iface_ptr);
        read_system_stats(&system_data);
        read_processes(&process_data);
        calculate_process_stats(&process_data, memory_data.total);
    
        pthread_mutex_lock(&data_mutex);
        update_ui(cpu_usage, core_count, &memory_data, &network_data, &system_data, &process_data);
        pthread_mutex_unlock(&data_mutex);

        free_cpu_set(&previous_cpu);
        previous_cpu = current_cpu;
        current_cpu.cores = NULL;
        current_cpu.count = 0;
    }

    // cleanup
    pthread_join(input_thread, NULL);
    pthread_mutex_destroy(&data_mutex);

    free_cpu_set(&current_cpu);
    free_cpu_set(&previous_cpu);
    free(cpu_usage);
    free_process_list(&process_data);
    shutdown_ui();
    
    return 0;
}

