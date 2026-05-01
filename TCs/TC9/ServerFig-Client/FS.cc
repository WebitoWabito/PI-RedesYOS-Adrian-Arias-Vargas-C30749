#include "FS.h"
#include <cstring>

using namespace std;

FileSystem::FileSystem() {
    for (int i = 0; i < MAX_CLUSTERS; i++) {
        FAT[i] = -1;
        bitmap[i] = false;
    }
}

string FileSystem::serializeFigure(string name, vector<string> pieces) {
    string result = "Figura: " + name + "\n\n";
    result += "Total: " + to_string(pieces.size()) + " piezas\n\n";

    for (auto &p : pieces)
        result += p + "\n";

    return result;
}

int FileSystem::findFreeCluster() {
    for (int i = 0; i < MAX_CLUSTERS; i++) {
        if (!bitmap[i])
            return i;
    }
    return -1;
}

void FileSystem::saveFigure(string name, vector<string> pieces) {

    for (auto &file : directory) {
        if (file.name == name)
            return;
    }

    string data = serializeFigure(name, pieces);

    int prev = -1;
    int firstCluster = -1;
    vector<int> usedClusters;

    for (int i = 0; i < data.size(); i += CLUSTER_SIZE) {

        int freeCluster = findFreeCluster();

        if (freeCluster == -1) {
            for (int c : usedClusters) {
                bitmap[c] = false;
                FAT[c] = -1;
            }
            return;
        }

        bitmap[freeCluster] = true;
        usedClusters.push_back(freeCluster);

        string chunk = data.substr(i, CLUSTER_SIZE);

        memset(disk[freeCluster].data, 0, CLUSTER_SIZE);
        memcpy(disk[freeCluster].data, chunk.c_str(), chunk.size());

        if (prev != -1)
            FAT[prev] = freeCluster;

        prev = freeCluster;

        if (firstCluster == -1)
            firstCluster = freeCluster;
    }

    FAT[prev] = -1;

    directory.push_back({name, firstCluster});
}

string FileSystem::readFigure(string name) {
    for (auto &file : directory) {
        if (file.name == name) {

            int current = file.startCluster;
            string result = "";

            while (current != -1) {
                result += disk[current].data;
                current = FAT[current];
            }

            return result;
        }
    }

    return "ERROR: Figura no encontrada";
}

string FileSystem::listFigures() {
    if (directory.empty())
        return "No hay figuras";

    string result = "";

    for (auto &file : directory) {
        result += file.name + ",";
    }

    return result;
}