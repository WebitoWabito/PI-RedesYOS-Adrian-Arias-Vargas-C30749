using namespace std;
#include "FileSystem.h"
#include <iostream>
#include <cstring>

FileSystem::FileSystem(const std::string& filename, int numClusters)
    : disk(filename), bitmap(numClusters) {
        loadBitmap();      
        loadDirectory();
}

void FileSystem::writeFile(const std::string& name, const std::string& data) {
    int firstCluster = -1;
    int prevCluster = -1;

    int i = 0;
    while (i < data.size()) {
        int freeCluster = bitmap.findFree();
        if (freeCluster == -1) {
            cout << "No space left on disk\n";
            return;
        }

        bitmap.markUsed(freeCluster);

        Cluster cluster{};
        int chunkSize = std::min(252, (int)data.size() - i);

        memcpy(cluster.data, data.c_str() + i, chunkSize);
        cluster.next = -1;

        if (prevCluster != -1) {
            Cluster prev = disk.readCluster(prevCluster);
            prev.next = freeCluster;
            disk.writeCluster(prevCluster, prev);
        }

        disk.writeCluster(freeCluster, cluster);

        if (firstCluster == -1)
            firstCluster = freeCluster;

        prevCluster = freeCluster;
        i += chunkSize;
    }

    directory.addFile(name, firstCluster, data.size());

    saveBitmap();     
    saveDirectory(); 
}

std::string FileSystem::readFile(const std::string& name) {

    FileEntry* file = directory.findFile(name);
    if (!file) return "";

    int clusterIndex = file->startCluster;
    int remaining = file->size;

    std::string result;

    while (clusterIndex != -1 && remaining > 0) {
        Cluster cluster = disk.readCluster(clusterIndex);

        int bytesToRead = std::min(252, remaining); 
        result.append(cluster.data, bytesToRead);

        remaining -= bytesToRead;
        clusterIndex = cluster.next;
    }

    return result;
}

void FileSystem::printDirectory() {
    std::cout << endl << "Directorio:" << endl;
    for (const auto& file : directory.getFiles()) {
        std::cout << "- " << file.name 
                  << " -> Cluster " << file.startCluster
                  << " (size: " << file.size << ")"
                  << std::endl;
    }
}

void FileSystem::printBitmap() {
    std::cout << endl << "Bitmap:" << endl;
    for (int i = 0; i < bitmap.getSize(); i++) {
        std::cout << bitmap.isUsed(i) << " ";
    }
    std::cout << std::endl;
}

void FileSystem::saveBitmap() {
    Cluster cluster{};
    
    for (int i = 0; i < bitmap.getSize() && i < 252; i++) {
        cluster.data[i] = bitmap.isUsed(i) ? 1 : 0;
    }

    cluster.next = -1;
    disk.writeCluster(1, cluster); 
}

void FileSystem::loadBitmap() {
    Cluster cluster = disk.readCluster(1);

    for (int i = 0; i < bitmap.getSize() && i < 252; i++) {
        if (cluster.data[i] == 1)
            bitmap.markUsed(i);
        else
            bitmap.markFree(i);
    }
}

void FileSystem::saveDirectory() {
    Cluster old = disk.readCluster(2);
    int toFree = old.next;
    while (toFree != -1) {
        Cluster c = disk.readCluster(toFree);
        bitmap.markFree(toFree);
        toFree = c.next;
    }

    const auto& files = directory.getFiles();
    int fileIndex = 0;
    int currentClusterIndex = 2; 

    while (fileIndex < (int)files.size()) {
        Cluster cluster{};
        int offset = 0;

        while (fileIndex < (int)files.size()) {
            const auto& file = files[fileIndex];
            int nameLen = file.name.size();
            int entrySize = sizeof(int) + nameLen + sizeof(int) + sizeof(int);

            if (offset + entrySize > 252)
                break; 

            memcpy(cluster.data + offset, &nameLen, sizeof(int));
            offset += sizeof(int);

            memcpy(cluster.data + offset, file.name.c_str(), nameLen);
            offset += nameLen;

            memcpy(cluster.data + offset, &file.startCluster, sizeof(int));
            offset += sizeof(int);

            memcpy(cluster.data + offset, &file.size, sizeof(int));
            offset += sizeof(int);

            fileIndex++;
        }

        if (fileIndex < (int)files.size()) {
            
            int nextCluster = bitmap.findFree();
            if (nextCluster == -1) {
                cout << "No space left for directory\n";
                cluster.next = -1;
                disk.writeCluster(currentClusterIndex, cluster);
                return;
            }
            bitmap.markUsed(nextCluster);
            cluster.next = nextCluster;
        } else {
            cluster.next = -1; 
        }

        disk.writeCluster(currentClusterIndex, cluster);
        currentClusterIndex = cluster.next;
    }

}

void FileSystem::loadDirectory() {
    int currentClusterIndex = 2; 

    while (currentClusterIndex != -1) {
        Cluster cluster = disk.readCluster(currentClusterIndex);
        int offset = 0;

        while (offset < 252) {
            int nameLen;
            memcpy(&nameLen, cluster.data + offset, sizeof(int));

            if (nameLen <= 0 || nameLen > 100) break; 

            offset += sizeof(int);

            std::string name(cluster.data + offset, nameLen);
            offset += nameLen;

            int startCluster;
            memcpy(&startCluster, cluster.data + offset, sizeof(int));
            offset += sizeof(int);

            int size;
            memcpy(&size, cluster.data + offset, sizeof(int));
            offset += sizeof(int);

            directory.addFile(name, startCluster, size);
        }

        currentClusterIndex = cluster.next; 
    }
}

Directory& FileSystem::getDirectory() {
    return this->directory;
}

void FileSystem::writeFigure(const Figure& fig) {
    std::string data = fig.serialize();
    writeFile(fig.name, data);
}

Figure FileSystem::readFigure(const std::string& name) {
    std::string data = readFile(name);
    return Figure::deserialize(name, data);
}