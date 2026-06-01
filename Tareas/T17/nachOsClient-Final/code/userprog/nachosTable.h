#ifndef NACHOSTABLA_H
#define NACHOSTABLA_H

#include "bitmap.h"

#define MAX_OPEN_FILES 128

class NachosOpenFilesTable {
public:
    NachosOpenFilesTable();
    ~NachosOpenFilesTable();

    int Open(int unixHandle);            // Registra un fd Unix, retorna handle NachOS
    int Close(int nachosHandle);         // Desregistra el handle NachOS
    bool isOpened(int nachosHandle);     // Verifica si está abierto
    int getUnixHandle(int nachosHandle); // Retorna el fd Unix correspondiente
    void addThread();                    // Un hilo más usa esta tabla
    void delThread();                    // Un hilo menos; si es el último, cierra todo
    bool isLastThread();                 // True si usage == 0 (nadie más la usa)

    void Print();

private:
    int * openFiles;       // vector: índice NachOS -> fd Unix
    BitMap * openFilesMap; // bitmap: 1 = ocupado, 0 = libre
    int usage;             // cuántos hilos usan esta tabla
};

#endif // NACHOSTABLA_H