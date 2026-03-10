#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <ctype.h>
#include <unistd.h>

#include "process.h"

static process_list_t prev_processes = {0};
static int processes_initialized = 0;

void calculate_process_stats(process_list_t *current, unsigned long long total_mem_kb) {
    if (!current) return;

    for (size_t i = 0; i < current->count; i++) {
        process_t *proc = &current->processes[i];

        if (total_mem_kb > 0) {
            proc->mem_percent = ((double)proc->vsize / 1024) / total_mem_kb * 100.0;
        }
        
        if (!processes_initialized) {
            proc->cpu_percent = 0.0;
            continue;
        }

        process_t *prev_proc = NULL;
        for (size_t j = 0; j < prev_processes.count; j++) {
            if (prev_processes.processes[j].pid == proc->pid) {
                prev_proc = &prev_processes.processes[j];
                break;
            }
        }

        if (prev_proc) {
            unsigned long long total_time = (proc->utime + proc->stime) - 
                                            (prev_proc->utime + prev_proc->stime);
        
            long hz = sysconf(_SC_CLK_TCK);
            proc->cpu_percent = ((double)total_time / hz) * 100.0;
        
            long num_cores = sysconf(_SC_NPROCESSORS_ONLN);

            if (num_cores > 0 && proc->cpu_percent > 100.0 * num_cores) {
                proc->cpu_percent = 100.0 * num_cores;
            }
        } else {
            proc->cpu_percent = 0.0;
        }
    }

    if (prev_processes.processes) {
        free(prev_processes.processes);
    }
    prev_processes.count = current->count;
    prev_processes.capacity = current->capacity;
    prev_processes.processes = malloc(current->count * sizeof(process_t));

    if (prev_processes.processes) {
        memcpy(prev_processes.processes, current->processes, current->count * sizeof(process_t));
    }
    processes_initialized = 1;
}

static int read_proc_stat(int pid, process_t *proc) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE *file = fopen(path, "r");
    if (!file) return -1;

    char state;
    unsigned long  utime, stime, vsize;
    int ppid, pgrp, session, tty_nr, tpgid;
    unsigned long flags, minflt, cminflt, majflt, cmajflt;
    long cutime, cstime, priority, nice, num_threads, itrealvalue;
    unsigned long long starttime;
    long rss;
    
    // see man proc if you're curious about this THING
    int scanned = fscanf(file, "%d %*s %c %d %d %d %d %d %lu %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld %ld %llu %lu %ld",
        &proc->pid, &state, &ppid, &pgrp, &session, &tty_nr, &tpgid,
        &flags, &minflt, &cminflt, &majflt, &cmajflt,
        &utime, &stime, &cutime, &cstime, &priority, &nice,
        &num_threads, &itrealvalue, &starttime, &vsize, &rss);

    fclose(file);

    if (scanned < 23) return -1;

    proc->state = state;
    proc->utime = utime;
    proc->stime = stime;
    proc->vsize = vsize;

    proc->priority = priority;
    proc->nice = nice;

    return 0;
}

static int read_proc_status(int pid, process_t *proc) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *file = fopen(path, "r");
    if (!file) return -1;

    char line[256];
    int uid = -1;

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "Uid:", 4) == 0) {
            sscanf(line + 4, "%d", &uid);
            break;
        }
    }

    fclose(file);

    if (uid >= 0) {
        struct passwd *pw = getpwuid(uid);
        if (pw) {
            strncpy(proc->user, pw->pw_name, sizeof(proc->user) - 1);
        } else {
            snprintf(proc->user, sizeof(proc->user), "%d", uid);
        }
    }

    return 0;
}

static int read_proc_cmdline(int pid, process_t *proc) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);

    FILE *file = fopen(path, "r");
    if (!file) return -1;

    size_t len = fread(proc->command, 1, sizeof(proc->command) - 1, file);
    fclose(file);

    if (len == 0) {
        snprintf(path, sizeof(path), "/proc/%d/comm", pid);
        file = fopen(path, "r");

        if (file) {
            fgets(proc->command, sizeof(proc->command), file);
            proc->command[strcspn(proc->command, "\n")] = 0;
            fclose(file);
        }
    } else {
        for (size_t i = 0; i < len; i++) {
            if (proc->command[i] == '\0') proc->command[i] = ' ';
        }
        proc->command[len] = '\0';
    }

    return 0;
}

int read_processes(process_list_t *list) {
    if (!list) return -1;

    list->count = 0;

    if (!list->processes) {
        list->capacity = 256;
        list->processes = malloc(list->capacity * sizeof(process_t));
    }

    DIR *proc_dir = opendir("/proc");
    if (!proc_dir) return -1;

    struct dirent *entry;
    while ((entry = readdir(proc_dir)) != NULL) {
        if (!isdigit(entry->d_name[0])) continue;

        int pid = atoi(entry->d_name);

        if (list->count >= list->capacity) {
            list->capacity *= 2;
            process_t *tmp = realloc(list->processes, list->capacity * sizeof(process_t));

            if (!tmp) {
                closedir(proc_dir);
                return -1;
            }
            list->processes = tmp;
        }

        process_t *proc = &list->processes[list->count];
        memset(proc, 0, sizeof(process_t));
        proc->pid = pid;

        if (read_proc_stat(pid, proc) != 0) continue;
        read_proc_status(pid, proc);
        read_proc_cmdline(pid, proc);

        proc->cpu_percent = 0.0;
        proc->mem_percent = 0.0;

        list->count++;
    }

    closedir(proc_dir);
    return 0;
}

void free_process_list(process_list_t *list) {
    if (list && list->processes) {
        free(list->processes);
        list->processes = NULL;
        list->count = 0;
        list->capacity = 0;
    }
}
