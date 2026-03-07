#ifndef UI_H
#define UI_H

#include "memory.h"
#include "network.h"
#include "system.h"

void shutdown_ui(void);
void initialize_ui(void);
void update_ui(double *cpu_usage, size_t core_count, const memory_stats_t *memory, const network_stats_t *network, const system_stats_t *system);

#endif // UI_H
