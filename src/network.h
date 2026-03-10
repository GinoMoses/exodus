#ifndef NETWORK_H
#define NETWORK_H

#include <stddef.h>

typedef struct {
    char name[32];
    unsigned long long rx_bytes;
    unsigned long long tx_bytes;
} network_stats_t;

int get_default_route_interface(char *out_iface, size_t out_size);
int read_network_stats(network_stats_t *stats, const char *interface);

#endif // network.h
