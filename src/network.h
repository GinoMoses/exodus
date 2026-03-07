#ifndef NETWORK_H
#define NETWORK_H

#include <stddef.h>

typedef struct {
    char name[32];
    unsigned long long rx_bytes;
    unsigned long long tx_bytes;
} network_stats_t;

int read_network_stats(network_stats_t *stats, const char *interface);

#endif // network.h
