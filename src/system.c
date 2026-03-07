#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <dirent.h>

#include "system.h"

int read_system_stats(system_stats_t *stats) {
    if (!stats) return -1;

    FILE *uptime_file = fopen("/proc/uptime", "r");
    if (uptime_file) {
        double uptime;
        if (fscanf(uptime_file, "%lf", &uptime) == 1) {
            stats->uptime_seconds = (long)uptime;
        }
        fclose(uptime_file);
    }

    FILE *loadavg_file = fopen("/proc/loadavg", "r");
    if (loadavg_file) {
        fscanf(loadavg_file, "%lf %lf %lf", 
                &stats->load_1min, &stats->load_5min, &stats->load_15min);
        fclose(loadavg_file);
    }

    stats->process_count = 0;
    DIR *proc_dir = opendir("/proc");
    if (proc_dir) {
        struct dirent *entry;
        while ((entry = readdir(proc_dir)) != NULL) {
            if (entry->d_type == DT_DIR) {
                char *endptr;
                strtol(entry->d_name, &endptr, 10);
                if (*endptr == '\0') {
                    stats->process_count++;
                }
            }
        }
        closedir(proc_dir);
    }

    struct utsname sys_info;
    if (uname(&sys_info) == 0) {
        snprintf(stats->kernel_version, sizeof(stats->kernel_version), 
                "%s", sys_info.release);
    }

    return 0;
}
