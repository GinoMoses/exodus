#ifndef UI_H
#define UI_H

#include <stddef.h>

#include "memory.h"
#include "network.h"

void shutdown_ui(void);
void initialize_ui(void);
void update_ui(double *cpu_usage, size_t core_count, const memory_stats_t *memory, const network_stats_t *network);

#endif // UI_H
