#ifndef PROCESS_COMPARE_H
#define PROCESS_COMPARE_H

#include "process.h"

int compare_by_cpu(const void *a, const void *b);
int compare_by_memory(const void *a, const void *b); 
int compare_by_pid(const void *a, const void *b);
int compare_by_user(const void *a, const void *b);
int compare_by_command(const void *a, const void *b);

#endif // PROCESS_COMPARE_H
