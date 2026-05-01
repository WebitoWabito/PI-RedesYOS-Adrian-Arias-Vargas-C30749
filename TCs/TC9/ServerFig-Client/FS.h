#ifndef FS_H
#define FS_H

#include <vector>
#include <string>

using namespace std;

const int MAX_CLUSTERS = 100;
const int CLUSTER_SIZE = 256;

struct Cluster {
    char data[CLUSTER_SIZE];
};

struct FileEntry {
    string name;
    int startCluster;
};

class FileSystem {
private:
    Cluster disk[MAX_CLUSTERS];
    bool bitmap[MAX_CLUSTERS];
    int FAT[MAX_CLUSTERS];

    vector<FileEntry> directory;

    int findFreeCluster();
    string serializeFigure(string name, vector<string> pieces);

public:
    FileSystem();

    void saveFigure(string name, vector<string> pieces);
    string readFigure(string name);
    string listFigures();
};

#endif