#pragma once
#include <cstdint>

const int CLUSTER_SIZE = 256;

struct Cluster {
    char data[252]; // Data
    int32_t next;   // Pointer to next cluster
};