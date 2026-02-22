#ifndef UI_H
#define UI_H

#include <stddef.h>

#include "memory.h"

void shutdown_ui(void);
void initialize_ui(void);
int draw_cpu(double *cpu_usage, size_t core_count);
void draw_memory(const memory_stats_t *memory, int cpu_rows);

#endif // UI_H
