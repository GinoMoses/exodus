#include"cpu.h"
#include <unistd.h>
#include <stdio.h>

int main() {
    cpu_stats current = {0}, previous = {0};

    if(read_cpu_stats(&current) != 0) {
        return -1;
    }

    while (1) {
        sleep(1);
        
        if(read_cpu_stats(&current) != 0) {
            break;
        }

        printf("CPU Usage: %.2f%%\n", calculate_cpu_usage(&current, &previous));
        previous = current; // update previous stats
    }

    return 0;
}
