#include "nachosTable.h"
#include <unistd.h>
#include <stdio.h>

NachosOpenFilesTable::NachosOpenFilesTable() {
    openFiles    = new int[MAX_OPEN_FILES];
    openFilesMap = new BitMap(MAX_OPEN_FILES);
    usage        = 0;

    // Reservar los primeros 3: stdin(0), stdout(1), stderr(2)
    openFiles[0] = 0;  openFilesMap->Mark(0);  // ConsoleInput
    openFiles[1] = 1;  openFilesMap->Mark(1);  // ConsoleOutput
    openFiles[2] = 2;  openFilesMap->Mark(2);  // ConsoleError
}

NachosOpenFilesTable::~NachosOpenFilesTable() {
    delete [] openFiles;
    delete openFilesMap;
}

int NachosOpenFilesTable::Open(int unixHandle) {
    int nachosHandle = openFilesMap->Find();
    if (nachosHandle == -1) return -1;
    openFiles[nachosHandle] = unixHandle;
    return nachosHandle;
}

int NachosOpenFilesTable::Close(int nachosHandle) {
    if (!isOpened(nachosHandle)) return -1;
    openFilesMap->Clear(nachosHandle);
    openFiles[nachosHandle] = -1;
    return 0;
}

bool NachosOpenFilesTable::isOpened(int nachosHandle) {
    if (nachosHandle < 0 || nachosHandle >= MAX_OPEN_FILES) return false;
    return openFilesMap->Test(nachosHandle);
}

int NachosOpenFilesTable::getUnixHandle(int nachosHandle) {
    if (!isOpened(nachosHandle)) return -1;
    return openFiles[nachosHandle];
}

void NachosOpenFilesTable::addThread() {
    usage++;
}

void NachosOpenFilesTable::delThread() {
    usage--;
    if (usage == 0) {
        // Cerrar todos los archivos abiertos (excepto stdin/stdout/stderr)
        for (int i = 3; i < MAX_OPEN_FILES; i++) {
            if (isOpened(i)) {
                close(openFiles[i]);
                Close(i);
            }
        }
    }
}

bool NachosOpenFilesTable::isLastThread() {
    return usage == 0;
}

void NachosOpenFilesTable::Print() {
    printf("NachosOpenFilesTable (usage=%d):\n", usage);
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (isOpened(i)) {
            printf("  NachOS[%d] -> Unix[%d]\n", i, openFiles[i]);
        }
    }
}
