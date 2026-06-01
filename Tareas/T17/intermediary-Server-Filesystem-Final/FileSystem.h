#pragma once
#include "Disk.h"
#include "Bitmap.h"
#include "Directory.h"
#include "Figure.h"
#include <shared_mutex>


class FileSystem {
private:
    Disk disk;
    Bitmap bitmap;
    Directory directory;
    std::shared_mutex fs_mutex;

public:
    FileSystem(const std::string& filename, int numClusters);

    void writeFile(const std::string& name, const std::string& data);
    std::string readFile(const std::string& name);
    void printDirectory();
    void printBitmap();
    void saveBitmap();
    void loadBitmap();
    void saveDirectory();
    void loadDirectory();
    Directory& getDirectory();
    void writeFigure(const Figure& fig);
    Figure readFigure(const std::string& name);
    std::vector<FileEntry> listFiles();
};