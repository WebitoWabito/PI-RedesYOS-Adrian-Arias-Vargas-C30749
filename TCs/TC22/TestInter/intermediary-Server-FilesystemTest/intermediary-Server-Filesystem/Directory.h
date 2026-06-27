#pragma once
#include <string>
#include <vector>

struct FileEntry {
    std::string name;
    int startCluster;
    int size;
    //Here I could add an array of pieces
};

class Directory {
private:
    std::vector<FileEntry> files;

public:
    void addFile(const std::string& name, int startCluster, int size);
    FileEntry* findFile(const std::string& name);
    const std::vector<FileEntry>& getFiles();
};