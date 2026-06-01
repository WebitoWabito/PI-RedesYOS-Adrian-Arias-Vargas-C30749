#include "Disk.h"

Disk::Disk(const std::string& filename) {
    file.open(filename, std::ios::in | std::ios::out | std::ios::binary);

    if (!file.is_open()) {
        file.open(filename, std::ios::out | std::ios::binary);
        file.close();

        file.open(filename, std::ios::in | std::ios::out | std::ios::binary);
    }
}

void Disk::writeCluster(int index, const Cluster& cluster) {
    file.seekp(index * sizeof(Cluster));
    file.write(reinterpret_cast<const char*>(&cluster), sizeof(Cluster));
}

Cluster Disk::readCluster(int index) {
    Cluster cluster;
    file.seekg(index * sizeof(Cluster));
    file.read(reinterpret_cast<char*>(&cluster), sizeof(Cluster));
    return cluster;
}