#pragma once
#include <vector>

class Bitmap {
private:
    std::vector<bool> map;

public:
    Bitmap(int size);
    int findFree();
    void markUsed(int index);
    void markFree(int index);
    int getSize() const;
    bool isUsed(int index) const;
};