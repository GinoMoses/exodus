#include <stdio.h>
#include <string.h>

#include "network.h"

int get_default_route_interface(char *out_iface, size_t out_size) {
    if (!out_iface || out_size == 0) return -1;

    FILE *file = fopen("/proc/net/route", "r");
    if (!file) return -1;

    char line[256];
    if (!fgets(line, sizeof(line), file)) {
        fclose(file);
        return -1;
    }

    while (fgets(line, sizeof(line), file)) {
        char iface[32];
        unsigned long dest = 0;

        if (sscanf(line, "%31s %lx", iface, &dest) != 2) continue;
        if (dest == 0) {
            snprintf(out_iface, out_size, "%s", iface);
            fclose(file);
            return 0;
        }
    }

    fclose(file);
    return -1;
}

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
            snprintf(stats->name, sizeof(stats->name), "%s", iface_trimmed);
            stats->rx_bytes = rx_bytes;
            stats->tx_bytes = tx_bytes;
            found = 1;
            break;
        }
    }

    fclose(file);
    return found ? 0 : -1;
}
