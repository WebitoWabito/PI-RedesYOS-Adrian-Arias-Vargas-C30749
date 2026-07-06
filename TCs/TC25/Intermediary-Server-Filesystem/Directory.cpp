#include "Directory.h"

void Directory::addFile(const std::string& name, int startCluster, int size) {
    files.push_back({name, startCluster, size});
}

FileEntry* Directory::findFile(const std::string& name) {
    for (auto& file : files) {
        if (file.name == name) return &file;
    }
    return nullptr;
}

bool Directory::deleteFile(const std::string& name) {
    for (auto it = files.begin(); it != files.end(); ++it) {
        if (it->name == name) {
            files.erase(it);
            return true;
        }
    }
    return false;
}

const std::vector<FileEntry>& Directory::getFiles() {
    return this->files;
}
