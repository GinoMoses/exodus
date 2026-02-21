#ifndef UI_H
#define UI_H

#include <stddef.h>

void shutdown_ui(void);
void initialize_ui(void);
void draw_cpu(double *cpu_usage, size_t core_count);

#endif // UI_H
