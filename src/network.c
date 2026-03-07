#include <stdio.h>
#include <string.h>

#include "network.h"

int read_network_stats(network_stats_t *stats, const char *interface) {
    if (!stats) return -1;

    FILE *file = fopen("/proc/net/dev", "r");
    if (!file) {
        perror("Failed to open /proc/net/dev");
        return -1;
    }

    char line[256];
    fgets(line, sizeof(line), file);
    fgets(line, sizeof(line), file);

    int found = 0;
    while (fgets(line, sizeof(line), file)) {
        char iface[32];
        unsigned long long rx_bytes, rx_packets, rx_errs, rx_drop, rx_fifo, rx_frame, rx_compressed, rx_multicast;
        unsigned long long tx_bytes, tx_packets, tx_errs, tx_drop, tx_fifo, tx_colls, tx_carrier, tx_compressed;
        int matches = sscanf(line, " %31[^:]: %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
            iface,
            &rx_bytes, &rx_packets, &rx_errs, &rx_drop, &rx_fifo, &rx_frame, &rx_compressed, &rx_multicast,
            &tx_bytes, &tx_packets, &tx_errs, &tx_drop, &tx_fifo, &tx_colls, &tx_carrier, &tx_compressed); 

        if (matches < 17) continue;

        char *iface_trimmed = iface;
        while (*iface_trimmed == ' ') iface_trimmed++;

        if (strcmp(iface_trimmed, "lo") == 0) continue;

        if (interface == NULL || strcmp(iface_trimmed, interface) == 0) {
            strncpy(stats->name, iface_trimmed, sizeof(stats->name) - 1);
            stats->rx_bytes = rx_bytes;
            stats->tx_bytes = tx_bytes;
            found = 1;
            break;
        }
    }

    fclose(file);
    return found ? 0 : -1;
}
