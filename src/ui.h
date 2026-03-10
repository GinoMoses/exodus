#ifndef UI_H
#define UI_H

#include "memory.h"
#include "network.h"
#include "system.h"
#include "process.h"

void shutdown_ui(void);
void initialize_ui(void);
void scroll_process_list(int offset);
void refresh_process_window(const process_list_t *processes);
void cycle_sort_mode(int direction);
const char* get_process_sort_mode_name(void);
int kill_process(const process_list_t *processes);
void update_ui(double *cpu_usage, size_t core_count, const memory_stats_t *memory, const network_stats_t *network, 
        const system_stats_t *system, const process_list_t *processes);

#endif // UI_H
