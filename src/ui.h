#ifndef UI_H
#define UI_H

#include <stddef.h>

void shutdown_ui();
int initialize_ui(double *cpu_usage, size_t *core_count);

#endif // UI_H
