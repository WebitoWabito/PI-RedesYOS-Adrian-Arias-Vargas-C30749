#pragma once
#include <fstream>
#include "Cluster.h"

class Disk {
private:
    std::fstream file;

public:
    Disk(const std::string& filename);
    void writeCluster(int index, const Cluster& cluster);
    Cluster readCluster(int index);
};