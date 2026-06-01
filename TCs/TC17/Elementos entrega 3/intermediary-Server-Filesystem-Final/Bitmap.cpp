#include "Bitmap.h"

//Constructor 
Bitmap::Bitmap(int size) : map(size, false) {}

//This method finds the index of the first free cluster, or returns -1 if none are free
int Bitmap::findFree() {
    for (int i = 0; i < map.size(); i++) {
        if (!map[i]) return i;
    }
    return -1;
}

//This method marks a cluster as used
void Bitmap::markUsed(int index) {
    map[index] = true;
}


//This method marks a cluster as free
void Bitmap::markFree(int index) {
    map[index] = false;
}

int Bitmap::getSize() const {
    return map.size();
}

bool Bitmap::isUsed(int index) const {
    return map[index];
}